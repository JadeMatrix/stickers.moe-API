#line 2 "common/auth.cpp"


#include "auth.hpp"

#include "logging.hpp"
#include "postgres.hpp"
#include "string_utils.hpp"
#include "../api/user.hpp"  // stickers::no_such_user
#include "../common/config.hpp"

#include <algorithm>    // std::set_difference()
#include <iterator>     // std::inserter


namespace
{
    bool extract_auth_from_token(
        const std::string& token_string,
        stickers::auth_info& info
    )
    {
        // DEBUG:
        STICKERS_LOG(
            stickers::log_level::DEBUG,
            "extract_auth_from_token(), token = ",
            token_string
        );
        
        try
        {
            auto auth_jwt = stickers::jwt::parse( token_string );
            
            auto user_id_found = auth_jwt.claims.find(
                "user_id"
            );
            auto permissions_found = auth_jwt.claims.find(
                "permissions"
            );
            
            stickers::permissions_type permissions;
            
            if(
                permissions_found != auth_jwt.claims.end()
                && permissions_found.value().is_array()
            )
                for( const auto& permission : permissions_found.value() )
                    permissions.insert( permission.get< std::string >() );
            else
                throw stickers::authentication_error{
                    "missing required claim \"permissions\""
                };
            
            if(
                user_id_found != auth_jwt.claims.end()
                && user_id_found.value().is_string()
            )
            {
                info = {
                    stickers::bigid::from_string(
                        user_id_found.value().get< std::string >()
                    ),
                    permissions
                };
                
                return true;
            }
            else
                throw stickers::authentication_error{
                    "missing required claim \"user_id\""
                };
        }
        catch( const stickers::jwt::structure_error& e )
        {
            // Ignore any tokens that aren't JWTs
            STICKERS_LOG(
                stickers::log_level::VERBOSE,
                "skipping unusable token (not a JWT: ",
                e.what(),
                ")"
            );
            return false;
        }
        catch( const stickers::jwt::validation_error& e )
        {
            // Ignore any tokesn that don't pass validation (may not
            // even belong to this site)
            STICKERS_LOG(
                stickers::log_level::VERBOSE,
                "skipping unusable token (not valid: ",
                e.what(),
                ")"
            );
            return false;
        }
    }
}


namespace stickers
{
    auth_info authenticate( const show::request& request )
    {
        bool header_found = false;
        bool   auth_found = false;
        
        auth_info info{ bigid::MIN(), {} };
        
        auto authorization_headers = request.headers().find( "Authorization" );
        if( authorization_headers != request.headers().end() )
        {
            header_found = true;
            std::string bearer_begin{ "Bearer " };
            
            for( const auto& header_value : authorization_headers -> second )
            {
                if( header_value.find( bearer_begin ) == 0 )
                {
                    auth_found = extract_auth_from_token(
                        header_value.substr( bearer_begin.size() ),
                        info
                    );
                    if( auth_found )
                        break;
                }
            }
        }
        
        auto cookie_headers = request.headers().find( "Cookie" );
        if( cookie_headers != request.headers().end() )
        {
            header_found = true;
            std::string cookie_begin{
                config()[ "auth" ][ "token_cookie_name" ].get< std::string >()
                + "="
            };
            
            for( const auto& header_value : cookie_headers -> second )
            {
                if( header_value.find( cookie_begin ) == 0 )
                {
                    auth_found = extract_auth_from_token(
                        header_value.substr( cookie_begin.size() ),
                        info
                    );
                    if( auth_found )
                        break;
                }
            }
        }
        
        if( !header_found )
            throw authentication_error{
                "missing auth header (\"Authorization\" or \"Cookie\")"
            };
        else if( !auth_found )
            throw authentication_error{
                "no usable authentication tokens found" 
            };
        else
            return info;
    }
    
    jwt generate_auth_token_for_user(
        bigid user_id,
        const audit::blame& blame
    )
    {
        auto permissions = get_user_permissions( user_id );
        
        auto token_lifetime = std::chrono::hours{
            config()[ "auth" ][ "token_lifetime_hours" ].get< int >()
        };
        
        jwt token{
            .iat = now(),
            .nbf = now(),
            .exp = now() + token_lifetime,
            .claims = {
                {
                    "user_id",
                    static_cast< std::string >( user_id )
                },
                { "permissions", permissions },
                { "blame", {
                    { "who"  , static_cast< std::string >( blame.who   ) },
                    { "what" ,                             blame.what    },
                    { "when" ,             to_iso8601_str( blame.when  ) },
                    { "where",                             blame.where   }
                } }
            }
        };
        
        STICKERS_LOG(
            log_level::INFO,
            "user ",
            blame.who,
            " generated auth token for user ",
            user_id,
            " from ",
            blame.where,
            " at ",
            to_iso8601_str( blame.when ),
            " ",
            blame.what
        );
        
        return token;
    }
    
    // void set_user_permissions(
    //     bigid user_id,
    //     const permissions_type& permissions
    // )
    // {
        
    // }
    
    permissions_type get_user_permissions( bigid user_id )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        pqxx::result result = transaction.exec_params(
            PSQL(
                SELECT p.permission AS permission
                FROM
                    users.users AS u
                    JOIN permissions.role_permissions AS rp
                        ON u.user_role_id = rp.role_id
                    JOIN permissions.permissions AS p
                        ON rp.permission_id = p.permission_id
                WHERE u.user_id = $1
                ;
            ),
            user_id
        );
        transaction.commit();
        
        if( result.size() < 1 )
        {
            // Assert that the user even exists; if so, continue to return an
            // empty set of permissions; if not, throw `no_such_user`
            // FIXME: This (possibly) requires an extra database access for e.g.
            // banned users, who we most likely _don't_ want causing extra load
            auto user_info = load_user( user_id );
        }
        
        permissions_type permissions;
        
        for( const auto& row : result )
            permissions.insert( row[ "permission" ].as< std::string >() );
        
        return permissions;
    }
    
    void permissions_assert_any(
        const permissions_type& got,
        const permissions_type& expect
    )
    {
        std::set< std::string > difference;
        
        std::set_difference(
            expect.begin(),
            expect.end(),
            got.begin(),
            got.end(),
            std::inserter( difference, difference.begin() )
        );
        
        if( difference.size() == expect.size() )
            throw authorization_error(
                "missing one of these permissions: \"" + join(
                    difference,
                    std::string{ "\", \"" }
                ) + "\""
            );
    }
    
    void permissions_assert_all(
        const permissions_type& got,
        const permissions_type& expect
    )
    {
        std::set< std::string > difference;
        
        std::set_difference(
            expect.begin(),
            expect.end(),
            got.begin(),
            got.end(),
            std::inserter( difference, difference.begin() )
        );
        
        if( difference.size() )
            throw authorization_error(
                "missing permissions: \"" + join(
                    difference,
                    std::string{ "\", \"" }
                ) + "\""
            );
    }
}

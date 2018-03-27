#include "auth.hpp"

#include "jwt.hpp"
#include "string_utils.hpp"

#include <algorithm>    // std::set_difference()
#include <iterator>     // std::inserter


namespace stickers
{
    auth_info authenticate( const show::request& request )
    {
        auto authorization_header = request.headers().find( "Authorization" );
        if( authorization_header == request.headers().end() )
            throw authentication_error( "no \"Authorization\" header" );
        
        std::string bearer_begin = "Bearer ";
        
        for( const auto& header_value : authorization_header -> second )
        {
            if( header_value.find( bearer_begin ) == 0 )
            {
                try
                {
                    auto auth_jwt = jwt::parse(
                        header_value.substr( bearer_begin.size() )
                    );
                    
                    auto user_id_found = auth_jwt.claims.find(
                        "user_id"
                    );
                    auto permissions_found = auth_jwt.claims.find(
                        "permissions"
                    );
                    
                    permissions_type permissions;
                    
                    if(
                        permissions_found != auth_jwt.claims.end()
                        && permissions_found.value().is_array()
                    )
                        for( const auto& permission : permissions_found.value() )
                            permissions.insert( permission.get< std::string >() );
                    else
                        throw authentication_error(
                            "missing required claim \"permissions\""
                        );
                    
                    if(
                        user_id_found != auth_jwt.claims.end()
                        && user_id_found.value().is_string()
                    )
                        return {
                            bigid::from_string(
                                user_id_found.value().get< std::string >()
                            ),
                            permissions
                        };
                    else
                        throw authentication_error(
                            "missing required claim \"user_id\""
                        );
                }
                catch( const jwt::structure_error& e )
                {
                    continue;
                }
                catch( const jwt::validation_error& e )
                {
                    throw authentication_error(
                        "invalid token signature " + std::string{ e.what() }
                    );
                }
            }
        }
        
        throw authentication_error(
            "no usable authentication tokens found"
        );
    }
    
    // std::string generate_auth_token_for_user( bigid user_id )
    // {
        
    // }
    
    // void set_user_permissions(
    //     bigid user_id,
    //     const permissions_type& permissions
    // )
    // {
        
    // }
    
    // permissions_type get_user_permissions( bigid user_id )
    // {
        
    // }
    
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
                    std::string( "\", \"" )
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
                    std::string( "\", \"" )
                ) + "\""
            );
    }
}

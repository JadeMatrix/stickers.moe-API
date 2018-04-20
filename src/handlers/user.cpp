#line 2 "handlers/user.cpp"


#include "handlers.hpp"

#include "../api/user.hpp"
#include "../common/auth.hpp"
#include "../common/json.hpp"
#include "../common/logging.hpp"
#include "../server/routing.hpp"
#include "../server/server.hpp"
#include "../server/parse.hpp"

#include <array>


namespace
{
    std::string image_hash_to_image( stickers::sha256 hash )
    {
        std::string hash_hex = hash.hex_digest();
        return (
              "/images/"
            + hash_hex.substr( 0, 2 )
            + "/"
            + hash_hex.substr( 2, 2 )
            + "/"
            + hash_hex
        );
    }
}


namespace stickers
{
    void handlers::create_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "create_user" }
        );
        
        auto details_json = parse_request_content( request );
        
        for( const auto field : {
            "password",
            "display_name",
            "email"
        } )
            if( details_json.find( field ) == details_json.end() )
                throw handler_exit{
                    { 400, "Bad Request" },
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !details_json[ field ].is_string() )
                throw handler_exit{
                    { 400, "Bad Request" },
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string"
                };
        
        user_info details;
        
        details.password     = details_json[ "password" ].get< std::string >();
        details.created      = now();
        details.display_name = details_json[ "display_name" ];
        details.email        = details_json[ "email"        ];
        
        auto created_user = create_user(
            details,
            {
                auth.user_id,
                "user signup",
                now(),
                request.client_address()
            }
        );
        
        details_json = {
            { "user_id"     , created_user.id                             },
            { "created"     , to_iso8601_str( created_user.info.created ) },
            { "revised"     , to_iso8601_str( created_user.info.revised ) },
            { "display_name", created_user.info.display_name              },
            { "email"       , created_user.info.email                     }
        };
        
        if( created_user.info.real_name )
            details_json[ "real_name" ] = *created_user.info.real_name;
        else
            details_json[ "real_name" ] = nullptr;
        
        if( created_user.info.avatar_hash )
            details_json[ "avatar" ] = image_hash_to_image(
                *created_user.info.avatar_hash
            );
        else
            details_json[ "avatar" ] = nullptr;
        
        auto user_json = details_json.dump();
        
        show::response response{
            request.connection(),
            show::HTTP_1_1,
            { 201, "Created" },
            {
                server_header,
                { "Content-Type", { "application/json" } },
                { "Content-Length", {
                    std::to_string( user_json.size() )
                } },
                { "Location", {
                    "/user/" + static_cast< std::string >( created_user.id )
                } }
            }
        };
        
        response.sputn( user_json.c_str(), user_json.size() );
    }
    
    void handlers::get_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_user_id_variable = variables.find( "user_id" );
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{ { 404, "Not Found" }, "need a valid user ID" };
        }
        
        try
        {
            auto info = load_user( user_id );
            
            nlj::json user = {
                { "user_id"     , user_id                        },
                { "created"     , to_iso8601_str( info.created ) },
                { "revised"     , to_iso8601_str( info.revised ) },
                { "display_name", info.display_name              },
                { "email"       , info.email                     }
            };
            
            if( info.real_name )
                user[ "real_name" ] = *info.real_name;
            else
                user[ "real_name" ] = nullptr;
            
            if( info.avatar_hash )
                user[ "avatar" ] = image_hash_to_image( *info.avatar_hash );
            else
                user[ "avatar" ] = nullptr;
            
            auto user_json = user.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                { 200, "OK" },
                {
                    server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( user_json.size() )
                    } }
                }
            };
            
            response.sputn( user_json.c_str(), user_json.size() );
        }
        catch( const no_such_user& nsu )
        {
            throw handler_exit{ { 404, "Not Found" }, "no such user" };
        }
    }
    
    void handlers::edit_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_user_id_variable = variables.find( "user_id" );
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{ { 404, "Not Found" }, "need a valid user ID" };
        }
        
        auto auth = authenticate( request );
        if( auth.user_id == user_id )
            permissions_assert_all(
                auth.user_permissions,
                { "edit_own_user" }
            );
        else
            permissions_assert_all(
                auth.user_permissions,
                { "edit_any_user" }
            );
        
        throw handler_exit{ { 500, "Not Implemented" }, "Not implemented" };
    }
    
    void handlers::delete_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_user_id_variable = variables.find( "user_id" );
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{ { 404, "Not Found" }, "need a valid user ID" };
        }
        
        auto auth = authenticate( request );
        if( auth.user_id == user_id )
            permissions_assert_all(
                auth.user_permissions,
                { "delete_own_user" }
            );
        else
            permissions_assert_all(
                auth.user_permissions,
                { "delete_any_user" }
            );
        
        try
        {
            delete_user(
                user_id,
                {
                    auth.user_id,
                    "delete user handler",
                    now(),
                    request.client_address()
                }
            );
            
            std::string null_json = "null";
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                { 200, "OK" },
                {
                    server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( null_json.size() )
                    } }
                }
            };
            
            response.sputn( null_json.c_str(), null_json.size() );
        }
        catch( const no_such_user& nsu )
        {
            throw handler_exit{ { 404, "Not Found" }, "no such user" };
        }
    }
}

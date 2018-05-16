#line 2 "handlers/user.cpp"


#include "handlers.hpp"

#include "../api/media.hpp"
#include "../api/user.hpp"
#include "../common/auth.hpp"
#include "../common/crud.hpp"
#include "../common/json.hpp"
#include "../common/logging.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>

#include <array>


namespace stickers
{
    void handlers::create_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth{ authenticate( request ) };
        permissions_assert_all(
            auth.user_permissions,
            { "create_user" }
        );
        
        auto details_doc{ parse_request_content( request ) };
        
        if( !details_doc.is_a< map_document >() )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "invalid data format"
            };
        
        auto& details_map{ details_doc.get< map_document >() };
        
        for( const auto& field : {
            std::string{ "password"     },
            std::string{ "display_name" },
            std::string{ "email"        }
        } )
            if( details_map.find( field ) == details_map.end() )
                throw handler_exit{
                    show::code::BAD_REQUEST,
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !details_map[ field ].is_a< string_document >() )
                throw handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string"
                };
        
        user_info details;
        
        details.password     = details_map[ "password"     ].get< string_document >();
        details.created      = now();
        details.display_name = details_map[ "display_name" ].get< string_document >();
        details.email        = details_map[ "email"        ].get< string_document >();
        
        try
        {
            auto created_user{ create_user(
                details,
                {
                    auth.user_id,
                    "create user handler",
                    now(),
                    request.client_address()
                }
            ) };
            
            nlj::json details_json{
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
                details_json[ "avatar" ] = load_media_info(
                    *created_user.info.avatar_hash
                ).file_url;
            else
                details_json[ "avatar" ] = nullptr;
            
            auto user_json{ details_json.dump() };
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::CREATED,
                {
                    show::server_header,
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
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::get_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_user_id_variable{ variables.find( "user_id" ) };
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "need a valid user ID" };
        }
        
        try
        {
            auto info{ load_user( user_id ) };
            
            nlj::json user_json{
                { "user_id"     , user_id                        },
                { "created"     , to_iso8601_str( info.created ) },
                { "revised"     , to_iso8601_str( info.revised ) },
                { "display_name", info.display_name              },
                { "email"       , info.email                     }
            };
            
            if( info.real_name )
                user_json[ "real_name" ] = *info.real_name;
            else
                user_json[ "real_name" ] = nullptr;
            
            if( info.avatar_hash )
                user_json[ "avatar" ] = load_media_info(
                    *info.avatar_hash
                ).file_url;
            else
                user_json[ "avatar" ] = nullptr;
            
            auto user_json_string = user_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( user_json_string.size() )
                    } }
                }
            };
            
            response.sputn( user_json_string.c_str(), user_json_string.size() );
        }
        catch( const no_such_user& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "no such user" };
        }
    }
    
    void handlers::edit_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        // Authenticate first, check permissions later
        auto auth{ authenticate( request ) };
        
        auto found_user_id_variable{ variables.find( "user_id" ) };
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "need a valid user ID" };
        }
        
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
        
        try
        {
            throw handler_exit{
                show::code::NOT_IMPLEMENTED,
                "Not implemented"
            };
        }
        catch( const no_such_user& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "no such user" };
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::delete_user(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        // Authenticate first, check permissions later
        auto auth{ authenticate( request ) };
        
        auto found_user_id_variable{ variables.find( "user_id" ) };
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "need a valid user ID" };
        }
        
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
                show::code::OK,
                {
                    show::server_header,
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
            throw handler_exit{ show::code::NOT_FOUND, "no such user" };
        }
    }
}

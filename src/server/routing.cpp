#line 2 "server/routing.cpp"


#include "routing.hpp"

#include "server.hpp"
#include "../common/auth.hpp"
#include "../common/config.hpp"
#include "../common/logging.hpp"
#include "../common/json.hpp"
#include "../common/string_utils.hpp"
#include "../handlers/handlers.hpp"

#include <exception>
#include <map>
#include <vector>


namespace
{
    struct routing_node
    {
        using methods_type = std::map<
            std::string,
            stickers::handler_type,
            show::_less_ignore_case_ASCII
        >;
        using subs_type = std::map<
            std::string,
            routing_node
        >;
        using variable_type = std::pair< std::string, routing_node >;
        
        methods_type   methods;
        subs_type      subs;
        variable_type* variable;
    };
    
    void handle_options_request(
        show::request& request,
        const routing_node* current_node
    )
    {
        std::vector< std::string > methods_list, subs_list;
        
        for( auto& method : current_node -> methods )
            methods_list.push_back( method.first );
        for( auto& sub : current_node -> subs )
            subs_list.push_back( sub.first );
        
        nlj::json options_object = {
            { "methods"     , methods_list },
            { "subs"        ,    subs_list },
            { "variable_sub", static_cast< bool >( current_node -> variable ) }
        };
        std::string json_string = options_object.dump();
        
        show::response response{
            request.connection(),
            show::HTTP_1_1,
            { 200, "OK" },
            {
                stickers::server_header,
                { "Content-Type"  , { "application/json" } },
                { "Content-Length", { std::to_string( json_string.size() ) } }
            }
        };
        
        response.sputn( json_string.c_str(), json_string.size() );
    }
}


namespace
{
    routing_node::variable_type user_manip{
        "user_id",
        {
            {
                { "GET"   , stickers::handlers::   get_user },
                { "PUT"   , stickers::handlers::  edit_user },
                { "DELETE", stickers::handlers::delete_user }
            },
            {},
            nullptr
        }
    };
    
    routing_node::variable_type list_entry_manip{
        "list_entry_id",
        {
            {
                { "PUT"   , stickers::handlers::update_list_item },
                { "DELETE", stickers::handlers::remove_list_item }
            },
            {},
            nullptr
        }
    };
    routing_node::variable_type list_subs{
        "user_id",
        {
            {
                { "GET" , stickers::handlers::get_list      },
                { "POST", stickers::handlers::add_list_item }
            },
            {},
            &list_entry_manip
        }
    };
    
    routing_node::variable_type person_manip{
        "person_id",
        {
            {
                { "GET"   , stickers::handlers::   get_person },
                { "PUT"   , stickers::handlers::  edit_person },
                { "DELETE", stickers::handlers::delete_person }
            },
            {},
            nullptr
        }
    };
    routing_node::variable_type shop_manip{
        "shop_id",
        {
            {
                { "GET"   , stickers::handlers::   get_shop },
                { "PUT"   , stickers::handlers::  edit_shop },
                { "DELETE", stickers::handlers::delete_shop }
            },
            {},
            nullptr
        }
    };
    routing_node::variable_type design_manip{
        "design_id",
        {
            {
                { "GET"   , stickers::handlers::   get_design },
                { "PUT"   , stickers::handlers::  edit_design },
                { "DELETE", stickers::handlers::delete_design }
            },
            {},
            nullptr
        }
    };
    routing_node::variable_type product_manip{
        "product_id",
        {
            {
                { "GET"   , stickers::handlers::   get_product },
                { "PUT"   , stickers::handlers::  edit_product },
                { "DELETE", stickers::handlers::delete_product }
            },
            {},
            nullptr
        }
    };
    
    const routing_node tree{
        {},
        {
            { "signup", {
                { { "POST", stickers::handlers::signup } },
                {},
                nullptr
            } },
            { "login", {
                { { "POST", stickers::handlers::login } },
                {},
                nullptr
            } },
            { "user", {
                { { "POST", stickers::handlers::create_user } },
                {},
                &user_manip
            } },
            { "list", {
                {},
                {},
                &list_subs
            } },
            { "person", {
                { { "POST", stickers::handlers::create_person } },
                {},
                &person_manip
            } },
            { "shop", {
                { { "POST", stickers::handlers::create_shop } },
                {},
                &shop_manip
            } },
            { "design", {
                { { "POST", stickers::handlers::create_design } },
                {},
                &design_manip
            } },
            { "product", {
                { { "POST", stickers::handlers::create_product } },
                {},
                &product_manip
            } },
        },
        nullptr
    };
}


namespace stickers
{
    void route_request( show::request& request )
    {
        bool handler_finished{ false };
        show::response_code error_code   { 400, "Bad Request" };
        std::string         error_message;
        show::headers_type  error_headers{ server_header };
        
        try
        {
            handler_vars_type variables;
            
            const auto* current_node = &tree;
            
            for( auto& element : request.path() )
            {
                auto found_sub = current_node -> subs.find( element );
                
                if( found_sub != current_node -> subs.end() )
                {
                    current_node = &( found_sub -> second );
                }
                else if( current_node -> variable )
                {
                    variables[ current_node -> variable -> first ] = element;
                    current_node = &( current_node -> variable -> second );
                }
                else
                    throw handler_exit{ { 404, "Not Found" }, "" };
            }
            
            auto found_method = current_node -> methods.find(
                request.method()
            );
            
            if( found_method != current_node -> methods.end() )
            {
                found_method -> second( request, variables );
            }
            else if( request.method() == "OPTIONS" )
            {
                handle_options_request( request, current_node );
            }
            else if( request.method() == "HEAD" )
            {
                // TODO: HEAD method implementation for CORS
                throw handler_exit{
                    { 500, "Not Implemented" },
                    "CORS not implemented"
                };
            }
            else
                throw handler_exit{ { 405, "Method Not Allowed" }, "" };
            
            handler_finished = true;
        }
        catch( const show::connection_interrupted& ci )
        {
            throw;
        }
        catch( const handler_exit& he )
        {
            error_code    = he.response_code;
            error_message = he.message;
        }
        catch( const authentication_error& ae )
        {
            error_code    = { 401, "Unauthorized" };    // HTTP :^)
            error_message = "this action requires authentication credentials";
            error_headers[ "WWW-Authenticate" ] = {
                "Bearer realm=\"stickers.moe JWT\""
            };
            STICKERS_LOG(
                log_level::INFO,
                "unauthenticated ",
                request.method(),
                " request from ",
                request.client_address(),
                " on ",
                join( request.path(), std::string{ "/" } ),
                ": ",
                ae.what()
            );
        }
        catch( const authorization_error& ae )
        {
            error_code    = { 403, "Forbidden" };
            error_message =
                "you are not permitted to perform this action ("
                + static_cast< std::string >( ae.what() )
                + ")"
            ;
            STICKERS_LOG(
                log_level::INFO,
                "unauthorized ",
                request.method(),
                " request from ",
                request.client_address(),
                " on ",
                join( request.path(), std::string{ "/" } ),
                ": ",
                ae.what()
            );
        }
        catch( const std::exception& e )
        {
            error_code    = { 500, "Server Error" };
            error_message = "please try again later";
            STICKERS_LOG(
                log_level::ERROR,
                "uncaught std::exception in route_request(show::request&): ",
                e.what()
            );
        }
        catch( ... )
        {
            error_code    = { 500, "Server Error" };
            error_message = "please try again later";
            STICKERS_LOG(
                log_level::ERROR,
                "uncaught non-std::exception in route_request(show::request&)"
            );
        }
        
        if( !request.unknown_content_length() )
            request.flush();
        
        if( handler_finished )
            return;
        
        nlj::json error_object = {
            { "message", error_message                   },
            { "contact", config()[ "server" ][ "admin" ] }
        };
        std::string error_json = error_object.dump();
        
        error_headers[ "Content-Type"   ] = { "application/json" };
        error_headers[ "Content-Length" ] = {
            std::to_string( error_json.size() )
        };
        
        show::response response{
            request.connection(),
            show::HTTP_1_1,
            error_code,
            error_headers
        };
        
        response.sputn( error_json.c_str(), error_json.size() );
    }
}

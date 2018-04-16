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
            if( request.path().size() > 0 )
            {
                if( request.path()[ 0 ] == "signup" )
                {
                    if( request.method() == "POST" )
                        handlers::signup( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "login" )
                {
                    if( request.method() == "POST" )
                        handlers::login( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "user" )
                {
                    if( request.method() == "POST" )
                        handlers::create_user( request );
                    else if( request.method() == "GET" )
                        handlers::get_user( request );
                    else if( request.method() == "PUT" )
                        handlers::edit_user( request );
                    else if( request.method() == "DELETE" )
                        handlers::delete_user( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "list" )
                {
                    if( request.path().size() > 1 )
                    {
                        if( request.method() == "POST" )
                            handlers::add_list_item( request );
                        else if( request.method() == "PUT" )
                            handlers::update_list_item( request );
                        else if( request.method() == "DELETE" )
                            handlers::remove_list_item( request );
                        else
                            throw handler_exit{
                                { 405, "Method Not Allowed" },
                                ""
                            };
                    }
                    else if( request.method() == "GET" )
                        handlers::get_list( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "person" )
                {
                    if( request.method() == "POST" )
                        handlers::create_person( request );
                    else if( request.method() == "GET" )
                        handlers::get_person( request );
                    else if( request.method() == "PUT" )
                        handlers::edit_person( request );
                    else if( request.method() == "DELETE" )
                        handlers::delete_person( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "shop" )
                {
                    if( request.method() == "POST" )
                        handlers::create_shop( request );
                    else if( request.method() == "GET" )
                        handlers::get_shop( request );
                    else if( request.method() == "PUT" )
                        handlers::edit_shop( request );
                    else if( request.method() == "DELETE" )
                        handlers::delete_shop( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "design" )
                {
                    if( request.method() == "POST" )
                        handlers::create_design( request );
                    else if( request.method() == "GET" )
                        handlers::get_design( request );
                    else if( request.method() == "PUT" )
                        handlers::edit_design( request );
                    else if( request.method() == "DELETE" )
                        handlers::delete_design( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else if( request.path()[ 0 ] == "product" )
                {
                    if( request.method() == "POST" )
                        handlers::create_product( request );
                    else if( request.method() == "GET" )
                        handlers::get_product( request );
                    else if( request.method() == "PUT" )
                        handlers::edit_product( request );
                    else if( request.method() == "DELETE" )
                        handlers::delete_product( request );
                    else
                        throw handler_exit{ { 405, "Method Not Allowed" }, "" };
                }
                else
                    throw handler_exit{ { 404, "Not Found" }, "" };
            }
            else
                throw handler_exit{ { 404, "Not Found" }, "need an API path" };
            
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

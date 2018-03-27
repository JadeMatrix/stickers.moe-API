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

#line __LINE__ "server/routing.cpp"


namespace stickers
{
    void route_request( show::request& request )
    {
        bool handler_finished = false;
        show::response_code error_code    = { 400, "Bad Request" };
        std::string         error_message = "";
        show::headers_type  error_headers = { server_header };
        
        try
        {
            if( request.path().size() > 0 )
            {
                if( request.path()[ 0 ] == "signup" )
                {
                    if( request.method() == "POST" )
                        handlers::signup( request );
                    else
                        throw handler_exit( { 405, "Method Not Allowed" }, "" );
                }
                else if( request.path()[ 0 ] == "login" )
                {
                    if( request.method() == "POST" )
                        handlers::login( request );
                    else
                        throw handler_exit( { 405, "Method Not Allowed" }, "" );
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
                        throw handler_exit( { 405, "Method Not Allowed" }, "" );
                }
                else
                    throw handler_exit( { 404, "Not Found" }, "" );
            }
            else
                throw handler_exit( { 404, "Not Found" }, "need an API path" );
            
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
                INFO,
                "unauthenticated ",
                request.method(),
                " request from ",
                request.client_address(),
                " on ",
                join( request.path(), std::string( "/" ) ),
                ": ",
                ae.what()
            );
        }
        catch( const authorization_error& ae )
        {
            error_code    = { 403, "Forbidden" };
            error_message =
                "you are not permitted to perform this action ("
                + std::string( ae.what() )
                + ")"
            ;
            STICKERS_LOG(
                INFO,
                "unauthorized ",
                request.method(),
                " request from ",
                request.client_address(),
                " on ",
                join( request.path(), std::string( "/" ) ),
                ": ",
                ae.what()
            );
        }
        catch( const std::exception& e )
        {
            error_code    = { 500, "Server Error" };
            error_message = "please try again later";
            STICKERS_LOG(
                ERROR,
                "uncaught std::exception in route_request(show::request&): ",
                e.what()
            );
        }
        catch( ... )
        {
            error_code    = { 500, "Server Error" };
            error_message = "please try again later";
            STICKERS_LOG(
                ERROR,
                "uncaught non-std::exception in route_request(show::request&)"
            );
        }
        
        if( !request.unknown_content_length() )
            while( !request.eof() ) request.sbumpc();
        
        if( handler_finished )
            return;
        
        nlj::json error_object = {
            { "message", error_message              },
            { "contact", config()[ "server" ][ "admin" ] }
        };
        std::string error_json = error_object.dump();
        
        error_headers[ "Content-Type"   ] = { "application/json" };
        error_headers[ "Content-Length" ] = {
            std::to_string( error_json.size() )
        };
        
        show::response response(
            request.connection(),
            show::HTTP_1_1,
            error_code,
            error_headers
        );
        
        response.sputn( error_json.c_str(), error_json.size() );
    }
}

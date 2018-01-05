#include "routing.hpp"

#include "handlers.hpp"
#include "server.hpp"
#include "../common/logging.hpp"
#include "../common/json.hpp"

#include <exception>
#include <map>


namespace stickers
{
    void route_request( show::request& request )
    {
        handlers::handler_rt error_info{ { 400, "Bad Request" }, "" };
        
        try
        {
            if( request.path().size() > 0 )
            {
                if( request.path()[ 0 ] == "user" )
                {
                    if( request.method() == "POST" )
                        error_info = handlers::create_user( request );
                    else if( request.method() == "GET" )
                        error_info = handlers::get_user( request );
                    else if( request.method() == "PUT" )
                        error_info = handlers::edit_user( request );
                    else if( request.method() == "DELETE" )
                        error_info = handlers::delete_user( request );
                    else
                        error_info = { { 405, "Method Not Allowed" }, "" };
                }
                else
                    error_info = { { 404, "Not Found" }, "" };
            }
            else
                error_info = { { 404, "Not Found" }, "need an API path" };
        }
        catch( const show::client_disconnected& cd )
        {
            throw;
        }
        catch( const show::connection_timeout& ct )
        {
            throw;
        }
        catch( const std::exception& e )
        {
            error_info = { { 500, "Server Error" }, "please try again later" };
            STICKERS_LOG(
                ERROR,
                "uncaught std::exception in route_request(show::request&): ",
                e.what()
            );
        }
        catch( ... )
        {
            error_info = { { 500, "Server Error" }, "please try again later" };
            STICKERS_LOG(
                ERROR,
                "uncaught non-std::exception in route_request(show::request&)"
            );
        }
        
        if( !request.unknown_content_length() )
            while( !request.eof() ) request.sbumpc();
        
        if( error_info.response_code.code < 300 )
            return;
        
        nlj::json error_object;
        error_object[ "message" ] = error_info.message;
        std::string error_json = error_object.dump();
        
        show::response response(
            request.connection(),
            show::HTTP_1_1,
            error_info.response_code,
            {
                server_header,
                { "Content-Type", { "application/json" } },
                { "Content-Length", {
                    std::to_string( error_json.size() )
                } }
            }
        );
        
        response.sputn( error_json.c_str(), error_json.size() );
    }
}

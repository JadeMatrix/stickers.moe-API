#include "routing.hpp"

#include "handlers.hpp"
#include "../common/logging.hpp"

#include <exception>
#include <map>


namespace stickers
{
    void route_request( show::request& request )
    {
        show::response_code error_code{ 400, "Bad Request" };
        
        try
        {
            if( request.path.size() > 0 )
            {
                if( request.path[ 0 ] == "user" )
                {
                    if( request.method == "GET" )
                    {
                        handlers::get_user( request );
                        return;
                    }
                    // else if( request.method == "POST" )
                    // {
                    //     handlers::create_user( request );
                    //     return;
                    // }
                    // else if( request.method == "PUT" )
                    // {
                    //     handlers::edit_user( request );
                    //     return;
                    // }
                    // else if( request.method == "DELETE" )
                    // {
                    //     handlers::delete_user( request );
                    //     return;
                    // }
                    else
                        error_code = { 405, "Method Not Allowed" };
                }
                else
                    error_code = { 404, "Not Found" };
            }
            else
                error_code = { 404, "Not Found" };
            
            if( !request.unknown_content_length )
                while( !request.eof() ) request.sbumpc();
        }
        catch( const show::client_disconnected& cd )
        {
            throw cd;
        }
        catch( const show::connection_timeout& ct )
        {
            throw ct;
        }
        catch( const std::exception& e )
        {
            error_code = { 500, "Server Error" };
            STICKERS_LOG(
                ERROR,
                "uncaught std::exception in route_request(show::request&): ",
                e.what()
            );
        }
        catch( ... )
        {
            error_code = { 500, "Server Error" };
            STICKERS_LOG(
                ERROR,
                "uncaught non-std::exception in route_request(show::request&)"
            );
        }
        
        std::string error_json = "null";
        
        show::response response(
            request,
            show::HTTP_1_1,
            error_code,
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

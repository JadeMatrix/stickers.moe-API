#include "server.hpp"

#include "../common/config.hpp"
#include "../common/json.hpp"

#include <show.hpp>

#include <list>
#include <thread>


// DEVEL:
#include "../common/user.hpp"


namespace
{
    const show::headers_t::value_type server_header = {
        "Server",
        {
            show::version.name
            + " v"
            + show::version.string
        }
    };
    
    std::list< std::thread > workers;
    
    void handle_request( show::request&& request )
    {
        if( !request.unknown_content_length )
            while( !request.eof() ) request.sbumpc();
        
        stickers::user_info info = stickers::load_user(
            stickers::BIGID_MIN
        );
        nlj::json user = {
            { "user_id", ( long long )( stickers::BIGID_MIN ) },
            { "password", {
                // { "type",   info.password.type   },
                { "hash",   info.password.hash   },
                { "salt",   info.password.salt   },
                { "factor", info.password.factor },
            } },
            { "created", info.created },
            { "revised", info.revised },
            { "display_name", info.display_name },
            { "real_name", info.real_name },
            // { "avatar_hash", info.avatar_hash },
            { "email", info.email }
        };
        switch( info.password.type )
        {
        case stickers::BCRYPT:
            user[ "password" ][ "type" ] = "bcrypt";
        default:
            user[ "password" ][ "type" ] = nullptr;
        }
        if( info.real_name == "" )
            user[ "real_name" ] = nullptr;
        // if( info.avatar_hash == "" )
        //     user[ "avatar_hash" ] = nullptr;
        user[ "user_id" ] = ( long long )( stickers::BIGID_MIN );
        
        std::string user_json = user.dump();
        
        show::response response(
            request,
            show::HTTP_1_0,
            { 200, "OK" },
            {
                server_header,
                { "Content-Type", { "application/json" } },
                { "Content-Length", {
                    std::to_string( user_json.size() )
                } }
            }
        );
        
        response.sputn( user_json.c_str(), user_json.size() );
    }
    
    void handle_connection( show::connection* connection )
    {
        connection -> timeout(
            stickers::config()[ "server" ][ "wait_for_connection" ]
        );
        
        while( true )
            try
            {
                handle_request( show::request( *connection ) );
            }
            catch( show::client_disconnected& cd )
            {
                break;
            }
            catch( show::connection_timeout& ct )
            {
                break;
            }
            catch( std::exception& e )
            {
                std::cerr
                    << "uncaught exception in handle_connection(): "
                    << e.what()
                    << std::endl
                ;
            }
    }
}


namespace stickers
{
    void run_server()
    {
        show::server server(
            stickers::config()[ "server" ][ "host" ],
            stickers::config()[ "server" ][ "port" ],
            stickers::config()[ "server" ][ "wait_for_connection" ]
        );
        
        while( true )
        {
            try
            {
                workers.push_back( std::thread(
                    handle_connection,
                    new show::connection( server.serve() )
                ) );
            }
            catch( show::connection_timeout& ct ) {}
            
            // Clean up any finished workers
            auto iter = workers.begin();
            while( iter != workers.end() )
                if( iter -> joinable() )
                {
                    iter -> join();
                    workers.erase( iter++ );
                }
                else
                    ++iter;
        }
    }
}

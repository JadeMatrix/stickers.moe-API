#include "server.hpp"

#include "../common/config.hpp"
#include "../common/formatting.hpp"
#include "../common/json.hpp"

#include <show.hpp>

#include <list>
#include <sstream>
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
    
    std::mutex worker_count_mutex;
    long long  worker_count = 0;
    
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
        // Really?
        std::stringstream worker_id;
        worker_id << std::this_thread::get_id();
        
        if( stickers::log_level() >= stickers::VERBOSE )
            ff::writeln(
                std::cout,
                "handling connection from ",
                connection -> client_address,
                " with worker ",
                worker_id.str()
            );
        
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
                if( stickers::log_level() >= stickers::VERBOSE )
                    ff::writeln(
                        std::cout,
                        "client ",
                        connection -> client_address,
                        " disconnected, closing connection"
                    );
                break;
            }
            catch( show::connection_timeout& ct )
            {
                if( stickers::log_level() >= stickers::VERBOSE )
                    ff::writeln(
                        std::cout,
                        "timed out waiting on client ",
                        connection -> client_address,
                        ", closing connection"
                    );
                break;
            }
            catch( std::exception& e )
            {
                if( stickers::log_level() >= stickers::ERRORS )
                    ff::writeln(
                        std::cerr,
                        "uncaught exception in handle_connection(): ",
                        e.what()
                    );
                break;
            }
        
        delete connection;
        
        {
            std::lock_guard< std::mutex > guard( worker_count_mutex );
            --worker_count;
        }
        
        if( stickers::log_level() >= stickers::VERBOSE )
            ff::writeln(
                std::cout,
                "cleaning up worker ",
                worker_id.str()
            );
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
                auto connection = new show::connection( server.serve() );
                
                std::lock_guard< std::mutex > guard( worker_count_mutex );
                ++worker_count;
                
                std::thread worker(
                    handle_connection,
                    connection
                );
                worker.detach();
            }
            catch( show::connection_timeout& ct )
            {
                if( stickers::log_level() >= stickers::VERBOSE )
                    ff::writeln(
                        std::cout,
                        "timed out waiting for connection, looping..."
                    );
            }
            
            if( stickers::log_level() >= stickers::VERBOSE )
            {
                std::lock_guard< std::mutex > guard( worker_count_mutex );
                ff::writeln(
                    std::cout,
                    "currently serving ",
                    worker_count,
                    " connections"
                );
            }
        }
    }
}

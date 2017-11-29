#include "server.hpp"

#include "routing.hpp"
#include "../common/config.hpp"
#include "../common/logging.hpp"

#include <show.hpp>

#include <list>
#include <sstream>
#include <thread>


namespace
{
    std::mutex worker_count_mutex;
    long long  worker_count = 0;
    
    void handle_connection( show::connection* connection )
    {
        // Really?
        std::stringstream worker_id;
        worker_id << std::this_thread::get_id();
        
        STICKERS_LOG(
            VERBOSE,
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
                stickers::route_request( show::request( *connection ) );
            }
            catch( show::client_disconnected& cd )
            {
                STICKERS_LOG(
                    VERBOSE,
                    "client ",
                    connection -> client_address,
                    " disconnected, closing connection"
                );
                break;
            }
            catch( show::connection_timeout& ct )
            {
                STICKERS_LOG(
                    VERBOSE,
                    "timed out waiting on client ",
                    connection -> client_address,
                    ", closing connection"
                );
                break;
            }
            catch( std::exception& e )
            {
                STICKERS_LOG(
                    ERROR,
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
        
        STICKERS_LOG(
            VERBOSE,
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
                STICKERS_LOG(
                    VERBOSE,
                    "timed out waiting for connection, looping..."
                );
            }
            
            if( stickers::log_level() >= stickers::VERBOSE )
            {
                std::lock_guard< std::mutex > guard( worker_count_mutex );
                STICKERS_LOG(
                    VERBOSE,
                    "currently serving ",
                    worker_count,
                    " connections"
                );
            }
        }
    }
}

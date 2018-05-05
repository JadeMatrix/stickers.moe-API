#line 2 "server/server.cpp"


#include "server.hpp"

#include "routing.hpp"
#include "../common/config.hpp"
#include "../common/timestamp.hpp"
#include "../common/logging.hpp"

#include <show.hpp>
#include <show/constants.hpp>

#include <list>
#include <mutex>
#include <sstream>
#include <thread> 


// Request global time handling ------------------------------------------------


namespace
{
    thread_local stickers::timestamp request_time;
    
    void set_request_time_to_now()
    {
        request_time = stickers::current_timestamp();
    }
}

namespace stickers
{
    const timestamp& now()
    {
        return request_time;
    }
}


// Connection workers ----------------------------------------------------------


namespace
{
    std::mutex worker_count_mutex;
    unsigned long long worker_count{ 0 };
    
    void handle_connection( show::connection* connection )
    {
        std::stringstream worker_id;
        worker_id << std::this_thread::get_id();
        
        STICKERS_LOG(
            stickers::log_level::VERBOSE,
            "handling connection from ",
            connection -> client_address(),
            " with worker ",
            worker_id.str()
        );
        
        connection -> timeout(
            stickers::config()[ "server" ][ "wait_for_connection" ]
        );
        
        while( true )
            try
            {
                show::request request{ *connection };
                
                set_request_time_to_now();
                
                stickers::route_request( request );
                
                // HTTP/1.1 support
                auto connection_header = request.headers().find(
                    "Connection"
                );
                if(
                    connection_header != request.headers().end()
                    && connection_header -> second.size() == 1
                )
                {
                    const std::string& ch_val{
                        connection_header -> second[ 0 ]
                    };
                    if( ch_val == "keep-alive" )
                        continue;
                    else if( ch_val == "close" )
                        break;
                }
                if( request.protocol() <= show::HTTP_1_0 )
                    break;
            }
            catch( const show::request_parse_error& rpe )
            {
                STICKERS_LOG(
                    stickers::log_level::INFO,
                    "client ",
                    connection -> client_address(),
                    " sent a malformed request"
                );
                
                show::response response{
                    *connection,
                    show::HTTP_1_1,
                    show::code::BAD_REQUEST,
                    {
                        show::server_header,
                        { "Content-Length", { "0" } }
                    }
                };
                
                break;
            }
            catch( const show::connection_interrupted& ci )
            {
                STICKERS_LOG(
                    stickers::log_level::VERBOSE,
                    "connection to client ",
                    connection -> client_address(),
                    (
                        " interrupted (client disconnected or timed out), "
                        "closing connection"
                    )
                );
                break;
            }
            catch( const std::exception& e )
            {
                STICKERS_LOG(
                    stickers::log_level::ERROR,
                    "uncaught exception in handle_connection(): ",
                    e.what()
                );
                break;
            }
        
        delete connection;
        
        {
            std::lock_guard< std::mutex > guard{ worker_count_mutex };
            --worker_count;
        }
        
        STICKERS_LOG(
            stickers::log_level::VERBOSE,
            "cleaning up worker ",
            worker_id.str()
        );
    }
}

namespace stickers
{
    void run_server()
    {
        show::server server{
            stickers::config()[ "server" ][ "host" ],
            stickers::config()[ "server" ][ "port" ],
            stickers::config()[ "server" ][ "wait_for_connection" ]
        };
        
        while( true )
        {
            try
            {
                auto connection = new show::connection{ server.serve() };
                
                std::lock_guard< std::mutex > guard{ worker_count_mutex };
                ++worker_count;
                
                std::thread worker{
                    handle_connection,
                    connection
                };
                worker.detach();
            }
            catch( const show::connection_timeout& ct )
            {
                STICKERS_LOG(
                    log_level::VERBOSE,
                    "timed out waiting for connection, looping..."
                );
            }
            
            if( current_log_level() >= log_level::VERBOSE )
            {
                std::lock_guard< std::mutex > guard{ worker_count_mutex };
                if( worker_count > 0 )
                    STICKERS_LOG(
                        log_level::VERBOSE,
                        "currently serving ",
                        worker_count,
                        " connections"
                    );
            }
        }
    }
}

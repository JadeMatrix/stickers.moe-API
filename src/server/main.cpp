#line 2 "server/main.cpp"


#include "server.hpp"
#include "../common/config.hpp"
#include "../common/json.hpp"
#include "../common/logging.hpp"

#include <fstream>
#include <iostream>
#include <cstdlib>  // std::srand()
#include <ctime>    // std::time()


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "usage: ",
            argv[ 0 ],
            " config.json"
        );
        return -1;
    }
    
    std::srand( std::time( nullptr ) );
    
    try
    {
        {
            nlj::json config;
            std::ifstream config_file{ argv[ 1 ] };
            
            if( config_file.is_open() )
            {
                config_file >> config;
                if( !config_file.good() )
                {
                    STICKERS_LOG(
                        stickers::log_level::ERROR,
                        "config file ",
                        argv[ 1 ],
                        " not a valid JSON file"
                    );
                    return -1;
                }
            }
            else
            {
                STICKERS_LOG(
                    stickers::log_level::ERROR,
                    "could not open config file ",
                    argv[ 1 ]
                );
                return -1;
            }
            
            stickers::set_config( config );
        }
        
        stickers::run_server();
    }
    catch( const std::exception &e )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "uncaught std::exception in main(): ",
            e.what()
        );
        return -1;
    }
    catch( ... )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "uncaught non-std::exception in main()"
        );
        return -1;
    }
    
    return 0;
}

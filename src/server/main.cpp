#include <fstream>
#include <iostream>

#include "server.hpp"
#include "../common/config.hpp"
#include "../common/logging.hpp"
#include "../common/json.hpp"


////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        STICKERS_LOG(
            ERRORS,
            "usage: ",
            argv[ 0 ],
            " config.json"
        );
        return 1;
    }
    
    try
    {
        {
            nlj::json config;
            std::ifstream config_file( argv[ 1 ] );
            
            if( config_file.is_open() )
                config_file >> config;
            else
            {
                STICKERS_LOG(
                    ERRORS,
                    "could not open config file ",
                    argv[ 1 ]
                );
                return 2;
            }
            
            stickers::set_config( config );
        }
        
        // DEBUG:
        STICKERS_LOG(
            DEBUG,
            "config: ",
            stickers::config().dump()
        );
        
        stickers::run_server();
    }
    catch( const std::exception &e )
    {
        STICKERS_LOG(
            ERRORS,
            "uncaught exception in main(): ",
            e.what()
        );
        return -1;
    }
    
    return 0;
}

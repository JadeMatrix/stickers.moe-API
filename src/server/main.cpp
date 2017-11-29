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
            ERROR,
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
                    ERROR,
                    "could not open config file ",
                    argv[ 1 ]
                );
                return 2;
            }
            
            stickers::set_config( config );
        }
        
        stickers::run_server();
    }
    catch( const std::exception &e )
    {
        STICKERS_LOG(
            ERROR,
            "uncaught std::exception in main(): ",
            e.what()
        );
        return -1;
    }
    catch( ... )
    {
        STICKERS_LOG(
            ERROR,
            "uncaught non-std::exception in main()"
        );
        return -1;
    }
    
    return 0;
}

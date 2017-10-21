#include <fstream>
#include <iostream>

#include "../common/config.hpp"
#include "../common/formatting.hpp"
#include "../common/json.hpp"

// DEVEL:
#include "../common/user.hpp"


////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        ff::writeln(
            std::cerr,
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
                ff::writeln(
                    std::cerr,
                    "could not open config file ",
                    argv[ 1 ]
                );
                return 2;
            }
            
            stickers::set_config( config );
        }
        
        // // DEBUG:
        ff::writeln(
            std::cout,
            "config: ",
            stickers::config().dump()
        );
        
        // DEVEL:
        stickers::user_info info = stickers::load_user( stickers::BIGID_MIN );
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
        if( info.real_name == "" )
            user[ "real_name" ] = nullptr;
        // if( info.avatar_hash == "" )
        //     user[ "avatar_hash" ] = nullptr;
        user[ "user_id" ] = ( long long )( stickers::BIGID_MIN );
        
        ff::writeln(
            std::cout,
            "user: ",
            user.dump()
        );
    }
    catch( const std::exception &e )
    {
        ff::writeln(
            std::cerr,
            "uncaught exception in main(): ",
            e.what()
        );
        return -1;
    }
    
    return 0;
}

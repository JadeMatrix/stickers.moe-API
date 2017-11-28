#include <fstream>
#include <iostream>

#include <show.hpp>
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
        
        // DEBUG:
        ff::writeln(
            std::cout,
            "config: ",
            stickers::config().dump()
        );
        
        show::server server(
            stickers::config()[ "server" ][ "host"    ],
            stickers::config()[ "server" ][ "port"    ],
            stickers::config()[ "server" ][ "timeout" ]
        );
        
        // DEVEL:
        while( true )
            try
            {
                show::connection connection( server.serve() );
                
                while( true )
                    try
                    {
                        show::request request( connection );
                        
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
                                { "Server", {
                                    show::version.name
                                    + " v"
                                    + show::version.string
                                } },
                                { "Content-Type", { "application/json" } },
                                { "Content-Length", {
                                    std::to_string( user_json.size() )
                                } }
                            }
                        );
                        
                        response.sputn( user_json.c_str(), user_json.size() );
                    }
                    catch( show::client_disconnected& cd )
                    {
                        break;
                    }
                    catch( show::connection_timeout& ct )
                    {
                        break;
                    }
            }
            catch( show::connection_timeout& ct ) {}
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

#include "server.hpp"

#include "../common/config.hpp"
#include "../common/json.hpp"

#include <show.hpp>

// DEVEL:
#include "../common/user.hpp"


namespace stickers
{
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
    
    
    void run_server()
    {
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
                        stickers::handle_request( show::request( connection ) );
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
}

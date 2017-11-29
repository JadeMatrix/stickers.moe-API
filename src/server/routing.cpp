#include "routing.hpp"


// DEVEL:
#include "../common/config.hpp"
#include "../common/json.hpp"
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
}


namespace stickers
{
    void route_request( show::request&& request )
    {
        if( !request.unknown_content_length )
            while( !request.eof() ) request.sbumpc();
        
        user_info info = load_user(
            BIGID_MIN
        );
        nlj::json user = {
            { "user_id", ( long long )( BIGID_MIN ) },
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
        case BCRYPT:
            user[ "password" ][ "type" ] = "bcrypt";
        default:
            user[ "password" ][ "type" ] = nullptr;
        }
        if( info.real_name == "" )
            user[ "real_name" ] = nullptr;
        // if( info.avatar_hash == "" )
        //     user[ "avatar_hash" ] = nullptr;
        // user[ "user_id" ] = ( long long )( BIGID_MIN );
        
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
}

#include "handlers.hpp"

#include "routing.hpp"

// DEVEL:
#include "../common/config.hpp"
#include "../common/json.hpp"
#include "../common/user.hpp"


namespace stickers
{
    show::response_code handlers::get_user( show::request& request )
    {
        // TODO: get user with specified ID
        
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
            show::HTTP_1_1,
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
        
        return { 200, "OK" };
    }
    
    show::response_code handlers::create_user( show::request& request )
    {
        return { 500, "Not Implemented" };
    }
    
    show::response_code handlers::edit_user( show::request& request )
    {
        return { 500, "Not Implemented" };
    }
    
    show::response_code handlers::delete_user( show::request& request )
    {
        return { 500, "Not Implemented" };
    }
    
}

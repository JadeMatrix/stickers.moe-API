#include "handlers.hpp"

#include "../api/user.hpp"
#include "../common/logging.hpp"
#include "../server/routing.hpp"
#include "../server/server.hpp"

// DEVEL:
#include "../common/config.hpp"
#include "../common/json.hpp"

#line __LINE__ "server/handlers.cpp"


// TODO: move to ../api/


namespace
{
    std::string image_hash_to_image( stickers::sha256 hash )
    {
        std::string hash_hex = hash.hex_digest();
        return (
              "/images/"
            + hash_hex.substr( 0, 2 )
            + "/"
            + hash_hex.substr( 2, 2 )
            + "/"
            + hash_hex
        );
    }
}


namespace stickers
{
    void handlers::create_user( show::request& request )
    {
        // throw handler_exit( { 500, "Not Implemented" }, "Not implemented" );
        
        if( request.path().size() > 1 )
            throw handler_exit( { 404, "Not Found" }, "" );
        
        std::istream request_stream( &request );
        nlj::json details_json;
        
        request_stream >> details_json;
        
        user_info details;
        
        details.password.type   = INVALID;//details_json[ "password" ][ "type"   ];
        details.password.hash   = details_json[ "password" ][ "hash"   ];
        details.password.salt   = details_json[ "password" ][ "salt"   ];
        details.password.factor = details_json[ "password" ][ "factor" ];
        
        details.created      = now();
        details.display_name = details_json[ "display_name" ];
        details.email        = details_json[ "email"        ];
        
        user created_user = create_user(
            details,
            {
                BIGID_MIN,
                "user signup",
                now(),
                request.client_address()
            }
        );
        
        details_json = {
            { "user_id", created_user.id },
            { "password", {
                { "hash",   created_user.info.password.hash   },
                { "salt",   created_user.info.password.salt   },
                { "factor", created_user.info.password.factor },
            } },
            { "created", to_iso8601_str( created_user.info.created ) },
            { "revised", to_iso8601_str( created_user.info.revised ) },
            { "display_name", created_user.info.display_name },
            { "email", created_user.info.email }
        };
        switch( created_user.info.password.type )
        {
        case BCRYPT:
            details_json[ "password" ][ "type" ] = "bcrypt";
        default:
            details_json[ "password" ][ "type" ] = nullptr;
        }
        
        if( created_user.info.real_name )
            details_json[ "real_name" ] = *created_user.info.real_name;
        else
            details_json[ "real_name" ] = nullptr;
        
        if( created_user.info.avatar_hash )
            details_json[ "avatar" ] = image_hash_to_image(
                *created_user.info.avatar_hash
            );
        else
            details_json[ "avatar" ] = nullptr;
        
        std::string user_json = details_json.dump();
        
        show::response response(
            request.connection(),
            show::HTTP_1_1,
            { 201, "Created" },
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
    
    void handlers::get_user( show::request& request )
    {
        if( request.path().size() < 2 )
            throw handler_exit( { 404, "Not Found" }, "need a user ID" );
        
        stickers::bigid user_id( BIGID_MIN );
        
        try
        {
            user_id = std::stoll( request.path()[ 1 ] );
        }
        catch( const std::exception& e )
        {
            throw handler_exit( { 404, "Not Found" }, "need a user ID" );
        }
        
        try
        {
            user_info info = load_user( user_id );
            
            nlj::json user = {
                { "user_id", user_id },
                // {
                //     "user_id",
                //     show::base64_encode( std::string( ( char* )( &user_id_ll ), sizeof( user_id_ll ) ) )
                // },
                { "password", {
                    // { "type",   info.password.type   },
                    { "hash",   info.password.hash   },
                    { "salt",   info.password.salt   },
                    { "factor", info.password.factor },
                } },
                { "created", to_iso8601_str( info.created ) },
                { "revised", to_iso8601_str( info.revised ) },
                { "display_name", info.display_name },
                { "email", info.email }
            };
            
            switch( info.password.type )
            {
            case BCRYPT:
                user[ "password" ][ "type" ] = "bcrypt";
            default:
                user[ "password" ][ "type" ] = nullptr;
            }
            
            if( info.real_name )
                user[ "real_name" ] = *info.real_name;
            else
                user[ "real_name" ] = nullptr;
            
            if( info.avatar_hash )
                user[ "avatar" ] = image_hash_to_image( *info.avatar_hash );
            else
                user[ "avatar" ] = nullptr;
            
            std::string user_json = user.dump();
            
            show::response response(
                request.connection(),
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
        }
        catch( const no_such_user& nsu )
        {
            throw handler_exit( { 404, "Not Found" }, "no such user" );
        }
    }
    
    void handlers::edit_user( show::request& request )
    {
        throw handler_exit( { 500, "Not Implemented" }, "Not implemented" );
    }
    
    void handlers::delete_user( show::request& request )
    {
        if( request.path().size() > 2 )
            throw handler_exit( { 404, "Not Found" }, "need a user ID" );
        
        stickers::bigid user_id( BIGID_MIN );
        
        try
        {
            user_id = std::stoll( request.path()[ 1 ] );
        }
        catch( const std::exception& e )
        {
            throw handler_exit( { 404, "Not Found" }, "need a user ID" );
        }
        
        try
        {
            delete_user(
                user_id,
                {
                    // DEVEL:
                    BIGID_MIN,
                    "delete user handler",
                    now(),
                    request.client_address()
                }
            );
            
            std::string null_json = "null";
            
            show::response response(
                request.connection(),
                show::HTTP_1_1,
                { 200, "OK" },
                {
                    server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( null_json.size() )
                    } }
                }
            );
            
            response.sputn( null_json.c_str(), null_json.size() );
        }
        catch( const no_such_user& nsu )
        {
            throw handler_exit( { 404, "Not Found" }, "no such user" );
        }
    }
}

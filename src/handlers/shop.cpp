#line 2 "handlers/shop.cpp"


#include "handlers.hpp"

#include "../api/shop.hpp"
#include "../common/auth.hpp"
#include "../common/json.hpp"
#include "../server/parse.hpp"
#include "../server/server.hpp"


namespace
{
    void shop_to_json(
        const stickers::bigid    & id,
        const stickers::shop_info& info,
        nlj::json                & shop_json
    )
    {
        shop_json = {
            { "shop_id"        , id                                       },
            { "created"        , stickers::to_iso8601_str( info.created ) },
            { "revised"        , stickers::to_iso8601_str( info.revised ) },
            { "name"           , info.name                                },
            { "url"            , info.url                                 },
            { "founded"        , nullptr                                  },
            { "closed"         , nullptr                                  },
            { "owner_person_id", info.owner_person_id                     }
        };
        
        if( info.founded )
            shop_json[ "founded" ] = stickers::to_iso8601_str( *info.founded );
        if( info.closed )
            shop_json[ "closed" ] = stickers::to_iso8601_str( *info.closed );
    }
    
    stickers::shop_info shop_info_from_json( const nlj::json& details_json )
    {
        for( const auto& field : {
            "name",
            "url",
            "owner_person_id"
        } )
            if( details_json.find( field ) == details_json.end() )
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !details_json[ field ].is_string() )
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string"
                };
        
        for( const auto& field : {
            "founded",
            "closed"
        } )
            if( details_json.find( field ) == details_json.end() )
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if(
                   !details_json[ field ].is_null  ()
                && !details_json[ field ].is_string()
            )
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string or null"
                };
        
        auto owner_person_id = stickers::bigid::MIN();
        try
        {
            owner_person_id = stickers::bigid::from_string(
                details_json[ "owner_person_id" ].get< std::string >()
            );
        }
        catch( const std::exception& e )
        {
            throw stickers::handler_exit{
                { 400, "Bad Request" },
                "required field \"owner_person_id\" not a valid ID"
            };
        }
        
        std::optional< stickers::timestamp > founded;
        if( !details_json[ "founded" ].is_null() )
            try
            {
                founded = stickers::from_iso8601_str(
                    details_json[ "founded" ].get< std::string >()
                );
            }
            catch( const std::invalid_argument& e )
            {
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "required field \"founded\" not a valid timestamp"
                };
            }
        
        std::optional< stickers::timestamp > closed;
        if( !details_json[ "closed" ].is_null() )
            try
            {
                closed = stickers::from_iso8601_str(
                    details_json[ "closed" ].get< std::string >()
                );
            }
            catch( const std::invalid_argument& e )
            {
                throw stickers::handler_exit{
                    { 400, "Bad Request" },
                    "required field \"closed\" not a valid timestamp"
                };
            }
        
        return {
            stickers::now(),
            stickers::now(),
            details_json[ "name" ].get< std::string >(),
            details_json[ "url"  ].get< std::string >(),
            owner_person_id,
            founded,
            closed
        };
    }
}


namespace stickers
{
    void handlers::create_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto created = create_shop(
            shop_info_from_json( parse_request_content( request ) ),
            {
                auth.user_id,
                "create shop handler",
                now(),
                request.client_address()
            }
        );
        
        nlj::json shop_json;
        shop_to_json( created.id, created.info, shop_json );
        auto shop_json_string = shop_json.dump();
        
        show::response response{
            request.connection(),
            show::HTTP_1_1,
            { 201, "Created" },
            {
                server_header,
                { "Content-Type", { "application/json" } },
                { "Content-Length", {
                    std::to_string( shop_json_string.size() )
                } },
                { "Location", {
                    "/shop/" + static_cast< std::string >( created.id )
                } }
            }
        };
        
        response.sputn(
            shop_json_string.c_str(),
            shop_json_string.size()
        );
    }
    
    void handlers::get_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_shop_id_variable = variables.find( "shop_id" );
        if( found_shop_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a shop ID" };
        
        stickers::bigid shop_id{ bigid::MIN() };
        try
        {
            shop_id = bigid::from_string(
                found_shop_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                { 404, "Not Found" },
                "need a valid shop ID"
            };
        }
        
        try
        {
            auto info = load_shop( shop_id );
            
            nlj::json shop_json;
            shop_to_json( shop_id, info, shop_json );
            auto shop_json_string = shop_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                { 200, "OK" },
                {
                    server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( shop_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                shop_json_string.c_str(),
                shop_json_string.size()
            );
        }
        catch( const no_such_shop& e )
        {
            throw handler_exit{ { 404, "Not Found" }, "no such shop" };
        }
    }
    
    void handlers::edit_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_shop_id_variable = variables.find( "shop_id" );
        if( found_shop_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a shop ID" };
        
        stickers::bigid shop_id{ bigid::MIN() };
        try
        {
            shop_id = bigid::from_string(
                found_shop_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                { 404, "Not Found" },
                "need a valid shop ID"
            };
        }
        
        try
        {
            auto updated_info = update_shop(
                {
                    shop_id,
                    shop_info_from_json( parse_request_content( request ) )
                },
                {
                    auth.user_id,
                    "update shop handler",
                    now(),
                    request.client_address()
                }
            );
            
            nlj::json shop_json;
            shop_to_json( shop_id, updated_info, shop_json );
            auto shop_json_string = shop_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                { 200, "OK" },
                {
                    server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( shop_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                shop_json_string.c_str(),
                shop_json_string.size()
            );
        }
        catch( const no_such_shop& e )
        {
            throw handler_exit{ { 404, "Not Found" }, "no such shop" };
        }
    }
    
    void handlers::delete_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_shop_id_variable = variables.find( "shop_id" );
        if( found_shop_id_variable == variables.end() )
            throw handler_exit{ { 404, "Not Found" }, "need a shop ID" };
        
        stickers::bigid shop_id{ bigid::MIN() };
        try
        {
            shop_id = bigid::from_string(
                found_shop_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                { 404, "Not Found" },
                "need a valid shop ID"
            };
        }
        
        delete_shop(
            shop_id,
            {
                auth.user_id,
                "delete shop handler",
                now(),
                request.client_address()
            }
        );
        
        std::string null_json = "null";
        
        show::response response{
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
        };
        
        response.sputn( null_json.c_str(), null_json.size() );
    }
}

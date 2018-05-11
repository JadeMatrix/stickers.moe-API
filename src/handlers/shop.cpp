#line 2 "handlers/shop.cpp"


#include "handlers.hpp"

#include "../api/shop.hpp"
#include "../common/auth.hpp"
#include "../common/crud.hpp"
#include "../common/json.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>


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
    
    stickers::shop_info shop_info_from_document(
        const stickers::document& details_doc
    )
    {
        if( !details_doc.is_a< stickers::map_document >() )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "invalid data format"
            };
        
        auto& details_map{ details_doc.get< stickers::map_document >() };
        
        for( const auto& field : {
            std::string{ "name"            },
            std::string{ "url"             },
            std::string{ "owner_person_id" }
        } )
            if( details_map.find( field ) == details_map.end() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !details_map[ field ].is_a< stickers::string_document >() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string"
                };
        
        for( const auto& field : {
            std::string{ "founded" },
            std::string{ "closed"  }
        } )
            if( details_map.find( field ) == details_map.end() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if(
                   !details_map[ field ].is_a< stickers::  null_document >()
                && !details_map[ field ].is_a< stickers::string_document >()
            )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string or null"
                };
        
        auto owner_person_id = stickers::bigid::MIN();
        try
        {
            owner_person_id = stickers::bigid::from_string(
                details_map[ "owner_person_id" ].get<
                    stickers::string_document
                >()
            );
        }
        catch( const std::exception& e )
        {
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "required field \"owner_person_id\" not a valid ID"
            };
        }
        
        std::optional< stickers::timestamp > founded;
        if( !details_map[ "founded" ].is_a< stickers::null_document >() )
        {
            bool valid{ false };
            
            if( details_map[ "founded" ].is_a< stickers::string_document >() )
                try
                {
                    founded = stickers::from_iso8601_str(
                        details_map[ "founded" ].get<
                            stickers::string_document
                        >()
                    );
                    valid = true;
                }
                catch( const std::invalid_argument& e ) {}
            
            if( !valid )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \"founded\" not a valid ISO 8601 timestamp"
                };
        }
        
        std::optional< stickers::timestamp > closed;
        if( !details_map[ "closed" ].is_a< stickers::null_document >() )
        {
            bool valid{ false };
            
            if( details_map[ "closed" ].is_a< stickers::string_document >() )
                try
                {
                    closed = stickers::from_iso8601_str(
                        details_map[ "closed" ].get<
                            stickers::string_document
                        >()
                    );
                    valid = true;
                }
                catch( const std::invalid_argument& e ) {}
            
            if( !valid )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \"closed\" not a valid ISO 8601 timestamp"
                };
        }
        
        return {
            stickers::now(),
            stickers::now(),
            details_map[ "name" ].get< stickers::string_document >(),
            details_map[ "url"  ].get< stickers::string_document >(),
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
        
        try
        {
            auto created = create_shop(
                shop_info_from_document( parse_request_content( request ) ),
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
                show::code::CREATED,
                {
                    show::server_header,
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
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::get_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_shop_id_variable = variables.find( "shop_id" );
        if( found_shop_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a shop ID" };
        
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
                show::code::NOT_FOUND,
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
                show::code::OK,
                {
                    show::server_header,
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
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
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
            throw handler_exit{ show::code::NOT_FOUND, "need a shop ID" };
        
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
                show::code::NOT_FOUND,
                "need a valid shop ID"
            };
        }
        
        try
        {
            auto updated_info = update_shop(
                {
                    shop_id,
                    shop_info_from_document( parse_request_content( request ) )
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
                show::code::OK,
                {
                    show::server_header,
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
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
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
            throw handler_exit{ show::code::NOT_FOUND, "need a shop ID" };
        
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
                show::code::NOT_FOUND,
                "need a valid shop ID"
            };
        }
        
        try
        {
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
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( null_json.size() )
                    } }
                }
            };
            
            response.sputn( null_json.c_str(), null_json.size() );
        }
        catch( const no_such_shop& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
}

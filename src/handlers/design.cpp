#line 2 "handlers/design.cpp"


#include "handlers.hpp"

#include "../api/design.hpp"
#include "../common/auth.hpp"
#include "../common/crud.hpp"
#include "../common/json.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>

#include <utility>  // std::move<>()


namespace
{
    void design_to_json(
        const stickers::bigid      & id,
        const stickers::design_info& info,
        nlj::json                  & design_json
    )
    {
        auto       images_array{ nlj::json::array() };
        auto contributors_array{ nlj::json::array() };
        
        for( auto& image_hash : info.images )
            images_array.push_back( image_hash.hex_digest() );
        for( auto& contributor_id : info.contributors )
            contributors_array.push_back( contributor_id );
        
        design_json = {
            { "design_id"   , id                                       },
            { "created"     , stickers::to_iso8601_str( info.created ) },
            { "revised"     , stickers::to_iso8601_str( info.revised ) },
            { "description" , info.description                         },
            { "images"      , images_array                             },
            { "contributors", contributors_array                       }
        };
    }
    
    stickers::design_info design_info_from_document(
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
            std::string{ "description" }
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
            std::string{ "images"       },
            std::string{ "contributors" }
        } )
            if( details_map.find( field ) == details_map.end() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !details_map[ field ].is_a< stickers::map_document >() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be an array"
                };
        
        std::vector< stickers::sha256 > images;
        std::vector< stickers::bigid  > contributors;
        
        for( auto& pair : details_map[ "images" ].get<
            stickers::map_document
        >() )
            try
            {
                images.emplace_back(
                    stickers::sha256::from_hex_string(
                        pair.second.get< stickers::string_document >()
                    )
                );
            }
            catch( const stickers::hash_error& e )
            {
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    (
                        "missing required field \"images\" must be an array of"
                        " SHA-256 hashes"
                    )
                };
            }
        for( auto& pair : details_map[ "contributors" ].get<
            stickers::map_document
        >() )
            try
            {
                contributors.emplace_back(
                    stickers::bigid::from_string(
                        pair.second.get< stickers::string_document >()
                    )
                );
            }
            catch( const std::invalid_argument& e )
            {
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    (
                        "missing required field \"contributors\" must be an"
                        " array of bigids"
                    )
                };
            }
        
        return {
            stickers::now(),
            stickers::now(),
            details_map[ "description" ].get< stickers::string_document >(),
            std::move( images       ),
            std::move( contributors )
        };
    }
}


namespace stickers
{
    void handlers::create_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth{ authenticate( request ) };
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        try
        {
            auto created{ create_design(
                design_info_from_document( parse_request_content( request ) ),
                {
                    auth.user_id,
                    "create design handler",
                    now(),
                    request.client_address()
                }
            ) };
            
            nlj::json design_json;
            design_to_json( created.id, created.info, design_json );
            auto design_json_string{ design_json.dump() };
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::CREATED,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( design_json_string.size() )
                    } },
                    { "Location", {
                        "/design/" + static_cast< std::string >( created.id )
                    } }
                }
            };
            
            response.sputn(
                design_json_string.c_str(),
                design_json_string.size()
            );
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::get_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_design_id_variable{ variables.find( "design_id" ) };
        if( found_design_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a design ID" };
        
        auto design_id{ bigid::MIN() };
        try
        {
            design_id = bigid::from_string(
                found_design_id_variable -> second
            );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid design ID"
            };
        }
        
        try
        {
            auto info{ load_design( design_id ) };
            
            nlj::json design_json;
            design_to_json( design_id, info, design_json );
            auto design_json_string{ design_json.dump() };
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( design_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                design_json_string.c_str(),
                design_json_string.size()
            );
        }
        catch( const no_such_design& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
    
    void handlers::edit_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth{ authenticate( request ) };
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_design_id_variable{ variables.find( "design_id" ) };
        if( found_design_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a design ID" };
        
        stickers::bigid design_id{ bigid::MIN() };
        try
        {
            design_id = bigid::from_string(
                found_design_id_variable -> second
            );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid design ID"
            };
        }
        
        try
        {
            auto updated_info{ update_design(
                {
                    design_id,
                    design_info_from_document(
                        parse_request_content( request )
                    )
                },
                {
                    auth.user_id,
                    "update design handler",
                    now(),
                    request.client_address()
                }
            ) };
            
            nlj::json design_json;
            design_to_json( design_id, updated_info, design_json );
            auto design_json_string = design_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( design_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                design_json_string.c_str(),
                design_json_string.size()
            );
        }
        catch( const no_such_design& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::delete_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth{ authenticate( request ) };
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_design_id_variable{ variables.find( "design_id" ) };
        if( found_design_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a design ID" };
        
        stickers::bigid design_id{ bigid::MIN() };
        try
        {
            design_id = bigid::from_string(
                found_design_id_variable -> second
            );
        }
        catch( const std::invalid_argument& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid design ID"
            };
        }
        
        try
        {
            delete_design(
                design_id,
                {
                    auth.user_id,
                    "delete design handler",
                    now(),
                    request.client_address()
                }
            );
            
            std::string null_json{ "null" };
            
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
        catch( const no_such_design& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
}

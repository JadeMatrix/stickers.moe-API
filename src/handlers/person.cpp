#line 2 "handlers/person.cpp"


#include "handlers.hpp"

#include "../api/person.hpp"
#include "../common/auth.hpp"
#include "../common/crud.hpp"
#include "../common/json.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>


namespace
{
    void person_to_json(
        const stickers::bigid      & id,
        const stickers::person_info& info,
        nlj::json                  & person_json
    )
    {
        person_json = {
            { "person_id", id                                       },
            { "created"  , stickers::to_iso8601_str( info.created ) },
            { "revised"  , stickers::to_iso8601_str( info.revised ) },
            { "about"    , info.about                               }
        };
        
        if( info.has_user() )
        {
            person_json[ "name"    ] = nullptr;
            person_json[ "user_id" ] = std::get< stickers::bigid >(
                info.identifier
            );
        }
        else
        {
            person_json[ "user_id" ] = nullptr;
            person_json[ "name"    ] = std::get< std::string >(
                info.identifier
            );
        }
    }
    
    stickers::person_info person_info_from_document(
        const stickers::document& details_doc
    )
    {
        if( !details_doc.is_a< stickers::map_document >() )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "invalid data format"
            };
        
        auto& details_map{ details_doc.get< stickers::map_document >() };
        
        if( details_map.find( "about" ) == details_map.end() )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "missing required field \"about\""
            };
        else if( !details_map[ "about" ].is_a< stickers::string_document >() )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "required field \"about\" must be a string"
            };
        
        bool has_user{ details_map.find( "user_id" ) != details_map.end() };
        if(
            !has_user
            && details_map.find( "name" ) == details_map.end()
        )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "missing one of \"name\" or \"user\""
            };
        
        stickers::person_info info{
            stickers::now(),
            stickers::now(),
            "",
            stickers::bigid::MIN()
        };
        
        if( has_user )
        {
            if( !details_map[ "user_id" ].is_a< stickers::string_document >() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \"user\" must be a string"
                };
            stickers::bigid user_id{ stickers::bigid::MIN() };
            try
            {
                user_id = stickers::bigid::from_string(
                    details_map[ "user_id" ].get< stickers::string_document >()
                );
            }
            catch( const stickers::bigid_out_of_range& e )
            {
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \"user\" is not a valid user ID"
                };
            }
            info = {
                stickers::now(),
                stickers::now(),
                details_map[ "about" ].get< stickers::string_document >(),
                user_id
            };
        }
        else
        {
            if( !details_map[ "name" ].is_a< stickers::string_document >() )
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    "required field \"name\" must be a string"
                };
            info = {
                stickers::now(),
                stickers::now(),
                details_map[ "about" ].get< stickers::string_document >(),
                details_map[ "name"  ].get< stickers::string_document >()
            };
        }
        
        return info;
    }
}


namespace stickers
{
    void handlers::create_person(
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
            auto created = create_person(
                person_info_from_document( parse_request_content( request ) ),
                {
                    auth.user_id,
                    "create person handler",
                    now(),
                    request.client_address()
                }
            );
            
            nlj::json person_json;
            person_to_json( created.id, created.info, person_json );
            auto person_json_string = person_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::CREATED,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( person_json_string.size() )
                    } },
                    { "Location", {
                        "/person/" + static_cast< std::string >( created.id )
                    } }
                }
            };
            
            response.sputn(
                person_json_string.c_str(),
                person_json_string.size()
            );
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::get_person(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_person_id_variable = variables.find( "person_id" );
        if( found_person_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a person ID" };
        
        bigid person_id{ bigid::MIN() };
        try
        {
            person_id = bigid::from_string(
                found_person_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid person ID"
            };
        }
        
        try
        {
            auto info = load_person( person_id );
            
            nlj::json person_json;
            person_to_json( person_id, info, person_json );
            auto person_json_string = person_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( person_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                person_json_string.c_str(),
                person_json_string.size()
            );
        }
        catch( const no_such_person& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
    
    void handlers::edit_person(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_person_id_variable = variables.find( "person_id" );
        if( found_person_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a person ID" };
        
        stickers::bigid person_id{ bigid::MIN() };
        try
        {
            person_id = bigid::from_string(
                found_person_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid person ID"
            };
        }
        
        try
        {
            auto updated_info = update_person(
                {
                    person_id,
                    person_info_from_document(
                        parse_request_content( request )
                    )
                },
                {
                    auth.user_id,
                    "update person handler",
                    now(),
                    request.client_address()
                }
            );
            
            nlj::json person_json;
            person_to_json( person_id, updated_info, person_json );
            auto person_json_string = person_json.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( person_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                person_json_string.c_str(),
                person_json_string.size()
            );
        }
        catch( const no_such_person& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
        catch( const no_such_record_error& e )
        {
            throw handler_exit{ show::code::BAD_REQUEST, e.what() };
        }
    }
    
    void handlers::delete_person(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        auto found_person_id_variable = variables.find( "person_id" );
        if( found_person_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a person ID" };
        
        stickers::bigid person_id{ bigid::MIN() };
        try
        {
            person_id = bigid::from_string(
                found_person_id_variable -> second
            );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid person ID"
            };
        }
        
        try
        {
            delete_person(
                person_id,
                {
                    auth.user_id,
                    "delete person handler",
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
        catch( const no_such_person& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
}

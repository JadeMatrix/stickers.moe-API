#line 2 "handlers/list.cpp"


#include "handlers.hpp"

#include "../api/user.hpp"
#include "../api/list.hpp"
#include "../common/json.hpp"

#include <show/constants.hpp>


namespace stickers
{
    void handlers::get_list(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_user_id_variable = variables.find( "user_id" );
        if( found_user_id_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need a user ID" };
        
        stickers::bigid user_id{ bigid::MIN() };
        try
        {
            user_id = bigid::from_string( found_user_id_variable -> second );
        }
        catch( const std::exception& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, "need a valid user ID" };
        }
        
        try
        {
            nlj::json list = nlj::json::array();
            
            for( auto& item : get_user_list( user_id ) )
                list.push_back( nlj::json{
                    {
                        "product_id",
                        static_cast< std::string >( item.product_id )
                    },
                    { "quantity", item.quantity                  },
                    { "updated" , to_iso8601_str( item.updated ) }
                } );
            
            auto list_json = list.dump();
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( list_json.size() )
                    } }
                }
            };
            
            response.sputn( list_json.c_str(), list_json.size() );
        }
        catch( const no_such_user& nsu )
        {
            throw handler_exit{ show::code::NOT_FOUND, "no such user" };
        }
    }
    
    void handlers::add_list_item(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::update_list_item(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::remove_list_item(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
}

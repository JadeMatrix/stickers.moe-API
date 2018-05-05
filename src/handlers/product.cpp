#line 2 "handlers/product.cpp"


#include "handlers.hpp"

#include <show/constants.hpp>


namespace stickers
{
    void handlers::create_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::get_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::edit_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::delete_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
}

#line 2 "handlers/product.cpp"


#include "handlers.hpp"


namespace stickers
{
    void handlers::create_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::get_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::edit_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::delete_product(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
}

#line 2 "handlers/shop.cpp"


#include "handlers.hpp"


namespace stickers
{
    void handlers::create_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::get_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::edit_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::delete_shop(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
}

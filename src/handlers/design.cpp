#line 2 "handlers/design.cpp"


#include "handlers.hpp"


namespace stickers
{
    void handlers::create_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::get_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::edit_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::delete_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
}

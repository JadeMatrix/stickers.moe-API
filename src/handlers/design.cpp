#line 2 "handlers/design.cpp"


#include "handlers.hpp"

#include <show/constants.hpp>


namespace stickers
{
    void handlers::create_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::get_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::edit_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
    
    void handlers::delete_design(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "" };
    }
}

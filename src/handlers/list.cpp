#line 2 "handlers/list.cpp"


#include "handlers.hpp"


namespace stickers
{
    void handlers::get_list( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::add_list_item( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::update_list_item( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::remove_list_item( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
}

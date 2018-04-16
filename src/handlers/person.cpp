#line 2 "handlers/person.cpp"


#include "handlers.hpp"


namespace stickers
{
    void handlers::create_person( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::get_person( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::edit_person( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
    
    void handlers::delete_person( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "" };
    }
}

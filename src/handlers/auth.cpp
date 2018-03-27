#include "handlers.hpp"


namespace stickers
{
    void handlers::signup( show::request& request )
    {
        throw handler_exit( { 500, "Not Implemented" }, "Not implemented" );
    }
    
    void handlers::login( show::request& request )
    {
        throw handler_exit( { 500, "Not Implemented" }, "Not implemented" );
    }
}

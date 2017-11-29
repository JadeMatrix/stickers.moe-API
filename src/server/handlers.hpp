#pragma once
#ifndef STICKERS_MOE_SERVER_HANDLERS_HPP
#define STICKERS_MOE_SERVER_HANDLERS_HPP


#include <show.hpp>


namespace stickers
{
    namespace handlers
    {
        show::response_code    get_user( show::request& );
        show::response_code create_user( show::request& );
        show::response_code   edit_user( show::request& );
        show::response_code delete_user( show::request& );
    }
}


#endif

#pragma once
#ifndef STICKERS_MOE_SERVER_HANDLERS_HPP
#define STICKERS_MOE_SERVER_HANDLERS_HPP


#include <show.hpp>


namespace stickers
{
    namespace handlers
    {
        void    get_user( show::request& );
        void create_user( show::request& );
        void   edit_user( show::request& );
        void delete_user( show::request& );
    }
}


#endif

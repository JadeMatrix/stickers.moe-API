#pragma once
#ifndef STICKERS_MOE_HANDLERS_HANDLERS_HPP
#define STICKERS_MOE_HANDLERS_HANDLERS_HPP


#include "../server/handler.hpp"


namespace stickers
{
    namespace handlers
    {
        void      signup( show::request& );
        void       login( show::request& );
        
        void create_user( show::request& );
        void    get_user( show::request& );
        void   edit_user( show::request& );
        void delete_user( show::request& );
    }
}


#endif

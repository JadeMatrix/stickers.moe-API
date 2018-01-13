#pragma once
#ifndef STICKERS_MOE_HANDLERS_HANDLERS_HPP
#define STICKERS_MOE_HANDLERS_HANDLERS_HPP


#include <show.hpp>


namespace stickers
{
    namespace handlers
    {
        struct handler_rt
        {
            show::response_code response_code;
            std::string         message;
        };
        
        handler_rt create_user( show::request& );
        handler_rt    get_user( show::request& );
        handler_rt   edit_user( show::request& );
        handler_rt delete_user( show::request& );
    }
}


#endif

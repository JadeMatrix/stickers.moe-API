#pragma once
#ifndef STICKERS_MOE_SERVER_HANDLER_HPP
#define STICKERS_MOE_SERVER_HANDLER_HPP


#include <show.hpp>

#include <functional>   // std::function


namespace stickers
{
    using handler_type = std::function< void( show::request& ) >;
    
    class handler_exit
    {
    public:
        show::response_code response_code;
        std::string         message;
        
        handler_exit(
            show::response_code response_code,
            const std::string&  message
        ) :
            response_code{ response_code },
            message{       message       }
        {}
    };
}


#endif
#pragma once
#ifndef STICKERS_MOE_SERVER_SERVER_HPP
#define STICKERS_MOE_SERVER_SERVER_HPP


#include <show.hpp>


namespace stickers
{
    static const show::headers_type::value_type server_header{
        "Server",
        {
            show::version::name
            + " v"
            + show::version::string
        }
    };
    
    void run_server();
}


#endif

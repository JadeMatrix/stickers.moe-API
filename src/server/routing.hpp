#pragma once
#ifndef STICKERS_MOE_SERVER_ROUTING_HPP
#define STICKERS_MOE_SERVER_ROUTING_HPP


#include <show.hpp>


namespace stickers
{
    static const show::headers_t::value_type server_header = {
        "Server",
        {
            show::version.name
            + " v"
            + show::version.string
        }
    };
    
    void route_request( show::request& );
}


#endif

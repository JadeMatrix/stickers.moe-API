#pragma once
#ifndef STICKERS_MOE_COMMON_DATETIME_HPP
#define STICKERS_MOE_COMMON_DATETIME_HPP


#include <chrono>
#include <string>


namespace stickers
{
    using datetime_type = std::chrono::system_clock::time_point;
    
    const datetime_type& now(); // Implemented in ../server/server.cpp
    
    std::string iso8601_str( const datetime_type& );
}


#endif

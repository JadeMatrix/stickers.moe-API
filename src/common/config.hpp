#pragma once
#ifndef STICKERS_MOE_COMMON_CONFIG_HPP
#define STICKERS_MOE_COMMON_CONFIG_HPP


#include <string>

#include "json.hpp"


namespace stickers
{
    const nlj::json& config();
    
    void  set_config( const nlj::json  & );
    void  set_config( const std::string& );
    // void open_config( const std::string& );
}


#endif

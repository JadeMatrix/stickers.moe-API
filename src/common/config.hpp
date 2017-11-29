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
    
    enum log_level_type
    {
        SILENT  = 00,
        ERROR   = 10,
        WARNING = 20,
        INFO    = 30,
        VERBOSE = 40,
        DEBUG   = 50
    };
    
    log_level_type log_level();
}


#endif

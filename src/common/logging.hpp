#pragma once
#ifndef STICKERS_MOE_COMMON_LOGGING_HPP
#define STICKERS_MOE_COMMON_LOGGING_HPP


#include "config.hpp"
#include "timestamp.hpp"
#include "formatting.hpp"

#include <ctime>
#include <iostream>
#include <sstream>


#define STICKERS_LOG( LEVEL, ... ) \
    if( stickers::log_level() >= stickers::LEVEL ) \
    { \
        ff::writeln( \
            ( stickers::LEVEL == stickers::ERROR ? std::cerr : std::cout ), \
            "[", #LEVEL, "]", \
            "[", stickers::to_iso8601_str( stickers::current_timestamp() ), "]", \
            stickers::log_level() >= stickers::DEBUG ? ( \
                std::string( "[" ) \
                + __FILE__ \
                + ":" \
                + std::to_string( __LINE__ ) \
                + "] " \
            ) : " ", \
            __VA_ARGS__ \
        ); \
    }


#endif

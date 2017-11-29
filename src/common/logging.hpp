#pragma once
#ifndef STICKERS_MOE_COMMON_LOGGING_HPP
#define STICKERS_MOE_COMMON_LOGGING_HPP


#include "config.hpp"
#include "formatting.hpp"

#include <ctime>
#include <iostream>


#define STICKERS_LOG( LEVEL, ... ) \
    if( stickers::log_level() >= stickers::LEVEL ) \
    { \
        char tb[ 256 ]; \
        auto t = std::time( nullptr ); \
        std::strftime( tb, sizeof( tb ), "%F %T%z", std::localtime( &t ) ); \
        ff::writeln( \
            ( stickers::LEVEL == stickers::ERROR ? std::cerr : std::cout ), \
            "[", #LEVEL, "]", \
            "[", tb, "]", \
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

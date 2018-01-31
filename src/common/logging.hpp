#pragma once
#ifndef STICKERS_MOE_COMMON_LOGGING_HPP
#define STICKERS_MOE_COMMON_LOGGING_HPP


#include "config.hpp"
#include "timestamp.hpp"
#include "formatting.hpp"

#include <ctime>
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>


namespace stickers
{
    template< typename... Args > void log(
        log_level_type level,
        std::string file_name,
        long long file_line,
        Args... args
    )
    {
        static const std::map< log_level_type, std::string > level_strings{
            { SILENT , "SILENT"  },
            { ERROR  , "ERROR"   },
            { WARNING, "WARNING" },
            { INFO   , "INFO"    },
            { VERBOSE, "VERBOSE" },
            { DEBUG  , "DEBUG"   }
        };
        
        if( stickers::log_level() >= level )
        {
            auto t = std::time( nullptr );
            std::stringstream time_string;
            time_string << std::put_time(
                std::localtime( &t ),
                "%F %T%z"
            );
            
            std::stringstream thread_id_string;
            thread_id_string << std::this_thread::get_id();
            
            ff::writeln(
                ( level == stickers::ERROR ? std::cerr : std::cout ),
                "[", level_strings.at( level ), "]",
                "[", time_string.str(), "]",
                "[thread ", thread_id_string.str(), "]",
                stickers::log_level() >= stickers::DEBUG ? (
                    std::string( "[" )
                    + file_name
                    + ":"
                    + std::to_string( file_line )
                    + "] "
                ) : "",
                args...
            );
        }
    }
    
    inline std::string log_sanitize( const std::string& raw )
    {
        std::stringstream sanitized;
        sanitized << std::hex;
        for( const auto& c : raw )
            if( isprint( c ) )
                sanitized << c;
            else
                sanitized
                    << "\\x"
                    << std::setw( 2 )
                    << std::setfill( '0' )
                    << ( unsigned int )( unsigned char )c
                ;
        return sanitized.str();
    }
}


#define STICKERS_LOG( LEVEL, ... ) stickers::log( \
    stickers::LEVEL, \
    __FILE__, \
    __LINE__, \
    __VA_ARGS__ \
)


#endif

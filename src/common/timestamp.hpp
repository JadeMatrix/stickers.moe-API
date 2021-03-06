#pragma once
#ifndef STICKERS_MOE_COMMON_TIMESTAMP_HPP
#define STICKERS_MOE_COMMON_TIMESTAMP_HPP


#include "postgres.hpp"

#include <date/date.h>

#include <string>


namespace stickers
{
    using timestamp = date::sys_time< std::chrono::microseconds >;
    
    const timestamp& now(); // Implemented in ../server/server.cpp
    timestamp current_timestamp();
    
    timestamp from_iso8601_str( const std::string&             );
    bool      from_iso8601_str( const std::string&, timestamp& );
    std::string to_iso8601_str( const timestamp& );
    std::string to_http_ts_str( const timestamp& );
    
    timestamp  from_unix_time( unsigned int );
    unsigned int to_unix_time( const timestamp& );
}


// Template specialization of `pqxx::string_traits<>(&)` for
// `stickers::timestamp`, which allows use of `pqxx::field::to<>(&)` and
// `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::timestamp >
    {
        using subject_type = stickers::timestamp;
        
        static constexpr const char* name() noexcept {
            return "stickers::timestamp";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const stickers::timestamp& ) { return false; }
        
        [[noreturn]] static stickers::timestamp null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], stickers::timestamp& ts )
        {
            if( !stickers::from_iso8601_str( std::string{ str } + "00", ts ) )
                throw argument_error{
                    "Failed conversion to "
                    + static_cast< std::string >( name() )
                    + ": '"
                    + static_cast< std::string >( str )
                    + "'"
                };
        }
        
        static std::string to_string( const stickers::timestamp& ts )
        {
            return stickers::to_iso8601_str( ts );
        }
    };
}


#endif

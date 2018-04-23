#line 2 "common/timestamp.cpp"


#include "timestamp.hpp"

#include <date/iso_week.h>

#include <iomanip>
#include <sstream>


namespace stickers
{
    timestamp current_timestamp()
    {
        return std::chrono::system_clock::now();
    }
    
    timestamp from_iso8601_str( const std::string& s )
    {
        timestamp ts;
        if( !from_iso8601_str( s, ts ) )
            throw std::invalid_argument{
                "failed to parse "
                + s
                + " as an ISO 8601 timestamp"
            };
        return ts;
    }
    
    bool from_iso8601_str( const std::string& s, timestamp& ts )
    {
        std::istringstream stream{ s };
        stream >> date::parse( "%F %T%z", ts );
        return !stream.fail();
    }
    
    std::string to_iso8601_str( const timestamp& ts )
    {
        return date::format( "%F %T%z", ts );
    }
    
    std::string to_http_ts_str( const timestamp& ts )
    {
        std::stringstream weekday_abbreviation;
        weekday_abbreviation << static_cast< iso_week::year_weeknum_weekday >(
            std::chrono::time_point_cast< date::days >( ts )
        ).weekday();
        
        return (
            weekday_abbreviation.str()
            // timestamps serialize to UTC/GMT by default
            + date::format(
                " %d-%m-%Y %H:%M:%S GMT",
                std::chrono::time_point_cast< std::chrono::seconds >( ts )
            )
        );
    }
    
    timestamp from_unix_time( unsigned int unix_time )
    {
        return timestamp{ std::chrono::duration_cast<
            std::chrono::microseconds
        >( std::chrono::seconds{ unix_time } ) };
    }
    
    unsigned int to_unix_time( const timestamp& ts )
    {
        return std::chrono::duration_cast<
            std::chrono::seconds
        >( ts.time_since_epoch() ).count();
    }
}

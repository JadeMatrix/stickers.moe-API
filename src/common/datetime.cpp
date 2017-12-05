#include "datetime.hpp"

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
            throw std::runtime_error(
                "failed to parse "
                + s
                + " as an ISO 8601 timestamp"
            );
        return ts;
    }
    
    bool from_iso8601_str( const std::string& s, timestamp& ts )
    {
        std::istringstream stream{ s };
        stream >> date::parse( "%F %T%z", ts );
        return !stream.fail();
    }
}

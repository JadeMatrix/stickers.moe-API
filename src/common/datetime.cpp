#include "datetime.hpp"

#include <iomanip>
#include <sstream>


namespace stickers
{
    std::string iso8601_str( const datetime_type& dt )
    {
        std::stringstream dt_str;
        
        // Localized timestamp w/ timezone offset
        std::time_t local_now_tt = std::chrono::system_clock::to_time_t( dt );
        dt_str << std::put_time( std::localtime( &local_now_tt ), "%F %T%z" );
        
        return dt_str.str();
    }
}

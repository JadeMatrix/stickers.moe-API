#pragma once
#ifndef STICKERS_MOE_AUDIT_UTIL_HPP
#define STICKERS_MOE_AUDIT_UTIL_HPP


#include <stdexcept>
#include <string>
#include <utility>

#include "../common/bigid.hpp"
#include "../common/datetime.hpp"


namespace stickers
{
    namespace audit
    {
        struct blame
        {
            bigid      * who;       // User ID
            std::string* what;      // High-level operation description
            timestamp  * when;      // Request timestamp
            std::string* where;     // IP address
        };
        
        enum blame_field
        {
            WHO,
            WHAT,
            WHEN,
            WHERE
        };
        
        class missing_blame : public std::invalid_argument
        {
        public:
            missing_blame(
                blame_field        field,
                const std::string& operation,
                const blame      & fields
            );
        };
    }
}


#endif

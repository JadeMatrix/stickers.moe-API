#pragma once
#ifndef STICKERS_MOE_AUDIT_BLAME_HPP
#define STICKERS_MOE_AUDIT_BLAME_HPP


#include <stdexcept>
#include <string>
#include <utility>

#include "../common/bigid.hpp"
#include "../common/timestamp.hpp"


namespace stickers
{
    namespace audit
    {
        struct blame
        {
            const bigid       who;      // User ID
            const std::string what;     // High-level operation description
            const timestamp   when;     // Request timestamp
            const std::string where;    // IP address
        };
    }
}


#endif

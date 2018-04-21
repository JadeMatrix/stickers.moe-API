#pragma once
#ifndef STICKERS_MOE_API_LIST_HPP
#define STICKERS_MOE_API_LIST_HPP


#include "../common/bigid.hpp"
#include "../common/timestamp.hpp"

#include <vector>


namespace stickers
{
    struct list_entry
    {
        bigid         product_id;
        unsigned long quantity;
        timestamp     updated;
    };
    
    std::vector< list_entry > get_user_list( const bigid& );
}


#endif

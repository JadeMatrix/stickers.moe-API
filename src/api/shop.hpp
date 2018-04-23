#pragma once
#ifndef STICKERS_MOE_API_SHOP_HPP
#define STICKERS_MOE_API_SHOP_HPP


#include "../audit/blame.hpp"
#include "../common/bigid.hpp"
#include "../common/postgres.hpp"
#include "../common/timestamp.hpp"

#include <exception>
#include <optional>
#include <string>


namespace stickers
{
    struct shop_info
    {
        timestamp   created;
        timestamp   revised;
        std::string name;
        std::string url;
        bigid       owner_person_id;
        // TODO: maybe use a date type?
        std::optional< timestamp > founded;
        std::optional< timestamp > closed;
    };
    
    struct shop
    {
        bigid     id;
        shop_info info;
    };
    
    shop      create_shop( const shop_info&, const audit::blame& );
    shop_info   load_shop( const bigid    &                      );
    shop_info update_shop( const shop     &, const audit::blame& );
    void      delete_shop( const bigid    &, const audit::blame& );
    
    // ACID-safe assert
    void assert_shop_exists( const bigid& id, pqxx::work& transaction );
    
    class no_such_shop : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}


#endif

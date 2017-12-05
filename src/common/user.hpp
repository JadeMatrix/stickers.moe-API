#pragma once
#ifndef STICKERS_MOE_COMMON_USER_HPP
#define STICKERS_MOE_COMMON_USER_HPP


#include <string>

#include "bigid.hpp"
#include "datetime.hpp"
#include "exception.hpp"
#include "hashing.hpp"
#include "postgres.hpp"


namespace stickers
{
    enum password_type
    {
        UNKNOWN = 0,
        INVALID,
        BCRYPT
    };
    
    struct password
    {
        password_type   type;
        std::string     hash;
        std::string     salt;
        int           factor;
    };
    
    struct user_info
    {
        password         password;
        datetime_type    created;
        datetime_type    revised;
        std::string      display_name;
        std::string      real_name;
        // sha256           avatar_hash;
        std::string      email;
    };
    
    struct user
    {
        bigid     id;
        user_info info;
    };
    
    bigid     create_user( const user_info&               );
    bigid     create_user( const user_info&, const bigid& );
    user_info   load_user( const bigid&                   );
    void        save_user( const user&     , const bigid& );
    void      delete_user( const bigid&    , const bigid& );
    
    class no_such_user : public exception
    {
    protected:
        std::string message;
    public:
        // no_such_user( const bigid& );
        no_such_user( const bigid&, const std::string& );
        virtual const char* what() const noexcept;
    };
}


// Template specialization of `pqxx::field::to<>(&)` for
// `stickers::password_type`, which also allows use of `pqxx::field::as<>(&)`
template<> inline bool pqxx::field::to< stickers::password_type >(
    stickers::password_type& pt
) const
{
    if ( is_null() )
        return false;
    else
    {
        std::string s = as< std::string >();
        if( s == "bcrypt" )
            pt = stickers::BCRYPT;
        else if( s == "invalid" )
            pt = stickers::INVALID;
        else
            return false;
        return true;
    }
}


#endif

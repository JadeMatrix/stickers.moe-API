#pragma once
#ifndef STICKERS_MOE_COMMON_USER_HPP
#define STICKERS_MOE_COMMON_USER_HPP


#include <string>

#include "bigid.hpp"
#include "exception.hpp"
#include "hashing.hpp"


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
        password    password;
        // created
        // revised
        std::string display_name;
        std::string real_name;
        // sha256      avatar_hash;
        std::string email;
    };
    
    struct user
    {
        bigid     id;
        user_info info;
    };
    
    /*
    class user
    {
    public:
        struct info
        {
            // ...
        };
        // ...
    protected:
        bigid _id;
        info  _info;
    };
    */
    
    user_info load_user( const bigid&     );
    // void      save_user( const user&      );
    // void    delete_user( const bigid&     );
    // bigid   create_user( const user_info& );
    
    class no_such_user : public exception
    {
    protected:
        std::string message;
    public:
        no_such_user( const bigid& );
        virtual const char* what() const noexcept;
    };
}


#endif

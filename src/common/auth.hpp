#pragma once
#ifndef STICKERS_MOE_COMMON_AUTH_HPP
#define STICKERS_MOE_COMMON_AUTH_HPP


#include "bigid.hpp"
#include "jwt.hpp"
#include "../audit/blame.hpp"

#include <show.hpp>

#include <exception>
#include <set>
#include <string>


namespace stickers
{
    using permissions_type = std::set< std::string >;
    
    struct auth_info
    {
        bigid            user_id;
        permissions_type user_permissions;
    };
    
    auth_info authenticate( const show::request& );
    jwt generate_auth_token_for_user( bigid, const audit::blame& );
    // void set_user_permissions( bigid, const permissions_type& );
    permissions_type get_user_permissions( bigid );
    
    void permissions_assert_any(
        const permissions_type& got,
        const permissions_type& expect
    );
    void permissions_assert_all(
        const permissions_type& got,
        const permissions_type& expect
    );
    
    class authentication_error : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };
    
    // Authorization error messages will be publicly displayed by the API
    class authorization_error : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };
}


#endif

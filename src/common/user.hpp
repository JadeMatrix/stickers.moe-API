#pragma once
#ifndef STICKERS_MOE_COMMON_USER_HPP
#define STICKERS_MOE_COMMON_USER_HPP


#include <string>

#include "bigid.hpp"
#include "timestamp.hpp"
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
        password    password;
        timestamp   created;
        timestamp   revised;
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


// Template specialization of `pqxx::string_traits<>(&)` for
// `stickers::password_type`, which allows use of `pqxx::field::to<>(&)` and
// `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::password_type >
    {
        using subject_type = stickers::password_type;
        
        static constexpr const char* name() noexcept {
            return "stickers::password_type";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const stickers::password_type& ) { return false; }
        
        [[noreturn]] static stickers::password_type null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], stickers::password_type& pt )
        {
            std::string s( str );
            if( s == "bcrypt" )
                pt = stickers::BCRYPT;
            else if( s == "invalid" )
                pt = stickers::INVALID;
            else
                throw argument_error(
                    "Failed conversion to "
                    + std::string( name() )
                    + ": '"
                    + std::string( str )
                    + "'"
                );
        }
        
        static std::string to_string( stickers::password_type pt )
        {
            switch( pt )
            {
            case stickers::BCRYPT:
                return "bcrypt";
            default:
                return "invalid";
            }
        }
    };
}


#endif

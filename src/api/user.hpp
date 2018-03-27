#pragma once
#ifndef STICKERS_MOE_API_USER_HPP
#define STICKERS_MOE_API_USER_HPP


#include <exception>
#include <optional>
#include <string>

#include "../audit/blame.hpp"
#include "../common/bigid.hpp"
#include "../common/hashing.hpp"
#include "../common/postgres.hpp"
#include "../common/timestamp.hpp"


namespace stickers // Passwords ////////////////////////////////////////////////
{
    enum class password_type
    {
        INVALID = 0,
        RAW,
        SCRYPT
    };
    
    class password
    {
    protected:
        password_type _type;
        union
        {
            void*       invalid_value;
            std::string raw_value;
            scrypt      scrypt_value;
        };
        
        void cleanup();
        const char* type_name() const;
        
    public:
        password();
        password( const password   & );
        password( const std::string& );
        password( const scrypt     & );
        ~password();
        
        password_type type() const { return _type; }
        template< typename T > T& value();
        
        std::string   hash() const;
        std::string   salt() const;
        long        factor() const;
        
        bool operator==( const password& ) const;
        bool operator!=( const password& ) const;
        
        password& operator=( const password   & );
        password& operator=( const std::string& );
        password& operator=( const scrypt     & );
    };
    
    template<> inline std::string& password::value< std::string >()
    {
        if( _type != password_type::RAW )
            throw std::runtime_error{
                "attempt to get wrong type of value (raw) from "
                "stickers::password ("
                + static_cast< std::string >( type_name() )
                + ")"
            };
        return raw_value;
    }
    template<> inline scrypt& password::value< scrypt >()
    {
        if( _type != password_type::SCRYPT )
            throw std::runtime_error{
                "attempt to get wrong type of value (scrypt) from "
                "stickers::password ("
                + static_cast< std::string >( type_name() )
                + ")"
            };
        return scrypt_value;
    }
    
    // Return a fresh hashing of a new password using the preferred method
    password hash_password( const std::string& );
}


namespace stickers // User management //////////////////////////////////////////
{
    struct user_info
    {
        password                     password;
        timestamp                    created;
        timestamp                    revised;
        std::string                  display_name;
        std::optional< std::string > real_name;
        std::optional< sha256      > avatar_hash;
        std::string                  email;
    };
    
    struct user
    {
        bigid     id;
        user_info info;
    };
    
    user      create_user( const user_info&, const audit::blame&, bool signup = true );
    user_info   load_user( const bigid    &                      );
    user_info   save_user( const user     &, const audit::blame& );
    void      delete_user( const bigid    &, const audit::blame& );
    
    // TODO: Move
    void send_validation_email( const bigid& );
}


namespace stickers // Exceptions ///////////////////////////////////////////////
{
    class no_such_user : public std::runtime_error
    {
    public:
        // no_such_user( const bigid& );
        no_such_user( const bigid&, const std::string& );
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
            if( s == "scrypt" )
                pt = stickers::password_type::SCRYPT;
            else if( s == "invalid" )
                pt = stickers::password_type::INVALID;
            else
                throw argument_error{
                    "Failed conversion to "
                    + static_cast< std::string >( name() )
                    + ": '"
                    + static_cast< std::string >( str )
                    + "'"
                };
        }
        
        static std::string to_string( stickers::password_type pt )
        {
            switch( pt )
            {
            case stickers::password_type::SCRYPT:
                return "scrypt";
            default:
                return "invalid";
            }
        }
    };
}


#endif

#pragma once
#ifndef STICKERS_MOE_COMMON_UUID_HPP
#define STICKERS_MOE_COMMON_UUID_HPP


#include "postgres.hpp"

#include <exception>    // std::invalid_argument
#include <sstream>      // std::ostringstream
#include <string>


namespace stickers
{
    class uuid
    {
        // So libpqxx can use uuid's protected default constructor
        friend uuid pqxx::field::as< uuid >() const;
        friend uuid pqxx::field::as< uuid >( const uuid& ) const;
        
    protected:
        std::string value;
        
        uuid();
        
    public:
        uuid( const std::string& );
        uuid( const char*, std::size_t );
        uuid( const uuid& );
        
        std::string raw_value           () const;
        std::string hex_value           () const;
        std::string hex_value_8_4_4_4_12() const;
        
        // Use a function rather than the default constructor as there is
        // overhead generating a new UUID which we'd like to avoid in some cases
        // where the default constructor would be used
        static uuid generate();
    };
}


// Template specialization of `pqxx::string_traits<>(&)` for `stickers::uuid`,
// which allows use of `pqxx::field::to<>(&)` and `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::uuid >
    {
        using subject_type = stickers::uuid;
        
        static constexpr const char* name() noexcept {
            return "stickers::uuid";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const stickers::uuid& ) { return false; }
        
        [[noreturn]] static stickers::uuid null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], stickers::uuid& v )
        {
            try
            {
                v = stickers::uuid( str );
            }
            catch( const std::invalid_argument& e )
            {
                throw argument_error(
                    "Failed conversion to "
                    + static_cast< std::string >( name() )
                    + ": '"
                    + static_cast< std::string >( str )
                    + "'"
                );
            }
        }
        
        static std::string to_string( const stickers::uuid& v )
        {
            std::string        encoded{ v.raw_value() };
            std::ostringstream decoded;
            decoded << std::hex;
            
            for( auto& b : encoded )
                if( std::isprint( b ) )
                    decoded << b;
                else
                    decoded << "\\x" << static_cast< unsigned int >( b );
            
            return decoded.str();
        }
    };
}


#endif

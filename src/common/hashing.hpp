#pragma once
#ifndef STICKERS_MOE_COMMON_HASHING_HPP
#define STICKERS_MOE_COMMON_HASHING_HPP


#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <cctype>
#include <exception>
#include <string>
#include <sstream>

#include "postgres.hpp"


namespace stickers
{
    class hash_error : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };
    
    class sha256
    {
        // So libpqxx can use sha256's protected default constructor
        friend sha256 pqxx::field::as< sha256 >() const;
        friend sha256 pqxx::field::as< sha256 >( const sha256& ) const;
        
    protected:
        sha256();
        
        CryptoPP::SecByteBlock digest;
        
    public:
        sha256( const CryptoPP::SecByteBlock& );
        sha256( const std::string& );
        sha256( const char*, size_t );
        sha256( const sha256& );
        
        std::string raw_digest() const;
        std::string hex_digest() const;
        
        static sha256 make( const char*, size_t );
        static sha256 make( const std::string& );
        
        static sha256 make_from_hex_string( const std::string& );
        
        bool     operator==( const sha256     & ) const;
        bool     operator!=( const sha256     & ) const;
    };
}


// Template specialization of `pqxx::string_traits<>(&)` for `stickers::sha256`,
// which allows use of `pqxx::field::to<>(&)` and `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::sha256 >
    {
        using subject_type = stickers::sha256;
        
        static constexpr const char* name() noexcept {
            return "stickers::sha256";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const stickers::sha256& ) { return false; }
        
        [[noreturn]] static stickers::sha256 null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], stickers::sha256& h )
        {
            try
            {
                h = stickers::sha256( str, strlen( str ) );
            }
            catch( const stickers::hash_error& he )
            {
                throw argument_error(
                    "Failed conversion to "
                    + std::string( name() )
                    + ": '"
                    + std::string( str )
                    + "'"
                );
            }
        }
        
        static std::string to_string( const stickers::sha256& h )
        {
            std::string        encoded = h.raw_digest();
            std::ostringstream decoded;
            decoded << std::hex;
            
            for( auto iter = encoded.begin(); iter != encoded.end(); ++iter )
                if( std::isprint( *iter ) )
                    decoded << *iter;
                else
                {
                    decoded << "\\x" << ( unsigned int )*iter;
                }
            
            return decoded.str();
        }
    };
}


#endif

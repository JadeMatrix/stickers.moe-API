#pragma once
#ifndef STICKERS_MOE_COMMON_HASHING_HPP
#define STICKERS_MOE_COMMON_HASHING_HPP


#include "postgres.hpp"

#include <cryptopp/sha.h>

#include <cctype>
#include <exception>
#include <sstream>
#include <string>


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
        sha256( const char*, std::size_t );
        sha256( const sha256& );
        
        std::string raw_digest() const;
        std::string hex_digest() const;
        
        static sha256 make( const char*, std::size_t );
        static sha256 make( const std::string& );
        
        static sha256 make_from_hex_string( const std::string& );
        
        bool operator ==( const sha256& ) const;
        bool operator !=( const sha256& ) const;
        bool operator  <( const sha256& ) const;
        bool operator  >( const sha256& ) const;
        bool operator <=( const sha256& ) const;
        bool operator >=( const sha256& ) const;
        
        class builder
        {
        protected:
            CryptoPP::SHA256 algorithm;
        public:
            void append( const char*, std::size_t );
            void append( const std::string& );
            
            sha256 generate_and_clear();
        };
    };
    
    class scrypt
    {
    protected:
        scrypt();
        
        std::string   digest;
        std::string   salt;
        unsigned char _factor;
        unsigned char _block_size;
        unsigned char _parallelization;
        
    public:
        scrypt( const scrypt& );
        scrypt( // Load from database fields
            const std::string& digest,
            const std::string& salt,
            unsigned char      factor,
            unsigned char      block_size,
            unsigned char      parallelization
        );
        
        std::string   raw_digest     () const;
        std::string   hex_digest     () const;
        std::string   raw_salt       () const;
        std::string   hex_salt       () const;
        unsigned char factor         () const;
        unsigned char block_size     () const;
        unsigned char parallelization() const;
        
        // Defaults from libscrypt v1.21
        static const unsigned char default_salt_size      { 16 };
        static const unsigned char default_factor         { 14 };
        static const unsigned char default_block_size     {  8 };
        static const unsigned char default_parallelization{ 16 };
        static const unsigned char default_digest_size    { 64 };
        
        static scrypt make(
            const char*   input,
            std::size_t   input_len,
            const char*   salt,
            std::size_t   salt_len,
            unsigned char factor          = default_factor,
            unsigned char block_size      = default_block_size,
            unsigned char parallelization = default_parallelization,
            std::size_t   digest_size     = default_digest_size
        );
        static scrypt make(
            const std::string& input,
            const std::string& salt,
            unsigned char      factor          = default_factor,
            unsigned char      block_size      = default_block_size,
            unsigned char      parallelization = default_parallelization,
            std::size_t        digest_size     = default_digest_size
        );
        
        static unsigned int make_libscrypt_mcf_factor(
            unsigned char factor          = default_factor,
            unsigned char block_size      = default_block_size,
            unsigned char parallelization = default_parallelization
        );
        static void split_libscrypt_mcf_factor(
            unsigned int   combined,
            unsigned char& factor,
            unsigned char& block_size,
            unsigned char& parallelization
        );
        
        bool operator ==( const scrypt& ) const;
        bool operator !=( const scrypt& ) const;
        bool operator  <( const scrypt& ) const;
        bool operator  >( const scrypt& ) const;
        bool operator <=( const scrypt& ) const;
        bool operator >=( const scrypt& ) const;
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
            auto len{ strlen( str ) };
            if( len == 64 + 2 && str[ 0 ] == '\\' && str[ 1 ] == 'x' )
                try
                {
                    h = stickers::sha256::make_from_hex_string( std::string(
                        str + 2,
                        len - 2
                    ) );
                    return;
                }
                catch( const stickers::hash_error& he )
                {}
            
            throw argument_error{
                "Failed conversion to "
                + std::string{ name() }
                + ": '"
                + std::string{ str }
                + "'"
            };
        }
        
        static std::string to_string( const stickers::sha256& h )
        {
            std::string        encoded{ h.raw_digest() };
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

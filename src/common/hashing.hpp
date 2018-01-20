#pragma once
#ifndef STICKERS_MOE_COMMON_HASHING_HPP
#define STICKERS_MOE_COMMON_HASHING_HPP


#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <exception>
#include <string>


namespace stickers
{
    class hash_error : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };
    
    class sha256
    {
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


#endif

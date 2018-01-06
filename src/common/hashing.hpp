#pragma once
#ifndef STICKERS_MOE_COMMON_HASHING_HPP
#define STICKERS_MOE_COMMON_HASHING_HPP


#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <string>


namespace stickers
{
    using sha256 = std::string;
    // class sha256
    // {
    // protected:
    //     unsigned char bytes[ 8 ];
    // public:
    //     // sha256( const std::string& );
    //     sha256( const char* );
    //     to_hex();
    // };
}


#endif

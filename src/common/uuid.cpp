#line 2 "common/uuid.cpp"


#include "uuid.hpp"

#include <cryptopp/hex.h>
#include <uuid/uuid.h>


namespace stickers
{
    uuid::uuid() {}
    
    uuid::uuid( const std::string& str )
    {
        if( str.size() != 16 )
            throw std::invalid_argument( "UUID string must be 16 bytes" );
        value = str;
    }
    
    uuid::uuid( const char* c_str, std::size_t c_str_len ) :
        uuid{ std::string{ c_str, c_str_len } }
    {}
    
    uuid::uuid( const uuid& o ) : value{ o.value } {}
    
    std::string uuid::raw_value() const
    {
        return value;
    }
    
    std::string uuid::hex_value() const
    {
        std::string hex;
        CryptoPP::StringSource{
            value,
            true,
            new CryptoPP::HexEncoder{
                new CryptoPP::StringSink{ hex }
            }
        };
        return hex;
    }
    
    std::string uuid::hex_value_8_4_4_4_12() const
    {
        auto unbroken = hex_value();
        return (
              unbroken.substr(  0,  8 )
            + "-"
            + unbroken.substr(  8,  4 )
            + "-"
            + unbroken.substr( 12,  4 )
            + "-"
            + unbroken.substr( 16,  4 )
            + "-"
            + unbroken.substr( 20, 12 )
        );
    }
    
    uuid uuid::generate()
    {
        uuid_t generated;
        uuid_generate( generated );
        return { reinterpret_cast< char* >( generated ), 16 };
    }
}

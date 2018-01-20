#include "hashing.hpp"

#line __LINE__ "common/hashing.cpp"


namespace
{
    char unhex( char c )
    {
        if( c >= '0' && c <= '9' )
            return c - '0';
        if( c >= 'A' && c <= 'F' )
            return c - 'A' + 10;
        if( c >= 'a' && c <= 'f' )
            return c - 'a' + 10;
        throw stickers::hash_error(
            "char with value " + std::to_string( c ) + " out of range for hex"
        );
    }
}


namespace stickers
{
    sha256::sha256() : digest( CryptoPP::SHA256::DIGESTSIZE ) {}
    
    sha256::sha256( const CryptoPP::SecByteBlock& b ) :
        digest( CryptoPP::SHA256::DIGESTSIZE )
    {
        if( b.size() == CryptoPP::SHA256::DIGESTSIZE )
            digest = b;
        else
            throw hash_error(
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( b.size() )
                + ")"
            );
    }
    
    sha256::sha256( const std::string& s ) :
        digest( CryptoPP::SHA256::DIGESTSIZE )
    {
        if( s.size() == CryptoPP::SHA256::DIGESTSIZE )
            for( size_t i = 0; i < CryptoPP::SHA256::DIGESTSIZE; ++i )
                digest.BytePtr()[ i ] = ( byte )s[ i ];
        else
            throw hash_error(
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( s.size() )
                + ")"
            );
    }
    
    sha256::sha256( const char* s, size_t l ) :
        digest( CryptoPP::SHA256::DIGESTSIZE )
    {
        if( l == CryptoPP::SHA256::DIGESTSIZE )
            for( size_t i = 0; i < CryptoPP::SHA256::DIGESTSIZE; ++i )
                digest.BytePtr()[ i ] = ( byte )s[ i ];
        else
            throw hash_error(
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( l )
                + ")"
            );
    }
    
    sha256::sha256( const sha256& o ) :
        digest( o.digest )
    {}
    
    std::string sha256::raw_digest() const
    {
        return std::string( ( char* )digest.data(), digest.size() );
    }
    
    std::string sha256::hex_digest() const
    {
        std::string hex;
        CryptoPP::HexEncoder( new CryptoPP::StringSink( hex ) ).Put(
            digest.begin(),
            digest.size()
        );
        return hex;
    }
    
    sha256 sha256::make( const char* s, size_t l )
    {
        sha256 h;
        
        CryptoPP::SHA256().CalculateDigest(
            h.digest.begin(),
            ( byte* )s,
            l
        );
        
        return h;
    }
    
    sha256 sha256::make( const std::string& s )
    {
        return make( s.c_str(), s.size() );
    }
    
    sha256 sha256::make_from_hex_string( const std::string& s )
    {
        if( s.size() != CryptoPP::SHA256::DIGESTSIZE * 2 )
            throw hash_error(
                "mismatch between digest and input sizes constructing a sha256 "
                "object from hex string (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE * 2 )
                + " chars, got "
                + std::to_string( s.size() )
                + ")"
            );
        
        sha256 h;
        
        size_t i = 0;
        for( auto iter = h.digest.begin(); iter != h.digest.end(); ++iter )
        {
            *iter = ( unhex( s[ i ] ) << 4 ) | ( unhex( s[ i + 1 ] ) );
            i += 2;
        }
        
        return h;
    }
    
    bool sha256::operator==( const sha256& h ) const
    {
        return digest == h.digest;
    }
    
    bool sha256::operator!=( const sha256& h ) const
    {
        return digest != h.digest;
    }
}

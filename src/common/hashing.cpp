#line 2 "common/hashing.cpp"


#include "hashing.hpp"

#include <cryptopp/hex.h>
#include <libscrypt.h>

#include <cmath>


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


namespace stickers // SHA256 ///////////////////////////////////////////////////
{
    sha256::sha256() : digest{ CryptoPP::SHA256::DIGESTSIZE } {}
    
    sha256::sha256( const CryptoPP::SecByteBlock& b ) :
        digest{ CryptoPP::SHA256::DIGESTSIZE }
    {
        if( b.size() == CryptoPP::SHA256::DIGESTSIZE )
            digest = b;
        else
            throw hash_error{
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( b.size() )
                + ")"
            };
    }
    
    sha256::sha256( const std::string& s ) :
        digest{ CryptoPP::SHA256::DIGESTSIZE }
    {
        if( s.size() == CryptoPP::SHA256::DIGESTSIZE )
            for( std::size_t i = 0; i < CryptoPP::SHA256::DIGESTSIZE; ++i )
                digest.BytePtr()[ i ] = ( CryptoPP::byte )s[ i ];
        else
            throw hash_error{
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( s.size() )
                + ")"
            };
    }
    
    sha256::sha256( const char* s, std::size_t l ) :
        digest{ CryptoPP::SHA256::DIGESTSIZE }
    {
        if( l == CryptoPP::SHA256::DIGESTSIZE )
            for( std::size_t i = 0; i < CryptoPP::SHA256::DIGESTSIZE; ++i )
                digest.BytePtr()[ i ] = static_cast< CryptoPP::byte >( s[ i ] );
        else
            throw hash_error{
                "mismatch between digest and input sizes constructing a sha256 "
                "object (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE )
                + " bytes, got "
                + std::to_string( l )
                + ")"
            };
    }
    
    sha256::sha256( const sha256& o ) :
        digest{ o.digest }
    {}
    
    std::string sha256::raw_digest() const
    {
        return {
            reinterpret_cast< const char* >( digest.data() ),
            digest.size()
        };
    }
    
    std::string sha256::hex_digest() const
    {
        std::string hex;
        CryptoPP::StringSource{
            reinterpret_cast< const CryptoPP::byte* >( digest.data() ),
            digest.size(),
            true,
            new CryptoPP::HexEncoder{
                new CryptoPP::StringSink{ hex }
            }
        };
        return hex;
    }
    
    sha256 sha256::make( const char* s, std::size_t l )
    {
        sha256 h;
        
        CryptoPP::SHA256{}.CalculateDigest(
            h.digest.begin(),
            reinterpret_cast< const CryptoPP::byte* >( s ),
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
            throw hash_error{
                "mismatch between digest and input sizes constructing a sha256 "
                "object from hex string (need "
                + std::to_string( CryptoPP::SHA256::DIGESTSIZE * 2 )
                + " chars, got "
                + std::to_string( s.size() )
                + ")"
            };
        
        sha256 h;
        
        std::size_t i = 0;
        for( auto& b : h.digest )
        {
            b = ( unhex( s[ i ] ) << 4 ) | ( unhex( s[ i + 1 ] ) );
            i += 2;
        }
        
        return h;
    }
    
    bool sha256::operator==( const sha256& o ) const
    {
        // `CryptoPP::SecBlock<>::operator==()` is constant-time
        return digest == o.digest;
    }
    
    bool sha256::operator!=( const sha256& o ) const
    {
        return digest != o.digest;
    }
    
    bool sha256::operator <( const sha256& o ) const
    {
        int first_diff{ 0 };
        
        for(
            CryptoPP::SecByteBlock::size_type i{ 0 };
            i < CryptoPP::SHA256::DIGESTSIZE;
            ++i
        )
            if( first_diff == 0 )
                first_diff = digest.data()[ i ] - o.digest.data()[ i ];
        
        return first_diff < 0;
    }
    
    bool sha256::operator >( const sha256& o ) const
    {
        return !( *this <= o );
    }
    
    bool sha256::operator <=( const sha256& o ) const
    {
        return *this < o || *this == o;
    }
    
    bool sha256::operator >=( const sha256& o ) const
    {
        return !( *this < o );
    }
}


namespace stickers // SCRYPT ///////////////////////////////////////////////////
{
    scrypt::scrypt() {}
    
    scrypt::scrypt( const scrypt& o ) :
        salt{             o.salt             },
        digest{           o.digest           },
        _factor{          o._factor          },
        _block_size{      o._block_size      },
        _parallelization{ o._parallelization }
    {}
    
    scrypt::scrypt(
        const std::string& digest,
        const std::string& salt,
        unsigned char      factor,
        unsigned char      block_size,
        unsigned char      parallelization
    ) :
        digest{           digest          },
        salt{             salt            },
        _factor{          factor          },
        _block_size{      block_size      },
        _parallelization{ parallelization }
    {}
    
    std::string scrypt::raw_digest() const
    {
        return digest;
    }
    
    std::string scrypt::hex_digest() const
    {
        std::string hex;
        CryptoPP::StringSource{
            digest,
            true,
            new CryptoPP::HexEncoder{
                new CryptoPP::StringSink{ hex }
            }
        };
        return hex;
    }
    
    std::string scrypt::raw_salt() const
    {
        return salt;
    }
    
    std::string scrypt::hex_salt() const
    {
        std::string hex;
        CryptoPP::StringSource{
            salt,
            true,
            new CryptoPP::HexEncoder{
                new CryptoPP::StringSink{ hex }
            }
        };
        return hex;
    }
    
    unsigned char scrypt::factor() const
    {
        return _factor;
    }
    
    unsigned char scrypt::block_size() const
    {
        return _block_size;
    }
    
    unsigned char scrypt::parallelization() const
    {
        return _parallelization;
    }
    
    scrypt scrypt::make(
        const char*   input,
        std::size_t   input_len,
        const char*   salt,
        std::size_t   salt_len,
        unsigned char factor,
        unsigned char block_size,
        unsigned char parallelization,
        std::size_t   digest_size
    )
    {
        scrypt sc;
        
        // Can't use list initialization to call this `std::string` constructor
        // when compiling with clang
        sc.digest = std::string( digest_size, '\0' );
        
        int result = libscrypt_scrypt(
            reinterpret_cast< const uint8_t* >( input ),
            input_len,
            reinterpret_cast< const uint8_t* >( salt ),
            salt_len,
            static_cast< uint64_t >( pow( 2, factor ) ),
            block_size,
            parallelization,
            reinterpret_cast< uint8_t* >( sc.digest.data() ),
            sc.digest.size()
        );
        if( result )
            throw hash_error{
                "failed to make scrypt (libscrypt_scrypt() returned "
                + std::to_string( result )
                + ")"
            };
        
        sc.salt             = std::string{ salt, salt_len };
        sc._factor          = factor;
        sc._block_size      = block_size;
        sc._parallelization = parallelization;
        
        return sc;
    }
    
    scrypt scrypt::make(
        const std::string& input,
        const std::string& salt,
        unsigned char      factor,
        unsigned char      block_size,
        unsigned char      parallelization,
        std::size_t        digest_size
    )
    {
        return make(
            input.c_str(),
            input.size(),
            salt.c_str(),
            salt.size(),
            factor,
            block_size,
            parallelization,
            digest_size
        );
    }
    
    unsigned int scrypt::make_libscrypt_mcf_factor(
        unsigned char factor,
        unsigned char block_size,
        unsigned char parallelization
    )
    {
        return (
              static_cast< unsigned int >( factor          << 16 )
            | static_cast< unsigned int >( block_size      <<  8 )
            | static_cast< unsigned int >( parallelization <<  0 )
        );
    }
    
    void scrypt::split_libscrypt_mcf_factor(
        unsigned int   combined,
        unsigned char& factor,
        unsigned char& block_size,
        unsigned char& parallelization
    )
    {
        factor          = combined >> 16;
        block_size      = combined >>  8;
        parallelization = combined >>  0;
    }
    
    bool scrypt::operator==( const scrypt& o ) const
    {
        bool equals{ true };
        
        equals = equals && ( _factor          == o._factor          );
        equals = equals && ( _block_size      == o._block_size      );
        equals = equals && ( _parallelization == o._parallelization );
        
        std::size_t slen;
        
        // `std::string::size()` is guaranteed constant-time as of C++11
        
        slen = salt.size() > o.salt.size() ? salt.size() : o.salt.size();
        for( std::size_t i = 0; i < slen; ++i )
        {
            char c1 = i >=   salt.size() ? ~o.salt[ i ] :   salt[ i ];
            char c2 = i >= o.salt.size() ? ~  salt[ i ] : o.salt[ i ];
            equals = equals && ( c1 == c2 );
        }
        
        slen = digest.size() > o.digest.size() ? digest.size() : o.digest.size();
        for( std::size_t i = 0; i < slen; ++i )
        {
            char c1 = i >=   digest.size() ? ~o.digest[ i ] :   digest[ i ];
            char c2 = i >= o.digest.size() ? ~  digest[ i ] : o.digest[ i ];
            equals = equals && ( c1 == c2 );
        }
        
        return equals;
    }
    
    bool scrypt::operator!=( const scrypt& o ) const
    {
        return !( *this == o );
    }
    
    bool scrypt::operator <( const scrypt& o ) const
    {
        std::size_t slen;
        
        // `std::string::size()` is guaranteed constant-time as of C++11
        
        auto first_diff = salt.size() - o.salt.size();
        
        if( first_diff == 0 )
            first_diff = digest.size() - o.digest.size();
        
        slen = salt.size() > o.salt.size() ? salt.size() : o.salt.size();
        for( std::size_t i = 0; i < slen; ++i )
        {
            char c1 = i >=   salt.size() ? o.salt[ i ] :   salt[ i ];
            char c2 = i >= o.salt.size() ?   salt[ i ] : o.salt[ i ];
            if( first_diff == 0 )
                first_diff = c1 - c2;
        }
        
        slen = digest.size() > o.digest.size() ? digest.size() : o.digest.size();
        for( std::size_t i = 0; i < slen; ++i )
        {
            char c1 = i >=   digest.size() ? o.digest[ i ] :   digest[ i ];
            char c2 = i >= o.digest.size() ?   digest[ i ] : o.digest[ i ];
            if( first_diff == 0 )
                first_diff = c1 - c2;
        }
        
        if( first_diff == 0 )
            first_diff = make_libscrypt_mcf_factor(
                _factor,
                _block_size,
                _parallelization
            ) - make_libscrypt_mcf_factor(
                o._factor,
                o._block_size,
                o._parallelization
            );
        
        return first_diff < 0;
    }
    
    bool scrypt::operator >( const scrypt& o ) const
    {
        return !( *this <= o );
    }
    
    bool scrypt::operator <=( const scrypt& o ) const
    {
        return *this < o || *this == o;
    }
    
    bool scrypt::operator >=( const scrypt& o ) const
    {
        return !( *this < o );
    }
}

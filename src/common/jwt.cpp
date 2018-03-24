#include "jwt.hpp"

#include "config.hpp"
#include "hashing.hpp"
#include "logging.hpp"
#include "string_utils.hpp"

#include <show/base64.hpp>
#include <cryptopp/hmac.h>
#include <cryptopp/hex.h>

#include <cstdlib>      // std::rand()


namespace stickers
{
    const jwt jwt::parse( const std::string& raw )
    {
        auto signing_keys = config()[ "auth" ][ "jwt_keys" ].get<
            std::map< std::string, std::string >
        >();
        
        for( auto& kv : signing_keys )
            kv.second = show::base64_decode( kv.second );
        
        return parse( raw, signing_keys );
    }
    
    const jwt jwt::parse(
        const std::string& raw,
        const std::map< std::string, std::string >& signing_keys
    )
    {
        auto token = parse_no_validate( raw );
        
        auto split_pos = raw.rfind( "." );
        std::string payload   = raw.substr( 0, split_pos );
        std::string signature;
        
        try
        {
            signature = show::base64_decode(
                raw.substr( split_pos + 1 ),
                show::base64_chars_urlsafe
            );
        }
        catch( const show::base64_decode_error& e )
        {
            throw validation_error( "signature segment is not valid base64" );
        }
        
        if( !token.kid )
        {
            STICKERS_LOG(
                WARNING,
                "attempt to authenticate with JWT without a signing key ID; full token: ",
                log_sanitize( raw )
            );
            throw validation_error( "missing signing key ID" );
        }
        
        auto signing_key_found = signing_keys.find( *token.kid );
        if( signing_key_found == signing_keys.end() )
        {
            STICKERS_LOG(
                WARNING,
                "attempt to authenticate with JWT using unknown signing key ",
                log_sanitize( *token.kid ),
                "; full token: ",
                log_sanitize( raw )
            );
            throw validation_error( "unknown signing key ID" );
        }
        
        switch( token.alg )
        {
        case signature_alg::HS512:
            {
                bool validated = false;
                
                CryptoPP::HMAC< CryptoPP::SHA512 > hmac(
                    ( CryptoPP::byte* )( signing_key_found -> second.c_str() ),
                    signing_key_found -> second.size()
                );
                
                // Seems like a good place to use
                // `CryptoPP::HashVerificationFilter::PUT_HASH`, but there's no
                // documentation on how to use that flag.  I hate Crypto++.
                CryptoPP::StringSource(
                    payload + signature,
                    true,
                    new CryptoPP::HashVerificationFilter(
                        hmac,
                        new CryptoPP::ArraySink(
                            ( CryptoPP::byte* )( &validated ),
                            sizeof( validated )
                        ),
                        (
                              CryptoPP::HashVerificationFilter::HASH_AT_END
                            | CryptoPP::HashVerificationFilter::PUT_RESULT
                        )
                    )
                );
                
                if( !validated )
                {
                    STICKERS_LOG(
                        WARNING,
                        "attempt to authenticate with JWT with invalid signature; full token: ",
                        log_sanitize( raw )
                    );
                    throw validation_error( "signature mismatch" );
                }
            }
            break;
        }
        
        if( token.nbf && *token.nbf > now() )
            throw validation_error( "premature token" );
        if( token.exp && *token.exp <= now() )
            throw validation_error( "expired token" );
        
        // TODO: check jti against blacklist
        
        return token;
    }
    
    const jwt jwt::parse_no_validate( const std::string& raw )
    {
        jwt token;
        
        std::string headers_string;
        std::string  claims_string;
        
        {
            auto jwt_segments = split< std::vector< std::string > >(
                raw,
                std::string( "." )
            );
            
            if( jwt_segments.size() != 3 )
                throw structure_error( "token does not have 3 segments" );
            
            try
            {
                headers_string = show::base64_decode(
                    jwt_segments[ 0 ],
                    show::base64_chars_urlsafe
                );
            }
            catch( const show::base64_decode_error& e )
            {
                // DEBUG:
                STICKERS_LOG(
                    DEBUG,
                    "invalid base64 header (",
                    e.what(),
                    "): ",
                    log_sanitize( jwt_segments[ 0 ] )
                );
                throw validation_error( "header segment is not valid base64" );
            }
            
            try
            {
                claims_string  = show::base64_decode(
                    jwt_segments[ 1 ],
                    show::base64_chars_urlsafe
                );
            }
            catch( const show::base64_decode_error& e )
            {
                throw validation_error( "claims segment is not valid base64" );
            }
        }
        
        nlj::json header;
        try
        {
            header = nlj::json::parse( headers_string );
        }
        catch( const nlj::json::parse_error& e )
        {
            throw structure_error( "header segment is not valid JSON" );
        }
        
        if( !header.is_object() )
            throw structure_error( "header segment is not a JSON object" );
        
        auto alg = header.find( "alg" );
        auto kid = header.find( "kid" );
        
        if( alg == header.end() )
            throw structure_error( "header missing required key \"alg\"" );
        if( kid == header.end() )
            throw structure_error( "header missing required key \"kid\"" );
        
        if( !alg.value().is_string() )
            throw structure_error( "header key \"alg\" is not a string" );
        if( !kid.value().is_string() )
            throw structure_error( "header key \"kid\" is not a string" );
        
        auto alg_string = alg.value().get< std::string >();
        
        if( alg_string == "HS512" )
            token.alg = signature_alg::HS512;
        else
        {
            // Officially, JWT specifies "none" as a possible algorithm, which
            // is a considerable security hole.  This just logs anyone possibly
            // trying to take advantage of that.
            STICKERS_LOG(
                WARNING,
                "attempt to authenticate with JWT with unknown hashing algorithm \"",
                log_sanitize( alg_string ),
                "\"; full token: ",
                log_sanitize( raw )
            );
            throw structure_error(
                "unsupported hashing algorithm \""
                + log_sanitize( alg_string )
                + "\""
            );
        }
        
        token.kid = kid.value().get< std::string >();
        
        try
        {
            token.claims = nlj::json::parse( claims_string );
        }
        catch( const nlj::json::parse_error& e )
        {
            throw structure_error( "claims segment is not valid JSON" );
        }
        if( !token.claims.is_object() )
            throw structure_error( "claims segment is not a JSON object" );
        
        // Check known header keys /////////////////////////////////////////////
        
        auto typ = header.find( "typ" );
        auto jti = header.find( "jti" );
        
        if( typ == header.end() )
            throw structure_error( "missing required header field \"typ\"" );
        else
            try
            {
                auto typ_string = typ.value().get< std::string >();
                if( typ_string == "JWT+" )
                    token.typ = jwt_type::JWT_PLUS;
                else if( typ_string == "JWT" )
                    token.typ = jwt_type::JWT;
                else
                    throw structure_error(
                        "unsupported JWT type \""
                        + log_sanitize( typ_string )
                        + "\""
                    );
            }
            catch( const nlj::json::parse_error& e )
            {
                throw structure_error(
                    "header field \"typ\" is not a JSON string"
                );
            }
        
        if( jti != header.end() )
            try
            {
                auto jti_string = jti.value().get< std::string >();
                try
                {
                    token.jti = uuid( jti_string );
                }
                catch( const std::invalid_argument& e )
                {
                    throw structure_error(
                        "unsupported non-UUID \"jti\" field \""
                        + log_sanitize( jti_string )
                        + "\""
                    );
                }
            }
            catch( const nlj::json::parse_error& e )
            {
                throw structure_error(
                    "header field \"jti\" is not a JSON string"
                );
            }
        
        // Check known claims //////////////////////////////////////////////////
        
        auto iat = token.claims.find( "iat" );
        auto nbf = token.claims.find( "nbf" );
        auto exp = token.claims.find( "exp" );
        
        if( iat != token.claims.end() )
            try
            {
                if( token.typ == jwt_type::JWT_PLUS )
                    token.iat = from_iso8601_str(
                        iat.value().get< std::string >()
                    );
                else
                    token.iat = from_unix_time(
                        iat.value().get< unsigned int >()
                    );
            }
            catch( const nlj::json::parse_error& e )
            {
                throw structure_error(
                    "claim field \"iat\" is not a JSON " + std::string(
                        token.typ == jwt_type::JWT_PLUS ?
                        "string" : "unsigned integer"
                    )
                );
            }
        
        if( nbf != token.claims.end() )
            try
            {
                if( token.typ == jwt_type::JWT_PLUS )
                    token.nbf = from_iso8601_str(
                        nbf.value().get< std::string >()
                    );
                else
                    token.nbf = from_unix_time(
                        nbf.value().get< unsigned int >()
                    );
            }
            catch( const nlj::json::parse_error& e )
            {
                throw structure_error(
                    "claim field \"nbf\" is not a JSON " + std::string(
                        token.typ == jwt_type::JWT_PLUS ?
                        "string" : "unsigned integer"
                    )
                );
            }
        
        if( exp != token.claims.end() )
            try
            {
                if( token.typ == jwt_type::JWT_PLUS )
                    token.exp = from_iso8601_str(
                        exp.value().get< std::string >()
                    );
                else
                    token.exp = from_unix_time(
                        exp.value().get< unsigned int >()
                    );
            }
            catch( const nlj::json::parse_error& e )
            {
                throw structure_error(
                    "claim field \"exp\" is not a JSON " + std::string(
                        token.typ == jwt_type::JWT_PLUS ?
                        "string" : "unsigned integer"
                    )
                );
            }
        
        return token;
    }
    
    std::string jwt::serialize( const jwt& token )
    {
        auto signing_keys = config()[ "auth" ][ "jwt_keys" ].get<
            std::map< std::string, std::string >
        >();
        
        for( auto& kv : signing_keys )
            kv.second = show::base64_decode( kv.second );
        
        return serialize(
            token,
            signing_keys
        );
    }
    
    std::string jwt::serialize(
        const jwt& token,
        const std::map< std::string, std::string >& signing_keys
    )
    {
        nlj::json header = { { "alg", "HS512" } };
        
        if( token.typ == jwt_type::JWT_PLUS )
            header[ "typ" ] = "JWT+";
        else
            header[ "typ" ] = "JWT";
        
        if( token.jti )
            header[ "jti" ] = ( *token.jti ).hex_value();
        else
            header[ "jti" ] = uuid::generate().hex_value();
        
        std::string signing_key_string;
        if( token.kid )
        {
            // Find the specified signing key in `signing_keys`
            auto key_found = signing_keys.find( *token.kid );
            if( key_found == signing_keys.end() )
                throw std::runtime_error(
                    "specified a JWT signing key that does not exist in the "
                    "available options"
                );
            header[ "kid" ]    = key_found -> first;
            signing_key_string = key_found -> second;
        }
        else
        {
            // Choose a random signing key from `signing_keys`
            int random_int = std::rand() % signing_keys.size();
            auto key_to_use = signing_keys.begin();
            for( int i = 0; i < random_int; ++i )
                ++key_to_use;
            header[ "kid" ]    = key_to_use -> first;
            signing_key_string = key_to_use -> second;
        }
        
        // Make a writable copy of the claims
        nlj::json claims = token.claims;
        
        timestamp iat = now();
        if( token.iat )
            iat = *token.iat;
        
        if( token.typ == jwt_type::JWT_PLUS )
            claims[ "iat" ] = to_iso8601_str( iat );
        else
            claims[ "iat" ] = to_unix_time( iat );
        
        if( token.nbf )
        {
            if( token.typ == jwt_type::JWT_PLUS )
                claims[ "nbf" ] = to_iso8601_str( *token.nbf );
            else
                claims[ "nbf" ] = to_unix_time( *token.nbf );
        }
        if( token.exp )
        {
            if( token.typ == jwt_type::JWT_PLUS )
                claims[ "exp" ] = to_iso8601_str( *token.exp );
            else
                claims[ "exp" ] = to_unix_time( *token.exp );
        }
        
        std::string header_claims_string = (
              show::base64_encode( header.dump(), show::base64_chars_urlsafe )
            + "."
            + show::base64_encode( claims.dump(), show::base64_chars_urlsafe )
        );
        std::string signature_string;
        
        CryptoPP::HMAC< CryptoPP::SHA512 > hmac(
            ( CryptoPP::byte* )signing_key_string.c_str(),
            signing_key_string.size()
        );
        
        CryptoPP::StringSource(
            header_claims_string,
            true,
            new CryptoPP::HashFilter(
                hmac,
                new CryptoPP::StringSink( signature_string )
            )
        );
        
        return header_claims_string + "." + show::base64_encode(
            signature_string,
            show::base64_chars_urlsafe
        );
    }
}
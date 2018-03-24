#pragma once
#ifndef STICKERS_MOE_COMMON_JWT_HPP
#define STICKERS_MOE_COMMON_JWT_HPP


#include "json.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

#include <exception>
#include <optional>
#include <string>


namespace stickers
{
    struct jwt
    {
        // Types ///////////////////////////////////////////////////////////////
        
        enum class signature_alg
        {
            // HS256,
            HS512
        };
        
        enum class jwt_type
        {
            JWT,
            JWT_PLUS
        };
        
        class structure_error : public std::runtime_error
        {
            using runtime_error::runtime_error;
        };
        
        class validation_error : public std::runtime_error
        {
            using runtime_error::runtime_error;
        };
        
        // Members /////////////////////////////////////////////////////////////
        
        jwt_type                             typ = jwt_type::JWT_PLUS  ;
        signature_alg                        alg = signature_alg::HS512;
        std::optional< uuid                > jti                       ;
        std::optional< std::string         > kid                       ;
        std::optional< stickers::timestamp > iat                       ;
        std::optional< stickers::timestamp > nbf                       ;
        std::optional< stickers::timestamp > exp                       ;
        nlj::json                         claims = "{}"_json           ;
        
        // Functions ///////////////////////////////////////////////////////////
        
        static const jwt parse( const std::string& raw );
        static const jwt parse(
            const std::string& raw,
            const std::map< std::string, std::string >& signing_keys
        );
        static const jwt parse_no_validate( const std::string& raw );
        static std::string serialize( const jwt& token );
        static std::string serialize(
            const jwt& token,
            const std::map< std::string, std::string >& signing_keys
        );
    };
    
    
    // class jwt
    // {
    // public:
    //     enum class signature_alg
    //     {
    //         // HS256,
    //         HS512
    //     };
    //     enum class jwt_type
    //     {
    //         JWT,
    //         JWT_PLUS
    //     };
        
    //     class structure_error : public std::runtime_error
    //     {
    //         using runtime_error::runtime_error;
    //     };
        
    //     class signature_error : public std::runtime_error
    //     {
    //         using runtime_error::runtime_error;
    //     };
        
    // // protected:
    //     jwt_type            _typ;
    //     uuid                _jti;
    //     std::string         _kid;
    //     stickers::timestamp _iat;
    //     stickers::timestamp _nbf;
    //     stickers::timestamp _exp;
        
    //     // std::string _signing_key;
    //     nlj::json   _claims;
        
    //     // void set_alg( signature_alg );
    //     // void set_default_kid();
        
    // public:
    //     // jwt();
    //     // // jwt( signature_alg alg );
    //     // // jwt( signature_alg alg, const std::string& kid );
    //     // jwt( const jwt& );
        
    //     // jwt_type           &    typ();
    //     // uuid               &    jti();
    //     // std::string        &    kid();
    //     // stickers::timestamp&    iat();
    //     // stickers::timestamp&    nbf();
    //     // stickers::timestamp&    exp();
    //     // nlj::json          & claims();
        
    //     // const jwt_type           &    typ() const;
    //     // const uuid               &    jti() const;
    //     // const std::string        &    kid() const;
    //     // const stickers::timestamp&    iat() const;
    //     // const stickers::timestamp&    nbf() const;
    //     // const stickers::timestamp&    exp() const;
    //     // const nlj::json          & claims() const;
        
    //     static const jwt parse(             const std::string& );
    //     static const jwt parse_no_validate( const std::string& );
    //     static std::string serialize( const jwt& );
    //     static std::string serialize(
    //         const jwt&                     j,
    //         const std::map< std::string >& signing_keys
    //     ) const;
    // };
}


#endif

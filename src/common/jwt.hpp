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
}


#endif

#pragma once
#ifndef STICKERS_MOE_COMMON_POSTGRES_HPP
#define STICKERS_MOE_COMMON_POSTGRES_HPP


#include "exception.hpp"

#include <pqxx/pqxx>

#include <string>
#include <vector>


#define PSQL( ... ) #__VA_ARGS__

namespace stickers
{
    // Utility to split composite types until libpqxx supports that
    
    std::vector< std::string > split_pg_field( const pqxx::field& );
    
    class non_composite_field : public exception
    {
    protected:
        std::string message;
    public:
        non_composite_field( const pqxx::field& );
        virtual const char* what() const noexcept;
    };
}


#endif

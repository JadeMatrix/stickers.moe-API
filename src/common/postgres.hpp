#pragma once
#ifndef STICKERS_MOE_COMMON_POSTGRES_HPP
#define STICKERS_MOE_COMMON_POSTGRES_HPP


#include <pqxx/pqxx>

#include <memory>
#include <string>


#define PSQL( ... ) #__VA_ARGS__


namespace stickers
{
    namespace postgres
    {
        std::unique_ptr< pqxx::connection > connect();
        std::unique_ptr< pqxx::connection > connect(
            const std::string& host,
            unsigned int       port,
            const std::string& user,
            const std::string& pass,
            const std::string& dbname
        );
    }
}


#endif

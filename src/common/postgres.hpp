#pragma once
#ifndef STICKERS_MOE_COMMON_POSTGRES_HPP
#define STICKERS_MOE_COMMON_POSTGRES_HPP


#define PQXX_HAVE_OPTIONAL
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
        
        // Format a sequence of query parameters as a comma-separated list which
        // can then be formatted into a query string surrounded by the
        // appropriate delimiters (parentheses etc.)
        template< typename Iterable > std::string format_variable_list(
            const pqxx::work& transaction,
            const Iterable  & values
        )
        {
            std::string s;
            
            // Use the begin() and end() functions like range-based `for`
            auto current = begin( values );
            auto    last =   end( values );
            
            while( true )
            {
                s += transaction.quote( *current );
                ++current;
                if( current == last )
                    break;
                else
                    s += ',';
            }
            
            return s;
        }
    }
}


#endif

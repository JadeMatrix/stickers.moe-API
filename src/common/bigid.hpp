#pragma once
#ifndef STICKERS_MOE_COMMON_BIGID_HPP
#define STICKERS_MOE_COMMON_BIGID_HPP


#include <exception>
#include <string>

#include "postgres.hpp"


namespace stickers
{
    class bigid
    {
        // So libpqxx can use bigid's protected default constructor
        friend bigid pqxx::field::as< bigid >() const;
        friend bigid pqxx::field::as< bigid >( const bigid& ) const;
        
    private:
        long long value;
        bigid();
        
    public:
        bigid( long long );
        bigid( const bigid& );
                     operator   long long(              ) const;
                     operator std::string(              ) const;
                     operator        bool(              ) const;
        const bigid& operator           =( const bigid& )      ;
        const bigid& operator           =( long long    )      ;
        bigid        operator           +( long long    ) const;
        bigid        operator           -( long long    ) const;
        const bigid& operator          ++(              )      ;
        const bigid& operator          --(              )      ;
        const bigid& operator          +=( long long    )      ;
        const bigid& operator          -=( long long    )      ;
        bool         operator          ==( const bigid& ) const;
        bool         operator          !=( const bigid& ) const;
        bool         operator           <( const bigid& ) const;
        bool         operator           >( const bigid& ) const;
        bool         operator          <=( const bigid& ) const;
        bool         operator          >=( const bigid& ) const;
        
        static bigid MIN();
        static bigid MAX();
        
        static bigid from_string( const std::string& );
    };
}


// Template specialization of `pqxx::string_traits<>(&)` for `stickers::bigid`,
// which allows use of `pqxx::field::to<>(&)` and `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::bigid >
    {
        using subject_type = stickers::bigid;
        
        static constexpr const char* name() noexcept {
            return "stickers::bigid";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const stickers::bigid& ) { return false; }
        
        [[noreturn]] static stickers::bigid null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], stickers::bigid& id )
        {
            bool conversion_error = false;
            try
            {
                long long llid;
                string_traits< long long >::from_string( str, llid );
                id = llid;
            }
            catch( const std::invalid_argument& e )
            {
                throw argument_error{
                    "Failed conversion to "
                    + static_cast< std::string >( name() )
                    + ": '"
                    + static_cast< std::string >( str )
                    + "'"
                };
            }
        }
        
        static std::string to_string( const stickers::bigid& id )
        {
            return string_traits< long long >::to_string( id );
        }
    };
}


#endif

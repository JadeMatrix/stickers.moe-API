#pragma once
#ifndef STICKERS_MOE_COMMON_BIGID_HPP
#define STICKERS_MOE_COMMON_BIGID_HPP


#include <string>

#include "exception.hpp"


namespace stickers
{
    class bigid
    {
    private:
        long long value;
    public:
        bigid( long long );
        bigid( const bigid& );
                     operator   long long()               const;
                     operator std::string()               const;
                     operator        bool()               const;
        const bigid& operator           =( const bigid& )      ;
        const bigid& operator           =( long long    )      ;
        bigid        operator           +( long long    ) const;
        bigid        operator           -( long long    ) const;
        const bigid& operator          ++()                    ;
        const bigid& operator          --()                    ;
        const bigid& operator          +=( long long    )      ;
        const bigid& operator          -=( long long    )      ;
        bool         operator          ==( const bigid& ) const;
        bool         operator          !=( const bigid& ) const;
    };
    
    const long long _BIGID_MIN = 1000000000000000000ll;
    const long long _BIGID_MAX = 9223372036854775807ll;
    
    const bigid BIGID_MIN( _BIGID_MIN );
    const bigid BIGID_MAX( _BIGID_MAX );
    
    class bigid_out_of_range : public exception, public std::out_of_range
    {
    public:
        bigid_out_of_range( long long value ) noexcept;
        // bigid_out_of_range(
        //     long long value,
        //     const std::string& operation
        // );
        // virtual const char* what() const noexcept;
        using std::out_of_range::what;
    };
}


#endif

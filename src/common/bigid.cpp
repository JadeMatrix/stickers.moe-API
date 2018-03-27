#include "bigid.hpp"

#line __LINE__ "common/bigid.cpp"


// bigid -----------------------------------------------------------------------


namespace stickers
{
    bigid::bigid( long long v )
    {
        *this = v;
    }
    
    bigid::bigid( const bigid& o )
    {
        *this = o;
    }
    
    bigid::operator long long() const
    {
        return value;
    }
    
    bigid::operator std::string() const
    {
        return std::to_string( value );
    }
    
    bigid::operator bool() const
    {
        // Should always be true
        return ( bool )value;
    }
    
    const bigid& bigid::operator =( const bigid& o )
    {
        value = o.value;
        return *this;
    }
    
    const bigid& bigid::operator =( long long v )
    {
        if( v >= _BIGID_MIN && v <= _BIGID_MAX )
            value = v;
        else
            throw bigid_out_of_range( v );
        return *this;
    }
    
    bigid bigid::operator +( long long i ) const
    {
        return bigid( value + i );
    }
    
    bigid bigid::operator -( long long i ) const
    {
        return bigid( value - i );
    }
    
    const bigid& bigid::operator ++()
    {
        ++value;
        return *this;
    }
    
    const bigid& bigid::operator --()
    {
        --value;
        return *this;
    }
    
    const bigid& bigid::operator +=( long long i )
    {
        *this = value + i;
        return *this;
    }
    
    const bigid& bigid::operator -=( long long i )
    {
        *this = value + i;
        return *this;
    }
    
    bool bigid::operator ==( const bigid& o ) const
    {
        return value == o.value;
    }
    
    bool bigid::operator !=( const bigid& o ) const
    {
        return value != o.value;
    }
    
    bigid bigid::from_string( const std::string& str )
    {
        try
        {
            long long llid;
            pqxx::string_traits< long long >::from_string( str.c_str(), llid );
            return llid;
        }
        catch( const pqxx::argument_error& e )
        {
            throw std::invalid_argument(
                "can't convert \"" + str + "\" to a big-ID"
            );
        }
    }
}


// bigid_out_of_range ----------------------------------------------------------


namespace stickers
{
    bigid_out_of_range::bigid_out_of_range( long long value ) noexcept :
        std::out_of_range( std::to_string( value ) + " out of range for bigid" )
    {}
    
    // bigid_out_of_range::bigid_out_of_range(
    //     long long value,
    //     const std::string& operation
    // ) noexcept :
    //     std::out_of_range(
    //         "operation"
    //         + std::to_string( value )
    //         + " "
    //         + operation
    //         + " would put bigid out-of-range"
    //     )
    // {}
}

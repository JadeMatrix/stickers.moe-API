#line 2 "common/bigid.cpp"


#include "bigid.hpp"


namespace stickers // Big-ID ///////////////////////////////////////////////////
{
    bigid::bigid() {}
    
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
        return static_cast< bool >( value );
    }
    
    const bigid& bigid::operator =( const bigid& o )
    {
        value = o.value;
        return *this;
    }
    
    const bigid& bigid::operator =( long long v )
    {
        if( v >= MIN().value && v <= MAX().value )
            value = v;
        else
            throw bigid_out_of_range{ v };
        return *this;
    }
    
    bigid bigid::operator +( long long i ) const
    {
        return bigid{ value + i };
    }
    
    bigid bigid::operator -( long long i ) const
    {
        return bigid{ value - i };
    }
    
    const bigid& bigid::operator ++()
    {
        *this = value + 1;
        return *this;
    }
    
    const bigid& bigid::operator --()
    {
        *this = value - 1;
        return *this;
    }
    
    const bigid& bigid::operator +=( long long i )
    {
        *this = value + i;
        return *this;
    }
    
    const bigid& bigid::operator -=( long long i )
    {
        *this = value - i;
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
    
    bool bigid::operator <( const bigid& o ) const
    {
        return value < o.value;
    }
    
    bool bigid::operator >( const bigid& o ) const
    {
        return value > o.value;
    }
    
    bool bigid::operator <=( const bigid& o ) const
    {
        return value <= o.value;
    }
    
    bool bigid::operator >=( const bigid& o ) const
    {
        return value >= o.value;
    }
    
    bigid bigid::MIN()
    {
        bigid min;
        min.value = 1000000000000000000ll;
        return min;
    }
    
    bigid bigid::MAX()
    {
        bigid max;
        max.value = 9223372036854775807ll;
        return max;
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
            throw std::invalid_argument{
                "can't convert \"" + str + "\" to a big-ID"
            };
        }
    }
}


namespace stickers // Big-ID exceptions ////////////////////////////////////////
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

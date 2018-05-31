#pragma once
#ifndef STICKERS_MOE_COMMON_SORTING_HPP
#define STICKERS_MOE_COMMON_SORTING_HPP


#include <optional>
#include <string>


namespace stickers
{
    class byte_string : public std::basic_string< std::uint8_t >
    {
    public:
        using std::basic_string< std::uint8_t >::basic_string;
        
        byte_string( const std::string& o ) : basic_string{
            reinterpret_cast< const std::uint8_t* >( o.c_str() ),
            o.size()
        } {}
        byte_string( std::string&& o ) :
            basic_string{ *reinterpret_cast< byte_string* >( &o ) }
        {}
        byte_string( const char* o ) : basic_string{
            reinterpret_cast< const std::uint8_t* >( o )
        } {}
    };
    
    byte_string next_sorting_key_between(
        std::optional< byte_string > before,
        std::optional< byte_string > after
    );
}


#endif

#pragma once
#ifndef STICKERS_MOE_COMMON_DOCUMENT_HPP
#define STICKERS_MOE_COMMON_DOCUMENT_HPP


#include <show.hpp>

#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>


namespace stickers
{
    class document;
    
    using document_key_type = document;
    
    using   null_document = std::monostate;
    using string_document = std::string;
    using   bool_document = bool;
    using    int_document = long;
    using  float_document = double;
    // Rather than just alias `std::map<...>` to `map_document`, make it a new
    // class that exposes `std::map::at()` using the access operator
    class map_document : public std::map< document_key_type, document >
    {
    public:
        using std::map< document_key_type, document >::map;
        using std::map< document_key_type, document >::operator [];
        const document& operator []( const document_key_type& ) const;
    };
    
    using _document_base = std::variant<
          null_document,
        string_document,
          bool_document,
           int_document,
         float_document,
           map_document
    >;
    
    // Requires public inheritance so comparison operators can be used
    class document : public _document_base
    {
    public:
        // Inherit all of `std::variant`'s constructors...
        using _document_base::variant;
        // ... and add a string-type constructor from C-strings because
        // `std::variant` (understandably) can't do this by default, but it's
        // nice to have for map access.
        document( const char* );
        
        std::optional< std::string > mime_type;
        std::optional< std::string > name;
        
        template< typename T > constexpr bool is_a() const
        {
            return std::holds_alternative< T >( *this );
        }
        
        template< typename T > constexpr const T& get() const
        {
            return std::get< T >( *this );
        }
        template< typename T > constexpr T& get()
        {
            return std::get< T >( *this );
        }
    };
    
    // Lossy, intended for debugging purposes
    std::ostream& operator <<( std::ostream&, const document& );
}


#endif

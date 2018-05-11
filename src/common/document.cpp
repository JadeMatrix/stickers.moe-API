#line 2 "common/document.cpp"


#include "document.hpp"

#include "../common/logging.hpp"

#include <sstream>


namespace stickers // Document /////////////////////////////////////////////////
{
    document::document( const char* c_str ) : document( std::string{ c_str } )
    {}
    
    const document& map_document::operator [](
        const document_key_type& key
    ) const
    {
        return this -> at( key );
    }
    
    std::ostream& operator <<( std::ostream& out, const document& doc )
    {
        out << "doc(";
        if( doc.mime_type )
            out
                << "mime=\""
                << log_sanitize( *doc.mime_type )
                << "\""
            ;
        if( doc.name )
            out
                << ( doc.mime_type ? ", " : "" )
                << "name=\""
                << log_sanitize( *doc.name )
                << "\""
            ;
        out << "):";
        
        if( doc.is_a< null_document >() )
        {
            out << "null";
        }
        else if( doc.is_a< string_document >() )
        {
            if( doc.get< string_document >().size() > 64 )
                out
                    << "<string["
                    << doc.get< string_document >().size()
                    << "]>"
                ;
            else
                out
                    << '"'
                    << log_sanitize( doc.get< string_document >() )
                    << '"'
                ;
        }
        else if( doc.is_a< bool_document >() )
        {
            out << ( doc.get< bool_document >() ? "true" : "false" );
        }
        else if( doc.is_a< int_document >() )
        {
            out << doc.get< int_document >();
        }
        else if( doc.is_a< float_document >() )
        {
            out << doc.get< float_document >();
        }
        else if( doc.is_a< map_document >() )
        {
            out << '{';
            for(
                auto iter = doc.get< map_document >().begin();
                iter != doc.get< map_document >().end();
                /* incremented in loop */
            )
            {
                out
                    << iter -> first
                    << " => "
                    << iter -> second
                    << (
                        ++iter != doc.get< map_document >().end() ?
                        ", " : ""
                    )
                ;
            }
            out << '}';
        }
        
        return out;
    }
}

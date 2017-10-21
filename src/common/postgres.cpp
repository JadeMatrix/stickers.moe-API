#include "postgres.hpp"

#include <string>


// split_pg_field() ------------------------------------------------------------


namespace
{
    std::string unescape_pg_field( const std::string& s )
    {
        if( s.front() == '"' && s.back() == '"' )
        {
            std::string buffer;
            
            // Remove opening & closing double quotes (size - 1 because start at
            // one)
            std::string::size_type s_size = s.size() - 1;
            for( std::string::size_type i = 1; i < s_size; ++i )
            {
                if( s[ i ] == '\\' )
                {
                    buffer += s[ i + 1 ];
                    ++i;
                }
                else
                    buffer += s[ i ];
            }
            
            return buffer;
        }
        else
            return s;
    }
}


namespace stickers
{
    std::vector< std::string > split_pg_field( const pqxx::field& f )
    {
        std::vector< std::string > split;
        
        std::string view( f.c_str() );
        
        if( view.front() != '(' || view.back() != ')' )
            throw non_composite_field( f );
        
        std::string buffer;
        
        // Remove opening & closing parentheses (size - 1 because start at one)
        std::string::size_type view_size = view.size() - 1;
        for( std::string::size_type i = 1; i < view_size; ++i )
        {
            if( view[ i ] == ',' && view[ i - 1 ] != '\\' )
            {
                split.push_back( unescape_pg_field( buffer ) );
                buffer.clear();
            }
            else
                buffer += view[ i ];
            
            if( i == view_size - 1 )
                split.push_back( unescape_pg_field( buffer ) );
        }
        
        return split;
    }
}


// non_composite_field ---------------------------------------------------------


namespace stickers
{
    non_composite_field::non_composite_field( const pqxx::field& f ) :
        message( "not a composite field: " + std::string( f.c_str() ) )
    {}
    
    const char* non_composite_field::what() const noexcept
    {
        return message.c_str();
    }
}

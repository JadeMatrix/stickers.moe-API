#line 2 "server/parse.cpp"


#include "parse.hpp"

#include "handler.hpp"
#include "../common/logging.hpp"
#include "../common/string_utils.hpp"

#include <iterator> // std::istreambuf_iterator


namespace
{
    stickers::document_type parse_form_urlencoded( const std::string& str )
    {
        stickers::document_type parsed{ "{}"_json };
        
        for( const auto& exp_set : stickers::split<
            std::vector< std::string >
        >( str, std::string{ "&" } ) )
        {
            auto exp = stickers::split< std::vector< std::string > >(
                exp_set,
                std::string{ "=" }
            );
            
            if( exp.size() == 1 )
                parsed[ show::url_decode( exp[ 0 ] ) ] = true;
            else
            {
                auto value = show::url_decode( exp.back() );
                for( auto i = 0; i < exp.size() - 1; ++i )
                    parsed[ show::url_decode( exp[ i ] ) ] = value;
            }
        }
        
        return parsed;
    }
}


namespace stickers
{
    document_type parse_request_content( show::request& request )
    {
        if( request.unknown_content_length() )
            throw handler_exit{
                { 400, "Bad Request" },
                "Missing \"Content-Length\" header"
            };
        
        auto content_type_found = request.headers().find( "Content-Type" );
        
        if(
            content_type_found == request.headers().end()
            || content_type_found -> second.size() != 1
        )
            throw handler_exit{
                { 400, "Bad Request" },
                "Indeterminate content type"
            };
        
        if( content_type_found -> second[ 0 ] == "application/json" )
        {
            document_type parsed{ "{}"_json };
            bool malformed_json = false;
            
            try
            {
                std::istream request_stream{ &request };
                request_stream >> parsed;
                
                malformed_json = !request.eof();
            }
            catch( const nlj::json::parse_error& e )
            {
                malformed_json = true;
            }
            
            if( malformed_json )
                throw handler_exit{
                    { 400, "Bad Request" },
                    "Malformed JSON payload"
                };
            else
                return parsed;
        }
        else if(
            content_type_found -> second[ 0 ]
            == "application/x-www-form-urlencoded"
        )
        {
            return parse_form_urlencoded( std::string{
                std::istreambuf_iterator< char >{ &request },
                {}
            } );
        }
        else
        {
            // Stick a warning in the log in case we're not supporting something
            // common
            STICKERS_LOG(
                log_level::WARNING,
                "got unsupported content type \"",
                log_sanitize( content_type_found -> second[ 0 ] ),
                "\" in \"",
                request.method(),
                "\" request"
            );
            throw handler_exit{
                { 400, "Bad Request" },
                (
                    "Unsupported content type '"
                    + log_sanitize( content_type_found -> second[ 0 ] )
                    + "'"
                )
            };
        }
    }
}

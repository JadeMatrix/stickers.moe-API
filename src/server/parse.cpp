#line 2 "server/parse.cpp"


#include "parse.hpp"

#include "handler.hpp"
#include "../common/config.hpp"
#include "../common/json.hpp"
#include "../common/logging.hpp"
#include "../common/string_utils.hpp"

#include <show/constants.hpp>
#include <show/multipart.hpp>

#include <cmath>
#include <iterator> // std::istreambuf_iterator
#include <optional>
#include <vector>


namespace // Utilities /////////////////////////////////////////////////////////
{
    struct content_type_info
    {
        std::string mime_type;
        std::optional< std::string > header_remainder;
    };
    
    std::optional< content_type_info > content_type_from_headers(
        const show::headers_type& headers
    )
    {
        auto found_content_type = headers.find( "Content-Type" );
        
        if(
            found_content_type == headers.end()
            || found_content_type -> second.size() != 1
        )
            return std::nullopt;
        
        auto& header_value{ found_content_type -> second[ 0 ] };
        auto split_pos{ header_value.find( ";" ) };
        auto mime_type{ header_value.substr(
            0,
            split_pos
        ) };
        
        if( split_pos != std::string::npos )
        {
            ++split_pos;
            while( split_pos < header_value.size() )
                switch( header_value[ split_pos ] )
                {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    ++split_pos;
                    break;
                default:
                    return content_type_info{
                        mime_type,
                        header_value.substr( split_pos )
                    };
                }
        }
        
        return content_type_info{ mime_type, std::nullopt };
    }
    
    std::string boundary_from_content_type_remainder(
        const std::string                 & mime_type,
        const std::optional< std::string >& header_remainder
    )
    {
        if( header_remainder )
        {
            std::string boundary_designator{ "boundary=" };
            auto boundary_begin{ header_remainder -> find(
                boundary_designator
            ) };
            
            if( boundary_begin != std::string::npos )
            {
                boundary_begin += boundary_designator.size();
                
                auto boundary_end{ header_remainder -> find(
                    ";",
                    boundary_begin
                ) };
                auto boundary{ header_remainder -> substr(
                    boundary_begin,
                    boundary_end
                ) };
                
                if( boundary.size() > 0 )
                    return boundary;
            }
        }
        
        throw stickers::handler_exit{
            show::code::BAD_REQUEST,
            (
                "Missing boundary for "
                + stickers::log_sanitize( mime_type )
            )
        };
    }
    
    struct multipart_info
    {
        std::optional< std::string > name;
        std::optional< std::string > filename;
    };
    
    std::optional< std::string > decode_filename(
        std::string::size_type    filename_start,
        const std::string       & header_value,
        const show::headers_type& request_headers
    )
    {
        auto found_user_agent = request_headers.find( "User-Agent" );
        
        if(
            found_user_agent != request_headers.end()
            && found_user_agent -> second.size() == 1
        )
        {
            auto& user_agent{ found_user_agent -> second[ 0 ] };
            
            if(
                   user_agent.find( "Chrome" ) != std::string::npos
                || user_agent.find( "Safari" ) != std::string::npos
            )
            {
                /*
                Parse from `filename="` to `"` non-inclusive, then replace all
                `%22` with `"` (but no other percent/URL-encoded sequences)
                */
                
                auto filename{ header_value.substr(
                    filename_start,
                    header_value.find( "\"", filename_start )
                ) };
                
                std::string::size_type esc_seq_pos;
                while(
                    ( esc_seq_pos = header_value.find( "%22" ) )
                    != std::string::npos
                )
                    filename.replace( esc_seq_pos, 3, "\"" );
                
                return filename;
            }
            else if( user_agent.find( "Firefox" ) != std::string::npos )
            {
                /*
                Parse from `filename="` to `"` non-inclusive, replacing all `\"`
                with `"` (but no other escape sequences, including `\\`)
                */
                
                auto raw_filename{ header_value.substr( filename_start ) };
                
                std::string filename;
                filename.reserve( raw_filename.size() );
                
                for(
                    std::string::size_type i{ 0 };
                    i < raw_filename.size();
                    ++i
                )
                    if(
                        raw_filename[ i ] == '\\'
                        && i + 1 < raw_filename.size()
                        && raw_filename[ i + 1 ] == '"'
                    )
                    {
                        filename += '"';
                        ++i;
                    }
                    else if( raw_filename[ i ] == '"' )
                        break;
                    else
                        filename += raw_filename[ i ];
                
                return filename;
            }
            else if( user_agent.find( "Windows" ) != std::string::npos )
                /*
                Apparently IE doesn't encode the filename at all because `"` is
                not permissable in NTFS filenames anyways.
                Source: https://github.com/rack/rack/issues/323
                */
                return header_value.substr(
                    filename_start,
                    header_value.find( "\"", filename_start ) - filename_start
                );
        }
        
        // Best-guess if no other method worked/matched
        
    #if 01
        auto end_pos{ header_value.find( "\"; ", filename_start ) };
        if( end_pos == std::string::npos )
            end_pos = header_value.find( "\"", filename_start );
        
        return header_value.substr(
            filename_start,
            end_pos - filename_start
        );
    #else
        return std::nullopt;
    #endif
    }
    
    std::optional< multipart_info > split_multipart_info(
        const show::headers_type& headers,
        const show::headers_type& request_headers
    )
    {
        auto found_content_disp{ headers.find( "Content-Disposition" ) };
        
        if(
            found_content_disp == headers.end()
            || found_content_disp -> second.size() != 1
        )
            return std::nullopt;
        
        multipart_info info;
        
        /*
        Examples:
        
        Content-Disposition: form-data; name="submit-name"
        Content-Disposition: file; filename="file1.txt"
        Content-Disposition: form-data; name="files"; filename="file1.txt"
        */
        
        auto& header_value{ found_content_disp -> second[ 0 ] };
        
        std::string     name_designator  {           "name=\"" };
        std::string filename_designator_1{ "filename*=UTF-8''" };
        std::string filename_designator_2{       "filename=\"" };
        
        auto name_begin{ header_value.find( name_designator ) };
        if( name_begin != std::string::npos )
        {
            name_begin += name_designator.size();
            
            auto name_end{ header_value.find( "\"; ", name_begin ) };
            if( name_end == std::string::npos )
                name_end = header_value.find( "\"", name_begin );
            
            info.name = header_value.substr(
                name_begin,
                name_end - name_begin
            );
        }
        
        auto filename_begin{ header_value.find( filename_designator_1 ) };
        
        if( filename_begin != std::string::npos )
        {
            filename_begin += filename_designator_1.size();
            
            try
            {
                info.filename = show::url_decode( header_value.substr(
                    filename_begin,
                    header_value.find( ";", filename_begin )
                ) );
            }
            catch( const show::url_decode_error& e )
            {
                throw stickers::handler_exit{
                    show::code::BAD_REQUEST,
                    (
                        "RFC 5987 filename in Content-Disposition header not "
                        "a valid URL-encoded string: "
                        + std::string{ e.what() }
                    )
                };
            }
        }
        else
        {
            filename_begin = header_value.find( filename_designator_2 );
            
            if( filename_begin != std::string::npos )
                info.filename = decode_filename(
                    filename_begin + filename_designator_2.size(),
                    header_value,
                    request_headers
                );
        }
        
        return info;
    }
    
    stickers::document json_to_document( nlj::json& parsed )
    {
        if( parsed.is_null() )
            return nullptr;
        else if( parsed.is_boolean() )
            return parsed.get< bool >();
        else if( parsed.is_number() )
        {
            double  integer_part;
            double fraction_part = std::modf(
                parsed.get< double >(),
                &integer_part
            );
            
            if( fraction_part > 0 )
                return parsed.get< double >();
            else
                return parsed.get< long >();
        }
        else if( parsed.is_object() )
        {
            stickers::document doc{ stickers::map_document{} };
            for( auto pair = parsed.begin(); pair != parsed.end(); ++pair )
                doc.get< stickers::map_document >()[ pair.key() ] = json_to_document( pair.value() );
            return doc;
        }
        else if( parsed.is_array() )
        {
            stickers::document doc{ stickers::map_document{} };
            for( stickers::int_document i = 0; i < parsed.size(); ++i )
                doc.get< stickers::map_document >()[ i ] = json_to_document( parsed[ i ] );
            return doc;
        }
        else /*if( parsed.is_string() )*/
            return std::move( parsed.get< std::string >() );
    }
}


namespace // Parser declarations ///////////////////////////////////////////////
{
    /*
    These (can be) recursive so they need the following arguments:
        buffer          stream buffer being read from
        headers         headers for the current segment of content (may be
                        request headers)
        method          request method for inclusion in error messages
        request_headers top-level request headers for user agent string, etc.
    */
    
    stickers::document parse_json(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    );
    stickers::document parse_form_urlencoded(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    );
    stickers::document parse_multipart(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    );
}


namespace // Recursive request parse implementation ////////////////////////////
{
    stickers::document parse_request_content_recursive(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    )
    {
        auto content_type{ content_type_from_headers( headers ) };
        
        if( content_type )
        {
            auto& mime_type{ content_type -> mime_type };
            auto mime_supertype{ mime_type.substr(
                0,
                mime_type.find( "/" )
            ) };
            
            if( mime_type == "application/json" )
                return parse_json(
                    buffer,
                    headers,
                    method,
                    request_headers
                );
            else if( mime_type == "application/x-www-form-urlencoded" )
                return parse_form_urlencoded(
                    buffer,
                    headers,
                    method,
                    request_headers
                );
            else if( mime_supertype == "multipart" )
                return parse_multipart(
                    buffer,
                    headers,
                    method,
                    request_headers
                );
        }
        
        // Default to a single binary blob
        stickers::document parsed{ std::string{
            std::istreambuf_iterator< char >{ &buffer },
            {}
        } };
        if( content_type )
            parsed.mime_type = content_type -> mime_type;
        return parsed;
    }
}


namespace // Parser implementations ////////////////////////////////////////////
{
    stickers::document parse_json(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    )
    {
        nlj::json parsed;
        bool malformed_json{ false };
        
        try
        {
            std::istream request_stream{ &buffer };
            request_stream >> parsed;
            malformed_json = !request_stream.good();
        }
        catch( const nlj::json::parse_error& e )
        {
            malformed_json = true;
        }
        
        if( malformed_json )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "Malformed JSON payload"
            };
        else
        {
            auto doc{ json_to_document( parsed ) };
            doc.mime_type = "application/json";
            return doc;
        }
    }
    
    stickers::document parse_form_urlencoded(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    )
    {
        stickers::document doc{ stickers::map_document{} };
        doc.mime_type = "application/x-www-form-urlencoded";
        
        for( const auto& exp_set : stickers::split<
            std::vector< std::string >
        >( std::string{
            std::istreambuf_iterator< char >{ &buffer },
            {}
        }, std::string{ "&" } ) )
        {
            auto exp{ stickers::split< std::vector< std::string > >(
                exp_set,
                std::string{ "=" }
            ) };
            
            if( exp.size() == 1 )
                doc.get< stickers::map_document >()[ show::url_decode( exp[ 0 ] ) ] = true;
            else
            {
                auto value{ show::url_decode( exp.back() ) };
                for( auto i = 0; i < exp.size() - 1; ++i )
                    doc.get< stickers::map_document >()[ show::url_decode( exp[ i ] ) ] = value;
            }
        }
        
        return doc;
    }
    
    stickers::document parse_multipart(
        std::streambuf          & buffer,
        const show::headers_type& headers,
        const std::string       & method,
        const show::headers_type& request_headers
    )
    {
        auto content_type{ content_type_from_headers( headers ) };
        
        if( !content_type )
            return std::string{
                std::istreambuf_iterator< char >{ &buffer },
                {}
            };
        
        show::multipart parser{
            buffer,
            boundary_from_content_type_remainder(
                content_type -> mime_type,
                content_type -> header_remainder
            )
        };
        
        stickers::document doc{ stickers::map_document{} };
        
        for( auto& segment : parser )
        {
            stickers::document segment_name{};
            auto segment_doc{ parse_request_content_recursive(
                segment,
                segment.headers(),
                method,
                request_headers
            ) };
            
            auto info{ split_multipart_info(
                segment.headers(),
                request_headers
            ) };
            if( info )
            {
                if( info -> filename )
                    segment_doc.name = *( info -> filename );
                
                if( info -> name )
                    segment_name = *( info -> name );
            }
            
            doc.get< stickers::map_document >().emplace( std::make_pair(
                std::move( segment_name ),
                std::move( segment_doc  )
            ) );
        }
        
        return doc;
    }
}


namespace stickers // Request parse ////////////////////////////////////////////
{
    document parse_request_content( show::request& request )
    {
        auto max_length{
            config()[ "server" ][ "max_request_bytes" ].get< std::streamsize >()
        };
        
        if( request.unknown_content_length() )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "Missing \"Content-Length\" header"
            };
        else if( request.content_length() > max_length )
            throw handler_exit{
                show::code::PAYLOAD_TOO_LARGE,
                (
                    "Maximum request content size is "
                    + std::to_string( max_length )
                    + " bytes"
                )
            };
        
        return parse_request_content_recursive(
            request,
            request.headers(),
            request.method(),
            request.headers()
        );
    }
}

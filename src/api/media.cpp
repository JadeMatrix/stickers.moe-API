#line 2 "api/media.cpp"


#include "media.hpp"

#include "../common/config.hpp"
#include "../common/formatting.hpp"
#include "../common/logging.hpp"
#include "../common/uuid.hpp"
#include "../handlers/handlers.hpp"
#include "../server/parse.hpp"

#include <show.hpp>
#include <show/constants.hpp>
#include <show/multipart.hpp>

#include <fstream>
#include <sstream>


namespace // Utilities /////////////////////////////////////////////////////////
{
    std::string standard_extension_for_mime_type( const std::string& mime_type )
    {
        if( mime_type == "image/jpeg" )
            return ".jpeg";
        else if( mime_type == "image/png" )
            return ".png";
        else if( mime_type == "image/gif" )
            return ".gif";
        else if( mime_type == "video/webm" )
            return ".webm";
        else if( mime_type == "text/plain" )
            return ".txt";
        else
            throw stickers::unacceptable_mime_type{ mime_type };
    }
    
    std::string format_image_subpath(
        const stickers::sha256& hash,
        const std::string     & mime_type
    )
    {
        std::string hash_hex = hash.hex_digest();
        return (
              hash_hex.substr( 0, 2 )
            + "/"
            + hash_hex.substr( 2, 2 )
            + "/"
            + hash_hex.substr( 4 )
            + standard_extension_for_mime_type( mime_type )
        );
    }
    
    std::experimental::filesystem::path image_hash_to_disk_path(
        const stickers::sha256& hash,
        const std::string     & mime_type
    )
    {
        return std::experimental::filesystem::u8path(
            stickers::config()[ "media" ][ "media_directory" ].get<
                std::string
            >()
            + "/"
            + format_image_subpath( hash, mime_type )
        );
    }
    
    std::string image_hash_to_url(
        const stickers::sha256& hash,
        const std::string     & mime_type
    )
    {
        return (
            stickers::config()[ "media" ][ "base_url" ].get< std::string >()
            + format_image_subpath( hash, mime_type )
        );
    }
    
    std::string guess_mime_type(
        const std::optional< std::string >& file_name,
        const                std::string  & beginning_chunk,
        const                std::string  &    ending_chunk
    )
    {
        std::string extension;
        if( file_name )
            extension = show::_ASCII_upper(
                file_name -> substr( file_name -> rfind( "." ) )
            );
        
        // See https://www.garykessler.net/library/file_sigs.html for more
        // information
        
        static const std::string magic_num_jpeg{ '\xff', '\xd8'                                                 };
        static const std::string magic_num_png { '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' };
        static const std::string magic_num_gif7{ '\x47', '\x49', '\x46', '\x38', '\x37', '\x61'                 };
        static const std::string magic_num_gif9{ '\x47', '\x49', '\x46', '\x38', '\x39', '\x61'                 };
        static const std::string magic_num_webm{ '\x1a', '\x45', '\xdf', '\xa3'                                 };
        
        static const std::string   trailer_jpeg{ '\xff', '\xd9'                                                 };
        static const std::string   trailer_png { '\x49', '\x45', '\x4e', '\x44', '\xae', '\x42', '\x60', '\x82' };
        static const std::string   trailer_gif { '\x00', '\x3b'                                                 };
        
        if(
            ( !file_name || ( extension == ".JPG" || extension == ".JPEG" ) )
            && beginning_chunk.substr( 0, magic_num_jpeg.size()                  ) == magic_num_jpeg
            &&    ending_chunk.substr( ending_chunk.size() - trailer_jpeg.size() ) == trailer_jpeg
            && beginning_chunk[ 2 ] == '\xff'
            && (
                   beginning_chunk[ 3 ] >= '\xe0'
                && beginning_chunk[ 3 ] <= '\xef'
            )
        )
            return "image/jpeg";
        else if(
            ( !file_name || extension == ".PNG" )
            && beginning_chunk.substr( 0, magic_num_png.size()                  ) == magic_num_png
            &&    ending_chunk.substr( ending_chunk.size() - trailer_png.size() ) == trailer_png
        )
            return "image/png";
        else if(
            ( !file_name || extension == ".GIF" )
            && (
                   beginning_chunk.substr( 0, magic_num_gif7.size() ) == magic_num_gif7
                || beginning_chunk.substr( 0, magic_num_gif9.size() ) == magic_num_gif9
            )
            && ending_chunk.substr( ending_chunk.size() - trailer_gif.size() ) == trailer_gif
        )
            return "image/gif";
        else if(
            ( !file_name || extension == ".WEBM" )
            && beginning_chunk.substr( 0, magic_num_webm.size() ) == magic_num_webm
        )
            return "video/webm";
        else
            throw stickers::indeterminate_mime_type{};
    }
}


namespace // Internal implementations //////////////////////////////////////////
{
    stickers::media_info load_media_info_impl(
        const stickers::sha256& hash,
        pqxx::work& transaction
    )
    {
        auto result = transaction.exec_params(
            PSQL(
                SELECT
                    mime_type,
                    decency,
                    original_filename,
                    uploaded,
                    uploaded_by
                FROM media.images
                WHERE image_hash = $1
                ;
            ),
            pqxx::binarystring{ hash.raw_digest() }
        );
        
        if( result.size() < 1 )
            throw stickers::no_such_media{ hash };
        
        auto& row = result[ 0 ];
        
        auto mime_type = row[ "mime_type" ].as< std::string >();
        stickers::media_info info{
            image_hash_to_disk_path( hash, mime_type ),
            image_hash_to_url      ( hash, mime_type ),
            mime_type,
            row[ "decency"     ].as< stickers::media_decency >(),
            std::nullopt,
            row[ "uploaded"    ].as< stickers::timestamp     >(),
            row[ "uploaded_by" ].as< stickers::bigid         >()
        };
        
        if( !row[ "original_filename" ].is_null() )
            info.original_filename =
                row[ "original_filename" ].as< std::string >();
        
        return info;
    }
    
    struct file_info
    {
        std::experimental::filesystem::path temp_file_path;
        stickers::sha256                    file_hash;
        std::string                         mime_type;
    };
    
    file_info save_temp_file(
        std::streambuf                    & file_contents,
        const std::optional< std::string >& original_filename
    )
    {
        auto temp_file_id = stickers::uuid::generate();
        
        auto temp_file_path{ std::experimental::filesystem::u8path(
            stickers::config()[ "media" ][ "temp_file_location" ].get<
                std::string
            >()
            + "/"
            + temp_file_id.hex_value()
        ) };
        std::experimental::filesystem::create_directories(
            temp_file_path.parent_path()
        );
        std::fstream temp_file{
            temp_file_path,
            std::ios::out | std::ios::in | std::ios::binary
        };
        
        std::istream file_stream{ &file_contents };
        std::streamsize file_size{ 0 };
        stickers::sha256::builder hash_builder;
        
        // Write the file to temp location in kilobyte-sized chunks until less
        // than a kilobyte is left (reading another kilobyte will fail), then
        // write what's left in the file
        char buffer[ 1024 ];
        while( file_stream.read( buffer, sizeof( buffer ) ) )
        {
            temp_file.write    ( buffer, sizeof( buffer ) );
            hash_builder.append( buffer, sizeof( buffer ) );
            file_size += sizeof( buffer );
        }
        auto remaining = file_stream.gcount();
        temp_file.write    ( buffer, remaining );
        hash_builder.append( buffer, remaining );
        file_size += remaining;
        
        STICKERS_LOG(
            stickers::log_level::VERBOSE,
            "wrote ",
            file_size,
            " bytes to temp file \"",
            stickers::log_sanitize( temp_file_path ),
            "\""
        );
        
        auto file_hash = hash_builder.generate_and_clear();
        
        temp_file.seekg( 0, std::ios::beg );
        std::streamsize chunk_bytes{ 64 };
        
        // Once again running afoul of the CLang stdlib bug where the
        // `std::string(char*, count)` constructor can't be used with curly
        // braces
        std::string beginning_chunk;
        if( temp_file.read( buffer, chunk_bytes ) )
            beginning_chunk = std::string( buffer, chunk_bytes );
        else
        {
            remaining = temp_file.gcount();
            temp_file.read( buffer, remaining );
            beginning_chunk = std::string( buffer, remaining );
        }
        
        std::string mime_type;
        try
        {
            if( file_size < chunk_bytes )
                mime_type = guess_mime_type(
                    original_filename,
                    beginning_chunk,
                    beginning_chunk
                );
            else
            {
                temp_file.seekg( file_size - chunk_bytes );
                temp_file.read( buffer, chunk_bytes );
                
                mime_type = guess_mime_type(
                    original_filename,
                    beginning_chunk,
                    std::string( buffer, chunk_bytes )
                );
            }
        }
        catch( ... )
        {
            std::experimental::filesystem::remove( temp_file_path );
            throw;
        }
        
        return {
            temp_file_path,
            file_hash,
            mime_type
        };
    }
    
    file_info save_temp_file(
        std::string                       & file_contents,
        const std::optional< std::string >& original_filename
    )
    {
        auto mime_type{ guess_mime_type(
            original_filename,
            file_contents.substr( 0, 64 ),
            file_contents.substr(
                file_contents.size() - 64
            )
        ) };
        auto file_hash{ stickers::sha256::make( file_contents ) };
        auto temp_file_id{ stickers::uuid::generate() };
        
        auto temp_file_path{ std::experimental::filesystem::u8path(
            stickers::config()[ "media" ][ "temp_file_location" ].get<
                std::string
            >()
            + "/"
            + temp_file_id.hex_value()
        ) };
        
        std::fstream temp_file{
            temp_file_path,
            std::ios::out | std::ios::binary
        };
        
        temp_file.write( file_contents.data(), file_contents.size() );
        
        STICKERS_LOG(
            stickers::log_level::VERBOSE,
            "wrote ",
            file_contents.size(),
            " bytes to temp file \"",
            stickers::log_sanitize( temp_file_path ),
            "\""
        );
        
        return {
            temp_file_path,
            file_hash,
            mime_type
        };
    }
    
    stickers::media save_media_impl(
        const std::experimental::filesystem::path& temp_file_path,
        const stickers::sha256                   & file_hash,
        const std::string                        & mime_type,
        stickers::media_decency                    decency,
        const std::optional< std::string >       & original_filename,
        const stickers::audit::blame             & blame
    )
    {
        auto connection = stickers::postgres::connect();
        pqxx::work transaction{ *connection };
        
        try
        {
            // Early return if media already exists
            return {
                file_hash,
                load_media_info_impl( file_hash, transaction )
            };
        }
        catch( const stickers::no_such_media& e ) {}
        
        transaction.exec_params(
            PSQL(
                INSERT INTO media.images (
                    image_hash,
                    mime_type,
                    decency,
                    original_filename,
                    uploaded,
                    uploaded_by,
                    uploaded_from
                )
                VALUES ( $1, $2, $3, $4, $5, $6, $7 )
                ;
            ),
            pqxx::binarystring{ file_hash.raw_digest() },
            mime_type,
            decency,
            original_filename,
            blame.when,
            blame.who,
            blame.where
        );
        
        auto final_file_path = image_hash_to_disk_path( file_hash, mime_type );
        
        std::experimental::filesystem::create_directories(
            final_file_path.parent_path()
        );
        std::experimental::filesystem::rename(
            temp_file_path,
            final_file_path
        );
        
        STICKERS_LOG(
            stickers::log_level::VERBOSE,
            "moved temp file \"",
            stickers::log_sanitize( temp_file_path ),
            "\" to final location \"",
            stickers::log_sanitize( temp_file_path ),
            "\""
        );
        
        // Commit transaction _after_ moving file
        transaction.commit();
        
        STICKERS_LOG(
            stickers::log_level::INFO,
            "user ",
            blame.who,
            " uploaded file with SHA-256 ",
            file_hash.hex_digest()
        );
        
        return {
            file_hash,
            {
                final_file_path,
                image_hash_to_url( file_hash, mime_type ),
                mime_type,
                decency,
                original_filename,
                blame.when,
                blame.who
            }
        };
    }
}


namespace stickers // Media ////////////////////////////////////////////////////
{
    media save_media(
        std::streambuf              & file_contents,
        std::optional< std::string >  original_filename,
        media_decency                 decency,
        const audit::blame          & blame
    )
    {
        auto info{ save_temp_file( file_contents, original_filename ) };
        
        return save_media_impl(
            info.temp_file_path,
            info.file_hash,
            info.mime_type,
            decency,
            original_filename,
            blame
        );
    }
    
    media save_media(
        show::request     & upload_request,
        const audit::blame& blame
    )
    {
        auto upload_doc{ parse_request_content( upload_request ) };
        
        if( !upload_doc.is_a< stickers::map_document >() )
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "invalid data format"
            };
        
        auto& upload_map{ upload_doc.get< stickers::map_document >() };
        
        auto found_decency_field = upload_map.find( "decency" );
        auto decency{ media_decency::SAFE };
        if( found_decency_field == upload_map.end() )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "missing required field \"decency\""
            };
        
        auto& decency_field{ found_decency_field -> second };
        
        if( !decency_field.is_a< string_document >() )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "required field \"decency\" must be a string"
            };
        else if( decency_field.get< string_document >() == "safe" )
            decency = media_decency::SAFE;
        else if( decency_field.get< string_document >() == "questionable" )
            decency = media_decency::QUESTIONABLE;
        else if( decency_field.get< string_document >() == "explicit" )
            decency = media_decency::EXPLICIT;
        else
            throw handler_exit{
                show::code::BAD_REQUEST,
                (
                    "required field \"decency\" must be one of \"safe\", "
                    "\"questionable\", or \"explicit\""
                )
            };
        
        // TODO: `stickers::file_document` type
        auto found_file_field = upload_map.find( "file" );
        if( found_file_field == upload_map.end() )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "missing required field \"file\""
            };
        
        auto& file_field{ found_file_field -> second };
        
        if(
            !file_field.is_a< string_document >()
            || !file_field.mime_type
        )
            throw handler_exit{
                show::code::BAD_REQUEST,
                "required field \"file\" must be a binary segment"
            };
        
        auto info{ save_temp_file(
            file_field.get< string_document >(),
            file_field.name
        ) };
        
        return save_media_impl(
            info.temp_file_path,
            info.file_hash,
            info.mime_type,
            decency,
            file_field.name,
            blame
        );
    }
    
    media_info load_media_info( const sha256& hash )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        return load_media_info_impl( hash, transaction );
    }
}


namespace stickers // Exceptions ///////////////////////////////////////////////
{
    no_such_media::no_such_media( const sha256& hash ) :
        no_such_record_error{
            "no such media record for file with hash " + hash.hex_digest()
        },
        hash{ hash }
    {}
    
    indeterminate_mime_type::indeterminate_mime_type() :
        std::runtime_error{ "indeterminate_mime_type" }
    {}
    
    unacceptable_mime_type::unacceptable_mime_type(
        const std::string& mime_type
    ) :
        std::invalid_argument{
            "unnaceptable or unsupported MIME type \""
            + log_sanitize( mime_type )
            + "\""
        },
        mime_type{ mime_type }
    {}
}


namespace stickers // Assertion ////////////////////////////////////////////////
{
    void _assert_media_exist_impl::exec(
        pqxx::work       & transaction,
        const std::string& ids_string
    )
    {
        std::string query_string;
        
        ff::fmt(
            query_string,
            PSQL(
                WITH lookfor AS (
                    SELECT UNNEST( ARRAY[ {0} ] ) AS image_hash
                )
                SELECT lookfor.image_hash
                FROM
                    lookfor
                    LEFT JOIN media.images AS img
                        ON img.image_hash = lookfor.image_hash
                WHERE img.image_hash IS NULL
                ;
            ),
            ids_string
        );
        
        auto result = transaction.exec( query_string );
        
        if( result.size() > 0 )
            throw no_such_media{ result[ 0 ][ 0 ].as< sha256 >() };
    }
}

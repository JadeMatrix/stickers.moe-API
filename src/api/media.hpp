#pragma once
#ifndef STICKERS_MOE_API_MEDIA_HPP
#define STICKERS_MOE_API_MEDIA_HPP


#include "../audit/blame.hpp"
#include "../common/crud.hpp"
#include "../common/hashing.hpp"
#include "../common/postgres.hpp"
#include "../common/timestamp.hpp"

#include <experimental/filesystem>
#include <optional>
#include <string>
#include <streambuf>

#include <show.hpp>


namespace stickers
{
    enum class media_decency
    {
        SAFE,
        QUESTIONABLE,
        EXPLICIT
    };
    
    struct media_info
    {
        std::experimental::filesystem::path file_path;
        std::string                         file_url;
        std::string                         mime_type;
        media_decency                       decency;
        std::optional< std::string >        original_filename;
        timestamp                           uploaded;
        bigid                               uploaded_by;
    };
    
    struct media
    {
        sha256     file_hash;
        media_info info;
    };
    
    // May throw `indeterminate_mime_type` or `unacceptable_mime_type`
    media save_media(
        std::streambuf                    & file_contents,
        const std::optional< std::string >&  original_filename,
        const std::optional< std::string >&  mime_type,
        media_decency                       decency,
        const audit::blame                & blame
    );
    // May throw `indeterminate_mime_type`, `unacceptable_mime_type`, or
    // `handler_exit`
    media save_media(
        show::request     & upload_request,
        const audit::blame& blame
    );
    
    media_info load_media_info( const sha256& );
    
    class _assert_media_exist_impl
    {
        template< class Container > friend void assert_media_exist(
            pqxx::work     &,
            const Container&
        );
        _assert_media_exist_impl();
        static void exec( pqxx::work&, const std::string& );
    };
    
    // ACID-safe assert; if any of the supplied hashes do not correspond to a
    // record, this will throw `no_such_media` for one of those hashes
    template< class Container = std::initializer_list< sha256 > >
    void assert_media_exist(
        pqxx::work     & transaction,
        const Container& hashes
    )
    {
        _assert_media_exist_impl::exec(
            transaction,
            postgres::format_variable_list( transaction, hashes )
        );
    }
    
    class no_such_media : public no_such_record_error
    {
    public:
        const sha256 hash;
        no_such_media( const sha256& );
    };
    
    class indeterminate_mime_type : public std::runtime_error
    {
    public:
        indeterminate_mime_type();
    };
    
    class unacceptable_mime_type : public std::invalid_argument
    {
    public:
        const std::string mime_type;
        unacceptable_mime_type( const std::string& mime_type );
    };
}


// Template specialization of `pqxx::string_traits<>(&)` for
// `stickers::media_decency`, which allows use of `pqxx::field::to<>(&)` and
// `pqxx::field::as<>(&)`
namespace pqxx
{
    template<> struct string_traits< stickers::media_decency >
    {
        using subject_type = stickers::media_decency;
        
        static constexpr const char* name() noexcept {
            return "stickers::media_decency";
        }
        
        static constexpr bool has_null() noexcept { return false; }
        
        static bool is_null( const subject_type& ) { return false; }
        
        [[noreturn]] static stickers::media_decency null()
        {
            internal::throw_null_conversion( name() );
        }
        
        static void from_string( const char str[], subject_type& v )
        {
            std::string strval{ str };
            if( strval == "safe" )
                v = stickers::media_decency::SAFE;
            else if( strval == "questionable" )
                v = stickers::media_decency::QUESTIONABLE;
            else if( strval == "explicit" )
                v = stickers::media_decency::EXPLICIT;
            else
                throw argument_error{
                    "Failed conversion to "
                    + static_cast< std::string >( name() )
                    + ": '"
                    + strval
                    + "'"
                };
        }
        
        static std::string to_string( const subject_type& v )
        {
            switch( v )
            {
            case stickers::media_decency::SAFE:
                return "safe";
            case stickers::media_decency::QUESTIONABLE:
                return "questionable";
            case stickers::media_decency::EXPLICIT:
                return "explicit";
            }
        }
    };
}


#endif

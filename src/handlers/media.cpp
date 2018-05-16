#line 2 "handlers/media.cpp"


#include "handlers.hpp"

#include "../api/media.hpp"
#include "../common/auth.hpp"
#include "../common/config.hpp"
#include "../common/json.hpp"
#include "../common/logging.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>


namespace
{
    void media_info_to_json(
        const stickers::sha256    & hash,
        const stickers::media_info& info,
        nlj::json                 & media_json
    )
    {
        media_json = {
            { "hash"             , hash.hex_digest()                         },
            { "location"         , info.file_url                             },
            { "mime_type"        , info.mime_type                            },
            { "original_filename", nullptr                                   },
            { "uploaded"         , stickers::to_iso8601_str( info.uploaded ) },
            { "uploaded_by"      , info.uploaded_by                          }
        };
        
        if( info.original_filename )
            media_json[ "original_filename" ] = *info.original_filename;
        
        switch( info.decency )
        {
        case stickers::media_decency::SAFE:
            media_json[ "decency" ] = "safe";
            break;
        case stickers::media_decency::QUESTIONABLE:
            media_json[ "decency" ] = "questionable";
            break;
        case stickers::media_decency::EXPLICIT:
            media_json[ "decency" ] = "explicit";
            break;
        }
    }
}


namespace stickers
{
    void handlers::upload_media(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto auth = authenticate( request );
        permissions_assert_all(
            auth.user_permissions,
            { "edit_public_pages" }
        );
        
        try
        {
            auto uploaded{ save_media(
                request,
                {
                    auth.user_id,
                    "upload media",
                    now(),
                    request.client_address()
                }
            ) };
            
            nlj::json media_json;
            media_info_to_json( uploaded.file_hash, uploaded.info, media_json );
            auto media_json_string{ media_json.dump() };
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::CREATED,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( media_json_string.size() )
                    } },
                    { "Location", { uploaded.info.file_url } }
                }
            };
            
            response.sputn(
                media_json_string.c_str(),
                media_json_string.size()
            );
        }
        catch( const indeterminate_mime_type& e )
        {
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "indeterminate mime type"
            };
        }
        catch( const unacceptable_mime_type& e )
        {
            throw stickers::handler_exit{
                show::code::BAD_REQUEST,
                "media not a supported MIME/file type"
            };
        }
    }
    
    void handlers::get_media_info(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto found_hash_variable{ variables.find( "hash" ) };
        if( found_hash_variable == variables.end() )
            throw handler_exit{ show::code::NOT_FOUND, "need an image hash" };
        
        try
        {
            auto hash{ sha256::from_hex_string(
                found_hash_variable -> second
            ) };
            
            auto info{ load_media_info( hash ) };
            
            nlj::json media_json;
            media_info_to_json( hash, info, media_json );
            auto media_json_string{ media_json.dump() };
            
            show::response response{
                request.connection(),
                show::HTTP_1_1,
                show::code::OK,
                {
                    show::server_header,
                    { "Content-Type", { "application/json" } },
                    { "Content-Length", {
                        std::to_string( media_json_string.size() )
                    } }
                }
            };
            
            response.sputn(
                media_json_string.c_str(),
                media_json_string.size()
            );
        }
        catch( const hash_error& e )
        {
            throw handler_exit{
                show::code::NOT_FOUND,
                "need a valid image hash"
            };
        }
        catch( const no_such_media& e )
        {
            throw handler_exit{ show::code::NOT_FOUND, e.what() };
        }
    }
}


// DEBUG: //////////////////////////////////////////////////////////////////////

namespace
{
    const std::string form{
R"###(<!doctype html>
<html>
    <head>
        <meta charset=utf-8>
        <title>Form Analyzer</title>
    </head>
    <body>
        <form action="http://0.0.0.0:9090/media/upload" method="post" enctype="multipart/form-data">
            <div>
                <label for="file">File:</label>
                <input type="file" id="file" name="file"></input>
            </div>
            <div>
                <label for="decency">Decency:</label>
                <select id="decency" name="decency">
                    <option value="safe">Safe</option>
                    <option value="questionable">Questionable</option>
                    <option value="explicit">Explicit</option>
                </select>
            </div>
            <div>
                <button type="submit">Upload</button>
            </div>
        </form>
    </body>
</html>
)###"
};
}

namespace stickers
{
    void handlers::upload_media_debug_form(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        show::response response{
            request.connection(),
            show::http_protocol::HTTP_1_0,
            { 200, "OK" },
            {
                show::server_header,
                { "Content-Type"  , { "text/html"                   } },
                { "Content-Length", { std::to_string( form.size() ) } }
            }
        };
        
        response.sputn( form.c_str(), form.size() );
    }
}

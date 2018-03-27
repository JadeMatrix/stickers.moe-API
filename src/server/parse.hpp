#pragma once
#ifndef STICKERS_MOE_SERVER_PARSE_HPP
#define STICKERS_MOE_SERVER_PARSE_HPP


#include "../common/json.hpp"

#include <show.hpp>


namespace stickers
{
    using document_type = nlj::json;
    
    document_type parse_request_content( show::request& );
}


#endif

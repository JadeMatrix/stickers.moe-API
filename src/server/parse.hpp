#pragma once
#ifndef STICKERS_MOE_SERVER_PARSE_HPP
#define STICKERS_MOE_SERVER_PARSE_HPP


#include "../common/document.hpp"


namespace stickers
{
    document parse_request_content( show::request& );
}


#endif

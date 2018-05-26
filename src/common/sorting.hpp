#pragma once
#ifndef STICKERS_MOE_COMMON_SORTING_HPP
#define STICKERS_MOE_COMMON_SORTING_HPP


#include <optional>
#include <string>


namespace stickers
{
    using binary_string = std::basic_string< unsigned char >;
    
    binary_string next_sorting_key_between(
        std::optional< binary_string > before,
        std::optional< binary_string > after
    );
}


#endif

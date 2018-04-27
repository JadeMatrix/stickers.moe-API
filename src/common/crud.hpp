#pragma once
#ifndef STICKERS_MOE_COMMON_CRUD_HPP
#define STICKERS_MOE_COMMON_CRUD_HPP


#include <exception>


namespace stickers
{
    class no_such_record_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}


#endif

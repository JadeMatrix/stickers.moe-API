#pragma once
#ifndef STICKERS_MOE_COMMON_FORMATTING_HPP
#define STICKERS_MOE_COMMON_FORMATTING_HPP


// #include <fastformat/inserters/ch.hpp>
// #include <fastformat/inserters/to_x.hpp>
// #include <fastformat/shims/conversion/filter_type/bool.hpp>
// #include <fastformat/shims/conversion/filter_type/reals.hpp>
// #include <fastformat/shims/conversion/filter_type/void_pointers.hpp>
// #include <fastformat/sinks/ostream.hpp>

#include <fastformat/ff.hpp>

#ifdef FASTFORMAT_NO_FILTER_TYPE_CONVERSION_SHIM_SUPPORT
#error Cannot compile this file with a compiler that does not support the filter_type mechanism (FastFormat)
#endif

// Double expansion trick to print the value of a macro
#define MACROTOSTR_A( D ) #D
#define   MACROTOSTR( D ) MACROTOSTR_A( D )

#define PTR_HEX_WIDTH ( ( int )( sizeof( void* ) * 2 ) )


#endif

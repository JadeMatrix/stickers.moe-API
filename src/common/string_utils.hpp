#pragma once
#ifndef STICKERS_MOE_COMMON_STRING_UTILS_HPP
#define STICKERS_MOE_COMMON_STRING_UTILS_HPP


#include <string>


namespace stickers
{
    template<
        typename CollectionType,
        typename ValueType
    > CollectionType split(
        const ValueType& v,
        const ValueType& joiner
    )
    {
        CollectionType c;
        auto segment_begin = v.begin();
        for( auto iter = v.begin(); iter != v.end(); ++iter )
        {
            // if( ValueType( segment_begin, iter )  )
            // if( iter - segment_begin >= joiner.size() )
            {
                auto joiner_candidate_begin = iter + 1 - joiner.size();
                auto joiner_candidate_end   = iter + 1                ;
                if( ValueType( joiner_candidate_begin, joiner_candidate_end ) == joiner )
                {
                    c.push_back( ValueType( segment_begin, joiner_candidate_begin ) );
                    segment_begin = joiner_candidate_end;
                }
            }
        }
        c.push_back( ValueType( segment_begin, v.end() ) );
        return c;
    }
    
    template< typename CollectionType, typename ValueType > ValueType join(
        const CollectionType& c,
        const ValueType&      joiner
    )
    {
        ValueType v;
        for( auto iter = c.begin(); iter != c.end(); /*++iter*/ )
        {
            v += *iter;
            if( ++iter != c.end() )
                v += joiner;
        }
        return v;
    }
}


#endif

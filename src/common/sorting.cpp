#line 2 "common/sorting.cpp"


#include "sorting.hpp"

#include <exception>


namespace
{
    static const stickers::binary_string::value_type key_segment_new     { 0x80 };
    static const stickers::binary_string::value_type key_segment_min_head{ 0x01 };
    static const stickers::binary_string::value_type key_segment_min     { 0x00 };
    static const stickers::binary_string::value_type key_segment_max     { 0xff };
}


namespace stickers
{
    binary_string next_sorting_key_between(
        std::optional< binary_string > before,
        std::optional< binary_string > after
    )
    {
        if(
               ( before && before -> size() < 1 )
            || (  after &&  after -> size() < 1 )
        )
            throw std::invalid_argument{ "zero-length sorting key" };
        else if( before && after && *before >= *after )
            throw std::invalid_argument{
                "'before' & 'after' sorting keys same or inverted"
            };
        
        if( !before && !after )
            return { key_segment_new };
        else if( before && !after )
        {
            binary_string new_key;
            
            auto before_iter{ before -> begin() };
            
            while(
                before_iter != before -> end()
                && *before_iter == key_segment_max
            )
            {
                new_key += key_segment_max;
                ++before_iter;
            }
            
            if( before_iter == after -> end() )
                new_key += key_segment_new;
            else
                new_key += *before_iter + 1;
            
            return new_key;
        }
        else if( !before && after )
        {
            binary_string new_key;
            
            auto after_iter{ after -> begin() };
            
            while(
                after_iter != after -> end()
                && *after_iter == key_segment_min
            )
            {
                new_key += key_segment_min;
                ++after_iter;
            }
            
            if( after_iter == after -> end() )
                throw std::invalid_argument{
                    "invalid sorting key passed for 'after' (all min octet)"
                };
            
            if( *after_iter == key_segment_min_head )
                new_key += { key_segment_min, key_segment_new };
            else
                new_key += *after_iter - 1;
            
            return new_key;
        }
        else
        {
            binary_string new_key;
            
            auto before_iter{ before -> begin() };
            auto  after_iter{  after -> begin() };
            
            decltype( *before_iter - *after_iter ) diff;
            
            while(
                  before_iter != before -> end()
                && after_iter !=  after -> end()
                && ( diff = *before_iter - *after_iter ) <= 1
            )
            {
                new_key += *before_iter;
                ++before_iter;
                ++ after_iter;
            }
            
            if(
                  before_iter == before -> end()
                || after_iter ==  after -> end()
            )
            {
                if( before_iter != before -> end() )
                    new_key += next_sorting_key_between(
                        binary_string{ before_iter, before -> end() },
                        std::nullopt
                    );
                else if( after_iter != after -> end() )
                    new_key += next_sorting_key_between(
                        std::nullopt,
                        binary_string{  after_iter,  after -> end() }
                    );
                else
                    new_key += next_sorting_key_between(
                        std::nullopt,
                        std::nullopt
                    );
            }
            else
                new_key += diff / 2;
            
            return new_key;
        }
    }
}

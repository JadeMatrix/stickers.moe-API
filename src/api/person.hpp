#pragma once
#ifndef STICKERS_MOE_API_PERSON_HPP
#define STICKERS_MOE_API_PERSON_HPP


#include "../audit/blame.hpp"
#include "../common/bigid.hpp"
#include "../common/postgres.hpp"
#include "../common/timestamp.hpp"

#include <exception>
#include <string>
#include <variant>


namespace stickers
{
    struct person_info
    {
        timestamp   created;
        timestamp   revised;
        std::string about;
        std::variant< std::string, bigid > identifier;
        
        bool has_user() const
        {
            return std::holds_alternative< bigid >( identifier );
        }
    };
    
    struct person
    {
        bigid       id;
        person_info info;
    };
    
    person      create_person( const person_info&, const audit::blame& );
    person_info   load_person( const bigid      &                      );
    person_info update_person( const person     &, const audit::blame& );
    void        delete_person( const bigid      &, const audit::blame& );
    
    // ACID-safe assert
    void assert_person_exists( const bigid& id, pqxx::work& transaction );
    
    class no_such_person : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}


#endif

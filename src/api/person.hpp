#pragma once
#ifndef STICKERS_MOE_API_PERSON_HPP
#define STICKERS_MOE_API_PERSON_HPP


#include "../audit/blame.hpp"
#include "../common/bigid.hpp"
#include "../common/crud.hpp"
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
    
    class _assert_people_exist_impl
    {
        template< class Container > friend void assert_people_exist(
            pqxx::work     &,
            const Container&
        );
        _assert_people_exist_impl();
        static void exec( pqxx::work&, const std::string& );
    };
    
    // ACID-safe assert; if any of the supplied IDs do not correspond to a
    // record, this will throw `no_such_person` for one of those IDs
    template< class Container = std::initializer_list< bigid > >
    void assert_people_exist(
        pqxx::work     & transaction,
        const Container& ids
    )
    {
        _assert_people_exist_impl::exec(
            transaction,
            postgres::format_variable_list( transaction, ids )
        );
    }
    
    class no_such_person : public no_such_record_error
    {
    public:
        const bigid id;
        no_such_person( const bigid& );
    };
}


#endif

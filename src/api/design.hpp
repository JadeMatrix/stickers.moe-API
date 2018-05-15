#pragma once
#ifndef STICKERS_MOE_API_DESIGN_HPP
#define STICKERS_MOE_API_DESIGN_HPP


#include "../audit/blame.hpp"
#include "../common/bigid.hpp"
#include "../common/hashing.hpp"
#include "../common/postgres.hpp"
#include "../common/timestamp.hpp"

#include <exception>
#include <string>
#include <vector>
#include <initializer_list>


namespace stickers
{
    struct design_info
    {
        timestamp             created;
        timestamp             revised;
        std::string           description;
        std::vector< sha256 > images;
        std::vector< bigid  > contributors;
    };
    
    struct design
    {
        bigid       id;
        design_info info;
    };
    
    design      create_design( const design_info&, const audit::blame& );
    design_info   load_design( const bigid      &                      );
    design_info update_design( const design     &, const audit::blame& );
    void        delete_design( const bigid      &, const audit::blame& );
    
    class _assert_designs_exist_impl
    {
        template< class Container > friend void assert_designs_exist(
            pqxx::work     &,
            const Container&
        );
        _assert_designs_exist_impl();
        static void exec( pqxx::work&, const std::string& );
    };
    
    // ACID-safe assert; if any of the supplied IDs do not correspond to a
    // record, this will throw `no_such_design` for one of those IDs
    template< class Container = std::initializer_list< bigid > >
    void assert_designs_exist(
        pqxx::work     & transaction,
        const Container& ids
    )
    {
        _assert_designs_exist_impl::exec(
            transaction,
            postgres::format_variable_list( transaction, ids )
        );
    }
    
    class no_such_design : public std::runtime_error
    {
    public:
        const bigid id;
        no_such_design( const bigid& );
    };
}


#endif

#line 2 "api/design.cpp"


#include "design.hpp"

#include "../api/person.hpp"
#include "../common/formatting.hpp"
#include "../common/logging.hpp"

#include <algorithm>    // std::set_difference()
#include <set>
#include <tuple>


namespace
{
    std::vector< stickers::sha256 > get_images_for_design(
        const stickers::bigid& id,
        pqxx::work& transaction
    )
    {
        auto result{ transaction.exec_params(
            PSQL(
                SELECT image_hash
                FROM designs.design_images
                WHERE
                    design_id = $1
                    AND NOT deleted
                ORDER BY weight
                ;
            ),
            id
        ) };
        
        std::vector< stickers::sha256 > images;
        
        for( const auto& row : result )
            images.push_back( row[ "image_hash" ].as< stickers::sha256 >() );
        
        return images;
    }
    
    std::vector< stickers::bigid > get_contributors_for_design(
        const stickers::bigid& id,
        pqxx::work& transaction
    )
    {
        auto result{ transaction.exec_params(
            PSQL(
                SELECT person_id
                FROM designs.design_contributors
                WHERE
                    design_id = $1
                    AND NOT deleted
                ;
            ),
            id
        ) };
        
        std::vector< stickers::bigid > contributors;
        
        for( const auto& row : result )
            contributors.push_back(
                row[ "person_id" ].as< stickers::bigid >()
            );
        
        return contributors;
    }
    
    stickers::bigid write_design_details(
        stickers::design            & design,
        const stickers::audit::blame& blame,
        bool                          generate_id
    )
    {
        auto connection{ stickers::postgres::connect() };
        pqxx::work transaction{ *connection };
        
        if( generate_id )
        {
            auto result{ transaction.exec_params(
                PSQL(
                    INSERT INTO designs.designs_core (
                        design_id,
                        _a_revision
                    )
                    VALUES (
                        DEFAULT,
                        $1
                    )
                    RETURNING design_id
                    ;
                ),
                blame.when
            ) };
            result[ 0 ][ "design_id" ].to< stickers::bigid >( design.id );
        }
        else
            stickers::assert_designs_exist( transaction, { design.id } );
        
        transaction.exec_params(
            PSQL(
                INSERT INTO designs.design_revisions (
                    design_id,
                    revised,
                    revised_by,
                    revised_from,
                    description
                )
                VALUES ( $1, $2, $3, $4, $5 )
                ;
            ),
            design.id,
            blame.when,
            blame.who,
            blame.where,
            design.info.description
        );
        
        std::vector< stickers::sha256 > old_images;
        std::set   < stickers::bigid  > contributors_to_remove;
        std::set   < stickers::bigid  > contributors_to_add;
        
        // Don't have to do as much work if we know there are no relations
        if( generate_id )
        {
            for( const auto& contributor : design.info.contributors )
                contributors_to_add.insert( contributor );
        }
        else
        {
            old_images = get_images_for_design( design.id, transaction );
            
            std::set< stickers::bigid > old_contributors;
            std::set< stickers::bigid > new_contributors;
            
            for( const auto& contributor : get_contributors_for_design(
                design.id,
                transaction
            ) )
                old_contributors.insert( contributor );
            for( const auto& contributor : design.info.contributors )
                new_contributors.insert( contributor );
            
            std::set_difference(    // In old but not in new
                old_contributors.begin(),
                old_contributors.end(),
                new_contributors.begin(),
                new_contributors.end(),
                std::inserter(
                    contributors_to_remove,
                    contributors_to_remove.begin()
                )
            );
            std::set_difference(    // In new but not in old
                new_contributors.begin(),
                new_contributors.end(),
                old_contributors.begin(),
                old_contributors.end(),
                std::inserter(
                    contributors_to_add,
                    contributors_to_add.begin()
                )
            );
        }
        
        if( design.info.images != old_images )
        {
            if( old_images.size() > 0 )
            {
                // TODO: remove images
            }
            
            if( design.info.images.size() > 0 )
            {
                // TODO: add images & weights
            }
        }
        
        if( contributors_to_remove.size() > 0 )
        {
            // No need to check these exist, they were just pulled from the DB
            
            std::string query_string;
            ff::fmt(
                query_string,
                PSQL(
                    UPDATE designs.design_contributor_revisions
                    SET
                        removed      = $2
                        removed_by   = $3
                        removed_from = $4
                    WHERE
                        design_id = $1
                        AND person_id IN ( {0} )
                        AND removed IS NULL
                    ;
                ),
                stickers::postgres::format_variable_list(
                    transaction,
                    contributors_to_remove
                )
            );
            
            transaction.exec_params(
                query_string,
                design.id,
                blame.when,
                blame.who,
                blame.where
            );
        }
        
        if( contributors_to_add.size() > 0 )
        {
            stickers::assert_people_exist(
                transaction,
                contributors_to_add
            );
            
            auto columns = {
                "design_id",
                "added",
                "added_by",
                "added_from",
                "person_id"
            };
            
            pqxx::tablewriter inserter(
                transaction,
                "designs.design_contributor_revisions",
                columns.begin(),
                columns.end()
            );
            
            for( const auto& contributor : contributors_to_add )
            {
                auto values = {
                    pqxx::string_traits< stickers::bigid     >::to_string( design.id   ),
                    pqxx::string_traits< stickers::timestamp >::to_string( blame.when  ),
                    pqxx::string_traits< stickers::bigid     >::to_string( blame.who   ),
                    pqxx::string_traits< std::string         >::to_string( blame.where ),
                    pqxx::string_traits< stickers::bigid     >::to_string( contributor )
                };
                inserter.insert( values );
            }
            
            inserter.complete();
        }
        
        transaction.commit();
        return design.id;
    }
}


namespace stickers // Design ///////////////////////////////////////////////////
{
    design create_design(
        const design_info & info,
        const audit::blame& blame
    )
    {
        design s{ bigid::MIN(), info };
        write_design_details( s, blame, true );
        return s;
    }
    
    design_info load_design( const bigid& id )
    {
        auto connection{ postgres::connect() };
        pqxx::work transaction{ *connection };
        
        auto result{ transaction.exec_params(
            PSQL(
                SELECT
                    created,
                    revised,
                    description
                FROM designs.designs
                WHERE
                    design_id = $1
                    AND NOT deleted
                ;
            ),
            id
        ) };
        
        if( result.size() < 1 )
            throw no_such_design{ id };
        
        auto& row{ result[ 0 ] };
        
        design_info info{
            row[ "created"     ].as< timestamp   >(),
            row[ "revised"     ].as< timestamp   >(),
            row[ "description" ].as< std::string >(),
            get_images_for_design      ( id, transaction ),
            get_contributors_for_design( id, transaction )
        };
        
        transaction.commit();
        
        return info;
    }
    
    design_info update_design( const design& s, const audit::blame& blame )
    {
        auto updated_design{ s };
        write_design_details( updated_design, blame, true );
        return updated_design.info;
    }
    
    void delete_design( const bigid& id, const audit::blame& blame )
    {
        auto connection{ postgres::connect() };
        pqxx::work transaction{ *connection };
        
        assert_designs_exist( transaction, { id } );
        
        auto result{ transaction.exec_params(
            PSQL(
                INSERT INTO designs.design_deletions (
                    design_id,
                    deleted,
                    deleted_by,
                    deleted_from
                )
                VALUES ( $1, $2, $3, $4 )
                ;
            ),
            id,
            blame.when,
            blame.who,
            blame.where
        ) };
        transaction.commit();
    }
}


namespace stickers // Exception ////////////////////////////////////////////////
{
    no_such_design::no_such_design( const bigid& id ) :
        std::runtime_error{
            "no such design with ID " + static_cast< std::string >( id )
        },
        id{ id }
    {}
}


namespace stickers // Assertion ////////////////////////////////////////////////
{
    void _assert_designs_exist_impl::exec(
        pqxx::work       & transaction,
        const std::string& ids_string
    )
    {
        std::string query_string;
        
        ff::fmt(
            query_string,
            PSQL(
                WITH lookfor AS (
                    SELECT UNNEST( ARRAY[ {0} ] ) AS design_id
                )
                SELECT lookfor.design_id
                FROM
                    lookfor
                    LEFT JOIN designs.designs_core AS dc
                        ON dc.design_id = lookfor.design_id
                    LEFT JOIN designs.design_deletions AS dd
                        ON dd.design_id = dc.design_id
                WHERE
                       dc.design_id IS     NULL
                    OR dd.design_id IS NOT NULL
                ;
            ),
            ids_string
        );
        
        auto result{ transaction.exec( query_string ) };
        
        if( result.size() > 0 )
            throw no_such_design{ result[ 0 ][ 0 ].as< bigid >() };
    }
}

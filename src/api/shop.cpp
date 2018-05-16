#line 2 "api/shop.cpp"


#include "shop.hpp"

#include "../api/person.hpp"
#include "../common/formatting.hpp"
#include "../common/logging.hpp"


namespace
{
    stickers::bigid write_shop_details(
        stickers::shop              & shop,
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
                    INSERT INTO shops.shops_core (
                        shop_id,
                        _a_revision
                    )
                    VALUES (
                        DEFAULT,
                        $1
                    )
                    RETURNING shop_id
                    ;
                ),
                blame.when
            ) };
            result[ 0 ][ "shop_id" ].to< stickers::bigid >( shop.id );
        }
        else
            stickers::assert_shops_exist( transaction, { shop.id } );
        
        stickers::assert_people_exist(
            transaction,
            { shop.info.owner_person_id }
        );
        
        transaction.exec_params(
            PSQL(
                INSERT INTO shops.shop_revisions (
                    shop_id,
                    revised,
                    revised_by,
                    revised_from,
                    shop_name,
                    shop_url,
                    founded,
                    closed,
                    owner_id
                )
                VALUES ( $1, $2, $3, $4, $5, $6, $7, $8, $9 )
                ;
            ),
            shop.id,
            blame.when,
            blame.who,
            blame.where,
            shop.info.name,
            shop.info.url,
            shop.info.founded,
            shop.info.closed,
            shop.info.owner_person_id
        );
        
        transaction.commit();
        return shop.id;
    }
}


namespace stickers // Person ////////////////////////////////////////////////////
{
    shop create_shop(
        const shop_info   & info,
        const audit::blame& blame
    )
    {
        shop s{ bigid::MIN(), info };
        write_shop_details( s, blame, true );
        return s;
    }
    
    shop_info load_shop( const bigid& id )
    {
        auto connection{ postgres::connect() };
        pqxx::work transaction{ *connection };
        
        auto result{ transaction.exec_params(
            PSQL(
                SELECT
                    created,
                    revised,
                    shop_name,
                    shop_url,
                    founded::TIMESTAMPTZ,
                    closed::TIMESTAMPTZ,
                    owner_id
                FROM shops.shops
                WHERE
                    shop_id = $1
                    AND NOT deleted
                ;
            ),
            id
        ) };
        transaction.commit();
        
        if( result.size() < 1 )
            throw no_such_shop{ id };
        
        auto& row{ result[ 0 ] };
        
        shop_info info{
            row[ "created"   ].as< timestamp   >(),
            row[ "revised"   ].as< timestamp   >(),
            row[ "shop_name" ].as< std::string >(),
            row[ "shop_url"  ].as< std::string >(),
            row[ "owner_id"  ].as< bigid       >(),
            std::nullopt,
            std::nullopt
        };
        
        if( !row[ "founded" ].is_null() )
            info.founded = row[ "founded" ].as< timestamp >();
        if( !row[ "closed" ].is_null() )
            info.closed = row[ "closed" ].as< timestamp >();
        
        return info;
    }
    
    shop_info update_shop( const shop& s, const audit::blame& blame )
    {
        auto updated_shop{ s };
        write_shop_details( updated_shop, blame, true );
        return updated_shop.info;
    }
    
    void delete_shop( const bigid& id, const audit::blame& blame )
    {
        auto connection{ postgres::connect() };
        pqxx::work transaction{ *connection };
        
        assert_shops_exist( transaction, { id } );
        
        auto result{ transaction.exec_params(
            PSQL(
                INSERT INTO shops.shop_deletions (
                    shop_id,
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
    no_such_shop::no_such_shop( const bigid& id ) :
        no_such_record_error{
            "no such shop with ID " + static_cast< std::string >( id )
        },
        id{ id }
    {}
}


namespace stickers // Assertion /////////////////////////////////////////////////
{
    void _assert_shops_exist_impl::exec(
        pqxx::work       & transaction,
        const std::string& ids_string
    )
    {
        std::string query_string;
        
        ff::fmt(
            query_string,
            PSQL(
                WITH lookfor AS (
                    SELECT UNNEST( ARRAY[ {0} ] ) AS shop_id
                )
                SELECT lookfor.shop_id
                FROM
                    lookfor
                    LEFT JOIN shops.shops_core AS sc
                        ON sc.shop_id = lookfor.shop_id
                    LEFT JOIN shops.shop_deletions AS sd
                        ON sd.shop_id = sc.shop_id
                WHERE
                       sc.shop_id IS     NULL
                    OR sd.shop_id IS NOT NULL
                ;
            ),
            ids_string
        );
        
        auto result{ transaction.exec( query_string ) };
        
        if( result.size() > 0 )
            throw no_such_shop{ result[ 0 ][ 0 ].as< bigid >() };
    }
}

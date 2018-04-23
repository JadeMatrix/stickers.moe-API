#line 2 "api/person.cpp"


#include "person.hpp"

#include "../common/formatting.hpp"
#include "../common/logging.hpp"
#include "../common/postgres.hpp"


namespace stickers
{
    stickers::bigid write_person_details(
        stickers::person            & person,
        const stickers::audit::blame& blame,
        bool                          generate_id
    )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        if( generate_id )
        {
            auto result = transaction.exec_params(
                PSQL(
                    INSERT INTO people.people_core (
                        person_id,
                        _a_revision
                    )
                    VALUES (
                        DEFAULT,
                        $1
                    )
                    RETURNING person_id
                    ;
                ),
                blame.when
            );
            result[ 0 ][ "person_id" ].to< stickers::bigid >( person.id );
        }
        
        std::string add_person_revision_query_string{ PSQL(
            INSERT INTO people.person_revisions (
                person_id,
                revised,
                revised_by,
                revised_from,
                person_name,
                person_user,
                about
            )
            VALUES ( $1, $2, $3, $4, $5, $6, $7 )
            ;
        ) };
        
        if( person.info.has_user() )
            transaction.exec_params(
                add_person_revision_query_string,
                person.id,
                blame.when,
                blame.who,
                blame.where,
                nullptr,
                std::get< stickers::bigid >( person.info.identifier ),
                person.info.about
            );
        else
            transaction.exec_params(
                add_person_revision_query_string,
                person.id,
                blame.when,
                blame.who,
                blame.where,
                std::get< std::string >( person.info.identifier ),
                nullptr,
                person.info.about
            );
        
        transaction.commit();
        return person.id;
    }
}


namespace stickers // Person ////////////////////////////////////////////////////
{
    person create_person(
        const person_info & info,
        const audit::blame& blame
    )
    {
        person p{ bigid::MIN(), info };
        write_person_details( p, blame, true );
        return p;
    }
    
    person_info load_person( const bigid& id )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        pqxx::result result = transaction.exec_params(
            PSQL(
                SELECT
                    created,
                    revised,
                    person_name,
                    person_user,
                    about
                FROM people.people
                WHERE
                    person_id = $1
                    AND NOT deleted
                ;
            ),
            id
        );
        transaction.commit();
        
        if( result.size() < 1 )
            throw no_such_person( id );
        
        auto& row = result[ 0 ];
        
        if( row[ "person_user" ].is_null() )
        {
            return {
                row[ "created"     ].as< timestamp   >(),
                row[ "revised"     ].as< timestamp   >(),
                row[ "about"       ].as< std::string >(),
                row[ "person_name" ].as< std::string >()
            };
        }
        else
        {
            return {
                row[ "created"     ].as< timestamp   >(),
                row[ "revised"     ].as< timestamp   >(),
                row[ "about"       ].as< std::string >(),
                row[ "person_user" ].as< bigid       >()
            };
        }
    }
    
    person_info update_person( const person& p, const audit::blame& blame )
    {
        // Assert person exists
        auto info = load_person( p.id );
        
        auto updated_person = p;
        write_person_details( updated_person, blame, true );
        return updated_person.info;
    }
    
    void delete_person( const bigid& id, const audit::blame& blame )
    {
        // Assert person exists
        auto info = load_person( id );
        
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        pqxx::result result = transaction.exec_params(
            PSQL(
                INSERT INTO people.person_deletions (
                    person_id,
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
        );
        transaction.commit();
    }
}

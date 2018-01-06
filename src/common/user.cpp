#include "user.hpp"

#include "config.hpp"
#include "timestamp.hpp"
#include "postgres.hpp"
#include "redis.hpp"

#line __LINE__ "common/user.cpp"


namespace
{
    stickers::bigid write_user_details(
        const stickers::user& user,
        const stickers::audit::blame& blame,
        bool generate_id = true
    )
    {
        
    }
}


// User management -------------------------------------------------------------


namespace stickers
{
    // bigid create_user( const user_info& info )
    // {
        
    // }
    
    bigid create_user( const user_info& info, const audit::blame& blame )
    {
        
    }
    
    user_info load_user( const bigid& id )
    {
        auto connection = postgres::connect();
        pqxx::work transaction( *connection );
        
        connection -> prepare(
            "load_user",
            PSQL(
                SELECT
                    user_id,
                    ( password ).type AS password_type,
                    ( password ).hash AS password_hash,
                    ( password ).salt AS password_salt,
                    ( password ).factor AS password_factor,
                    created,
                    revised,
                    display_name,
                    real_name,
                    avatar_hash,
                    email
                FROM users.users
                WHERE user_id = $1
                ;
            )
        );
        pqxx::result result = transaction.exec_prepared( "load_user", id );
        transaction.commit();
        
        if( result.size() < 1 )
            throw no_such_user( id, "loading" );
        
        password found_pass = {
            result[ 0 ][ "password_type" ].as< password_type >( UNKNOWN ),
            pqxx::binarystring( result[ 0 ][ "password_hash" ] ).str(),
            pqxx::binarystring( result[ 0 ][ "password_salt" ] ).str(),
            result[ 0 ][ "password_factor" ].as< int >()
        };
        
        user_info found_info = {
            found_pass,
            result[ 0 ][ "created"      ].as< timestamp   >(),
            result[ 0 ][ "revised"      ].as< timestamp   >(),
            result[ 0 ][ "display_name" ].as< std::string >(),
            result[ 0 ][ "real_name"    ].as< std::string >( "" ),
            result[ 0 ][ "avatar_hash"  ].as< std::string >( "" ),
            result[ 0 ][ "email"        ].as< std::string >(),
        };
        
        return found_info;
    }
    
    void save_user( const user& u, const audit::blame& blame )
    {
        
    }
    
    void delete_user( const bigid& id, const audit::blame& blame )
    {
        auto connection = postgres::connect();
        pqxx::work transaction( *connection );
        
        connection -> prepare(
            "delete_user",
            PSQL(
                INSERT INTO users.user_deletions (
                    user_id,
                    deleted,
                    deleted_by,
                    deleted_from
                ) VALUES ( $1, $2, $3, $4 )
                ;
            )
        );
        
        pqxx::result result = transaction.exec_prepared(
            "delete_user",
            id,
            blame.when,
            blame.who,
            blame.where
        );
        
        transaction.commit();
    }
}


// no_such_user ----------------------------------------------------------------


namespace stickers
{
    no_such_user::no_such_user( const bigid& id, const std::string& purpose ) :
        message(
            "no such user with ID "
            + ( std::string )id
            + " ("
            + purpose
            + ")"
        )
    {}
    
    const char* no_such_user::what() const noexcept
    {
        return message.c_str();
    }
}

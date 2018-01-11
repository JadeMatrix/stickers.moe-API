#include "user.hpp"

#include "config.hpp"
#include "timestamp.hpp"
#include "postgres.hpp"
#include "redis.hpp"
#include "../common/logging.hpp"

#line __LINE__ "common/user.cpp"


namespace
{
    stickers::bigid write_user_details(
        std::unique_ptr< pqxx::connection >& connection,
        stickers::user                     & user,
        const stickers::audit::blame       & blame,
        bool                                 generate_id,
        bool                                 signup = false
    )
    {
        pqxx::work transaction( *connection );
        std::string current_email;
        
        if( generate_id )
        {
            // IMPLEMENT: Hash password
            connection -> prepare(
                "create_user_core",
                PSQL(
                    INSERT INTO users.user_core
                    VALUES ( DEFAULT, $1, TRUE, ROW( $2, $3, $4, $5 ) )
                    RETURNING user_id
                    ;
                )
            );
            pqxx::result result = transaction.exec_prepared(
                "create_user_core",
                blame.when,
                user.info.password.type,
                user.info.password.hash,
                user.info.password.salt,
                user.info.password.factor
            );
            result[ 0 ][ "user_id" ].to< stickers::bigid >( user.id );
            
            // DEBUG:
            STICKERS_LOG(
                DEBUG,
                "generated id ",
                user.id
            );
        }
        else
        {
            connection -> prepare(
                "get_current_user_email",
                PSQL(
                    SELECT email
                    FROM users.user_emails
                    WHERE
                        user_id = $1
                        AND current
                    ;
                )
            );
            pqxx::result result = transaction.exec_prepared(
                "get_current_user_email",
                user.id
            );
            if( result.size() >= 1 )
                current_email = result[ 0 ][ "email" ].as< std::string >();
            
            // DEBUG:
            STICKERS_LOG(
                DEBUG,
                "did not generate id"
            );
        }
        
        connection -> prepare(
            "add_user_revision",
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-pp-token"
            PSQL(
                INSERT INTO users.user_revisions
                VALUES
                    (
                        $1, $2, $3, $4, $5,
                        CASE WHEN $6 = '' THEN NULL ELSE $6 END,
                        CASE WHEN $7 = '' THEN NULL ELSE $7::util.raw_sha256 END
                    )
                ;
            )
#pragma clang diagnostic pop
        );
        transaction.exec_prepared(
            "add_user_revision",
            user.id,
            blame.when,
            ( signup ? user.id : blame.who ),
            blame.where,
            user.info.display_name,
            user.info.real_name,
            user.info.avatar_hash
        );
        
        if( user.info.email != current_email )
        {
            // TODO: figure out a way to have an "invalid" signup email
            connection -> prepare(
                "add_user_email_revision",
                PSQL(
                    INSERT INTO users.user_emails
                    VALUES ( $1, $2, $3, $4, $5, $6 )
                    ;
                )
            );
            transaction.exec_prepared(
                "add_user_email_revision",
                user.id,
                user.info.email,
                signup,
                blame.when,
                blame.who,
                blame.where
            );
            stickers::send_validation_email( user.id );
        }
        
        transaction.commit();
        return user.id;
    }
}


// User management -------------------------------------------------------------


namespace stickers
{
    user create_user(
        const user_info& info,
        const audit::blame& blame,
        bool signup
    )
    {
        auto connection = postgres::connect();
        
        user new_user{
            blame.who,
            info
        };
        
        new_user.id = write_user_details(
            connection,
            new_user,
            blame,
            true,
            signup
        );
        
        return new_user;
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
    
    user_info save_user( const user& u, const audit::blame& blame )
    {
        auto connection = postgres::connect();
        
        user updated_user = u;
        
        write_user_details(
            connection,
            updated_user,
            blame,
            false
        );
        
        return updated_user.info;
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
    
    void send_validation_email( const bigid& id )
    {
        // IMPLEMENT:
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

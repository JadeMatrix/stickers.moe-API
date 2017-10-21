#include "config.hpp"
#include "postgres.hpp"
#include "redis.hpp"
#include "user.hpp"

// DEBUG:
#include <iostream>
#include "formatting.hpp"
#include <vector>


// User management -------------------------------------------------------------


namespace stickers
{
    user_info load_user( const bigid& id )
    {
        const nlj::json& pg_config = config()[ "database" ];
        pqxx::connection connection(
               "user="   + pg_config[ "user"   ].get< std::string >()
            + " host="   + pg_config[ "host"   ].get< std::string >()
            + " port="   + std::to_string( pg_config[ "port" ].get< int >() )
            + " dbname=" + pg_config[ "dbname" ].get< std::string >()
        );
        pqxx::work transaction( connection );
        
        connection.prepare(
            "load_user",
            PSQL(
                SELECT
                    user_id,
                    password,
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
        pqxx::result result = transaction.prepared( "load_user" )(
            std::to_string( ( long long )id )
        ).exec();
        transaction.commit();
        
        if( result.size() < 1 )
            throw no_such_user( id );
        
        ff::writeln(
            std::cout,
            result[ 0 ][ "password" ].c_str()
        );
        
        auto p = split_pg_field( result[ 0 ][ "password" ] );
        
        // std::string pass_type = result[ 0 ][ "password" ][ "type" ].as< std::string >();
        password_type found_pass_type = UNKNOWN;
        if( p[ 0 ] == "bcrypt" )
            found_pass_type = BCRYPT;
        else if( p[ 0 ] == "invalid" )
            found_pass_type = INVALID;
        
        password found_pass = {
            found_pass_type,
            p[ 1 ],
            p[ 2 ],
            std::stoi( p[ 3 ] )
        };
        
        user_info found_info = {
            found_pass,
            // result[ 0 ][ "created"      ].as< std::string >(),
            // result[ 0 ][ "revised"      ].as< std::string >(),
            result[ 0 ][ "display_name" ].as< std::string >(),
            result[ 0 ][ "real_name"    ].as< std::string >( "" ),
            // result[ 0 ][ "avatar_hash"  ].as< std::string >( "" ),
            result[ 0 ][ "email"        ].as< std::string >(),
        };
        
        return found_info;
    }
    
    // void save_user( const user& u )
    // {
        
    // }
    
    // void delete_user( const bigid& id )
    // {
        
    // }
    
    // bigid create_user( const user_info& info )
    // {
        
    // }
}


// no_such_user ----------------------------------------------------------------


namespace stickers
{
    no_such_user::no_such_user( const bigid& id ) :
        message( "no such user with ID " + ( std::string )id )
    {}
    
    const char* no_such_user::what() const noexcept
    {
        return message.c_str();
    }
}

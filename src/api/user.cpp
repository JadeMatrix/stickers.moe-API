#line 2 "api/user.cpp"


#include "user.hpp"

#include "../common/config.hpp"
#include "../common/formatting.hpp"
#include "../common/logging.hpp"
#include "../common/postgres.hpp"
#include "../common/redis.hpp"
#include "../common/timestamp.hpp"

#include <fstream>


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
        pqxx::work transaction{ *connection };
        std::string current_email;
        
        if( generate_id )
        {
            auto result = transaction.exec_params(
                PSQL(
                    INSERT INTO users.user_core (
                        user_id,
                        _a_revision,
                        _email_current,
                        password
                    )
                    VALUES (
                        DEFAULT,
                        $1,
                        TRUE,
                        ROW( $2, $3, $4, $5 )
                    )
                    RETURNING user_id
                    ;
                ),
                blame.when,
                user.info.password.type(),
                pqxx::binarystring( user.info.password.hash() ),
                pqxx::binarystring( user.info.password.salt() ),
                user.info.password.factor()
            );
            result[ 0 ][ "user_id" ].to< stickers::bigid >( user.id );
        }
        else
        {
            auto result = transaction.exec_params(
                PSQL(
                    SELECT email
                    FROM users.user_emails
                    WHERE
                        user_id = $1
                        AND current
                    ;
                ),
                user.id
            );
            if( result.size() >= 1 )
                current_email = result[ 0 ][ "email" ].as< std::string >();
        }
        
        // TODO: update user password
        std::string add_user_revision = PSQL(
            INSERT INTO users.user_revisions (
                user_id,
                revised,
                revised_by,
                revised_from,
                display_name,
                real_name,
                avatar_hash
            )
            VALUES ( $1, $2, $3, $4, $5, $6, $7 )
            ;
        );
        transaction.exec_params(
            add_user_revision,
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
            transaction.exec_params(
                PSQL(
                    INSERT INTO users.user_emails
                    VALUES ( $1, $2, $3, $4, $5, $6 )
                    ;
                ),
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
    
    template< typename T > pqxx::result query_user_records_by(
        pqxx::work& transaction,
        const std::string& field,
        const T& iden
    )
    {
        std::string query_string;
        
        ff::fmt(
            query_string,
            PSQL(
                SELECT
                    user_id,
                    ( password ).type   AS password_type,
                    ( password ).hash   AS password_hash,
                    ( password ).salt   AS password_salt,
                    ( password ).factor AS password_factor,
                    created,
                    revised,
                    display_name,
                    real_name,
                    avatar_hash,
                    email
                FROM users.users
                WHERE {0} = $1
                AND NOT deleted
                ;
            ),
            field
        );
        
        pqxx::result result = transaction.exec_params(
            query_string,
            iden
        );
        transaction.commit();
        
        return result;
    }
    
    stickers::user_info compile_user_info_from_row( const pqxx::row& row )
    {
        stickers::password pw;
        
        if(
            row[ "password_type" ].as< stickers::password_type >()
            == stickers::password_type::SCRYPT
        )
        {
            unsigned char factor, block_size, parallelization;
            
            stickers::scrypt::split_libscrypt_mcf_factor(
                row[ "password_factor" ].as< unsigned int >(),
                factor,
                block_size,
                parallelization
            );
            
            pw = stickers::scrypt{
                pqxx::binarystring( row[ "password_hash" ] ).str(),
                pqxx::binarystring( row[ "password_salt" ] ).str(),
                factor,
                block_size,
                parallelization
            };
        }
        
        return {
            pw,
            row[ "created"      ]. as< stickers::timestamp >(),
            row[ "revised"      ]. as< stickers::timestamp >(),
            row[ "display_name" ]. as<      std::string    >(),
            row[ "real_name"    ].get<      std::string    >(),
            row[ "avatar_hash"  ].get< stickers::sha256    >(),
            row[ "email"        ]. as<      std::string    >(),
        };
    }
}


namespace stickers // Passwords ////////////////////////////////////////////////
{
    void password::cleanup()
    {
        switch( _type )
        {
        case password_type::RAW:
        {
            // Clang fails to compile this without the `using` statement
            using std::string;
            ( &raw_value ) -> string::~string();
            break;
        }
        case password_type::SCRYPT:
            ( &scrypt_value ) -> scrypt::~scrypt();
            break;
        default:
            break;
        }
        _type = password_type::INVALID;
        invalid_value = nullptr;
    }
    
    const char* password::type_name() const
    {
        switch( _type )
        {
        case password_type::RAW:
            return "raw";
        case password_type::SCRYPT:
            return "scrypt";
        default:
            return "invalid";
        }
    }
    
    password::password() :
        _type{         password_type::INVALID },
        invalid_value{ nullptr                }
    {}
    
    password::password( const password& o ) : _type{ password_type::INVALID }
    {
        *this = o;
    }
    
    password::password( const std::string& v ) :
        _type{     password_type::RAW },
        raw_value{ v                  }
    {}
    
    password::password( const scrypt& v ) :
        _type{        password_type::SCRYPT },
        scrypt_value{ v                     }
    {}
    
    password::~password()
    {
        cleanup();
    }
    
    bool password::operator==( const password& o ) const
    {
        switch( _type )
        {
        case password_type::RAW:
        {
            bool equals = true;
            
            std::size_t slen = (
                raw_value.size() > o.raw_value.size()
                ? raw_value.size() : o.raw_value.size()
            );
            for( std::size_t i = 0; i < slen; ++i )
            {
                char c1 = i >=   raw_value.size() ? o.raw_value[ i ] :   raw_value[ i ];
                char c2 = i >= o.raw_value.size() ?   raw_value[ i ] : o.raw_value[ i ];
                equals = equals && ( c1 == c2 );
            }
            
            return equals;
        }
        case password_type::SCRYPT:
            return scrypt_value == o.scrypt_value;
        default:
            return false;
        }
    }
    
    bool password::operator!=( const password& o ) const
    {
        return !( *this == o );
    }
    
    bool password::operator==( const std::string& raw ) const
    {
        switch( _type )
        {
        case password_type::RAW:
            return *this == password( raw );
            break;
        case password_type::SCRYPT:
            return scrypt_value == scrypt::make(
                raw,
                scrypt_value.raw_salt(),
                scrypt_value.factor(),
                scrypt_value.block_size(),
                scrypt_value.parallelization(),
                scrypt_value.raw_digest().size()
            );
            break;
        default:
            return false;
        }
    }
    
    bool password::operator!=( const std::string& raw ) const
    {
        return !( *this == raw );
    }
    
    password& password::operator=( const password& o )
    {
        switch( o._type )
        {
        case password_type::RAW:
            *this = o.raw_value;
            break;
        case password_type::SCRYPT:
            *this = o.scrypt_value;
            break;
        default:
            cleanup();
            break;
        }
        return *this;
    }
    
    password& password::operator=( const std::string& v )
    {
        cleanup();
        _type = password_type::RAW;
        new( &raw_value ) std::string( v );
        return *this;
    }
    
    password& password::operator=( const scrypt& v )
    {
        cleanup();
        _type = password_type::SCRYPT;
        new( &scrypt_value ) scrypt( v );
        return *this;
    }
    
    std::string password::hash() const
    {
        switch( _type )
        {
        case password_type::SCRYPT:
            return scrypt_value.raw_digest();
        default:
            return "";
        }
    }
    
    std::string password::salt() const
    {
        switch( _type )
        {
        case password_type::SCRYPT:
            return scrypt_value.raw_salt();
        default:
            return "";
        }
    }
    
    long password::factor() const
    {
        switch( _type )
        {
        case password_type::SCRYPT:
            return scrypt::make_libscrypt_mcf_factor(
                scrypt_value.         factor(),
                scrypt_value.     block_size(),
                scrypt_value.parallelization()
            );
        default:
            return 0;
        }
    }
    
    password hash_password( const std::string& raw )
    {
        std::ifstream urandom( "/dev/urandom", std::ios::binary );
        if( !urandom.good() )
            throw hash_error{ "failed to open /dev/urandom" };
        
        char salt[ scrypt::default_salt_size ];
        
        urandom.read( salt, scrypt::default_salt_size );
        if( !urandom.good() )
            throw hash_error{ "failed to read from /dev/urandom" };
        
        return password{ scrypt::make(
            raw.c_str(),
            raw.size(),
            salt,
            scrypt::default_salt_size
        ) };
    }
}


namespace stickers // User management //////////////////////////////////////////
{
    user create_user(
        const user_info& info,
        const audit::blame& blame,
        bool signup
    )
    {
        STICKERS_LOG(
            log_level::VERBOSE,
            "creating new user"
        );
        
        auto connection = postgres::connect();
        
        user new_user{
            blame.who,
            info
        };
        
        if( new_user.info.password.type() == password_type::RAW )
        {
            new_user.info.password = hash_password(
                new_user.info.password.value< std::string >()
            );
        }
        else if( new_user.info.password.type() != password_type::INVALID )
            STICKERS_LOG(
                log_level::WARNING,
                "creating user with a pre-set password"
            );
        
        new_user.id = write_user_details(
            connection,
            new_user,
            blame,
            true,
            signup
        );
        
        STICKERS_LOG(
            log_level::INFO,
            "created new user with id ",
            new_user.id
        );
        
        return new_user;
    }
    
    user_info load_user( const bigid& id )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        auto result = query_user_records_by(
            transaction,
            "user_id",
            id
        );
        
        if( result.size() < 1 )
            throw no_such_user::by_id( id, "loading" );
        
        return compile_user_info_from_row( result[ 0 ] );
    }
    
    user_info update_user( const user& u, const audit::blame& blame )
    {
        auto connection = postgres::connect();
        
        user updated_user = u;
        
        if( updated_user.info.password.type() == password_type::RAW )
            updated_user.info.password = hash_password(
                updated_user.info.password.value< std::string >()
            );
        
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
        pqxx::work transaction{ *connection };
        
        auto result = transaction.exec_params(
            PSQL(
                INSERT INTO users.user_deletions (
                    user_id,
                    deleted,
                    deleted_by,
                    deleted_from
                ) VALUES ( $1, $2, $3, $4 )
                ON CONFLICT DO NOTHING
                ;
            ),
            id,
            blame.when,
            blame.who,
            blame.where
        );
        
        transaction.commit();
    }
    
    user load_user_by_email( const std::string& email )
    {
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        auto result = query_user_records_by(
            transaction,
            "email",
            email
        );
        
        if( result.size() < 1 )
            throw no_such_user::by_email( email, "loading" );
        
        return {
            result[ 0 ][ "user_id" ].as< bigid >(),
            compile_user_info_from_row( result[ 0 ] )
        };
    }
    
    void send_validation_email( const bigid& id )
    {
        // IMPLEMENT:
    }
}


namespace stickers // Exceptions ///////////////////////////////////////////////
{
    no_such_user::no_such_user( const std::string& msg ) :
        no_such_record_error( msg )
    {}
    
    no_such_user no_such_user::by_id(
        const bigid& id,
        const std::string& purpose
    )
    {
        return no_such_user{
            "no such user with ID "
            + static_cast< std::string >( id )
            + " ("
            + purpose
            + ")"
        };
    }
    
    no_such_user no_such_user::by_email(
        const std::string& email,
        const std::string& purpose
    )
    {
        return no_such_user{
            "no such user with email "
            + email
            + " ("
            + purpose
            + ")"
        };
    }
}


namespace stickers // Assertions ///////////////////////////////////////////////
{
    void _assert_users_exist_impl::exec(
        pqxx::work       & transaction,
        const std::string& ids_string
    )
    {
        std::string query_string;
        
        ff::fmt(
            query_string,
            PSQL(
                WITH lookfor AS (
                    SELECT UNNEST( ARRAY[ {0} ] ) AS user_id
                )
                SELECT lookfor.user_id
                FROM
                    lookfor
                    LEFT JOIN users.users_core AS uc
                        ON uc.user_id = lookfor.user_id
                    LEFT JOIN users.user_deletions AS ud
                        ON ud.user_id = uc.user_id
                WHERE
                       uc.user_id IS     NULL
                    OR ud.user_id IS NOT NULL
                ;
            ),
            ids_string
        );
        
        auto result = transaction.exec( query_string );
        
        if( result.size() > 0 )
            throw no_such_user::by_id(
                result[ 0 ][ 0 ].as< bigid >(),
                "assert"
            );
    }
}

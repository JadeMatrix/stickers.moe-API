#include "user.hpp"

#include <fstream>

#include "../common/config.hpp"
#include "../common/timestamp.hpp"
#include "../common/postgres.hpp"
#include "../common/redis.hpp"
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
            pqxx::result result = transaction.exec_params(
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
            pqxx::result result = transaction.exec_params(
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
}


// User management -------------------------------------------------------------


namespace stickers
{
    // Password ////////////////////////////////////////////////////////////////
    
    
    void password::cleanup()
    {
        switch( _type )
        {
        case RAW:
        {
            // Clang fails to compile this without the `using` statement
            using std::string;
            ( &raw_value ) -> string::~string();
            break;
        }
        case SCRYPT:
            ( &scrypt_value ) -> scrypt::~scrypt();
            break;
        default:
            break;
        }
        _type = INVALID;
        invalid_value = nullptr;
    }
    
    const char* password::type_name() const
    {
        switch( _type )
        {
        case RAW:
            return "raw";
        case SCRYPT:
            return "scrypt";
        default:
            return "invalid";
        }
    }
    
    password::password() : _type( INVALID ), invalid_value( nullptr ) {}
    
    password::password( const std::string& v ) :
        _type(      RAW ),
        raw_value( v   )
    {}
    
    password::password( const scrypt& v ) :
        _type(         SCRYPT ),
        scrypt_value( v      )
    {}
    
    password::password( const password& o ) : _type( INVALID )
    {
        *this = o;
    }
    
    password::~password()
    {
        cleanup();
    }
    
    bool password::operator==( const password& o ) const
    {
        switch( _type )
        {
        case RAW:
        {
            bool equals = true;
            
            size_t slen = (
                raw_value.size() > o.raw_value.size()
                ? raw_value.size() : o.raw_value.size()
            );
            for( size_t i = 0; i < slen; ++i )
            {
                char c1 = i >=   raw_value.size() ? o.raw_value[ i ] :   raw_value[ i ];
                char c2 = i >= o.raw_value.size() ?   raw_value[ i ] : o.raw_value[ i ];
                equals = equals && ( c1 == c2 );
            }
            
            return equals;
        }
        case SCRYPT:
            return scrypt_value == o.scrypt_value;
        default:
            return false;
        }
    }
    
    bool password::operator!=( const password& o ) const
    {
        return !( *this == o );
    }
    
    password& password::operator=( const password& o )
    {
        switch( o._type )
        {
        case RAW:
            *this = o.raw_value;
            break;
        case SCRYPT:
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
        _type = RAW;
        new( &raw_value ) std::string( v );
        return *this;
    }
    
    password& password::operator=( const scrypt& v )
    {
        cleanup();
        _type = SCRYPT;
        new( &scrypt_value ) scrypt( v );
        return *this;
    }
    
    std::string password::hash() const
    {
        switch( _type )
        {
        case SCRYPT:
            return scrypt_value.raw_digest();
        default:
            return "";
        }
    }
    
    std::string password::salt() const
    {
        switch( _type )
        {
        case SCRYPT:
            return scrypt_value.raw_salt();
        default:
            return "";
        }
    }
    
    long password::factor() const
    {
        switch( _type )
        {
        case SCRYPT:
            return scrypt::make_libscrypt_mcf_factor(
                scrypt_value.factor(),
                scrypt_value.block_size(),
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
            throw hash_error( "failed to open /dev/urandom" );
        
        char salt[ scrypt::default_salt_size ];
        
        urandom.read( salt, scrypt::default_salt_size );
        if( !urandom.good() )
            throw hash_error( "failed to read from /dev/urandom" );
        
        return password( scrypt::make(
            raw.c_str(),
            raw.size(),
            salt,
            scrypt::default_salt_size
        ) );
    }
    
    
    // User ////////////////////////////////////////////////////////////////////
    
    
    user create_user(
        const user_info& info,
        const audit::blame& blame,
        bool signup
    )
    {
        STICKERS_LOG(
            VERBOSE,
            "creating new user"
        );
        
        auto connection = postgres::connect();
        
        user new_user{
            blame.who,
            info
        };
        
        if( new_user.info.password.type() == RAW )
        {
            new_user.info.password = hash_password(
                new_user.info.password.value< std::string >()
            );
        }
        else if( new_user.info.password.type() != INVALID )
            STICKERS_LOG(
                WARNING,
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
            INFO,
            "created new user with id ",
            new_user.id
        );
        
        return new_user;
    }
    
    user_info load_user( const bigid& id )
    {
        auto connection = postgres::connect();
        pqxx::work transaction( *connection );
        
        pqxx::result result = transaction.exec_params(
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
                WHERE user_id = $1
                ;
            ),
            id
        );
        transaction.commit();
        
        if( result.size() < 1 )
            throw no_such_user( id, "loading" );
        
        password pw;
        if( result[ 0 ][ "password_type" ].as< password_type >() == SCRYPT )
        {
            unsigned char factor, block_size, parallelization;
            
            scrypt::split_libscrypt_mcf_factor(
                result[ 0 ][ "password_factor" ].as< unsigned int >(),
                factor,
                block_size,
                parallelization
            );
            
            pw = scrypt(
                pqxx::binarystring( result[ 0 ][ "password_hash" ] ).str(),
                pqxx::binarystring( result[ 0 ][ "password_salt" ] ).str(),
                factor,
                block_size,
                parallelization
            );
        }
        
        user_info found_info = {
            pw,
            result[ 0 ][ "created"      ].as<  timestamp   >(),
            result[ 0 ][ "revised"      ].as<  timestamp   >(),
            result[ 0 ][ "display_name" ].as<  std::string >(),
            result[ 0 ][ "real_name"    ].get< std::string >(),
            result[ 0 ][ "avatar_hash"  ].get< sha256      >(),
            result[ 0 ][ "email"        ].as<  std::string >(),
        };
        
        return found_info;
    }
    
    user_info save_user( const user& u, const audit::blame& blame )
    {
        auto connection = postgres::connect();
        
        user updated_user = u;
        
        if( updated_user.info.password.type() == RAW )
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
        pqxx::work transaction( *connection );
        
        pqxx::result result = transaction.exec_params(
            PSQL(
                INSERT INTO users.user_deletions (
                    user_id,
                    deleted,
                    deleted_by,
                    deleted_from
                ) VALUES ( $1, $2, $3, $4 )
                ;
            ),
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

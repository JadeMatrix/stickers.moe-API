#include <iostream>
#include <memory>
#include <string>

#include "common/formatting.hpp"
#include "common/hashing.hpp"
#include "common/json.hpp"
#include "common/postgres.hpp"
#include "common/redis.hpp"


////////////////////////////////////////////////////////////////////////////////


std::unique_ptr< pqxx::connection > connect( const std::string& dbname )
{
    return std::unique_ptr< pqxx::connection >( new pqxx::connection(
        "user=postgres dbname=" + dbname
    ) );
}


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        ff::writeln( std::cerr, "usage: ", argv[ 0 ], " <dbname>" );
        return -1;
    }
    
    try
    {
        auto connection = connect( std::string( argv[ 1 ] ) );
        pqxx::work transaction( *connection );
        
        nlj::json test_json = {
            { "foo", true },
            { "bar", { 1234123, "Hello World", true, 3.14 } }
        };
        
        connection -> prepare(
            "test_query",
            PSQL(
                SELECT $1::JSONB->'bar' AS test_json;
            )
        );
        
        pqxx::result result = transaction.prepared( "test_query" )(
            test_json.dump()
        ).exec();
        
        transaction.commit();
        
        redox::Redox redox;
        if( !redox.connect( "localhost", 6379 ) )
        {
            ff::writeln( std::cerr, "could not connect to Redis server" );
            return 1;
        }
        
        redox.set(
            "test_json",
            result[ 0 ][ "test_json" ].as< std::string >()
        );
        
        test_json = nlj::json::parse(
            redox.get( "test_json" )
        );
        std::string message = test_json[ 1 ];
        
        redox.disconnect();
        
        ff::writeln( std::cout, message );
        
        CryptoPP::SecByteBlock abDigest( CryptoPP::SHA256::DIGESTSIZE );
        CryptoPP::SHA256().CalculateDigest(
            abDigest.begin(),
            ( byte* )message.c_str(),
            message.size()
        );
        std::string message_hash;
        CryptoPP::HexEncoder( new CryptoPP::StringSink( message_hash ) ).Put(
            abDigest.begin(),
            abDigest.size()
        );
        
        ff::writeln( std::cout, message_hash );
    }
    catch( const std::exception &e )
    {
        ff::writeln( std::cerr, e.what() );
        return 1;
    }
    catch( ... )
    {
        ff::writeln( std::cerr, "uncaught non-std::exception in main()" );
        return 2;
    }
    
    return 0;
}

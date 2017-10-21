#include <iostream>
#include <string>

#include "common/formatting.hpp"
#include "common/json.hpp"
#include "common/postgres.hpp"
#include "common/redis.hpp"


////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        ff::writeln( std::cerr, "usage: ", argv[ 0 ], " <dbname>" );
        return -1;
    }
    
    try
    {
        pqxx::connection connection(
            "user=postgres dbname=" + std::string( argv[ 1 ] )
        );
        pqxx::work transaction( connection );
        
        nlj::json test_json = {
            { "foo", true },
            { "bar", { 1234123, "Hello World", true, 3.14 } }
        };
        
        connection.prepare(
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
    }
    catch( const std::exception &e )
    {
        ff::writeln( std::cerr, e.what() );
        return 1;
    }
    
    return 0;
}

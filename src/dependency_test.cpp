#include <iostream>
#include <string>

#include <pqxx/pqxx>
#include <json.hpp>


namespace nlj = nlohmann;


#define PSQL( ... ) #__VA_ARGS__


////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] )
{
    try
    {
        pqxx::connection connection(
            "user=postgres"
        );
        pqxx::work transaction( connection );
        
        nlj::json test_json = {
            { "foo", true },
            { "bar", { 1234123, "Hello World", true, 3.14 } }
        };
        
        connection.prepare(
            "test_query",
            PSQL(
                SELECT $1::JSONB->'bar'->>1 AS test_json;
            )
        );
        
        pqxx::result result = transaction.prepared( "test_query" )(
            test_json.dump()
        ).exec();
        
        transaction.commit();
        
        std::cout
            << result[ 0 ][ "test_json" ].as< std::string >()
            << std::endl
        ;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

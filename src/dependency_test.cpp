#include <iostream>
#include <string>

#include <pqxx/pqxx>


#define PSQL( ... ) #__VA_ARGS__


////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] )
{
    try
    {
        pqxx::connection connection(
            "user=postgres"
        );
        pqxx::work w( connection );
        
        std::string test_string = "Robert'); DROP TABLE Students;--";
        
        std::cout
            <<   "    raw string: "
            << test_string
            << "\nescaped string: "
            << connection.esc( test_string )
            << "\n"
        ;
        
        pqxx::result result = w.exec( PSQL(
            SELECT 1
        ) );
        w.commit();
        std::cout << result[ 0 ][ 0 ].as< int >() << std::endl;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

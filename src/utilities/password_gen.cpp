#line 2 "utilities/password_gen.cpp"


#include "../api/user.hpp"
#include "../common/logging.hpp"
#include "../common/postgres.hpp"

#include <fstream>
#include <iostream>
#include <cstdlib>  // std::srand()
#include <ctime>    // std::time()


int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "usage: ",
            argv[ 0 ],
            " config.json password"
        );
        return -1;
    }
    
    std::srand( std::time( nullptr ) );
    
    try
    {
        {
            nlj::json config;
            std::ifstream config_file{ argv[ 1 ] };
            
            if( config_file.is_open() )
                config_file >> config;
            else
            {
                STICKERS_LOG(
                    stickers::log_level::ERROR,
                    "could not open config file ",
                    argv[ 1 ]
                );
                return 2;
            }
            
            stickers::set_config( config );
        }
        
        auto pw = stickers::hash_password( argv[ 2 ] );
        
        std::cout
            << "('"
            << pqxx::string_traits< stickers::password_type >::to_string(
                pw.type()
            )
            << "', decode('"
            << pw.value< stickers::scrypt >().hex_digest()
            << "', 'hex'), decode('"
            << pw.value< stickers::scrypt >().hex_salt()
            << "', 'hex'), "
            << pw.factor()
            << ")::users.password"
            << std::endl
        ;
    }
    catch( const std::exception &e )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "uncaught std::exception in main(): ",
            e.what()
        );
        return -1;
    }
    catch( ... )
    {
        STICKERS_LOG(
            stickers::log_level::ERROR,
            "uncaught non-std::exception in main()"
        );
        return -1;
    }
    
    return 0;
}
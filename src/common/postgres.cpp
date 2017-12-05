#include "postgres.hpp"

#include "config.hpp"


namespace stickers
{
    namespace postgres
    {
        std::unique_ptr< pqxx::connection > connect()
        {
            const nlj::json& pg_config = config()[ "database" ];
            
            // TODO: Make all of these optional
            return connect(
                pg_config[ "host"   ].get< std::string >(),
                pg_config[ "port"   ].get< int         >(),
                pg_config[ "user"   ].get< std::string >(),
                pg_config[ "pass"   ].get< std::string >(),
                pg_config[ "dbname" ].get< std::string >()
            );
        }
        
        std::unique_ptr< pqxx::connection > connect(
            const std::string& host,
            unsigned int       port,
            const std::string& user,
            const std::string& pass,
            const std::string& dbname
        )
        {
            std::unique_ptr< pqxx::connection > connection(
                new pqxx::connection(
                        "host="    + ( host   == "" ? "''" : host   )
                    + " port="     + std::to_string( port )
                    + " user="     + ( user   == "" ? "''" : user   )
                    + " password=" + ( pass   == "" ? "''" : pass   )
                    + " dbname="   + ( dbname == "" ? "''" : dbname )
                )
            );
        }
    }
}
#include "config.hpp"


namespace
{
    nlj::json global_config;
}


namespace stickers
{
    const nlj::json& config()
    {
        return global_config;
    }
    
    void  set_config( const nlj::json& o )
    {
        if( global_config == nullptr )
            global_config = o;
    }
    
    void  set_config( const std::string& s )
    {
        if( global_config == nullptr )
            global_config = nlj::json::parse( s );
    }
    
    // void open_config( const std::string& f )
    // {
    //     std::ifstream config_file( f );
        
    //     if( config_file.is_open() )
    //         config_file >> global_config;
    //     else
    //     {
    //         ff::writeln(
    //             std::cerr,
    //             "could not open config file ",
    //             argv[ 1 ]
    //         );
    //         return 2;
    //     }
    // }
}

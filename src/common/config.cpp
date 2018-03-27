#line 2 "common/config.cpp"


#include "config.hpp"

#include <thread>


namespace
{
    std::mutex config_mutex;
    
    nlj::json global_config;
    
    // Start at verbose so anything that happens before the config is loaded can
    // be diagnosed
    stickers::log_level_type log_level_cache = stickers::VERBOSE;
    
    void set_log_level()
    {
        auto level_setting = global_config.find( "log_level" );
        
        if(
            level_setting == global_config.end()
            || !level_setting -> is_string()
        )
            log_level_cache = stickers::INFO;
        else if( *level_setting == "SILENT"   )
            log_level_cache = stickers::SILENT;
        else if( *level_setting == "ERROR"   )
            log_level_cache = stickers::ERROR;
        else if( *level_setting == "WARNING" )
            log_level_cache = stickers::WARNING;
        else if( *level_setting == "INFO"   )
            log_level_cache = stickers::INFO;
        else if( *level_setting == "VERBOSE"  )
            log_level_cache = stickers::VERBOSE;
        else if( *level_setting == "DEBUG"    )
            log_level_cache = stickers::DEBUG;
        else
            log_level_cache = stickers::INFO;
    }
}


namespace stickers
{
    const nlj::json& config()
    {
        // Not exactly great, but should be fine most of the time :^)
        std::lock_guard< std::mutex > guard( config_mutex );
        return global_config;
    }
    
    void set_config( const nlj::json& o )
    {
        std::lock_guard< std::mutex > guard( config_mutex );
        if( global_config == nullptr )
            global_config = o;
        set_log_level();
    }
    
    void set_config( const std::string& s )
    {
        std::lock_guard< std::mutex > guard( config_mutex );
        if( global_config == nullptr )
            global_config = nlj::json::parse( s );
        set_log_level();
    }
    
    // void open_config( const std::string& f )
    // {
    //     std::lock_guard< std::mutex > guard( config_mutex );
        
    //     std::ifstream config_file( f );
        
    //     if( config_file.is_open() )
    //     {
    //         config_file >> global_config;
    //         set_log_level();
    //     }
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
    
    log_level_type log_level()
    {
        std::lock_guard< std::mutex > guard( config_mutex );
        return log_level_cache;
    }
}

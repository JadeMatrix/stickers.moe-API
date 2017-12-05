#include "util.hpp"

#include <sstream>


namespace
{
    std::string format_missing_blame_message(
        stickers::audit::blame_field  field,
        const std::string           & operation,
        const stickers::audit::blame& fields
    )
    {
        std::stringstream message;
        
        std::string field_name;
        switch( field )
        {
        case stickers::audit::WHO:
            field_name = "who";
            break;
        case stickers::audit::WHAT:
            field_name = "what";
            break;
        case stickers::audit::WHEN:
            field_name = "when";
            break;
        case stickers::audit::WHERE:
            field_name = "where";
            break;
        default:
            field_name = "????";
            break;
        }
        
        message
            << "missing audit field \""
            << field_name
            << "\" for "
            << operation
            << " {"
        ;
        
        if( field != stickers::audit::WHO )
        {
            message << "who=";
            if( fields.who == nullptr )
                message << "null";
            else
                message << std::to_string( ( long long )( *fields.who ) );
            message << ";";
        }
        
        if( field != stickers::audit::WHAT )
        {
            message << "what=";
            if( fields.what == nullptr )
                message << "null";
            else
                message << "\"" << *fields.what << "\"";
            message << ";";
        }
        
        if( field != stickers::audit::WHEN )
        {
            message << "when=";
            if( fields.when == nullptr )
                message << "null";
            else
                message
                    << "\""
                    << stickers::to_iso8601_str( *fields.when )
                    << "\""
                ;
            message << ";";
        }
        
        if( field != stickers::audit::WHERE )
        {
            message << "where=";
            if( fields.where == nullptr )
                message << "null";
            else
                message << "\"" << *fields.where << "\"";
            message << ";";
        }
        
        message << "}";
        
        return message.str();
    }
}


namespace stickers
{
    namespace audit
    {
        missing_blame::missing_blame(
            // const std::string& field_name,
            blame_field        field,
            const std::string& operation,
            const blame      & fields
        ) : std::invalid_argument( format_missing_blame_message(
            field,
            operation,
            fields
        ) ) {}
    }
}

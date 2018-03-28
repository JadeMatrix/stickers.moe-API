#line 2 "handlers/auth.cpp"


#include "handlers.hpp"

#include "../api/user.hpp"
#include "../common/auth.hpp"
#include "../server/parse.hpp"
#include "../server/server.hpp"


namespace stickers
{
    void handlers::signup( show::request& request )
    {
        throw handler_exit{ { 500, "Not Implemented" }, "Not implemented" };
    }
    
    void handlers::login( show::request& request )
    {
        auto content = parse_request_content( request );
        
        for( const auto field : {
            "email",
            "password"
        } )
            if( content.find( field ) == content.end() )
                throw handler_exit{
                    { 400, "Bad Request" },
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !content[ field ].is_string() )
                throw handler_exit{
                    { 400, "Bad Request" },
                    "required field \""
                    + static_cast< std::string >( field )
                    + "\" must be a string"
                };
        
        try
        {
            auto user = load_user_by_email(
                content[ "email" ].get< std::string >()
            );
            
            if(
                user.info.password
                == content[ "password" ].get< std::string >()
            )
            {
                show::response response{
                    request.connection(),
                    show::HTTP_1_1,
                    { 200, "OK" },
                    {
                        server_header,
                        { "Authorization", {
                            "Bearer " + generate_auth_token_for_user(
                                user.id,
                                {
                                    user.id,
                                    "user login",
                                    now(),
                                    request.client_address()
                                }
                            )
                        } },
                        { "Content-Length", { "0" } },
                        { "Location", {
                            "/user/" + static_cast< std::string >( user.id )
                        } }
                    }
                };
                
                return;
            }
        }
        catch( const no_such_user& e )
        {}
        
        throw handler_exit{
            { 401, "Unauthorized" },
            "check email & password"
        };
    }
}

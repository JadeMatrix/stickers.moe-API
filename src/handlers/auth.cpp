#line 2 "handlers/auth.cpp"


#include "handlers.hpp"

#include "../api/user.hpp"
#include "../common/auth.hpp"
#include "../common/config.hpp"
#include "../server/parse.hpp"

#include <show/constants.hpp>


namespace stickers
{
    void handlers::signup(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        throw handler_exit{ show::code::NOT_IMPLEMENTED, "Not implemented" };
    }
    
    void handlers::login(
        show::request& request,
        const handler_vars_type& variables
    )
    {
        auto content = parse_request_content( request );
        
        for( const auto field : {
            "email",
            "password"
        } )
            if( content.find( field ) == content.end() )
                throw handler_exit{
                    show::code::BAD_REQUEST,
                    "missing required field \""
                    + static_cast< std::string >( field )
                    + "\""
                };
            else if( !content[ field ].is_string() )
                throw handler_exit{
                    show::code::BAD_REQUEST,
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
                permissions_assert_all(
                    get_user_permissions( user.id ),
                    { "log_in" }
                );
                
                auto auth_jwt = generate_auth_token_for_user(
                    user.id,
                    {
                        user.id,
                        "user login",
                        now(),
                        request.client_address()
                    }
                );
                auto auth_token = jwt::serialize( auth_jwt );
                
                auto token_message_json = nlj::json{
                    { "jwt", auth_token }
                }.dump();
                
                show::response response{
                    request.connection(),
                    show::HTTP_1_1,
                    show::code::OK,
                    {
                        show::server_header,
                        { "Authorization", {
                            "Bearer " + auth_token
                        } },
                        { "Content-Type", { "application/json" } },
                        { "Content-Length", {
                            std::to_string( token_message_json.size() )
                        } },
                        { "Location", {
                            "/user/" + static_cast< std::string >( user.id )
                        } },
                        { "Set-Cookie", {
                            config()[ "auth" ][ "token_cookie_name" ].get<
                                std::string
                            >()
                            + "="
                            + auth_token
                            + "; expires="
                            + to_http_ts_str( *auth_jwt.exp )
                            + "; domain="
                            + config()[ "auth" ][ "token_cookie_domain" ].get<
                                std::string
                            >()
                        } }
                    }
                };
                
                response.sputn(
                    token_message_json.c_str(),
                    token_message_json.size()
                );
                
                return;
            }
        }
        catch( const no_such_user& e )
        {}
        
        throw handler_exit{
            show::code::UNAUTHORIZED,
            "check email & password"
        };
    }
}

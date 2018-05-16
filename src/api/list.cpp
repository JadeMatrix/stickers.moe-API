#line 2 "api/list.cpp"


#include "list.hpp"

#include "../api/user.hpp"
#include "../common/postgres.hpp"


namespace
{
    
}


namespace stickers
{
    std::vector< list_entry > get_user_list( const bigid& user_id )
    {
        // Assert user exists
        auto user_info{ load_user( user_id ) };
        
        auto connection = postgres::connect();
        pqxx::work transaction{ *connection };
        
        auto result = transaction.exec_params(
            PSQL(
                SELECT
                    product_id,
                    revised,
                    quantity
                FROM lists.user_product_lists
                WHERE user_id = $1
            ),
            user_id
        );
        transaction.commit();
        
        std::vector< list_entry > list;
        
        for( const auto& row : result )
            list.push_back( {
                row[ "product_id" ].as< bigid         >(),
                row[ "quantity"   ].as< unsigned long >(),
                row[ "revised"    ].as< timestamp     >()
            } );
        
        return list;
    }
}

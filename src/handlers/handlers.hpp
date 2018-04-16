#pragma once
#ifndef STICKERS_MOE_HANDLERS_HANDLERS_HPP
#define STICKERS_MOE_HANDLERS_HANDLERS_HPP


#include "../server/handler.hpp"


namespace stickers
{
    namespace handlers
    {
        void           signup( show::request& );
        void            login( show::request& );
        
        void      create_user( show::request& );
        void         get_user( show::request& );
        void        edit_user( show::request& );
        void      delete_user( show::request& );
        
        void         get_list( show::request& );
        void    add_list_item( show::request& );
        void update_list_item( show::request& );
        void remove_list_item( show::request& );
        
        void    create_person( show::request& );
        void       get_person( show::request& );
        void      edit_person( show::request& );
        void    delete_person( show::request& );
        
        void      create_shop( show::request& );
        void         get_shop( show::request& );
        void        edit_shop( show::request& );
        void      delete_shop( show::request& );
        
        void    create_design( show::request& );
        void       get_design( show::request& );
        void      edit_design( show::request& );
        void    delete_design( show::request& );
        
        void   create_product( show::request& );
        void      get_product( show::request& );
        void     edit_product( show::request& );
        void   delete_product( show::request& );
    }
}


#endif

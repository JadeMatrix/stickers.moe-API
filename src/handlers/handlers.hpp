#pragma once
#ifndef STICKERS_MOE_HANDLERS_HANDLERS_HPP
#define STICKERS_MOE_HANDLERS_HANDLERS_HPP


#include "../server/handler.hpp"


namespace stickers
{
    namespace handlers
    {
        void           signup( show::request&, const handler_vars_type& );
        void            login( show::request&, const handler_vars_type& );
        
        void      create_user( show::request&, const handler_vars_type& );
        void         get_user( show::request&, const handler_vars_type& );
        void        edit_user( show::request&, const handler_vars_type& );
        void      delete_user( show::request&, const handler_vars_type& );
        
        void         get_list( show::request&, const handler_vars_type& );
        void    add_list_item( show::request&, const handler_vars_type& );
        void update_list_item( show::request&, const handler_vars_type& );
        void remove_list_item( show::request&, const handler_vars_type& );
        
        void    create_person( show::request&, const handler_vars_type& );
        void       get_person( show::request&, const handler_vars_type& );
        void      edit_person( show::request&, const handler_vars_type& );
        void    delete_person( show::request&, const handler_vars_type& );
        
        void      create_shop( show::request&, const handler_vars_type& );
        void         get_shop( show::request&, const handler_vars_type& );
        void        edit_shop( show::request&, const handler_vars_type& );
        void      delete_shop( show::request&, const handler_vars_type& );
        
        void    create_design( show::request&, const handler_vars_type& );
        void       get_design( show::request&, const handler_vars_type& );
        void      edit_design( show::request&, const handler_vars_type& );
        void    delete_design( show::request&, const handler_vars_type& );
        
        void   create_product( show::request&, const handler_vars_type& );
        void      get_product( show::request&, const handler_vars_type& );
        void     edit_product( show::request&, const handler_vars_type& );
        void   delete_product( show::request&, const handler_vars_type& );
    }
}


#endif

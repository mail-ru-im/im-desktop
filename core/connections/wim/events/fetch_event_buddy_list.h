#ifndef __FETCH_EVENT_BUDDY_LIST_H_
#define __FETCH_EVENT_BUDDY_LIST_H_
#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        class contactlist;

        class fetch_event_buddy_list : public fetch_event
        {
            std::shared_ptr<core::wim::contactlist>	cl_;

        public:

            std::shared_ptr<core::wim::contactlist>	get_contactlist() const;

            fetch_event_buddy_list();
            virtual ~fetch_event_buddy_list();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }

}



#endif//__FETCH_EVENT_BUDDY_LIST_H_
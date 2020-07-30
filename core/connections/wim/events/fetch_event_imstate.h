#ifndef __FETCH_EVENT_IMSTATE_H_
#define __FETCH_EVENT_IMSTATE_H_

#pragma once

#include "fetch_event.h"
#include "../imstate.h"

namespace core
{
    namespace wim
    {
        class fetch_event_imstate : public fetch_event
        {
            imstate_list states_;

        public:

            fetch_event_imstate();
            virtual ~fetch_event_imstate();

            const imstate_list& get_states() const;

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}


#endif//__FETCH_EVENT_IMSTATE_H_

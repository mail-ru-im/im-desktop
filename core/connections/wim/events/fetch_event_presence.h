#ifndef __FETCH_EVENT_PRESENCE_H_
#define __FETCH_EVENT_PRESENCE_H_

#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        struct cl_presence;

        class fetch_event_presence : public fetch_event
        {
            std::string						aimid_;
            std::shared_ptr<cl_presence>	presense_;

        public:

            fetch_event_presence();
            virtual ~fetch_event_presence();

            const std::string& get_aimid() { return aimid_; }
            std::shared_ptr<cl_presence> get_presence() { return presense_; }

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}


#endif//__FETCH_EVENT_PRESENCE_H_
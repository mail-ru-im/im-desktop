#ifndef __FETCH_EVENT_HIDDEN_CHAT_H_
#define __FETCH_EVENT_HIDDEN_CHAT_H_

#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        class fetch_event_hidden_chat : public fetch_event
        {
            std::string aimid_;
            int64_t last_msg_id_ = 0;

        public:

            fetch_event_hidden_chat();
            virtual ~fetch_event_hidden_chat();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_aimid() const { return aimid_; }
            int64_t get_last_msg_id() const { return last_msg_id_; }
        };
    }
}


#endif//__FETCH_EVENT_HIDDEN_CHAT_H_
#ifndef __FETCH_EVENT_MCHAT_H_
#define __FETCH_EVENT_MCHAT_H_

#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        class fetch_event_mchat : public fetch_event
        {
        private:
            std::string aimid_;
            int64_t sip_code_ = 0;

        public:

            fetch_event_mchat();
            virtual ~fetch_event_mchat();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_aimid() const { return aimid_; }
            int64_t get_sip_code() const { return sip_code_; }
        };
    }
}


#endif//__FETCH_EVENT_HIDDEN_CHAT_H_
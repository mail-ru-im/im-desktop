#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        class fetch_event_typing: public fetch_event
        {
            bool is_typing_ = false;

            std::string aimid_;

            std::string chatter_aimid_;

            std::string chatter_name_;

        public:

            fetch_event_typing();

           virtual  ~fetch_event_typing();

            inline bool is_typing() const { return is_typing_; }

            inline const std::string &aim_id() const { return aimid_; }

            inline const std::string &chatter_aim_id() const { return chatter_aimid_; }

            inline const std::string &chatter_name() const { return chatter_name_; }

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;

            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}
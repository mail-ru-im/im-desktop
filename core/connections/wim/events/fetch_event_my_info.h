#pragma once

#include "fetch_event.h"
#include "../my_info.h"

namespace core
{
    namespace wim
    {
        class fetch_event_my_info : public fetch_event
        {
            std::shared_ptr<my_info>			info_;

        public:

            fetch_event_my_info();
            virtual ~fetch_event_my_info();

            std::shared_ptr<my_info> get_info() { return info_; }

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}
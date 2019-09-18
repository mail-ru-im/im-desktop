#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        class fetch_event_appsdata : public fetch_event
        {
            bool refresh_stickers_ = false;

        public:

            fetch_event_appsdata();

            virtual  ~fetch_event_appsdata();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;

            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            bool is_refresh_stickers() const;
        };

    }
}
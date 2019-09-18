#pragma once

#include "fetch_event.h"
#include "../../../archive/gallery_cache.h"

namespace core
{
    namespace wim
    {
        class fetch_event_gallery_notify : public fetch_event
        {
            archive::gallery_storage gallery_;
            std::string my_aimid_;

        public:

            fetch_event_gallery_notify(const std::string& _my_aimid);

            virtual  ~fetch_event_gallery_notify();

            const archive::gallery_storage& get_gallery() const;

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;

            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}
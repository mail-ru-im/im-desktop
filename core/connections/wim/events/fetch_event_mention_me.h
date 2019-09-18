#pragma once

#include "fetch_event.h"
#include "../my_info.h"
#include "../persons_packet.h"

namespace core
{
    namespace archive
    {
        class history_message;
    }

    namespace wim
    {
        class fetch_event_mention_me : public fetch_event, public persons_packet
        {
            std::shared_ptr<my_info> info_;

            std::string aimid_;

            std::shared_ptr<archive::history_message> message_;

            std::shared_ptr<core::archive::persons_map> persons_;

        public:

            fetch_event_mention_me();
            virtual ~fetch_event_mention_me();

            std::shared_ptr<my_info> get_info() { return info_; }

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_aimid() const;
            std::shared_ptr<archive::history_message> get_message() const;
            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
        };

    }
}
#pragma once

#include "fetch_event.h"
#include "../persons_packet.h"

namespace core
{
    namespace wim
    {
        struct chat_heads
        {
            std::map<int64_t, std::vector<std::string>> chat_heads_;
            std::string chat_aimid_;
            bool reset_state_;
            std::shared_ptr<core::archive::persons_map> persons_;

            chat_heads();
            void merge(const chat_heads& _other);
        };
        using chat_heads_sptr = std::shared_ptr<chat_heads>;

        class fetch_event_chat_heads : public fetch_event, public persons_packet
        {
            chat_heads_sptr heads_;

        public:

            fetch_event_chat_heads();
            virtual ~fetch_event_chat_heads();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
            chat_heads_sptr get_heads() const { return heads_; }
            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return heads_->persons_; }
        };
    }
}
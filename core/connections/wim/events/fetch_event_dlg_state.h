#pragma once

#include "fetch_event.h"
#include "../../../archive/dlg_state.h"
#include "../persons_packet.h"

namespace core
{
    namespace wim
    {
        class fetch_event_dlg_state : public fetch_event, public persons_packet
        {
            std::string aimid_;
            archive::dlg_state state_;
            std::shared_ptr<archive::history_block> tail_messages_;
            std::shared_ptr<archive::history_block> intro_messages_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:

            fetch_event_dlg_state();
            virtual ~fetch_event_dlg_state();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_aim_id() const { return aimid_; }
            const archive::dlg_state& get_dlg_state() const { return state_; }
            const std::shared_ptr<archive::history_block>& get_tail_messages() const { return tail_messages_; }
            const std::shared_ptr<archive::history_block>& get_intro_messages() const { return intro_messages_; }

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
        };
    }
}
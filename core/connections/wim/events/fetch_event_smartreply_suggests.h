#pragma once

#include "fetch_event.h"

namespace core
{
    namespace smartreply
    {
        class suggest;
    }

    namespace wim
    {
        class fetch_event_smartreply_suggest : public fetch_event
        {
        private:
            std::shared_ptr<std::vector<smartreply::suggest>> suggests_;
        public:
            fetch_event_smartreply_suggest();
            virtual ~fetch_event_smartreply_suggest();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
            const std::shared_ptr<std::vector<smartreply::suggest>>& get_suggests() const { return suggests_; }
        };
    }
}

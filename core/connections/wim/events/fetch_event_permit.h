#pragma once

#include "fetch_event.h"

namespace core
{
    namespace wim
    {
        typedef std::unordered_set<std::string> ignorelist_cache;

        class permit_info;

        class fetch_event_permit: public fetch_event
        {
        private:
            std::unique_ptr<permit_info> permit_info_;

        public:
            fetch_event_permit();
            ~fetch_event_permit();

            const ignorelist_cache& ignore_list() const;

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}
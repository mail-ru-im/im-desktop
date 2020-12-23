#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    namespace wim
    {
        class remove_members : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

            std::string	aimid_;
            std::vector<std::string> members_to_delete_;

        public:
            remove_members(wim_packet_params _params, std::string _aimid, std::vector<std::string> _members_to_delete);
            virtual std::string_view get_method() const override;
        };
    }
}
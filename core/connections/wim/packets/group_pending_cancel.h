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
        class group_pending_cancel : public robusto_packet
        {
        private:
            std::string sn_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:
            group_pending_cancel(wim_packet_params _params, std::string _sn);
            virtual std::string_view get_method() const override;
        };

    }

}
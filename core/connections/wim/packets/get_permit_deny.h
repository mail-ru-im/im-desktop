#ifndef __WIM_GET_PERMIT_DENY_H_
#define __WIM_GET_PERMIT_DENY_H_

#pragma once

#include "../wim_packet.h"

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
        class permit_info;

        class get_permit_deny : public wim_packet
        {
        private:

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            typedef std::unordered_set<std::string> ignorelist_cache;

            std::unique_ptr<permit_info> permit_info_;

        public:

            get_permit_deny(const wim_packet_params& _params);

            virtual ~get_permit_deny();

            const permit_info& get_ignore_list() const;
        };

    }

}


#endif// __WIM_GET_PERMIT_DENY_H_
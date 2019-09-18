#ifndef __WIM_SEARCH_CONTACTS_H_
#define __WIM_SEARCH_CONTACTS_H_

#pragma once

#include "../wim_packet.h"
#include "../persons_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace profile
    {
        class info;
        using profiles_list = std::vector<std::shared_ptr<info>>;
    }

    namespace wim
    {
        class get_profile : public wim_packet, public persons_packet
        {
            std::string             	aimId_;
            profile::profiles_list		search_result_;

            std::shared_ptr<core::archive::persons_map> persons_;


            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;


        public:

            get_profile(
                wim_packet_params _params,
                const std::string& _aimId);

            virtual ~get_profile();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }

            const profile::profiles_list& get_result() const;
        };

    }

}


#endif// __WIM_SEARCH_CONTACTS_H_
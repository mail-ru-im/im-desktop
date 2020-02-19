#ifndef __WIM_PHONEINFO_H_
#define __WIM_PHONEINFO_H_

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
        class phoneinfo : public wim_packet
        {
        private:
            std::string phone_;
            std::string gui_locale_;

        public:
            std::string info_operator_;
            std::string info_phone_;
            std::string info_iso_country_;
            std::vector<std::string> printable_;
            std::string status_;
            std::string trunk_code_;
            std::string modified_phone_number_;
            std::vector<int32_t> remaining_lengths_;
            std::vector<std::string> prefix_state_;
            std::string modified_prefix_;
            bool is_phone_valid_;

        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> response) override;

        public:
            phoneinfo(wim_packet_params params, const std::string &phone, const std::string &gui_locale);
            bool is_valid() const override { return true; }
            virtual ~phoneinfo();
        };

    }

}

#endif // __WIM_PHONEINFO_H_

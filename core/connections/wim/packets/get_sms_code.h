#ifndef __GET_SMS_CODE_H_
#define __GET_SMS_CODE_H_

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
        class validate_phone : public wim_packet
        {
            std::string		phone_;
            std::string		trans_id_;
            std::string     locale_;
            std::string     ivr_url_;
            std::string     checks_;
            int32_t         code_length_;
            bool			existing_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

        public:

            validate_phone(
                wim_packet_params params,
                const std::string& phone,
                const std::string& locale);

            virtual ~validate_phone();

            const int32_t get_code_length() const {return code_length_;}
            const std::string& get_phone() const {return phone_;}
            const bool get_existing() const {return existing_;}
            const std::string& get_trans_id() const {return trans_id_;}
            const std::string& get_ivr_url() const { return ivr_url_; }
            const std::string& get_checks() const { return checks_; }
            bool is_valid() const override { return true; }
            virtual std::string_view get_method() const override;
        };

        class get_code_by_phone_call : public wim_packet
        {
            std::string ivr_url_;
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:
            get_code_by_phone_call(wim_packet_params params, const std::string& _ivr_url);
            virtual ~get_code_by_phone_call();
            bool is_valid() const override { return true; }
            virtual std::string_view get_method() const override;
        };
    }
}


#endif //__GET_SMS_CODE_H_
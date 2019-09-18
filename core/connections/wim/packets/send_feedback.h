#ifndef __WIM_SEND_FEEDBACK_H_
#define __WIM_SEND_FEEDBACK_H_

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
        class send_feedback: public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> response) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

            std::string url_;
            std::map<std::string, std::string> fields_;
            std::vector<std::string> attachments_;

            std::wstring log_;

        public:
            send_feedback(wim_packet_params _params, const std::string &url, const std::map<std::string, std::string>& fields, const std::vector<std::string>& attachments);
            virtual ~send_feedback();

            bool result_;
        };

    }

}

#endif // __WIM_SEND_FEEDBACK_H_

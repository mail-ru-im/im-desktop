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
        class get_attach_phone_info : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

            std::string app_name_;
            std::string platform_;
            std::string locale_;
            std::string country_;
            std::string version_;

            bool show_;
            bool close_;
            int32_t timeout_;
            std::string text_;
            std::string title_;

        public:

            get_attach_phone_info(
                wim_packet_params _params,
                const std::string& _app_name,
                const std::string& _platform,
                const std::string& _locale,
                const std::string& _country,
                const std::string& _version
                );

            virtual ~get_attach_phone_info();

            bool get_show() const { return show_; }
            bool get_close() const { return close_; }
            int32_t get_timeout() const { return timeout_; }
            std::string get_text() const { return text_; }
            std::string get_title() const { return title_; }
        };

    }

}

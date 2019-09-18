#ifndef __WIM_WEBRTC_H__
#define __WIM_WEBRTC_H__

#include "../wim_packet.h"
#include "../../../Voip/VoipManagerDefines.h"

namespace core {
    namespace wim {

        /*
        virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
        virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
        virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;
        virtual int32_t on_response_error_code() override;

        bool			need_relogin_;
        std::string		fetch_url_;
        std::string		aimsid_;
        std::string		aimid_;

        int64_t			ts_;

        public:

        bool need_relogin()						{ return need_relogin_; }
        const std::string get_fetch_url() const	{ return fetch_url_; }
        const std::string get_aimsid() const	{ return aimsid_; }
        const time_t get_ts() const				{ return ts_; }
        const std::string get_aimid() const		{ return aimid_; }
        */

        class wim_allocate : public wim_packet {
            std::string _internal_params;
            std::shared_ptr<core::tools::binary_stream> _data;

            int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            int32_t parse_response(std::shared_ptr<core::tools::binary_stream> response) override;

        public:
            wim_allocate(wim_packet_params params, const std::string& internal_params);
            virtual ~wim_allocate();

            std::shared_ptr<core::tools::binary_stream> getRawData() override;
        };

        class wim_webrtc : public wim_packet {
            voip_manager::VoipProtoMsg _internal_params;
            std::shared_ptr<core::tools::binary_stream> _data;

            int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            int32_t parse_response(std::shared_ptr<core::tools::binary_stream> response) override;

        public:
            wim_webrtc(wim_packet_params params, const voip_manager::VoipProtoMsg& internal_params);
            virtual ~wim_webrtc();

            std::shared_ptr<core::tools::binary_stream> getRawData() override;
        };


        //class wim_webrtc : public wim_packet {
        //    std::string _internal_params;

        //    int InitRequest       (std::shared_ptr<http_request_simple> request);
        //    int ParseResponseData (const wim_response& _wim_response);

        //public:
        //    wim_webrtc(const wim_packet_params& params, const std::string& internal_params);
        //    virtual ~wim_webrtc();
        //};


    }
}

#endif//__WIM_WEBRTC_H__
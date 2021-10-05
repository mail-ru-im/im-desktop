#include "stdafx.h"
#include "wim_webrtc.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

namespace core {
    namespace wim {

        wim_allocate::wim_allocate(wim_packet_params params, const std::string& internal_params)
            : wim_packet(std::move(params))
            , _internal_params(internal_params) {

        }

        wim_allocate::~wim_allocate() {

        }

        std::string_view wim_allocate::get_method() const
        {
            return "webrtcAlloc";
        }

        int32_t wim_allocate::init_request(const std::shared_ptr<core::http_request_simple>& _request) {
            if (!_request) { im_assert(false); return 1; }
            if (params_.aimsid_.empty()) { im_assert(false); return 1; }
            if (params_.dev_id_.empty()) { im_assert(false); return 1; }

            std::stringstream ss_host;
            ss_host << urls::get_url(urls::url_type::webrtc_host) << std::string_view("/webrtc/alloc");

            std::map<std::string, std::string> params;
            params["aimsid"] = escape_symbols(params_.aimsid_);
            params["f"] = "json";
            params["k"] = params_.dev_id_;
            params["r"] = core::tools::system::generate_guid();

            if (!_internal_params.empty()) {
                auto amp_pos = std::string::npos;
                auto start_pos = amp_pos;
                do {
                    amp_pos++;
                    start_pos = amp_pos;
                    amp_pos = _internal_params.find('&', amp_pos);
                    std::string sstr = amp_pos != std::string::npos ? _internal_params.substr(start_pos, amp_pos - start_pos) : _internal_params.substr(start_pos);
                    auto eq_pos = sstr.find('=');
                    if (std::string::npos != eq_pos) {
                        std::string nam = sstr.substr(0, eq_pos);
                        std::string val = sstr.substr(eq_pos + 1);

                        if (!nam.empty() && !val.empty()) {
                            params[std::move(nam)] = std::move(val);
                        } else {
                            im_assert(false);
                        }
                    }
                } while(amp_pos != std::string::npos);
            }

            std::stringstream ss_url;
            ss_url << ss_host.str();

            bool first = true;
            for (const auto& it : params) {
                if (first) {
                    ss_url << '?';
                } else {
                    ss_url << '&';
                }
                first = false;
                ss_url << it.first << '=' << it.second;
            }

            _request->set_connect_timeout(std::chrono::seconds(10));
            _request->set_timeout(std::chrono::seconds(15));
            _request->set_url(ss_url.str());
            _request->set_normalized_url(get_method());

            if (!params_.full_log_)
            {
                log_replace_functor f;
                f.add_marker("a");
                _request->set_replace_log_function(f);
            }

            return 0;
        }

        int32_t wim_allocate::parse_response(const std::shared_ptr<core::tools::binary_stream>& response) {
            if (!response->available()) {
                im_assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
            return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_allocate::getRawData() const {
            return _data;
        }


        wim_webrtc::wim_webrtc(wim_packet_params params, const voip_manager::VoipProtoMsg& internal_params)
            : wim_packet(std::move(params))
            , _internal_params(internal_params) {

        }

        wim_webrtc::~wim_webrtc() {

        }

        std::string_view wim_webrtc::get_method() const
        {
            return "webrtcMsg";
        }

        int32_t wim_webrtc::init_request(const std::shared_ptr<core::http_request_simple>& _request) {
            if (!_request) { im_assert(false); return 1; }

            std::stringstream ss_host;
            ss_host << urls::get_url(urls::url_type::webrtc_host) << std::string_view("/voip/webrtcMsg");

            std::stringstream ss_params;
            ss_params << "json" << '&' << _internal_params.request;

            _request->push_post_parameter("aimsid", escape_symbols(params_.aimsid_));
            _request->push_post_parameter("r", escape_symbols(_internal_params.requestId));
            _request->push_post_parameter("f", ss_params.str());

            _request->set_url(ss_host.str());
            _request->set_normalized_url(get_method());

            if (!params_.full_log_)
            {
                log_replace_functor f;
                f.add_marker("aimsid", aimsid_range_evaluator());
                _request->set_replace_log_function(f);
            }
            return 0;
        }

        int32_t wim_webrtc::execute_request(const std::shared_ptr<core::http_request_simple>& request)
        {
            url_ = request->get_url();

            if (auto error_code = get_error(request->post()))
                return *error_code;

            http_code_ = (uint32_t)request->get_response_code();

            if (http_code_ != 200)
                return wpie_http_error;

            return 0;
        }

        int32_t wim_webrtc::parse_response(const std::shared_ptr<core::tools::binary_stream>& response) {
            if (!response->available()) {
                im_assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
            return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_webrtc::getRawData() const {
            return _data;
        }
    }
}

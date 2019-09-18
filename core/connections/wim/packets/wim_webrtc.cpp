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

        int32_t wim_allocate::init_request(std::shared_ptr<core::http_request_simple> _request) {
            if (!_request) { assert(false); return 1; }
            if (params_.a_token_.empty()) { assert(false); return 1; }
            if (params_.dev_id_.empty()) { assert(false); return 1; }

            std::stringstream ss_host;
            ss_host << urls::get_url(urls::url_type::webrtc_host) << std::string_view("/webrtc/alloc");

            const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

            std::map<std::string, std::string> params;
            params["a"] = escape_symbols(params_.a_token_);
            params["f"] = "json";
            params["k"] = params_.dev_id_;
            params["r"] = core::tools::system::generate_guid();
            params["ts"] = tools::from_int64(ts);

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
                            assert(false);
                        }
                    }
                } while(amp_pos != std::string::npos);
            }

            auto sha256 = escape_symbols(get_url_sign(ss_host.str(), params, params_, false));
            params["sig_sha256"] = std::move(sha256);
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
            _request->set_normalized_url("webrtcAlloc");

            if (!params_.full_log_)
            {
                log_replace_functor f;
                f.add_marker("a");
                _request->set_replace_log_function(f);
            }

            return 0;
        }

        int32_t wim_allocate::parse_response(std::shared_ptr<core::tools::binary_stream> response) {
            if (!response->available()) {
                assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
            return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_allocate::getRawData() {
            return _data;
        }


        wim_webrtc::wim_webrtc(wim_packet_params params, const voip_manager::VoipProtoMsg& internal_params)
            : wim_packet(std::move(params))
            , _internal_params(internal_params) {

        }

        wim_webrtc::~wim_webrtc() {

        }


        int32_t wim_webrtc::init_request(std::shared_ptr<core::http_request_simple> _request) {
            if (!_request) { assert(false); return 1; }

            std::stringstream ss_host;
            ss_host << urls::get_url(urls::url_type::webrtc_host) << std::string_view("/voip/webrtcMsg");

            std::stringstream ss_url;
            ss_url << ss_host.str()
                << "?aimsid=" << escape_symbols(params_.aimsid_)
                << "&f=json"
                << "&r=" << escape_symbols(_internal_params.requestId);

            if (!_internal_params.request.empty()) {
                ss_url << '&' << _internal_params.request;
            }

            _request->set_url(ss_url.str());
            _request->set_normalized_url("webrtcMsg");

            if (!params_.full_log_)
            {
                log_replace_functor f;
                f.add_marker("aimsid", aimsid_range_evaluator());
                _request->set_replace_log_function(f);
            }

            return 0;
        }

        int32_t wim_webrtc::parse_response(std::shared_ptr<core::tools::binary_stream> response) {
            if (!response->available()) {
                assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
            return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_webrtc::getRawData() {
            return _data;
        }
    }
}

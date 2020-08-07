#ifndef __WIMPACKET_H_
#define __WIMPACKET_H_

#pragma once

#include "../../async_task.h"
#include "../../proxy_settings.h"

namespace core
{
    class http_request_simple;

    namespace tools
    {
        class binary_stream;
    }
}

namespace core
{
    namespace wim
    {

#define WIM_CLIENT_DIST_ID		30016

#define WIM_CAP_VOIP_VOICE         "094613504c7f11d18222444553540000"
#define WIM_CAP_VOIP_VIDEO         "094613514c7f11d18222444553540000"
#define WIM_CAP_VOIP_RINGING       "094613564c7f11d18222444553540000"
#define WIM_CAP_FOCUS_GROUP_CALLS  "094613503c7f11d18222444553540000"
#define WIM_CAP_FILETRANSFER       "094613434c7f11d18222444553540000"
#define WIM_CAP_UNIQ_REQ_ID        "094613534c7f11d18222444553540000"
#define WIM_CAP_EMOJI              "094613544c7f11d18222444553540000"
#define WIM_CAP_MENTIONS           "0946135b4c7f11d18222444553540000"
#define WIM_CAP_MAIL_NOTIFICATIONS "094613594c7f11d18222444553540000"
#define WIM_CAP_INTRO_DLG_STATE    "0946135a4c7f11d18222444553540000"
#define WIM_CAP_CHAT_HEADS         "0946135c4c7f11d18222444553540000"
#define WIM_CAP_GALLERY_NOTIFY     "0946135e4c7f11d18222444553540000"
#define WIM_CAP_GROUP_SUBSCRIPTION "1f99494e76cbc880215d6aeab8e42268"
#define WIM_CAP_RECENT_CALLS       "0946135d4c7f11d18222444553540000"
#define WIM_CAP_REACTIONS          "a20c362cd4944b6ea3d1e77642201fd8"

#define SAAB_SESSION_OLDER_THAN_AUTH_UPDATE          1010



        //////////////////////////////////////////////////////////////////////////
        // wim protocol
        //////////////////////////////////////////////////////////////////////////
        enum wim_protocol_internal_error
        {
            wpie_invalid_login = 1,
            wpie_network_error = 2,
            wpie_http_error = 3,
            wpie_http_empty_response = 4,
            wpie_http_parse_response = 5,
            wpie_wrong_login = 6,
            wpie_request_timeout = 7,
            wpie_login_unknown_error = 8,

            wpie_start_session_wrong_credentials = 9,
            wpie_start_session_invalid_request = 10,
            wpie_start_session_request_timeout = 11,
            wpie_start_session_rate_limit = 12,
            wpie_start_session_wrong_devid = 13,
            wpie_start_session_unknown_error = 14,

            wpie_error_parse_response = 15,

            wpie_error_request_canceled = 16,

            wpie_error_too_fast_sending = 17,
            wpie_error_dest_not_avalible = 18,
            wpie_error_rate_limit = 19,

            wpie_error_message_unknown = 20,
            wpie_error_no_members_found = 21,

            wpie_get_sms_code_unknown_error = 22,
            wpie_phone_not_verified = 23,
            wpie_invalid_phone_number = 24,

            wpie_error_robusto_token_invalid = 25,
            wpie_error_robusto_bad_app_key = 26,
            wpie_error_robusto_icq_auth_expired = 27,
            wpie_error_robusto_bad_sigsha = 28,
            wpie_error_robusto_bad_ts = 29,

            wpie_error_profile_not_loaded = 30,
            wpie_error_invalid_request = 31,
            wpie_error_profile_not_found = 32,

            wpie_error_robusto_you_are_not_chat_member = 33,

            wpie_error_attach_busy_phone = 34,

            wpie_error_too_large_file = 35,

            wpie_error_request_canceled_wait_timeout = 36,

            wpie_error_robusto_you_are_blocked = 37,

            wpie_wrong_login_2x_factor = 38,

            wpie_error_resend = 39,

            wpie_error_cannot_add_member = 40,

            wpie_error_robusto_target_not_found = 41,

            wpie_error_invalid_session_hash = 42,

            wpie_error_user_blocked = 43,

            wpie_client_http_error = 400,
            wpie_robusto_timeout = 500,

            wpie_error_need_relogin = 1000,
            wpie_error_session_killed = 1001,

            wpie_error_task_canceled = 2000,

            wpie_error_empty_avatar_data = 4000,

            wpie_error_metainfo_not_found = 8000,

            wpie_error_nickname_bad_value = 9000,
            wpie_error_nickname_already_used = 9001,
            wpie_error_nickname_not_allowed = 9002,
            wpie_error_robuso_too_fast_sending = 9003,
        };

        enum wim_protocol_error
        {
            OK = 200,
            MORE_AUTHN_REQUIRED = 330,
            INVALID_REQUEST = 400,
            AUTHN_REQUIRED = 401,
            REQUEST_TIMEOUT = 408,
            SEND_IM_RATE_LIMIT = 430,
            SESSION_NOT_FOUND = 451,
            MISSING_REQUIRED_PARAMETER = 460,
            PARAMETER_ERROR = 462,
            GENERIC_SERVER_ERROR = 500,
            INVALID_TARGET = 600,
            TARGET_DOESNT_EXIST = 601,
            TARGET_NOT_AVAILABLE = 602,
            CAPCHA = 603,
            MESSAGE_TOO_BIG = 606,
            TARGET_RATE_LIMIT_REACHED = 607
        };

        enum wim_protocol_subcodes
        {
            no_such_session = 7
        };

        struct robusto_packet_params
        {
            uint32_t robusto_req_id_;

            explicit robusto_packet_params(const uint32_t robusto_req_id)
                : robusto_req_id_(robusto_req_id)
            {
            }
        };

        struct wim_packet_params
        {
            std::function<bool()> stop_handler_;
            std::string a_token_;
            std::string session_key_;
            std::string dev_id_;
            std::string aimsid_;
            time_t time_offset_;
            time_t time_offset_local_;
            std::string uniq_device_id_;
            std::string aimid_;
            proxy_settings proxy_;
            bool full_log_;
            int64_t nonce_;
            std::string locale_;

            wim_packet_params(
                std::function<bool()> _stop_handler,
                const std::string& _a_token,
                const std::string& _session_key,
                const std::string& _dev_id,
                const std::string& _aimsid,
                const std::string& _uniq_device_id,
                const std::string& _aimid,
                const time_t _time_offset,
                const time_t _time_offset_local,
                const proxy_settings& _proxy,
                const bool _full_log,
                const std::string& _locale)
                :
                stop_handler_(std::move(_stop_handler)),
                a_token_(_a_token),
                session_key_(_session_key),
                dev_id_(_dev_id),
                aimsid_(_aimsid),
                time_offset_(_time_offset),
                time_offset_local_(_time_offset_local),
                uniq_device_id_(_uniq_device_id),
                aimid_(_aimid),
                proxy_(_proxy),
                full_log_(_full_log),
                locale_(_locale)
            {
                static int64_t nonce_counter = 0;
                nonce_ = ++nonce_counter;
            }

            wim_packet_params(const wim_packet_params&) = default;
            wim_packet_params(wim_packet_params&&) = default;

            wim_packet_params& operator=(const wim_packet_params&) = default;
            wim_packet_params& operator=(wim_packet_params&&) = default;

            bool is_auth_valid() const
            {
                return (!a_token_.empty() && !session_key_.empty() && !dev_id_.empty() && !aimsid_.empty());
            }

        };

        class wim_packet
            : public async_task
            , public std::enable_shared_from_this<wim_packet>
        {
            std::string response_str_;
            std::string header_str_;

            bool hosts_scheme_changed_;

            static bool is_network_error_or_canceled(const int32_t _error) noexcept;
            static bool is_timeout_error(const int32_t _error) noexcept;
            bool has_valid_token() const;

            static std::string get_url_sign_impl(std::string_view host, std::string_view _query_string, const wim_packet_params& _wim_params, bool post_method, bool make_escape_symbols = true);

        public:
            using handler_t = std::function<void (int32_t _result)>;

        protected:

            void load_response_str(const char* buf, size_t size);
            const std::string& response_str() const;
            const std::string& header_str() const;

            uint32_t status_code_;
            uint32_t status_detail_code_;
            std::string status_text_;
            uint32_t http_code_;
            uint32_t repeat_count_;
            bool can_change_hosts_scheme_;

            wim_packet_params params_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& request);
            virtual int32_t execute_request(const std::shared_ptr<core::http_request_simple>& request);
            virtual void execute_request_async(const std::shared_ptr<core::http_request_simple>& request, handler_t _handler);
            virtual int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& response);
            virtual int32_t parse_response_data(const rapidjson::Value& _data);
            virtual void parse_response_data_on_error(const rapidjson::Value& _data);

            virtual int32_t on_empty_data();
            virtual int32_t on_response_error_code();

            virtual int32_t on_http_client_error();

            const wim_packet_params& get_params() const;
            std::string extract_etag() const;

        public:

            virtual bool is_valid() const { return has_valid_token(); };
            virtual bool support_async_execution() const;

            int32_t execute() override final;
            void execute_async(handler_t _handler);

            virtual priority_t get_priority() const { return packets_priority(); }
            virtual bool is_post() const { return false; }

            static std::string escape_symbols(std::string_view data);
            static std::string escape_symbols_data(std::string_view data);
            template<typename R>
            static std::string format_get_params(const R& _params)
            {
                std::string result;

                for (const auto& [key, value] : _params)
                {
                    result += key;
                    result += '=';
                    result += value;
                    result += '&';
                }
                if (!result.empty())
                    result.pop_back();

                return result;
            }

            template<typename R>
            static inline std::string get_url_sign(std::string_view host, R&& params, const wim_packet_params& _wim_params, bool post_method, bool make_escape_symbols = true)
            {
                return get_url_sign_impl(host, format_get_params(std::forward<R>(params)), _wim_params, post_method, make_escape_symbols);
            }

            static std::string detect_digest(std::string_view hashed_data, std::string_view session_key);

            virtual std::shared_ptr<core::tools::binary_stream> getRawData() const { return {}; }
            uint32_t get_status_code() const { return status_code_; }
            uint32_t get_status_detail_code() const { return status_detail_code_; }
            const std::string& get_status_text() const { return status_text_; }
            uint32_t get_http_code() const { return http_code_; }
            void set_http_code(uint32_t _code) { http_code_ = _code; }
            uint32_t get_repeat_count() const;
            void set_repeat_count(const uint32_t _count);
            bool is_stopped() const;

            static bool needs_to_repeat_failed(int32_t _error) noexcept;

            wim_packet(wim_packet_params _params);
            virtual ~wim_packet();
        };
    }

    class log_replace_functor
    {
        public:
            using ranges = std::vector<std::pair<std::ptrdiff_t,std::ptrdiff_t>>;
        private:
            using ranges_evaluator = std::function<ranges(std::string_view s)>;
            using marker_item = std::pair<std::string, ranges_evaluator>;
        public:
            void add_marker(std::string_view _marker, ranges_evaluator _re = nullptr);
            void add_url_marker(std::string_view _marker, ranges_evaluator _re = nullptr);
            void add_json_marker(std::string_view _marker, ranges_evaluator _re = nullptr);

            //! It's able to find url-encoded array but doesn't mask it unless ranges_evaluator is specified
            void add_json_array_marker(std::string_view _marker, ranges_evaluator _re = nullptr);

            void add_message_markers();

            void operator()(tools::binary_stream& _bs) const;

            bool is_null() const;

        private:
            static void mask_char(char *c);

            std::vector<marker_item> markers_;
            std::vector<marker_item> markers_json_;
            std::vector<marker_item> markers_json_encoded_;
    };

    class aimsid_range_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class aimsid_single_range_evaluator: public aimsid_range_evaluator
    {
    public:
        std::pair<std::ptrdiff_t, std::ptrdiff_t> operator()(std::string_view s) const;
    };

    class speechtotext_fileid_range_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class poll_responses_ranges_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class tail_from_last_range_evaluator
    {
    public:
        tail_from_last_range_evaluator(const char _chr);

        log_replace_functor::ranges operator()(std::string_view _str) const;

    private:
        const char chr_;
    };

    class distance_range_evaluator
    {
    public:
        distance_range_evaluator(std::ptrdiff_t _distance);

        log_replace_functor::ranges operator()(std::string_view _str) const;

    private:
        const std::ptrdiff_t distance_;
    };
}



#endif //__WIMPACKET_H_

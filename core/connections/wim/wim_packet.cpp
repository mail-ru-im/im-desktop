#include "stdafx.h"

#include "wim_packet.h"

#include "../../http_request.h"
#include "../../tools/hmac_sha_base64.h"
#include "../../../common.shared/json_helper.h"
#include "../../log/log.h"
#include "../../utils.h"
#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"
#include "../urls_cache.h"

#include "../../../libomicron/include/omicron/omicron.h"

using namespace core;
using namespace wim;

wim_packet::wim_packet(wim_packet_params params)
    :
    hosts_scheme_changed_(false),
    status_code_(std::numeric_limits<uint32_t>::max()),
    status_detail_code_(std::numeric_limits<uint32_t>::max()),
    http_code_(std::numeric_limits<uint32_t>::max()),
    repeat_count_(0),
    can_change_hosts_scheme_(false),
    params_(std::move(params))
{
}

wim_packet::~wim_packet() = default;

bool wim_packet::support_async_execution() const
{
    return get_priority() > priority_protocol();
}

bool wim_packet::support_partially_async_execution() const
{
    return false;
}

bool wim_packet::support_self_resending() const
{
    return false;
}

int32_t wim_packet::execute()
{
    auto request = std::make_shared<core::http_request_simple>(params_.proxy_, utils::get_user_agent(params_.aimid_), get_priority(), params_.stop_handler_);

    ++repeat_count_;

    int32_t err = init_request(request);
    if (err != 0)
        return err;

    err = execute_request(request);
    if (err != 0)
        return err;

    auto response = std::static_pointer_cast<tools::binary_stream>(request->get_response());
    im_assert(response);
    err = parse_response(response);
    if (err != 0 && g_core->is_im_stats_enabled())
    {
        core::stats::event_props_type props;
        props.emplace_back("endpoint", request->get_normalized_url());
        props.emplace_back("error", std::to_string(err));
        props.emplace_back("status", std::to_string(status_code_));

        if (err == wpie_http_parse_response)
            g_core->insert_im_stats_event(stats::im_stat_event_names::u_network_parsing_error_event, std::move(props));
        else if (status_code_ != 200 && status_code_ != 20000)
            g_core->insert_im_stats_event(stats::im_stat_event_names::u_network_api_error_event, std::move(props));
    }

    return err;
}

void wim_packet::execute_async(handler_t _handler)
{
    im_assert(support_async_execution() || support_partially_async_execution());

    auto request = std::make_shared<core::http_request_simple>(params_.proxy_, utils::get_user_agent(params_.aimid_), get_priority(), params_.stop_handler_);

    if (int32_t err = init_request(request); err != 0)
    {
        _handler(err);
        return;
    }

    execute_request_async(request, [wr_this = weak_from_this(), request, _handler = std::move(_handler)](int32_t _err)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_err != 0)
        {
            _handler(_err);
            return;
        }

        auto response = std::static_pointer_cast<tools::binary_stream>(request->get_response());
        im_assert(response);
        const auto err = ptr_this->parse_response(response);
        if (err != 0 && g_core->is_im_stats_enabled())
        {
            core::stats::event_props_type props;
            props.emplace_back("endpoint", request->get_normalized_url());
            props.emplace_back("error", std::to_string(err));
            props.emplace_back("status", std::to_string(ptr_this->get_status_code()));

            if (err == wpie_http_parse_response || err == wpie_http_empty_response)
                g_core->insert_im_stats_event(stats::im_stat_event_names::u_network_parsing_error_event, std::move(props));
            else if (ptr_this->get_status_code() != 200 && ptr_this->get_status_code() != 20000)
                g_core->insert_im_stats_event(stats::im_stat_event_names::u_network_api_error_event, std::move(props));
        }

        _handler(err);
    });
}

void wim_packet::update_params(wim_packet_params _params)
{
    params_ = std::move(_params);
}

bool wim_packet::is_stopped() const
{
    return params_.stop_handler_();
}

bool core::wim::wim_packet::has_valid_token() const
{
    return !params_.aimsid_.empty();
}

bool wim_packet::needs_to_repeat_failed(int32_t _error) noexcept
{
    return is_network_error_or_canceled(_error) || is_timeout_error(_error);
}

int32_t wim_packet::init_request(const std::shared_ptr<core::http_request_simple>& /*request*/)
{
    return 0;
}

int32_t wim_packet::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();
    auto res = is_post() ? request->post() : request->get();
    if (res != curl_easy::completion_code::success)
    {
        if (res == curl_easy::completion_code::resolve_failed)
            return wpie_couldnt_resolve_host;

        return wpie_network_error;
    }

    http_code_ = (uint32_t)request->get_response_code();

    if (request->get_header()->available())
    {
        const auto& header = request->get_header();
        auto size = header->available();
        auto buf = (const char *)header->read(size);

        if (buf && size)
        {
            header_str_.assign(buf, size);
        }

        header->reset_out();
    }

    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();
        else if (http_code_ >= 500 && http_code_ < 600)
            return wpie_error_resend;

        return wpie_http_error;
    }

    return 0;
}

void wim_packet::execute_request_async(const std::shared_ptr<core::http_request_simple>& _request, handler_t _handler)
{
    auto handler = [_request, _handler = std::move(_handler), wr_this = weak_from_this()](curl_easy::completion_code _completion_code)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_completion_code != curl_easy::completion_code::success)
        {
            if (_handler)
                _handler(_completion_code == curl_easy::completion_code::resolve_failed ? wpie_couldnt_resolve_host : wpie_network_error);
            return;
        }

        ptr_this->http_code_ = (uint32_t)_request->get_response_code();

        if (const auto& header = _request->get_header(); header->available())
        {
            const auto size = header->available();
            auto buf = (const char*)header->read(size);

            if (buf && size)
            {
                ptr_this->header_str_.assign(buf, size);
            }

            header->reset_out();
        }

        if (!_handler)
            return;

        if (ptr_this->http_code_ == 200)
        {
            _handler(0);
        }
        else if (ptr_this->http_code_ > 400 && ptr_this->http_code_ < 500)
        {
            _handler(ptr_this->on_http_client_error());
        }
        else if (ptr_this->http_code_ >= 500 && ptr_this->http_code_ < 600)
        {
            _handler(wpie_error_resend);
        }
        else
        {
            _handler(wpie_http_error);
        }
    };
    url_ = _request->get_url();
    if (is_post())
        _request->post_async(std::move(handler));
    else
        _request->get_async(std::move(handler));
}

std::optional<wim_protocol_internal_error> wim_packet::get_error(curl_easy::completion_code code)
{
    if (code != curl_easy::completion_code::success)
    {
        if (code == curl_easy::completion_code::resolve_failed)
            return wpie_couldnt_resolve_host;

        return wpie_network_error;

    }
    return std::nullopt;
}

void wim_packet::load_response_str(const char* buf, size_t size)
{
    im_assert(buf && size);
    if (buf && size)
    {
        response_str_.assign(buf, size);
    }
    else
    {
        response_str_.clear();
    }
}

const std::string& wim_packet::response_str() const
{
    return response_str_;
}

const std::string& wim_packet::header_str() const
{
    return header_str_;
}

int32_t wim_packet::parse_response(const std::shared_ptr<core::tools::binary_stream>& response)
{
    if (!response->available())
        return wpie_http_empty_response;

    int32_t err = 0;

    response->write((char) 0);

    auto size = response->available();

    load_response_str((const char*) response->read(size), size);

    response->reset_out();

    try
    {
        const auto json_str = response->read(response->available());

#ifdef DEBUG__OUTPUT_NET_PACKETS
        puts(json_str);
#endif // DEBUG__OUTPUT_NET_PACKETS

#ifdef DEBUG
        const std::string json_str_dbg(json_str);
#endif

        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        auto iter_response = doc.FindMember("response");
        if (iter_response == doc.MemberEnd())
            return wpie_http_parse_response;

        auto iter_status = iter_response->value.FindMember("statusCode");
        if (iter_status == iter_response->value.MemberEnd() || !iter_status->value.IsUint())
            return wpie_http_parse_response;

        status_code_ = iter_status->value.GetUint();

        auto iter_status_text = iter_response->value.FindMember("statusText");
        if (iter_status_text != iter_response->value.MemberEnd() && iter_status_text->value.IsString())
            status_text_ = rapidjson_get_string(iter_status_text->value);

        auto iter_status_detail = iter_response->value.FindMember("statusDetailCode");
        if (iter_status_detail != iter_response->value.MemberEnd() && iter_status_detail->value.IsUint())
            status_detail_code_ = (uint32_t) iter_status_detail->value.GetUint();

        if (status_code_ == 200)
        {
            auto iter_data = iter_response->value.FindMember("data");
            if (iter_data == iter_response->value.MemberEnd())
            {
                err = on_empty_data();
                if (err != 0)
                    return err;
            }
            else
            {
                return parse_response_data(iter_data->value);
            }
        }
        else
        {
            if (const auto iter_data = iter_response->value.FindMember("data"); iter_data != iter_response->value.MemberEnd())
                parse_response_data_on_error(iter_data->value);

            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}

int32_t wim_packet::on_response_error_code()
{
    switch (status_code_)
    {
        case INVALID_REQUEST:
            return wpie_error_invalid_request;

        case AUTHN_REQUIRED:
        case MISSING_REQUIRED_PARAMETER:
            return wpie_error_need_relogin;

        case SEND_IM_RATE_LIMIT:
        case TARGET_RATE_LIMIT_REACHED:
            return wpie_error_too_fast_sending;

        default:
            return wpie_error_message_unknown;
    }
}

int32_t wim_packet::on_http_client_error()
{
    if (http_code_ >= 500 && http_code_ < 600)
        return wpie_error_resend;
    return wpie_client_http_error;
}


int32_t wim_packet::on_empty_data()
{
    return wpie_http_parse_response;
}

std::string wim_packet::escape_symbols(std::string_view _data)
{
    std::string res;
    res.reserve(size_t(_data.size() * 1.1));

    std::array<char, 100> buffer;

    for (auto sym : _data)
    {
        if (core::tools::is_latin(sym) || core::tools::is_digit(sym) || strchr("-._~", sym))
        {
            res += sym;
        }
        else
        {
#ifdef _WIN32
            sprintf_s(buffer.data(), buffer.size(), "%%%.2X", (unsigned char) sym);
#else
            sprintf(buffer.data(), "%%%.2X", (unsigned char) sym);
#endif
            res += buffer.data();
        }
    }

    return res;
}

std::string wim_packet::escape_symbols_data(std::string_view _data)
{
    std::string res;
    res.reserve(size_t(_data.size() * 1.1));

    std::array<char, 100> buffer;

    for (auto sym : _data)
    {
        if (core::tools::is_latin(sym) || core::tools::is_digit(sym))
        {
            res += sym;
        }
        else
        {
#ifdef _WIN32
            sprintf_s(buffer.data(), buffer.size(), "%%%.2X", (unsigned char)sym);
#else
            sprintf(buffer.data(), "%%%.2X", (unsigned char)sym);
#endif
            res += buffer.data();
        }
    }

    return res;
}

int32_t wim_packet::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

void wim_packet::parse_response_data_on_error(const rapidjson::Value& _data)
{
}

const wim_packet_params& core::wim::wim_packet::get_params() const
{
    return params_;
}

uint32_t core::wim::wim_packet::get_repeat_count() const
{
    return repeat_count_;
}

void core::wim::wim_packet::set_repeat_count(uint32_t _count)
{
    repeat_count_ = _count;
}

void core::wim::wim_packet::increment_repeat_count()
{
    ++repeat_count_;
}

std::string wim_packet::extract_etag() const
{
    if (auto header = header_str(); !header.empty())
    {
        std::transform(header.begin(), header.end(), header.begin(), ::tolower);

        for (const std::string_view etag_prefix : { "etag: \"", "etag: w/\"" })
        {
            if (const auto etag_pos_begin = header.find(etag_prefix); etag_pos_begin != std::string::npos)
            {
                constexpr auto etag_postfix = std::string_view("\"");

                const auto etag_pos_end = header.find(etag_postfix, etag_pos_begin + etag_prefix.size());

                if (etag_pos_end != std::string::npos)
                    return std::string(header.begin() + etag_pos_begin + etag_prefix.size(), header.begin() + etag_pos_end);
            }
        }
    }

    return std::string();
}

bool wim_packet::is_network_error(const int32_t _error) noexcept
{
    return wim_protocol_internal_error::wpie_network_error == _error
        || wim_protocol_internal_error::wpie_couldnt_resolve_host == _error;
}

bool wim_packet::is_network_error_or_canceled(const int32_t _error) noexcept
{
    return is_network_error(_error)
        || wim_protocol_internal_error::wpie_error_resend == _error
        || wim_protocol_internal_error::wpie_error_task_canceled == _error;
}

bool wim_packet::is_timeout_error(const int32_t _error) noexcept
{
    return _error == wim_protocol_internal_error::wpie_robusto_timeout;
}

bool core::wim::wim_packet::supports_current_api_version() const
{
    return minimal_supported_api_version() <= core::urls::api_version::instance().preferred();
}

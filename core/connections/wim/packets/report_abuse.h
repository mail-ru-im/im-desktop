#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

enum class report_reason
{
    porn = 0,
    spam = 1,
    violation = 2,
    other
};

class report_abuse : public robusto_packet
{
public:
    report_abuse(wim_packet_params _params);

protected:
    virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
    virtual int32_t on_response_error_code() override;
};

class report_contact : public report_abuse
{
public:
    report_contact(wim_packet_params _params, const std::string& _aimid, const report_reason& _reason);

protected:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

private:
    std::string aimId_;
    report_reason reason_;
};

class report_stickerpack : public report_abuse
{
public:
    report_stickerpack(wim_packet_params _params, const int32_t _id, const report_reason& _reason);

protected:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

private:
    int32_t id_;
    report_reason reason_;
};

class report_sticker : public report_abuse
{
public:
    report_sticker(wim_packet_params _params, const std::string& _id, const report_reason& _reason, const std::string& _aimId, const std::string& _chatId);

protected:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

private:
    std::string id_;
    report_reason reason_;
    std::string aimId_;
    std::string chatId_;
};

class report_message : public report_abuse
{
public:
    report_message(wim_packet_params _params, const int64_t _id, const std::string& _text, const report_reason& _reason, const std::string& _aimId, const std::string& _chatId);

protected:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

private:
    int64_t id_;
    std::string text_;
    report_reason reason_;
    std::string aimId_;
    std::string chatId_;
};

report_reason get_reason_by_string(const std::string& _reason);

CORE_WIM_NS_END
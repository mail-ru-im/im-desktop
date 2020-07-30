#pragma once

namespace core
{
    class http_request_simple;
}

namespace core
{
    using feedback_callback_t = std::function<void(bool _error)>;

    class send_feedback : public std::enable_shared_from_this<send_feedback>
    {
    private:
        std::string url_;
        std::map<std::string, std::string> fields_;
        std::vector<std::string> attachments_;

        std::wstring log_;
        std::shared_ptr<core::http_request_simple> request_;

        void init_request();
    public:
        send_feedback(const std::string& _aimid, std::string _url, std::map<std::string, std::string> _fields, std::vector<std::string> _attachments);
        virtual ~send_feedback();

        void send(feedback_callback_t _callback);
    };
}

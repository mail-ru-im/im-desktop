#include "stdafx.h"
#include "curl_handler.h"
#include "curl_context.h"
#include "curl_easy_handler.h"
#include "curl_multi_handler.h"

namespace core
{
    curl_task::curl_task(std::weak_ptr<curl_context> _context, curl_easy::completion_function _completion_func)
        : context_(_context)
        , completion_func_(std::move(_completion_func))
        , async_(true)
    {
    }

    curl_task::curl_task(std::weak_ptr<curl_context> _context, curl_easy::promise_t&& _promise)
        : context_(_context)
        , promise_(std::move(_promise))
        , async_(false)
    {
    }

    void curl_task::execute(CURL* _curl)
    {
        auto result = [this](CURLcode _err, bool _resolve_failed)
        {
            auto res = get_completion_code(_err, _resolve_failed);
            if (async_)
                completion_func_(res);
            else
                promise_.set_value(res);
        };

        auto ctx = context_.lock();
        if (!ctx)
            return result(CURLE_FAILED_INIT, false);

        if (ctx->stop_func_ && ctx->stop_func_())
            return result(CURLE_ABORTED_BY_CALLBACK, false);

        if (!ctx->init_handler(_curl))
        {
            result(CURLE_FAILED_INIT, false);
            curl_easy_reset(_curl);
            return;
        }

        auto err = ctx->execute_handler(_curl);
        result(err, ctx->is_resolve_failed());
        curl_easy_reset(_curl);
    }

    curl_task::init_result curl_task::init_handler()
    {
        auto result = [this](CURLcode _err)
        {
            auto res = get_completion_code(_err, false);
            if (async_)
                completion_func_(res);
            else
                promise_.set_value(res);
        };

        auto ctx = context_.lock();
        if (!ctx)
        {
            result(CURLE_FAILED_INIT);
            return { nullptr, CURLE_FAILED_INIT };
        }

        if (ctx->stop_func_ && ctx->stop_func_())
        {
            result(CURLE_ABORTED_BY_CALLBACK);
            return { nullptr, CURLE_ABORTED_BY_CALLBACK };
        }

        auto curl_handler = curl_easy_init();
        if (!ctx->init_handler(curl_handler))
        {
            result(CURLE_FAILED_INIT);
            curl_easy_cleanup(curl_handler);
            return { nullptr, CURLE_FAILED_INIT };
        }

        return { curl_handler, CURLE_OK };
    }

    CURLcode curl_task::execute_multi(CURLM* _multi, CURL* _curl)
    {
        auto result = [this](CURLcode _err)
        {
            auto res = get_completion_code(_err, false);
            if (async_)
                completion_func_(res);
            else
                promise_.set_value(res);
        };

        auto ctx = context_.lock();
        if (!ctx)
        {
            result(CURLE_FAILED_INIT);
            return CURLE_FAILED_INIT;
        }

        if (ctx->stop_func_ && ctx->stop_func_())
        {
            result(CURLE_ABORTED_BY_CALLBACK);
            return CURLE_ABORTED_BY_CALLBACK;
        }

        if (const auto res = ctx->execute_multi_handler(_multi, _curl); res != CURLM_OK)
        {
            curl_easy_cleanup(_curl);
            result(CURLE_FAILED_INIT);
            return CURLE_FAILED_INIT;
        }

        return CURLE_OK;
    }

    void curl_task::finish_multi(CURLM* _multi, CURL* _curl, CURLcode _res)
    {
        auto result = [this](CURLcode _err, bool _resolve_failed)
        {
            auto res = get_completion_code(_err, _resolve_failed);
            if (async_)
                completion_func_(res);
            else
                promise_.set_value(res);
        };

        auto ctx = context_.lock();
        if (!ctx)
            return result(CURLE_FAILED_INIT, false);

        if (ctx->stop_func_ && ctx->stop_func_())
            _res = CURLE_ABORTED_BY_CALLBACK;

        ctx->load_info(_curl, _res);
        ctx->write_log(_res);

        curl_multi_remove_handle(_multi, _curl);
        curl_easy_cleanup(_curl);
        result(_res, ctx->is_resolve_failed());
    }

    priority_t curl_task::get_priority() const
    {
        if (const auto ctx = context_.lock(); ctx)
            return ctx->get_priority();
        return default_priority();
    }

    int64_t curl_task::get_id() const
    {
        if (const auto ctx = context_.lock(); ctx)
            return ctx->get_id();
        return -1;
    }

    void curl_task::cancel()
    {
        auto res = get_completion_code(CURLE_ABORTED_BY_CALLBACK, false);
        if (async_)
            completion_func_(res);
        else
            promise_.set_value(res);
    }

    curl_easy::completion_code curl_task::get_completion_code(CURLcode _err, bool _resolve_failed)
    {
        switch (_err)
        {
        case CURLE_OK:
            return curl_easy::completion_code::success;
        case CURLE_ABORTED_BY_CALLBACK:
            return curl_easy::completion_code::cancelled;
        case CURLE_COULDNT_RESOLVE_HOST:
            return curl_easy::completion_code::resolve_failed;
        case CURLE_OPERATION_TIMEDOUT:
            return _resolve_failed ? curl_easy::completion_code::resolve_failed : curl_easy::completion_code::failed;
        default:
            return curl_easy::completion_code::failed;
        }
    }

    bool curl_task::is_stopped() const
    {
        if (auto c = context_.lock())
            return c->is_stopped();

        return false;
    }

    std::string curl_task::normalized_url() const
    {
        if (auto c = context_.lock())
            return c->normalized_url();

        return std::string();
    }

    curl_handler::curl_handler()
        : easy_handler_(std::make_unique<curl_easy_handler>())
        , multi_handler_(std::make_unique<curl_multi_handler>())
    {
    }

    curl_handler& curl_handler::instance()
    {
        static curl_handler handler;
        return handler;
    }

    curl_easy::future_t curl_handler::perform(const std::shared_ptr<curl_context>& _context)
    {
        if (_context->is_multi_task())
            return multi_handler_->perform(_context);

        return easy_handler_->perform(_context);
    }

    void curl_handler::perform_async(const std::shared_ptr<curl_context>& _context, curl_easy::completion_function _completion_func)
    {
        if (_context->is_multi_task())
            multi_handler_->perform_async(_context, std::move(_completion_func));
        else
            easy_handler_->perform_async(_context, std::move(_completion_func));
    }

    void curl_handler::resolve_host(std::string_view _host, std::function<void(std::string _result, int _error)> _callback)
    {
        multi_handler_->resolve_host(_host, std::move(_callback));
    }

    void curl_handler::raise_task(int64_t _id)
    {
        easy_handler_->raise_task(_id);
        multi_handler_->raise_task(_id);
    }

    void curl_handler::init()
    {
        if constexpr (platform::is_windows())
            curl_global_init(CURL_GLOBAL_ALL);
        else
            curl_global_init(CURL_GLOBAL_SSL);

        easy_handler_->init();
        multi_handler_->init();
    }

    void curl_handler::cleanup()
    {
        easy_handler_->cleanup();
        multi_handler_->cleanup();

        curl_global_cleanup();
    }

    void curl_handler::reset()
    {
        multi_handler_->reset();
    }

    void curl_handler::reset_sockets(bool _network_is_down)
    {
        multi_handler_->reset_sockets(_network_is_down);
    }

    void curl_handler::process_stopped_tasks()
    {
        multi_handler_->process_stopped_tasks();
    }

    bool curl_handler::is_stopped() const
    {
        return easy_handler_->is_stopped() || multi_handler_->is_stopped();
    }
}

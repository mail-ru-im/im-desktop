#include "stdafx.h"

#include <algorithm>

#include "avatar_loader.h"
#include "../../async_task.h"
#include "../../tools/system.h"
#include "../../configuration/host_config.h"

#include "packets/request_avatar.h"

#include "../common.shared/string_utils.h"

#include <boost/range/adaptor/reversed.hpp>

namespace
{
    bool load_avatar_from_file(std::shared_ptr<core::wim::avatar_context> _context)
    {
        if (!core::tools::system::is_exist(_context->avatar_file_path_))
            return false;

        if (!_context->force_)
            _context->write_time_ = core::tools::system::get_file_lastmodified(_context->avatar_file_path_);

        if (!_context->avatar_data_.load_from_file(_context->avatar_file_path_))
            return false;

        _context->avatar_exist_ = true;

        return true;
    }
}

using namespace core;
using namespace wim;

//////////////////////////////////////////////////////////////////////////
// avatar_task
//////////////////////////////////////////////////////////////////////////
avatar_task::avatar_task(
    int64_t _task_id,
    const std::shared_ptr<avatar_context>& _context,
    const std::shared_ptr<avatar_load_handlers>& _handlers)
    : context_(_context)
    , handlers_(_handlers)
    , task_id_(_task_id)
{
}

std::shared_ptr<avatar_context> avatar_task::get_context() const
{
    return context_;
}

std::shared_ptr<avatar_load_handlers> avatar_task::get_handlers() const
{
    return handlers_;
}

int64_t avatar_task::get_id() const
{
    return task_id_;
}


//////////////////////////////////////////////////////////////////////////
avatar_loader::avatar_loader()
    : task_id_(0)
    , working_(false)
    , network_error_(false)
    , local_thread_(std::make_shared<async_executer>("avl_local"))
    , server_thread_(std::make_shared<async_executer>("avl_server"))
{
}

avatar_loader::~avatar_loader() = default;

std::string_view avatar_loader::get_avatar_type_by_size(int32_t _size) const
{
    constexpr static std::pair<int32_t, std::string_view> sizes[] = { {0, "128"}, {128, "256"}, {256, "1024"} };

    for (auto [size, name] : boost::adaptors::reverse(sizes))
        if (_size > size)
            return name;

    return std::begin(sizes)->second;
}

std::wstring avatar_loader::get_avatar_path(const std::wstring& _avatars_data_path, std::string_view _contact, std::string_view _avatar_type) const
{
    std::wstring path = core::tools::from_utf8(_contact);
    std::replace(path.begin(), path.end(), L'|', L'_');

    if (_avatar_type.empty())
        return _avatars_data_path + path;

    return su::wconcat(_avatars_data_path, path, L'/', core::tools::from_utf8(_avatar_type), L".jpg");
}

void avatar_loader::execute_task(std::shared_ptr<avatar_task> _task, std::function<void(int32_t)> _on_complete)
{
    time_t write_time = _task->get_context()->avatar_exist_ ? _task->get_context()->write_time_ : 0;

    if (!wim_params_)
    {
        im_assert(false);
        _on_complete(wim_protocol_internal_error::wpie_network_error);

        return;
    }

    auto packet = std::make_shared<request_avatar>(
        *wim_params_,
        _task->get_context()->contact_,
        _task->get_context()->avatar_type_,
        write_time);

    server_thread_->run_async_task(packet)->on_result_ = [wr_this = weak_from_this(), _task, packet, _on_complete = std::move(_on_complete)](int32_t _error) mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->local_thread_->run_async_function([avatar_data = packet->get_data(), _task]()->int32_t
            {
                auto size = avatar_data->available();
                im_assert(size);
                if (size == 0)
                    return wpie_error_empty_avatar_data;

                _task->get_context()->avatar_data_.write(avatar_data->read(size), size);
                avatar_data->reset_out();

                avatar_data->save_2_file(_task->get_context()->avatar_file_path_);
                return 0;

            })->on_result_ = [wr_this, _on_complete = std::move(_on_complete), _task](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == 0)
                {
                    if (_task->get_context()->avatar_exist_)
                        _task->get_handlers()->updated_(_task->get_context());
                    else
                        _task->get_handlers()->completed_(_task->get_context());
                }
                else
                {
                    _task->get_handlers()->failed_(_task->get_context(), _error);
                }

                _on_complete(_error);
            };
        }
        else
        {
            if (!_task->get_context()->avatar_exist_)
                _task->get_handlers()->failed_(_task->get_context(), _error);

            _on_complete(_error);

            if (_error == wpie_couldnt_resolve_host)
                config::hosts::switch_to_ip_mode(packet->get_url(), _error);
            else if (_error == wpie_network_error)
                config::hosts::switch_to_dns_mode(packet->get_url(), _error);
        }
    };
}

void avatar_loader::run_tasks_loop()
{
    working_ = true;

    im_assert(!network_error_);

    auto task = get_next_task();
    if (!task)
    {
        working_ =  false;
        return;
    }

    execute_task(task, [wr_this = weak_from_this(), task](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (wim_packet::is_network_error(_error))
        {
            ptr_this->network_error_ = true;
            ptr_this->working_ = false;
            return;
        }

        ptr_this->remove_task(task);
        ptr_this->run_tasks_loop();
    });
}

void avatar_loader::remove_task(std::shared_ptr<avatar_task> _task)
{
    requests_queue_.remove_if([_task](const std::shared_ptr<avatar_task>& _current_task)->bool
    {
        return (_current_task->get_id() == _task->get_id());
    });
}

void avatar_loader::add_task(std::shared_ptr<avatar_task> _task)
{
    requests_queue_.push_front(_task);
}

std::shared_ptr<avatar_task> avatar_loader::get_next_task()
{
    if (requests_queue_.empty())
        return std::shared_ptr<avatar_task>();

    return requests_queue_.front();
}

void avatar_loader::load_avatar_from_server(
    const std::shared_ptr<avatar_context>& _context,
    const std::shared_ptr<avatar_load_handlers>& _handlers)
{
    add_task(std::make_shared<avatar_task>(++task_id_, _context, _handlers));

    if (!working_ && !network_error_)
        run_tasks_loop();

    return;
}

std::shared_ptr<avatar_load_handlers> avatar_loader::get_contact_avatar_async(const wim_packet_params& _params, std::shared_ptr<avatar_context> _context)
{
    if (!wim_params_)
        wim_params_ = std::make_shared<wim_packet_params>(_params);
    else
        *wim_params_ = _params;

    _context->avatar_type_ = get_avatar_type_by_size(_context->avatar_size_);
    _context->avatar_file_path_ = get_avatar_path(_context->avatars_data_path_, _context->contact_, _context->avatar_type_);

    auto handlers = std::make_shared<avatar_load_handlers>();

    local_thread_->run_async_function([_context, handlers]()->int32_t
    {
        return (load_avatar_from_file(_context) ? 0 : -1);

    })->on_result_ = [wr_this = weak_from_this(), handlers, _context](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
            handlers->completed_(_context);

        ptr_this->load_avatar_from_server(_context, handlers);
    };

    return handlers;
}

std::shared_ptr<core::async_task_handlers> avatar_loader::remove_contact_avatars(const std::string& _contact, const std::wstring& _avatars_data_path)
{
    return local_thread_->run_async_function([_contact, _avatars_data_path, wr_this = weak_from_this()]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        const auto avatar_dir_path_ = ptr_this->get_avatar_path(_avatars_data_path, _contact);
        if (tools::system::is_exist(avatar_dir_path_))
            tools::system::clean_directory(avatar_dir_path_);

        return 0;
    });
}

void avatar_loader::resume(const wim_packet_params& _params)
{
    if (!network_error_)
        return;

    network_error_ = false;

    if (!wim_params_)
        wim_params_ = std::make_shared<wim_packet_params>(_params);
    else
        *wim_params_ = _params;

    im_assert(!working_);
    if (!working_)
        run_tasks_loop();
}

void avatar_loader::show_contact_avatar(const std::string& _contact, const int32_t _avatar_size)
{
    for (auto iter = requests_queue_.begin(); iter != requests_queue_.end(); ++iter)
    {
        if (const auto ctx = (*iter)->get_context(); ctx->avatar_size_ == _avatar_size && ctx->contact_ == _contact)
        {
            auto task = *iter;

            requests_queue_.erase(iter);

            requests_queue_.push_back(std::move(task));

            break;
        }
    }
}

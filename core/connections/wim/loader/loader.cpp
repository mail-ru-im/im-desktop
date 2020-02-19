#include "stdafx.h"

#include "../../../async_task.h"
#include "../wim_packet.h"
#include "upload_task.h"
#include "loader_handlers.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "web_file_info.h"
#include "../../../http_request.h"
#include "../../../tools/md5.h"
#include "../../../tools/system.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../utils.h"
#include "../../../log/log.h"
#include "../../../disk_cache/disk_cache.h"
#include "../../../profiling/profiler.h"
#include "../../../../common.shared/loader_errors.h"


#include "loader.h"

using namespace core;
using namespace wim;

namespace
{
    bool is_suspendable_error(const loader_errors _error);

    template<typename T>
    T find_task_by_id(T first, T last, const std::string &_id)
    {
        return std::find_if(
            first,
            last,
            [&_id]
        (const auto &_item)
        {
            assert(_item);
            return (_item->get_id() == _id);
        });
    }
}

loader::loader(std::wstring _cache_dir)
    : file_sharing_threads_(std::make_unique<async_executer>("fs_loader", 1))
    , cache_(disk_cache::disk_cache::make(std::move(_cache_dir)))
{
    initialize_tasks_runners();
}

loader::~loader() = default;

void loader::add_file_sharing_task(const std::shared_ptr<fs_loader_task>& _task)
{
    assert(_task);

    const auto &task_id = _task->get_id();
    assert(!task_id.empty());

    const auto iter_task = find_task_by_id(file_sharing_tasks_.cbegin(), file_sharing_tasks_.cend(), task_id);
    if (iter_task != file_sharing_tasks_.cend())
    {
        assert(!"task with the same id already exist");
        return;
    }

    file_sharing_tasks_.push_back(_task);
}

void loader::remove_file_sharing_task(const std::string &_id)
{
    assert(!_id.empty());

    const auto iter_task = find_task_by_id(file_sharing_tasks_.cbegin(), file_sharing_tasks_.cend(), _id);
    if (iter_task == file_sharing_tasks_.cend())
    {
        return;
    }

    file_sharing_tasks_.erase(iter_task);
}

void loader::on_file_sharing_task_result(const std::shared_ptr<fs_loader_task>& _task, int32_t _error)
{
    _task->on_result(_error);

    remove_file_sharing_task(_task->get_id());
}

void loader::on_file_sharing_task_progress(const std::shared_ptr<fs_loader_task>& _task)
{
    _task->on_progress();
}

bool loader::has_file_sharing_task(const std::string &_id) const
{
    assert(!_id.empty());

    return find_task_by_id(file_sharing_tasks_.cbegin(), file_sharing_tasks_.cend(), _id) != file_sharing_tasks_.cend();
}

void loader::resume_file_sharing_tasks()
{
    for (const auto &task : file_sharing_tasks_)
    {
        if (task->get_last_error() != 0)
        {
            task->set_last_error(0);
            task->resume(*this);
        }
    }
}

void loader::send_task_ranges_async(std::weak_ptr<upload_task> _wr_task)
{
    file_sharing_threads_->run_async_function([_wr_task]
    {
        auto task = _wr_task.lock();
        if (!task)
            return -1;

        return (int32_t)task->send_next_range();

    })->on_result_ = [wr_this = weak_from_this(), _wr_task](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto task = _wr_task.lock();
        if (!task)
            return;

        if (_error != 0)
        {
            task->set_last_error(_error);

            if (_error != (int32_t)loader_errors::network_error)
                ptr_this->on_file_sharing_task_result(task, _error);

            return;
        }

        ptr_this->on_file_sharing_task_progress(task);

        if (!task->is_end())
        {
            ptr_this->send_task_ranges_async(task);
        }
        else
        {
            ptr_this->on_file_sharing_task_result(task, 0);

            const auto current_time = std::chrono::system_clock::now();

            core::stats::event_props_type props;
            props.emplace_back("time", std::to_string((current_time - task->get_start_time())/std::chrono::milliseconds(1)));
            props.emplace_back("size", std::to_string(task->get_file_size()/1024));
            g_core->insert_event(core::stats::stats_event_names::filesharing_sent_success, props);
        }
    };
}

std::shared_ptr<upload_progress_handler> loader::upload_file_sharing(
    const std::string &_guid,
    upload_file_params&& _file_params,
    const wim_packet_params& _params)
{
    assert(!_guid.empty());
    assert(!_file_params.file_name.empty());

    auto task = std::make_shared<upload_task>(_guid, _params, std::move(_file_params));
    task->set_handler(std::make_shared<upload_progress_handler>());
    add_file_sharing_task(task);

    std::weak_ptr<upload_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task]
        {
            auto task = wr_task.lock();
            if (!task)
                return -1;

            return (int32_t)task->open_file();

        }
    )->on_result_ =
        [wr_this = weak_from_this(), wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                ptr_this->on_file_sharing_task_result(task, _error);
                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            ptr_this->file_sharing_threads_->run_async_function(
                [wr_task]
                {
                    auto task = wr_task.lock();
                    if (!task)
                        return (int32_t)loader_errors::undefined;

                    return (int32_t)task->get_gate();
                }
            )->on_result_ =
                [wr_this, wr_task](int32_t _error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    auto task = wr_task.lock();
                    if (!task)
                        return;

                    if (_error != 0)
                    {
                        ptr_this->on_file_sharing_task_result(task, _error);
                        return;
                    }

                    ptr_this->on_file_sharing_task_progress(task);

                    ptr_this->send_task_ranges_async(task);
                };
        };

    return task->get_handler();
}


void cleanup_cache(const std::wstring& _cache_folder)
{
    // copy'n'paste from async_loader.cpp. TODO: deduplicate!
    constexpr int32_t max_files_to_delete = 100;
    constexpr auto delete_files_older = std::chrono::hours(7 * 24);
    constexpr auto cleanup_period = std::chrono::minutes(10);

    static std::chrono::system_clock::time_point last_cleanup_time = std::chrono::system_clock::now();

    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();

    if (current_time - last_cleanup_time < cleanup_period)
        return;

    tools::binary_stream bs;
    bs.write<std::string_view>("Start cleanup files cache\r\n");

    last_cleanup_time = current_time;

    try
    {
        std::vector<boost::filesystem::wpath> files_to_delete;

        boost::filesystem::wpath cache_path(_cache_folder);

        boost::filesystem::directory_iterator end_iter;

        boost::system::error_code error;

        if (boost::filesystem::exists(cache_path, error) && boost::filesystem::is_directory(cache_path, error))
        {
            for (boost::filesystem::directory_iterator dir_iter(cache_path, error); (dir_iter != end_iter) && (files_to_delete.size() < max_files_to_delete) && !error; dir_iter.increment(error))
            {
                if (boost::filesystem::is_regular_file(dir_iter->status()))
                {
                    const auto write_time = std::chrono::system_clock::from_time_t(boost::filesystem::last_write_time(dir_iter->path(), error));
                    if ((current_time - write_time) > delete_files_older)
                        files_to_delete.push_back(*dir_iter);
                }
            }
        }

        for (const auto& _file_path : files_to_delete)
        {
            boost::filesystem::remove(_file_path, error);

            bs.write<std::string_view>("Delete file: ");
            bs.write<std::string_view>(_file_path.string());
            bs.write<std::string_view>("\r\n");

        }
    }
    catch (const std::exception&)
    {

    }

    bs.write<std::string_view>("Finish cleanup\r\n");
    g_core->write_data_to_network_log(std::move(bs));
}

void loader::add_task(loader_task_sptr _task)
{
}

void loader::initialize_tasks_runners()
{
}

void loader::run_next_task(const tasks_runner_slot _slot)
{
}

void loader::resume_task(
    const int64_t _id,
    const wim_packet_params &_wim_params)
{
}

void loader::abort_file_sharing_process(const std::string &_process_id)
{
    assert(!_process_id.empty());

    remove_file_sharing_task(_process_id);
}

namespace
{
    bool is_suspendable_error(const loader_errors _error)
    {
        return (_error == loader_errors::network_error) ||
               (_error == loader_errors::suspend);
    }
}

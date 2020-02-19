#pragma once

#include "../../../namespaces.h"
#include "../../../http_request.h"

CORE_NS_BEGIN

class async_executer;
struct async_task_handlers;
enum class file_sharing_function;

typedef std::unique_ptr<async_executer> async_executer_uptr;

CORE_NS_END

CORE_DISK_CACHE_NS_BEGIN

class disk_cache;

typedef std::shared_ptr<disk_cache> disk_cache_sptr;

CORE_DISK_CACHE_NS_END

CORE_WIM_NS_BEGIN

struct wim_packet_params;
class fs_loader_task;
class upload_task;
class download_task;
struct upload_progress_handler;
struct download_progress_handler;
struct download_image_handler;
struct download_link_metainfo_handler;
struct get_file_direct_uri_handler;
class loader_task;
enum class tasks_runner_slot;

typedef std::shared_ptr<loader_task> loader_task_sptr;

typedef std::weak_ptr<loader_task> loader_task_wptr;

struct upload_file_params
{
    std::wstring file_name;
    std::optional<std::string> locale;
    std::optional<int64_t> duration;
    std::optional<file_sharing_base_content_type> base_content_type;
};

class loader : public std::enable_shared_from_this<loader>
{
    struct tasks_runner;

    typedef std::shared_ptr<fs_loader_task> fs_task_sptr;

    typedef std::list<fs_task_sptr> fs_tasks_queue;

    typedef fs_tasks_queue::const_iterator const_fs_tasks_iter;

    typedef std::unique_ptr<tasks_runner> tasks_runner_uptr;

    fs_tasks_queue file_sharing_tasks_;

    std::unique_ptr<async_executer> file_sharing_threads_;

    disk_cache::disk_cache_sptr cache_;

    std::string priority_contact_;

    void add_file_sharing_task(const std::shared_ptr<fs_loader_task>& _task);

    void remove_file_sharing_task(const std::string &_id);

    void on_file_sharing_task_result(const std::shared_ptr<fs_loader_task>& _task, int32_t _error);

    void on_file_sharing_task_progress(const std::shared_ptr<fs_loader_task>& _task);

    void add_task(loader_task_sptr _task);

    void initialize_tasks_runners();

    void run_next_task(const tasks_runner_slot _slot);

    void resume_task(
        const int64_t _id,
        const wim_packet_params &_wim_params);

public:
    void send_task_ranges_async(std::weak_ptr<upload_task> _wr_task);

    std::shared_ptr<upload_progress_handler> upload_file_sharing(
        const std::string &_guid,
        upload_file_params&& _file_params,
        const wim_packet_params& _params);

    void abort_file_sharing_process(const std::string &_process_id);

    bool has_file_sharing_task(const std::string &_id) const;

    void resume_file_sharing_tasks();

    loader(std::wstring _cache_dir);

    virtual ~loader();

};

CORE_WIM_NS_END
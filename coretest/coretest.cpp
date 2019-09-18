// coretest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"
#include "../corelib/corelib.h"

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <charconv>
#include <conio.h>
#include <boost/algorithm/string.hpp>


void gui_thread_func();

class task
{
public:

    virtual bool execute() = 0;
};


class quit_task final : public task
{
    virtual bool execute() override
    {
        return true;
    }
};




std::queue<std::unique_ptr<task>> tasks;
std::mutex queue_mutex;
std::condition_variable condition;


class gui_connector : public core::iconnector
{
    std::atomic<int> ref_count_;

    // ibase interface
    virtual int addref() override
    {
        return ++ref_count_;
    }

    virtual int release() override
    {
        if (0 == (--ref_count_))
            delete this;

        return ref_count_;
    }

    // iconnector interface
    virtual void link(iconnector*, const common::core_gui_settings&) override
    {

    }
    virtual void unlink() override
    {

    }

    virtual void receive(std::string_view _message, int64_t _seq, core::icollection* _collection) override
    {
        printf("receive message = %s\r\n", std::string(_message).c_str());
    }

public:

    gui_connector() : ref_count_(1) {}
};


void post_task(std::unique_ptr<task>&& _task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        tasks.emplace(std::move(_task));
    }

    condition.notify_one();
}


core::icore_interface* core_face = nullptr;
core::iconnector* core_connector = nullptr;


class holes_task final : public task
{
    std::string archive_;

    bool execute() override
    {
        core::ifptr<core::icore_factory> factory(core_face->get_factory());
        core::coll_helper helper(factory->create_collection(), true);
        helper.set_value_as_string("archive", archive_);
        core_connector->receive("archive/make/holes", 1001, helper.get());

        return false;
    }

public:

    holes_task(std::string_view _archive)
        : archive_(_archive)
    {
    }
};

class invalidate_message_data_task final : public task
{
    std::string archive_;
    std::vector<int64_t> ids_;

    int64_t from_ = -1;
    int64_t before_ = -1;
    int64_t after_ = -1;

    bool execute() override
    {
        core::ifptr<core::icore_factory> factory(core_face->get_factory());
        core::coll_helper helper(factory->create_collection(), true);
        helper.set_value_as_string("aimid", archive_);
        if (!ids_.empty())
        {
            core::ifptr<core::iarray> array(helper->create_array());
            array->reserve(ids_.size());
            for (auto id : std::as_const(ids_))
            {
                core::ifptr<core::ivalue> val(helper->create_value());
                val->set_as_int64(id);
                array->push_back(val.get());
            }
            helper.set_value_as_array("ids", array.get());
        }
        else
        {
            helper.set_value_as_int64("from", from_);
            helper.set_value_as_int64("before", before_);
            helper.set_value_as_int64("after", after_);
        }
        core_connector->receive("archive/invalidate/message_data", 1002, helper.get());

        return false;
    }

public:

    invalidate_message_data_task(std::string_view _archive, std::vector<int64_t>&& _ids)
        : archive_(_archive)
        , ids_(std::move(_ids))
    {
    }

    invalidate_message_data_task(std::string_view _archive, int64_t _from, int64_t _before, int64_t _after)
        : archive_(_archive)
        , from_(_from)
        , before_(_before)
        , after_(_after)
    {
    }
};

class make_gallery_hole_task final : public task
{
    std::string archive_;
    int64_t from_;
    int64_t till_;

    bool execute() override
    {
        core::ifptr<core::icore_factory> factory(core_face->get_factory());
        core::coll_helper helper(factory->create_collection(), true);
        helper.set_value_as_string("aimid", archive_);
        helper.set_value_as_int64("from", from_);
        helper.set_value_as_int64("till", till_);
        core_connector->receive("make_gallery_hole", 1003, helper.get());

        return true;
    }

public:

    make_gallery_hole_task(std::string_view _archive, int64_t _from, int64_t _till)
        : archive_(_archive)
        , from_(_from)
        , till_(_till)
    {

    }
};


void gui_thread_func()
{
    if (!get_core_instance(&core_face))
        return;

    core_connector = core_face->get_core_connector();
    if (!core_connector)
        return;

    gui_connector connector;
    core_connector->link(&connector, common::core_gui_settings());

    bool quit = false;

    while (!quit)
    {
        std::unique_ptr<task> t;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            condition.wait(lock, []()
            {
                return !tasks.empty();
            });

            t = std::move(tasks.front());
        }

        if (t)
            quit = t->execute();

        tasks.pop();
    }

    core_connector->unlink();
    core_connector->release();
    core_face->release();
}


void make_archive_holes(std::string_view _archive_name)
{
    post_task(std::make_unique<holes_task>(_archive_name));
}

template<class ...Types>
void invalidate_msg_data(std::string_view _archive_name, Types&&... args)
{
    post_task(std::make_unique<invalidate_message_data_task>(_archive_name, std::forward<Types>(args)...));
}

void make_gallery_hole(std::string_view _arhive_name, int64_t _from, int64_t _till)
{
    post_task(std::make_unique<make_gallery_hole_task>(_arhive_name, _from, _till));
}

void processCmdLine(int _argc, char *_argv[])
{
    for (auto i = 1; i < _argc; ++i)
    {
        std::string_view command(_argv[i]);

        if (command == std::string_view("--make_archive_holes"))
        {
            if (i + 1 < _argc)
            {
                std::string_view archive_name(_argv[++i]);
                make_archive_holes(archive_name);
            }
        }
        else if (command == std::string_view("--invalidate_msg_data"))
        {
            if (i + 1 < _argc)
            {
                std::string_view archive_name(_argv[++i]);
                if (i + 1 < _argc)
                {
                    std::string_view param_str(_argv[++i]);
                    if (param_str == std::string_view("ids"))
                    {
                        std::vector<std::string> strs;
                        if (i + 1 < _argc)
                        {
                            std::string ids_str(_argv[++i]);
                            boost::algorithm::split(strs, ids_str, boost::is_any_of(","));
                            std::vector<int64_t> ids;
                            ids.reserve(strs.size());
                            for (const auto& s : strs)
                            {
                                int64_t id = 0;
                                std::from_chars(s.data(), s.data() + s.size(), id);
                                if (id > 0)
                                    ids.push_back(id);
                            }
                            invalidate_msg_data(archive_name, std::move(ids));
                        }
                    }
                    else
                    {
                        int64_t from = -1;
                        int64_t before = 0;
                        int64_t after = 0;
                        std::from_chars(param_str.data(), param_str.data() + param_str.size(), from);

                        if (from > 0 && (i + 1 < _argc))
                        {
                            std::string_view before_str(_argv[++i]);
                            std::from_chars(before_str.data(), before_str.data() + before_str.size(), before);
                            if (i + 1 < _argc)
                            {
                                std::string_view after_str(_argv[++i]);
                                std::from_chars(after_str.data(), after_str.data() + after_str.size(), after);
                            }
                            invalidate_msg_data(archive_name, from, before, after);
                        }
                    }
                }
            }
        }
        else if (command == std::string_view("--make_gallery_hole"))
        {
            std::string_view archive;
            int64_t from = -1;
            int64_t till = -1;

            if (i + 1 < _argc)
                archive = _argv[i + 1];

            if (i + 2 < _argc)
            {
                std::string_view param(_argv[i + 2]);
                std::from_chars(param.data(), param.data() + param.size(), from);
            }

            if (i + 3 < _argc)
            {
                std::string_view param(_argv[i + 3]);
                std::from_chars(param.data(), param.data() + param.size(), till);
            }

            if (!archive.empty())
                make_gallery_hole(archive, from, till);
        }
    }
}

int main(int _argc, char *_argv[])
{
    if (_argc < 2 || _argv[1] == std::string_view("--help"))
    {
        printf("\r\nUsage: --make_archive_holes <archive_name>\r\n");
        printf("\r\nUsage: --invalidate_msg_data <archive_name> ids id1,id2,id3\r\n");
        printf("\r\nUsage: --invalidate_msg_data <archive_name> <from> <count_before> <count_after>\r\n");
        printf("\r\nUsage: --make_gallery_hole <archive_name> <from_msg_id> <till_msg_id> (from is newer than till)\r\n");

        return 0;
    }

    std::thread gui_thread(gui_thread_func);

    processCmdLine(_argc, _argv);

    _getch();

    post_task(std::make_unique<quit_task>());

    gui_thread.join();

    return 0;
}

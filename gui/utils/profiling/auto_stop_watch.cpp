#include "stdafx.h"

#include "auto_stop_watch.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

namespace
{
#ifdef _WIN32
    std::atomic<qint64> process_uid_ = std::numeric_limits<int32_t>::max();
#else
    std::atomic<qint64> process_uid_ = { std::numeric_limits<int32_t>::max() };
#endif
}

namespace Profiling
{

    auto_stop_watch::auto_stop_watch(const char *_process_name)
    {
        assert(_process_name);
        assert(::strlen(_process_name));

        id_ = ++process_uid_;
        assert(id_ > 0);

        name_ = _process_name;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("name", name_);
        collection.set_value_as_int64("id", id_);
        collection.set_value_as_int64("ts", QDateTime::currentMSecsSinceEpoch());

        Ui::GetDispatcher()->post_message_to_core("profiler/proc/start", collection.get());
    }

    auto_stop_watch::~auto_stop_watch()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("name", name_);
        collection.set_value_as_int64("id", id_);
        collection.set_value_as_int64("ts", QDateTime::currentMSecsSinceEpoch());

        Ui::GetDispatcher()->post_message_to_core("profiler/proc/stop", collection.get());
    }

}
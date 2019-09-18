#include "stdafx.h"
#include "gui_memory_consumption_reporter.h"
#include "../corelib/collection_helper.h"
#include "core.h"

CORE_NS_BEGIN

gui_memory_consumption_reporter::gui_memory_consumption_reporter()
{

}

gui_memory_consumption_reporter::~gui_memory_consumption_reporter()
{

}

void gui_memory_consumption_reporter::request_report(const memory_stats::request_handle &_req_handle)
{
    coll_helper coll(g_core->create_collection(), true);
    _req_handle.serialize(coll);

    g_core->post_message_to_gui("request_memory_consumption_gui_components", 0, coll.get());
}

CORE_NS_END

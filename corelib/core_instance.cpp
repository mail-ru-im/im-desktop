#include "stdafx.h"

#include "core_instance.h"

#include "../core/core.h"
#include "collection.h"
#include "collection_helper.h"

using namespace core;

core::core_instance::core_instance()
{
    ref_count_ = 1;
    gui_connector_ = nullptr;

    if (g_core)
    {
        assert(!"core must be single");
    }

    g_core = std::make_unique<core::core_dispatcher>();
}

core::core_instance::~core_instance()
{
    if (gui_connector_)
        gui_connector_->release();

    g_core.reset();
}

int32_t core::core_instance::addref()
{
    return ++ref_count_;
}

int32_t core::core_instance::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}

iconnector* core::core_instance::get_core_connector()
{
    addref();
    return this;
}

iconnector* core::core_instance::get_gui_connector()
{
    gui_connector_->addref();
    return gui_connector_;
}

icore_factory* core::core_instance::get_factory()
{
    addref();
    return this;
}

icollection* core::core_instance::create_collection()
{
    return (new core::collection());
}

void core::core_instance::link(iconnector* _connector, const common::core_gui_settings& _settings)
{
    if (gui_connector_)
        gui_connector_->release();

    gui_connector_ = _connector;
    gui_connector_->addref();

    g_core->link_gui(this, _settings);
}

void core::core_instance::unlink()
{
    g_core->unlink_gui();

    if (gui_connector_)
        gui_connector_->release();

    gui_connector_ = nullptr;
}

void core::core_instance::receive(std::string_view _method, int64_t _seq, icollection* _value)
{
    g_core->receive_message_from_gui(_method, _seq, _value);
}


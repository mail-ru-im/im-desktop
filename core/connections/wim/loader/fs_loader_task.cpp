#include "stdafx.h"
#include "fs_loader_task.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;


fs_loader_task::fs_loader_task(std::string _id, const wim_packet_params& _params)
    :   id_(std::move(_id)),
        wim_params_(std::make_unique<wim_packet_params>(_params)),
        error_(0)
{
    assert(!id_.empty());
}


fs_loader_task::~fs_loader_task()
{
}

const wim_packet_params& fs_loader_task::get_wim_params() const
{
    return *wim_params_;
}

const std::string& fs_loader_task::get_id() const
{
    assert(!id_.empty());

    return id_;
}

void fs_loader_task::set_last_error(int32_t _error)
{
    error_ = _error;
}

int32_t fs_loader_task::get_last_error() const
{
    return error_;
}

void fs_loader_task::resume(loader& _loader)
{
    assert(false);
}
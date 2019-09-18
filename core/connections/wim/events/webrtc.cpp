#include "stdafx.h"

#include "webrtc.h"
#include "../wim_im.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

webrtc_event::webrtc_event() = default;


webrtc_event::~webrtc_event() = default;

int32_t webrtc_event::parse(const std::string& raw) {
    _raw = raw;
    return 0;
}

void webrtc_event::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) {
    if (!!_im) {
        _im->on_voip_proto_msg(false, _raw.c_str(), (int32_t)_raw.length(), _on_complete);
    } else {
        assert(false);
    }
}

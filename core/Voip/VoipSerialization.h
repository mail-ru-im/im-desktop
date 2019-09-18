#ifndef __VOIP_SERIALIZATION_H__
#define __VOIP_SERIALIZATION_H__

#include "VoipManagerDefines.h"
#include "../../corelib/collection_helper.h"

inline void operator>>(const voip2::ButtonType& _type, core::coll_helper& _coll)
{
    switch (_type)
    {
    case voip2::ButtonType_Close: _coll.set_value_as_string("button_type", "close"); return;
    case voip2::ButtonType_GoToChat: _coll.set_value_as_string("button_type", "goToChat"); return;
    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::ButtonTap& _button_tap, core::coll_helper& _coll)
{
    _coll.set_value_as_string("account", _button_tap.account.c_str());
    _coll.set_value_as_string("contact", _button_tap.contact.c_str());
    _coll.set_value_as_int64("hwnd", (int64_t)_button_tap.hwnd);
    _coll.set_value_as_int("type", _button_tap.type);
    _button_tap.type >> _coll;
}

inline void operator << (voip_manager::ButtonTap& _button_tap, core::coll_helper& _coll)
{

    _button_tap.account = _coll.get_value_as_string("account");
    _button_tap.contact = _coll.get_value_as_string("contact");
    _button_tap.hwnd = (void*)_coll.get_value_as_int64("hwnd");
    _button_tap.type = (voip2::ButtonType)_coll.get_value_as_int("type");

    assert(!_button_tap.account.empty());
    assert(!_button_tap.contact.empty());

    _coll.set_value_as_string("account", _button_tap.account.c_str());
    _coll.set_value_as_string("contact", _button_tap.contact.c_str());
    _coll.set_value_as_int64("hwnd", (int64_t)_button_tap.hwnd);
    _button_tap.type >> _coll;
}

inline void operator>>(const std::vector<std::string>& _contacts, core::coll_helper& _coll)
{
    for (unsigned ix = 0; ix < _contacts.size(); ix++)
    {
        const std::string& contact = _contacts[ix];
        if (contact.empty())
        {
            assert(false);
            continue;
        }

        std::stringstream ss;
        ss << "contact_" << ix;

        _coll.set_value_as_string(ss.str().c_str(), contact.c_str());
    }
}

inline void operator>>(const voip2::DeviceType& _type, core::coll_helper& _coll)
{
    const char* name = "device_type";
    switch (_type)
    {
    case voip2::VideoCapturing: _coll.set_value_as_string(name, "video_capture"); return;
    case voip2::AudioRecording: _coll.set_value_as_string(name, "audio_capture"); return;
    case voip2::AudioPlayback: _coll.set_value_as_string(name, "audio_playback"); return;

    default: assert(false); break;
    }
}

inline void operator<<(voip2::DeviceType& _type, const core::coll_helper& _coll)
{
    std::string typeStr = _coll.get_value_as_string("device_type");
    if (typeStr == "video_capture")
    {
        _type = voip2::VideoCapturing;
    }
    else if (typeStr == "audio_capture")
    {
        _type = voip2::AudioRecording;
    }
    else if (typeStr == "audio_playback")
    {
        _type = voip2::AudioPlayback;
    }
    else
    {
        assert(false);
    }
}

inline void operator>>(const voip2::LayoutType& _type, core::coll_helper& _coll)
{
    const char* name = "layout_type";
    switch (_type)
    {
    case voip2::LayoutType_One: _coll.set_value_as_string(name, "square_with_detach_preview"); return;
    case voip2::LayoutType_Two: _coll.set_value_as_string(name, "square_with_attach_preview"); return;
    case voip2::LayoutType_Three: _coll.set_value_as_string(name, "primary_with_detach_preview"); return;
    case voip2::LayoutType_Four: _coll.set_value_as_string(name, "primary_with_attach_preview"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::MouseTap& _type, core::coll_helper& _coll)
{
    const char* name = "mouse_tap_type";
    switch (_type)
    {
    case voip2::MouseTap_Single: _coll.set_value_as_string(name, "single"); return;
    case voip2::MouseTap_Double: _coll.set_value_as_string(name, "double"); return;
    case voip2::MouseTap_Long: _coll.set_value_as_string(name, "long"); return;
    case voip2::MouseTap_Over: _coll.set_value_as_string(name, "over"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::ViewArea& _type, core::coll_helper& _coll)
{
    const char* name = "view_area_type";
    switch (_type)
    {
    case voip2::ViewArea_Primary:    _coll.set_value_as_string(name, "primary"); return;
    case voip2::ViewArea_Detached:   _coll.set_value_as_string(name, "detached"); return;
    case voip2::ViewArea_Default:    _coll.set_value_as_string(name, "default"); return;
    case voip2::ViewArea_Background: _coll.set_value_as_string(name, "background"); return;
    case voip2::ViewArea_Tray:       _coll.set_value_as_string(name, "tray"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::DeviceState& _state, core::coll_helper& _coll)
{
    _state.type >> _coll;
    _coll.set_value_as_bool("success", _state.success);
    if (!_state.uid.empty())
    {
        _coll.set_value_as_string("uid", _state.uid.c_str());
    }
    else
    {
        assert(false);
    }
}

inline void operator>>(const intptr_t& _ptr, core::coll_helper& _coll)
{
    _coll.set_value_as_int64("pointer", (int64_t)_ptr);
}

inline void operator<<(intptr_t& _ptr, core::coll_helper& _coll)
{
    _ptr = _coll.get_value_as_int64("pointer");
}

inline void operator>>(const voip_manager::DeviceVol& _vol, core::coll_helper& _coll)
{
    _vol.type >> _coll;
    _coll.set_value_as_int("volume_percent", int(_vol.volume * 100.0f));
}

inline void operator>>(const voip_manager::DeviceMute& _mute, core::coll_helper& _coll)
{
    _mute.type >> _coll;
    _coll.set_value_as_bool("muted", _mute.mute);
}

inline void operator>>(const bool& _param, core::coll_helper& _coll)
{
    _coll.set_value_as_bool("param", _param);
}

inline void operator>>(const voip_manager::DeviceInterrupt& _inter, core::coll_helper& _coll)
{
    _inter.type >> _coll;
    _coll.set_value_as_bool("interrupt",_inter.interrupt);
}

inline void operator>>(const voip_manager::LayoutChanged& _type, core::coll_helper& _coll)
{
    _type.layout_type >> _coll;
    _coll.set_value_as_bool("tray", _type.tray);
    _coll.set_value_as_int64("hwnd", (int64_t)_type.hwnd);
}

inline void operator>>(const voip_manager::device_description& _device, core::coll_helper& _coll)
{
    _coll.set_value_as_string("name", _device.name);
    _coll.set_value_as_string("uid", _device.uid);
    _coll.set_value_as_bool("is_active", _device.isActive);
    _coll.set_value_as_int("video_capture_type", _device.videoCaptureType);
    _device.type >> _coll;
}

inline void operator<<(voip_manager::device_description& _device, const core::coll_helper& _coll)
{
    _device.name = _coll.get_value_as_string("name");
    _device.uid  = _coll.get_value_as_string("uid");
    _device.isActive = _coll.get_value_as_bool("is_active");
    _device.videoCaptureType = (voip2::VideoCaptureType)_coll.get_value_as_int("video_capture_type");
    _device.type << _coll;
}

inline void operator>>(const voip_manager::device_list& _deviceList, core::coll_helper& _coll)
{
    _coll.set_value_as_int("count", (int)_deviceList.devices.size());
    _coll.set_value_as_int("type",  (int)_deviceList.type);

    for (unsigned ix = 0; ix < _deviceList.devices.size(); ix++)
    {
        const voip_manager::device_description& desc = _deviceList.devices[ix];
        core::coll_helper device_coll(_coll->create_collection(), true);
        desc >> device_coll;

        std::stringstream sstream;
        sstream << "device_" << ix;

        _coll.set_value_as_collection(sstream.str(), device_coll.get());
    }
}

inline void operator>>(const voip_manager::FrameSize& _fs, core::coll_helper& _coll)
{
    _coll.set_value_as_int64("wnd", _fs.hwnd);
    _coll.set_value_as_double("aspect_ratio", _fs.aspect_ratio);
}

inline void operator>>(const voip_manager::MouseTap& _tap, core::coll_helper& _coll)
{
    _coll.set_value_as_string("account", _tap.account.c_str());
    _coll.set_value_as_string("contact", _tap.contact.c_str());

    int64_t hwnd = (int64_t)_tap.hwnd;
    _coll.set_value_as_int64("hwnd", hwnd);

    _tap.tap >> _coll;
    _tap.area >> _coll;
}

inline void operator>>(const voip_manager::MissedCall& _missed_call, core::coll_helper& _coll)
{
    _coll.set_value_as_string("account", _missed_call.account.c_str());
    _coll.set_value_as_string("contact", _missed_call.contact.c_str());
    _coll.set_value_as_int("ts", _missed_call.ts);
}

inline void operator>>(const std::string& _param, core::coll_helper& _coll)
{
    _coll.set_value_as_string("param", _param.c_str());
}

inline void operator>>(const voip_manager::Contact& _contact, core::coll_helper& _coll)
{
    _coll.set_value_as_string("account", _contact.account.c_str());
    _coll.set_value_as_string("contact", _contact.contact.c_str());
}

inline void operator<<(voip_manager::Contact& _contact, core::coll_helper& _coll)
{
    _contact.account = _coll.get_value_as_string("account");
    _contact.contact = _coll.get_value_as_string("contact");

    assert(!_contact.account.empty());
    assert(!_contact.contact.empty());
}

inline void operator>>(const voip_manager::ContactEx& _contact_ex, core::coll_helper& _coll)
{
    _contact_ex.contact >> _coll;
    _coll.set_value_as_int("call_count", _contact_ex.call_count);
    _coll.set_value_as_int("connection_count", _contact_ex.connection_count);
    _coll.set_value_as_bool("incoming", _contact_ex.incoming);
    _coll.set_value_as_int("window_number", _contact_ex.windows.size());
    for (size_t i = 0, size = _contact_ex.windows.size(); i < size; ++i)
    {
        std::string paramName = "window_" + std::to_string(i);
        _coll.set_value_as_int64(paramName.c_str(), (int64_t)_contact_ex.windows[i]);
    }
}

inline void operator<<(voip_manager::ContactEx& _contact_ex, core::coll_helper& _coll)
{
    _contact_ex.contact << _coll;
    _contact_ex.call_count       = _coll.get_value_as_int("call_count");
    _contact_ex.connection_count = _coll.get_value_as_int("connection_count");
    _contact_ex.incoming = _coll.get_value_as_bool("incoming");
    auto windowNumber = _coll.get_value_as_int("window_number");
    for (int i = 0; i < windowNumber; i++)
    {
        std::string paramName = "window_" + std::to_string(i);
        _contact_ex.windows.push_back((void*)_coll.get_value_as_int64(paramName.c_str()));
    }
}

inline void operator<<(voip_manager::FrameSize& _fs, core::coll_helper& _coll)
{
    _fs.aspect_ratio = _coll.get_value_as_double("aspect_ratio");
    _fs.hwnd = _coll.get_value_as_int64("wnd");
}

inline void operator>>(const voip_manager::CipherState& _val, core::coll_helper& _coll)
{
    _coll.set_value_as_int("state", int(_val.state));
    _coll.set_value_as_string("secure_code", _val.secureCode);
}

inline void operator<<(voip_manager::CipherState& val, core::coll_helper& _coll)
{
    val.state = (voip_manager::CipherState::State)_coll.get_value_as_int("state");
    val.secureCode = _coll.get_value_as_string("secure_code");
}

inline void operator>>(void* _hwnd, core::coll_helper& _coll)
{
    _coll.set_value_as_int64("hwnd", (int64_t)_hwnd);
}

inline void operator << (void*& _hwnd, core::coll_helper& _coll)
{
    _hwnd = (void *)_coll.get_value_as_int64("hwnd");
}


template <typename T> void appendVector (const std::vector<T>& _vector, core::coll_helper& _coll, const std::string& _prefix)
{
    _coll.set_value_as_int(_prefix + "Count", (int)_vector.size());
    for (unsigned ix = 0; ix < _vector.size(); ix++)
    {
        const T& cont = _vector[ix];

        core::coll_helper contact_coll(_coll->create_collection(), true);
        (T)cont >> contact_coll;

        std::stringstream sstream;
        sstream << _prefix << ix;

        _coll.set_value_as_collection(sstream.str(), contact_coll.get());
    }
}

template <typename T> void readVector (std::vector<T>& _vector, core::coll_helper& _coll, const std::string& _prefix)
{
    const auto count = _coll.get_value_as_int((_prefix + "Count").c_str());
    if (!count)
    {
        return;
    }
    _vector.clear();
    _vector.reserve(count);
    for (int ix = 0; ix < count; ix++)
    {
        std::stringstream ss;
        ss << _prefix << ix;

        auto contact = _coll.get_value_as_collection(ss.str().c_str());
        assert(contact);
        if (!contact)
        {
            continue;
        }

        core::coll_helper contact_helper(contact, false);
        T cont_desc;
        cont_desc << contact_helper;

        _vector.push_back(cont_desc);
    }
}


inline void operator>>(const voip_manager::ContactsList& _contactsList, core::coll_helper& _coll)
{

    auto contacts = _contactsList.contacts;
    appendVector(contacts, _coll, "contacts");


    auto windows = _contactsList.windows;
    appendVector(windows, _coll, "windows");

    _coll.set_value_as_bool("isActive", _contactsList.isActive);
}

inline void operator<<(voip_manager::ContactsList& _contactsList, core::coll_helper& _coll)
{

    readVector(_contactsList.contacts, _coll, "contacts");
    readVector(_contactsList.windows, _coll, "windows");
    _contactsList.isActive = _coll.get_value_as_bool("isActive");
}

inline void operator>>(const voip_manager::EnableParams& _mdw, core::coll_helper& _coll)
{
    _coll.set_value_as_bool("enable", _mdw.enable);
}

inline void operator >> (const voip_manager::NamedResult& _value, core::coll_helper& _coll)
{
    _coll.set_value_as_bool("result", _value.result);
    _coll.set_value_as_string("name", _value.name);
}

inline void operator >> (const voip_manager::VideoEnable& _value, core::coll_helper& _coll)
{
    _coll.set_value_as_bool("enable", _value.enable);
    _value.contact >> _coll;
}

inline void operator << (voip_manager::VideoEnable& _value, core::coll_helper& _coll)
{
    _value.enable = _coll.get_value_as_bool("enable");
    _value.contact << _coll;
}

inline void operator >> (const voip_manager::MainVideoLayout& _value, core::coll_helper& _coll)
{
    _coll.set_value_as_int64("hwnd", (int64_t)_value.hwnd);
    _coll.set_value_as_int("type", (int)_value.type);
    _coll.set_value_as_int("layout", (int)_value.layout);
}

inline void operator << (voip_manager::MainVideoLayout& _value, core::coll_helper& _coll)
{
    _value.hwnd = (void*)_coll.get_value_as_int64("hwnd");
    _value.type = (voip_manager::MainVideoLayoutType)_coll.get_value_as_int("type");
    _value.layout = (voip_manager::VideoLayout)_coll.get_value_as_int("layout");
}

inline void operator>>(const voip_manager::eNotificationTypes& _type, core::coll_helper& _coll)
{
    const char* name = "sig_type";

    using namespace voip_manager;
    switch (_type)
    {
    case kNotificationType_Undefined:   _coll.set_value_as_string(name, "undefined");    return;
    case kNotificationType_CallCreated: _coll.set_value_as_string(name, "call_created"); return;
    case kNotificationType_CallInvite:  _coll.set_value_as_string(name, "call_invite");  return;
    case kNotificationType_CallOutAccepted: _coll.set_value_as_string(name, "call_out_accepted"); return;
    case kNotificationType_CallInAccepted: _coll.set_value_as_string(name, "call_in_accepted"); return;
    case kNotificationType_CallConnected: _coll.set_value_as_string(name, "call_connected"); return;
    case kNotificationType_CallDisconnected: _coll.set_value_as_string(name, "call_disconnected"); return;
    case kNotificationType_CallDestroyed: _coll.set_value_as_string(name, "call_destroyed"); return;
    case kNotificationType_CallPeerListChanged: _coll.set_value_as_string(name, "call_peer_list_changed"); return;

    case kNotificationType_QualityChanged: _coll.set_value_as_string(name, "quality_changed"); return;

    case kNotificationType_MediaLocAudioChanged: _coll.set_value_as_string(name, "media_loc_a_changed"); return;
    case kNotificationType_MediaLocVideoChanged: _coll.set_value_as_string(name, "media_loc_v_changed"); return;
    case kNotificationType_MediaRemVideoChanged: _coll.set_value_as_string(name, "media_rem_v_changed"); return;
    case kNotificationType_MediaRemAudioChanged: _coll.set_value_as_string(name, "media_rem_a_changed"); return;
    case kNotificationType_MediaLocVideoDeviceChanged: _coll.set_value_as_string(name, "media_rem_v_device_changed"); return;

    case kNotificationType_DeviceListChanged: _coll.set_value_as_string(name, "device_list_changed"); return;
    case kNotificationType_DeviceStarted: _coll.set_value_as_string(name, "device_started"); return;
    case kNotificationType_DeviceMuted: _coll.set_value_as_string(name, "device_muted"); return;
    case kNotificationType_DeviceVolChanged: _coll.set_value_as_string(name, "device_vol_changed"); return;
    case kNotificationType_DeviceInterrupt: _coll.set_value_as_string(name, "device_interrupt"); return;

    case kNotificationType_MouseTap: _coll.set_value_as_string(name, "mouse_tap"); return;
    case kNotificationType_ButtonTap: _coll.set_value_as_string(name, "button_tap"); return;
    case kNotificationType_LayoutChanged: _coll.set_value_as_string(name, "layout_changed"); return;

    case kNotificationType_MissedCall: _coll.set_value_as_string(name, "missed_call"); return;
    case kNotificationType_ShowVideoWindow: _coll.set_value_as_string(name, "video_window_show"); return;
    case kNotificationType_FrameSizeChanged: _coll.set_value_as_string(name, "frame_size_changed"); return;
    case kNotificationType_VoipResetComplete: _coll.set_value_as_string(name, "voip_reset_complete"); return;
    case kNotificationType_VoipWindowRemoveComplete: _coll.set_value_as_string(name, "voip_window_remove_complete"); return;
    case kNotificationType_VoipWindowAddComplete: _coll.set_value_as_string(name, "voip_window_add_complete"); return;

    case kNotificationType_CipherStateChanged:  _coll.set_value_as_string(name, "voip_cipher_state_changed"); return;

    case kNotificationType_MinimalBandwidthChanged: _coll.set_value_as_string(name, "voip_minimal_bandwidth_state_changed"); return;
    case kNotificationType_MaskEngineEnable: _coll.set_value_as_string(name, "voip_mask_engine_enable"); return;
    case kNotificationType_LoadMask: _coll.set_value_as_string(name, "voip_load_mask"); return;

    case kNotificationType_ConnectionDestroyed: /* Nothing to do for now */ return;

    case kNotificationType_MainVideoLayoutChanged: _coll.set_value_as_string(name, "voip_main_video_layout_changed"); return;

    default: assert(false); return;
    }
}

#endif//__VOIP_SERIALIZATION_H__
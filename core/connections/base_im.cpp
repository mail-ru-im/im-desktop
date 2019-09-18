#include "stdafx.h"

#include "../utils.h"

#include "../Voip/VoipManager.h"
#include "stats/memory/memory_stats_collector.h"
#include "stats/memory/memory_stats_request.h"
#include "stats/memory/memory_stats_response.h"

#include "../tools/md5.h"
#include "../tools/system.h"

#include "../archive/contact_archive.h"
#include "../../corelib/enumerations.h"

#include "im_login.h"

#include "../masks/masks.h"

// stickers
#include "../stickers/stickers.h"

#include "../themes/wallpaper_loader.h"

#include "base_im.h"
#include "../async_task.h"

using namespace core;

namespace fs = boost::filesystem;

namespace
{
    void __on_voip_user_bitmap(std::shared_ptr<voip_manager::VoipManager> vm, const std::string& contact, voip2::AvatarType type, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
    {
#ifndef STRIP_VOIP
        voip_manager::UserBitmap bmp;
        bmp.bitmap.data = (void*)data;
        bmp.bitmap.size = size;
        bmp.bitmap.w    = w;
        bmp.bitmap.h    = h;
        bmp.contact     = contact;
        bmp.type        = type;
        bmp.theme       = theme;

        vm->get_window_manager()->window_set_bitmap(bmp);
#endif
    }
}

base_im::base_im(const im_login_id& _login,
                 std::shared_ptr<voip_manager::VoipManager> _voip_manager,
                 std::shared_ptr<memory_stats::memory_stats_collector> _memory_stats_collector)
    :   voip_manager_(_voip_manager),
        id_(_login.get_id()),
        memory_stats_collector_(_memory_stats_collector)
{
}


base_im::~base_im()
{
}

void base_im::set_id(int32_t _id)
{
    id_ = _id;
}

int32_t base_im::get_id() const
{
    return id_;
}

std::shared_ptr<stickers::face> base_im::get_stickers()
{
    if (!stickers_)
        stickers_ = std::make_shared<stickers::face>(get_stickers_path());

    return stickers_;
}

std::shared_ptr<themes::wallpaper_loader> base_im::get_wallpaper_loader()
{
    if (!wp_loader_)
    {
        wp_loader_ = std::make_shared<themes::wallpaper_loader>();
        wp_loader_->clear_missing_wallpapers();
    }

    return wp_loader_;
}

std::vector<std::string> base_im::get_audio_devices_names()
{
    std::vector<std::string> result;

    if (voip_manager_)
    {
        std::vector<voip_manager::device_description> device_list;
        voip_manager_->get_device_manager()->get_device_list(voip2::DeviceType::AudioRecording, device_list);

        result.reserve(device_list.size());
        for (auto & device : device_list)
            result.push_back(device.name);
    }

    return result;
}

std::vector<std::string> base_im::get_video_devices_names()
{
    std::vector<std::string> result;

    if (voip_manager_)
    {
        std::vector<voip_manager::device_description> device_list;
        voip_manager_->get_device_manager()->get_device_list(voip2::DeviceType::VideoCapturing, device_list);

        result.reserve(device_list.size());
        for (auto & device : device_list)
        {
            if (device.videoCaptureType == voip2::VC_DeviceCamera)
                result.push_back(device.name);
        }
    }

    return result;
}

void base_im::connect()
{

}

std::wstring core::base_im::get_contactlist_file_name() const
{
    return (get_im_data_path() + L"/contacts/cache.cl");
}

std::wstring core::base_im::get_my_info_file_name() const
{
    return (get_im_data_path() + L"/info/cache");
}

std::wstring base_im::get_active_dilaogs_file_name() const
{
    return (get_im_data_path() + L"/dialogs/" + archive::cache_filename());
}

std::wstring base_im::get_favorites_file_name() const
{
    return (get_im_data_path() + L"/favorites/" + archive::cache_filename());
}

std::wstring base_im::get_mailboxes_file_name() const
{
    return (get_im_data_path() + L"/mailboxes/" + archive::cache_filename());
}

std::wstring core::base_im::get_im_data_path() const
{
    return (utils::get_product_data_path() + L'/' + get_im_path());
}

std::wstring core::base_im::get_avatars_data_path() const
{
    return get_im_data_path() + L"/avatars/";
}

std::wstring base_im::get_file_name_by_url(const std::string& _url) const
{
    return (get_content_cache_path() + L'/' + core::tools::from_utf8(core::tools::md5(_url.c_str(), (int32_t)_url.length())));
}

void base_im::create_masks(std::weak_ptr<wim::im> _im)
{
    const auto& path = get_masks_path();
    const auto version = voip_manager_ ? voip_manager_->get_mask_manager()->version() : 0;
    masks_ = std::make_shared<masks>(_im, path, version);
}

std::wstring base_im::get_masks_path() const
{
    return (get_im_data_path() + L"/masks");
}

std::wstring base_im::get_stickers_path() const
{
    return (get_im_data_path() + L"/stickers");
}

std::wstring base_im::get_im_downloads_path(const std::string &alt) const
{
#ifdef __linux__
    typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter_t;
#else
    typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter_t;
#endif

    auto user_downloads_path = core::tools::system::get_user_downloads_dir();

    converter_t converter;
    const auto walt = converter.from_bytes(alt.c_str());
    if (!walt.empty())
    {
        user_downloads_path = walt;
    }

    if (!core::tools::system::is_dir_writable(user_downloads_path))
        user_downloads_path = core::tools::system::get_user_downloads_dir();

    if constexpr (platform::is_apple())
        return user_downloads_path;

    return (user_downloads_path + L"/" + core::tools::from_utf8(build::product_name_full()));
}

std::wstring base_im::get_content_cache_path() const
{
    return (get_im_data_path() + L"/content.cache");
}

std::wstring core::base_im::get_search_history_path() const
{
    return (get_im_data_path() + L"/search");
}

// voip
void core::base_im::on_voip_call_set_proxy(const voip_manager::VoipProxySettings& proxySettings)
{
#ifndef STRIP_VOIP
    auto call_manager = voip_manager_->get_call_manager();
    assert(!!call_manager);

    if (!!call_manager)
    {
        call_manager->call_set_proxy(proxySettings);
    }
#endif
}

void core::base_im::on_voip_call_start(std::string contact, bool video, bool attach)
{
#ifndef STRIP_VOIP
    assert(!contact.empty());
    if (contact.empty())
    {
        return;
    }

    auto call_manager = voip_manager_->get_call_manager();
    assert(!!call_manager);

    auto account = _get_protocol_uid();
    assert(!account.empty());

    if (!!call_manager && !account.empty() && !contact.empty())
    {
        call_manager->call_create(voip_manager::Contact(account, contact), video, attach);
    }
#endif
}

void core::base_im::on_voip_call_request_calls()
{
#ifndef STRIP_VOIP
    voip_manager_->get_call_manager()->call_request_calls();
#endif
}

void core::base_im::on_voip_window_set_offsets(void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_set_offsets(hwnd, l, t, r, b);
#endif
}

bool core::base_im::on_voip_avatar_actual_for_voip(const std::string& contact, unsigned avatar_size)
{
#ifndef STRIP_VOIP
    auto account = _get_protocol_uid();
    assert(!account.empty());
    return
        !account.empty() &&
        !contact.empty() &&
        voip_manager_->get_call_manager()->call_have_call(voip_manager::Contact(account, contact));
#else
    return false;
#endif
}

void core::base_im::on_voip_user_update_avatar_no_video(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserNoVideo, data, size, h, w, theme);
}

//void core::base_im::on_voip_user_update_avatar_camera_off(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
//{
//    __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_Camera, data, size, h, w, theme);
//}
//
//void core::base_im::on_voip_user_update_avatar_no_camera(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
//{
//    __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_CameraCrossed, data, size, h, w, theme);
//}

void core::base_im::on_voip_user_update_avatar_text(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserText, data, size, h, w, theme);
}

void core::base_im::on_voip_user_update_avatar_text_header(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_Header, data, size, h, w, theme);
}

void core::base_im::on_voip_user_update_avatar(const std::string& contact, const unsigned char* data, unsigned size, unsigned avatar_h, unsigned avatar_w, voip_manager::AvatarThemeType theme)
{
    return __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserMain, data, size, avatar_h, avatar_w, theme);
}

void core::base_im::on_voip_user_update_avatar_background(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    return __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_Background, data, size, h, w, theme);
}

void core::base_im::on_voip_call_end(std::string contact, bool busy)
{
#ifndef STRIP_VOIP
    auto account = _get_protocol_uid();
    assert(!account.empty());
    if (!account.empty() && !contact.empty())
    {
        voip_manager_->get_call_manager()->call_decline(voip_manager::Contact(account, contact), false);
    }
#endif
}

void core::base_im::on_voip_device_changed(const std::string& dev_type, const std::string& uid)
{
#ifndef STRIP_VOIP
    if ("audio_playback" == dev_type)
    {
        voip_manager_->get_device_manager()->set_device(voip2::AudioPlayback, uid);
    }
    else if ("audio_capture" == dev_type)
    {
        voip_manager_->get_device_manager()->set_device(voip2::AudioRecording, uid);
    }
    else if ("video_capture" == dev_type)
    {
        voip_manager_->get_device_manager()->set_device(voip2::VideoCapturing, uid);
    }
    else
    {
        assert(false);
    }
#endif
}

void core::base_im::on_voip_window_update_background(void* hwnd, const unsigned char* data, unsigned size, unsigned w, unsigned h)
{
#ifndef STRIP_VOIP
    voip_manager::WindowBitmap bmp;
    bmp.hwnd = hwnd;
    bmp.bitmap.data = (void*)data;
    bmp.bitmap.size = size;
    bmp.bitmap.w = w;
    bmp.bitmap.h = h;

    voip_manager_->get_window_manager()->window_set_bitmap(bmp);
#endif
}

void core::base_im::on_voip_call_accept(std::string contact, bool video)
{
#ifndef STRIP_VOIP
    auto account = _get_protocol_uid();
    assert(!account.empty());

    if (!account.empty() && !contact.empty())
    {
        voip_manager_->get_call_manager()->call_accept(voip_manager::Contact(account, contact), video);
    }
#endif
}

void core::base_im::on_voip_add_window(voip_manager::WindowParams& windowParams)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_add(windowParams);
#endif
}

void core::base_im::on_voip_remove_window(void* hwnd)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_remove(hwnd);
#endif
}

void core::base_im::on_voip_call_stop()
{
#ifndef STRIP_VOIP
    voip_manager_->get_call_manager()->call_stop();
#endif
}

void core::base_im::on_voip_switch_media(bool video)
{
#ifndef STRIP_VOIP
    if (video)
    {
        const bool enabled = voip_manager_->get_media_manager()->local_video_enabled();
        voip_manager_->get_media_manager()->media_video_en(!enabled);
    }
    else
    {
        const bool enabled = voip_manager_->get_media_manager()->local_audio_enabled();
        voip_manager_->get_media_manager()->media_audio_en(!enabled);
    }
#endif
}

void core::base_im::on_voip_mute_incoming_call_sounds(bool mute)
{
#if defined(_WIN32) && !defined(STRIP_VOIP)
    voip_manager_->get_call_manager()->mute_incoming_call_sounds(mute);
#endif
}

void core::base_im::on_voip_volume_change(int vol)
{
#ifndef STRIP_VOIP
    const float vol_fp = std::max(std::min(vol, 100), 0) / 100.0f;
    voip_manager_->get_device_manager()->set_device_volume(voip2::AudioPlayback, vol_fp);
#endif
}

void core::base_im::on_voip_mute_switch()
{
#ifndef STRIP_VOIP
    const bool mute = voip_manager_->get_device_manager()->get_device_mute(voip2::AudioPlayback);
    voip_manager_->get_device_manager()->set_device_mute(voip2::AudioPlayback, !mute);
#endif
}

void core::base_im::on_voip_set_mute(bool mute)
{
#ifndef STRIP_VOIP
    voip_manager_->get_device_manager()->set_device_mute(voip2::AudioPlayback, mute);
#endif
}

void core::base_im::on_voip_reset()
{
#ifndef STRIP_VOIP
    voip_manager_->get_voip_manager()->reset();
#endif
}

void core::base_im::on_voip_proto_ack(const voip_manager::VoipProtoMsg& msg, bool success)
{
#ifndef STRIP_VOIP
    auto account = _get_protocol_uid();
    assert(!account.empty());

    if (!account.empty())
        voip_manager_->get_connection_manager()->ProcessVoipAck(account.c_str(), msg, success);
#endif
}

void core::base_im::on_voip_proto_msg(bool allocate, const char* data, unsigned len, std::shared_ptr<auto_callback> _on_complete) {
#ifndef STRIP_VOIP
    auto account = _get_protocol_uid();
    assert(!account.empty());
    if (!account.empty())
    {
        const auto msg_type = allocate ? voip2::WIM_Incoming_allocated : voip2::WIM_Incoming_fetch_url;
        voip_manager_->get_connection_manager()->ProcessVoipMsg(account.c_str(), msg_type, data, len);
    }
    _on_complete->callback(0);
#endif
}

void core::base_im::on_voip_update()
{
#ifndef STRIP_VOIP
    voip_manager_->get_device_manager()->update();
#endif
}

void core::base_im::on_voip_minimal_bandwidth_switch()
{
#ifndef STRIP_VOIP
    voip_manager_->get_call_manager()->minimal_bandwidth_switch();
#endif
}

void core::base_im::on_voip_load_mask(const std::string& path)
{
#ifndef STRIP_VOIP
    voip_manager_->get_mask_manager()->load_mask(path);
#endif
}

void core::base_im::voip_set_model_path(const std::string& _local_path)
{
#ifndef STRIP_VOIP
    voip_manager_->get_mask_manager()->set_model_path(_local_path);
#endif
}

bool core::base_im::has_created_call()
{
#ifndef STRIP_VOIP
    return voip_manager_->get_call_manager()->has_created_call();
#else
    return false;
#endif
}

void base_im::send_voip_calls_quality_report(int _score, const std::string &_survey_id,
                                             const std::vector<std::string> &_reasons,  const std::string& _aimid)
{
    voip_manager_->get_call_manager()->call_report_user_rating(voip_manager::UserRatingReport(_score, _survey_id, _reasons, _aimid));
}

void core::base_im::on_voip_window_set_primary(void* hwnd, const std::string& contact)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_set_primary(hwnd, contact);
#endif
}

void core::base_im::on_voip_window_set_hover(void* hwnd, bool hover)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_set_hover(hwnd, hover);
#endif
}

void core::base_im::on_voip_window_set_large(void* hwnd, bool large)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_set_large(hwnd, large);
#endif
}

void core::base_im::on_voip_init_mask_engine()
{
#ifndef STRIP_VOIP
    voip_manager_->get_mask_manager()->init_mask_engine();
#endif
}

void core::base_im::on_voip_window_set_conference_layout(void* hwnd, int layout)
{
#ifndef STRIP_VOIP
    voip_manager_->get_window_manager()->window_set_conference_layout(hwnd, (voip_manager::VideoLayout)layout);
#endif
}

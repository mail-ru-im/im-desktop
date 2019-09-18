#include "stdafx.h"
#include "im_container.h"
#include "login_info.h"
#include "im_login.h"
#include "wim/chat_params.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "../archive/gallery_cache.h"
#include "../stats/memory/memory_stats_collector.h"
#include "wim/wim_im.h"
#include "wim/wim_packet.h"
#include "wim/auth_parameters.h"
#include "../Voip/VoipManager.h"

#include "../core.h"
#include "../tools/system.h"
#include "../utils.h"
#include "../archive/contact_archive.h"
#include "../archive/history_message.h"
#include "../statistics.h"
#include "../proxy_settings.h"

#include "../configuration/app_config.h"
#include "accept_agreement_info.h"

namespace
{
constexpr int VOIP_RATING_STARS_SCORE_COEFFICIENT = 10;
}

using namespace core;

voip_manager::BitmapDescription get_bitmap_from_stream(core::istream* stream, int w, int h)
{
    voip_manager::BitmapDescription res;
    res.size = stream->size();
    res.data = (void*)stream->read(stream->size());
    res.w = w;
    res.h = h;

    return res;
}

im_container::im_container(const std::shared_ptr<voip_manager::VoipManager>& voip_manager,
                           const std::shared_ptr<memory_stats::memory_stats_collector>& memory_stats_collector)
    : logins_(std::make_unique<im_login_list>(utils::get_product_data_path() + L"/settings/ims.stg"))
    , voip_manager_(voip_manager),
      memory_stats_collector_(memory_stats_collector)
{
    REGISTER_IM_MESSAGE("login_by_password", on_login_by_password);
    REGISTER_IM_MESSAGE("login_by_password_for_attach_uin", on_login_by_password_for_attach_uin);
    REGISTER_IM_MESSAGE("login_get_sms_code", on_login_get_sms_code);
    REGISTER_IM_MESSAGE("login_by_phone", on_login_by_phone);
    REGISTER_IM_MESSAGE("logout", on_logout);
    REGISTER_IM_MESSAGE("connect_after_migration", on_connect_after_migration);
    REGISTER_IM_MESSAGE("avatars/get", on_get_contact_avatar);
    REGISTER_IM_MESSAGE("avatars/show", on_show_contact_avatar);
    REGISTER_IM_MESSAGE("avatars/remove", on_remove_contact_avatars);
    REGISTER_IM_MESSAGE("send_message", on_send_message);
    REGISTER_IM_MESSAGE("update_message", on_update_message);
    REGISTER_IM_MESSAGE("message/typing", on_message_typing);
    REGISTER_IM_MESSAGE("feedback/send", on_feedback);
    //REGISTER_IM_MESSAGE("set_state", on_set_state);
    REGISTER_IM_MESSAGE("archive/buddies/get", on_get_archive_messages_buddies);
    REGISTER_IM_MESSAGE("archive/messages/get", on_get_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete", on_delete_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete_from", on_delete_archive_messages_from);
    REGISTER_IM_MESSAGE("archive/messages/delete_all", on_delete_archive_all_messages);
    REGISTER_IM_MESSAGE("archive/mentions/get", on_get_archive_mentions);
    REGISTER_IM_MESSAGE("archive/log/model", on_archive_log_model);
    REGISTER_IM_MESSAGE("messages/context/get", on_get_message_context);
    REGISTER_IM_MESSAGE("archive/make/holes", on_make_archive_holes);
    REGISTER_IM_MESSAGE("archive/invalidate/message_data", on_invalidate_archive_data);

    REGISTER_IM_MESSAGE("dialogs/search/local", on_dialogs_search_local);
    REGISTER_IM_MESSAGE("dialogs/search/local/end", on_dialogs_search_local_ended);
    REGISTER_IM_MESSAGE("dialogs/search/server", on_dialogs_search_server);
    REGISTER_IM_MESSAGE("dialogs/search/add_pattern", on_dialogs_search_add_pattern);
    REGISTER_IM_MESSAGE("dialogs/search/remove_pattern", on_dialogs_search_remove_pattern);

    REGISTER_IM_MESSAGE("dialogs/add", on_add_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/remove", on_remove_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/hide", on_hide_chat);
    REGISTER_IM_MESSAGE("dialogs/mute", on_mute_chat);
    REGISTER_IM_MESSAGE("dialogs/set_attention_attribute", on_set_attention_attribute);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read", on_set_last_read);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read_mention", on_set_last_read_mention);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read_partial", on_set_last_read_partial);
    REGISTER_IM_MESSAGE("dlg_state/close_stranger", on_close_stranger);
    REGISTER_IM_MESSAGE("voip_call", on_voip_call_message);
    REGISTER_IM_MESSAGE("files/upload", on_upload_file_sharing);
    REGISTER_IM_MESSAGE("files/upload/abort", on_abort_file_sharing_uploading);
    REGISTER_IM_MESSAGE("files/preview_size", on_get_file_sharing_preview_size);
    REGISTER_IM_MESSAGE("files/download/metainfo", on_download_file_sharing_metainfo);
    REGISTER_IM_MESSAGE("files/download", on_download_file);
    REGISTER_IM_MESSAGE("files/download/abort", on_abort_file_downloading);
    REGISTER_IM_MESSAGE("image/download", on_download_image);
    REGISTER_IM_MESSAGE("loader/download/cancel", on_cancel_loader_task);
    REGISTER_IM_MESSAGE("link_metainfo/download", on_download_link_preview);
    REGISTER_IM_MESSAGE("download/raise_priority", on_download_raise_priority);

    REGISTER_IM_MESSAGE("stickers/meta/get", on_get_stickers_meta);
    REGISTER_IM_MESSAGE("stickers/sticker/get", on_get_sticker);
    REGISTER_IM_MESSAGE("stickers/fs_by_ids/get", on_get_fs_stickers_by_ids);
    REGISTER_IM_MESSAGE("stickers/pack/info", on_get_stickers_pack_info);
    REGISTER_IM_MESSAGE("stickers/pack/add", on_add_stickers_pack);
    REGISTER_IM_MESSAGE("stickers/pack/remove", on_remove_stickers_pack);
    REGISTER_IM_MESSAGE("stickers/store/get", on_get_stickers_store);
    REGISTER_IM_MESSAGE("stickers/store/search", on_search_stickers_store);
    REGISTER_IM_MESSAGE("stickers/big_set_icon/get", on_get_set_icon_big);
    REGISTER_IM_MESSAGE("stickers/big_set_icon/clean", on_clean_set_icon_big);
    REGISTER_IM_MESSAGE("stickers/set/order", on_set_sticker_order);

    REGISTER_IM_MESSAGE("chats/info/get", on_get_chat_info);
    REGISTER_IM_MESSAGE("chats/home/get", on_get_chat_home);
    REGISTER_IM_MESSAGE("chats/pending/resolve", on_resolve_pending);
    REGISTER_IM_MESSAGE("chats/member/info", on_get_chat_member_info);
    REGISTER_IM_MESSAGE("chats/members/get", on_get_chat_members);
    REGISTER_IM_MESSAGE("chats/members/search", on_search_chat_members);
    REGISTER_IM_MESSAGE("chats/contacts/get", on_get_chat_contacts);
    REGISTER_IM_MESSAGE("contacts/search/server", on_search_contacts_server);
    REGISTER_IM_MESSAGE("contacts/search/local", on_search_contacts_local);
    REGISTER_IM_MESSAGE("contacts/profile/get", on_profile);
    REGISTER_IM_MESSAGE("contacts/add", on_add_contact);
    REGISTER_IM_MESSAGE("contacts/remove", on_remove_contact);
    REGISTER_IM_MESSAGE("contacts/rename", on_rename_contact);
    REGISTER_IM_MESSAGE("contacts/ignore", on_ignore_contact);
    REGISTER_IM_MESSAGE("contacts/get_ignore", on_get_ignore_contacts);
    REGISTER_IM_MESSAGE("contact/switched", on_contact_switched);
    REGISTER_IM_MESSAGE("dlg_states/hide", on_hide_dlg_state);
    REGISTER_IM_MESSAGE("remove_members", on_remove_members);
    REGISTER_IM_MESSAGE("add_members", on_add_members);
    REGISTER_IM_MESSAGE("add_chat", on_add_chat);
    REGISTER_IM_MESSAGE("modify_chat", on_modify_chat);
    REGISTER_IM_MESSAGE("sign_url", on_sign_url);
    REGISTER_IM_MESSAGE("stats", on_stats);
    REGISTER_IM_MESSAGE("im_stats", on_im_stats);
    REGISTER_IM_MESSAGE("addressbook/sync", on_syncronize_addressbook);

    REGISTER_IM_MESSAGE("themes/meta/check", on_check_theme_meta_updates);
    REGISTER_IM_MESSAGE("themes/wallpaper/get", on_get_theme_wallpaper);
    REGISTER_IM_MESSAGE("themes/wallpaper/preview/get", on_get_theme_wallpaper_preview);
    REGISTER_IM_MESSAGE("themes/wallpaper/user/set", on_set_user_wallpaper);
    REGISTER_IM_MESSAGE("themes/wallpaper/user/remove", on_set_user_wallpaper);

    REGISTER_IM_MESSAGE("files/set_url_played", on_url_played);
    REGISTER_IM_MESSAGE("files/speech_to_text", on_speech_to_text);
    REGISTER_IM_MESSAGE("favorite", on_favorite);
    REGISTER_IM_MESSAGE("unfavorite", on_unfavorite);
    REGISTER_IM_MESSAGE("update_profile", on_update_profile);
    REGISTER_IM_MESSAGE("set_user_proxy_settings", on_set_user_proxy);
    REGISTER_IM_MESSAGE("livechat/join", on_join_livechat);
    REGISTER_IM_MESSAGE("set_locale", on_set_locale);
    REGISTER_IM_MESSAGE("set_avatar", on_set_avatar);

    REGISTER_IM_MESSAGE("chats/create", on_create_chat);

    REGISTER_IM_MESSAGE("chats/mod/params", on_mod_chat_params);
    REGISTER_IM_MESSAGE("chats/mod/name", on_mod_chat_name);
    REGISTER_IM_MESSAGE("chats/mod/about", on_mod_chat_about);
    REGISTER_IM_MESSAGE("chats/mod/rules", on_mod_chat_rules);
    REGISTER_IM_MESSAGE("chats/mod/public", on_mod_chat_public);
    REGISTER_IM_MESSAGE("chats/mod/join", on_mod_chat_join);
    REGISTER_IM_MESSAGE("chats/mod/link", on_mod_chat_link);
    REGISTER_IM_MESSAGE("chats/mod/ro", on_mod_chat_ro);

    REGISTER_IM_MESSAGE("chats/block", on_block_chat_member);
    REGISTER_IM_MESSAGE("chats/role/set", on_set_chat_member_role);
    REGISTER_IM_MESSAGE("chats/message/pin", on_chat_pin_message);
    REGISTER_IM_MESSAGE("phoneinfo", on_phoneinfo);
    REGISTER_IM_MESSAGE("masks/get_id_list", on_get_mask_id_list);
    REGISTER_IM_MESSAGE("masks/preview/get", on_get_mask_preview);
    REGISTER_IM_MESSAGE("masks/model/get", on_get_mask_model);
    REGISTER_IM_MESSAGE("masks/get", on_get_mask);
    REGISTER_IM_MESSAGE("masks/get/existent", on_get_existent_masks);
    REGISTER_IM_MESSAGE("mrim/get_key", on_mrim_get_key);
    REGISTER_IM_MESSAGE("merge_account", on_merge_account);

    REGISTER_IM_MESSAGE("mentions/suggests/get", on_get_mentions_suggests);
    REGISTER_IM_MESSAGE("get_code_by_phone_call", on_get_code_by_phone_call);
    REGISTER_IM_MESSAGE("get_attach_phone_info", on_get_attach_phone_info);

    REGISTER_IM_MESSAGE("get_logs_path", on_get_logs_path);
    REGISTER_IM_MESSAGE("change_app_config", on_change_app_config);
    REGISTER_IM_MESSAGE("remove_content_cache", on_remove_content_cache);
    REGISTER_IM_MESSAGE("clear_avatars", on_clear_avatars);

    REGISTER_IM_MESSAGE("report/contact", on_report_contact);
    REGISTER_IM_MESSAGE("report/stickerpack", on_report_stickerpack);
    REGISTER_IM_MESSAGE("report/sticker", on_report_sticker);
    REGISTER_IM_MESSAGE("report/message", on_report_message);

    REGISTER_IM_MESSAGE("agreement/gdpr", on_user_accept_gdpr);

    REGISTER_IM_MESSAGE("get_voip_calls_quality_popup_conf", on_get_voip_calls_quality_popup_conf);
    REGISTER_IM_MESSAGE("send_voip_calls_quality_report", on_send_voip_calls_quality_report);

    REGISTER_IM_MESSAGE("get_dialog_gallery", on_get_dialog_gallery);
    REGISTER_IM_MESSAGE("get_dialog_gallery_by_msg", on_get_dialog_gallery_by_msg);
    REGISTER_IM_MESSAGE("request_gallery_state", on_request_gallery_state);
    REGISTER_IM_MESSAGE("get_gallery_index", on_get_gallery_index);
    REGISTER_IM_MESSAGE("make_gallery_hole", on_make_gallery_hole);

    REGISTER_IM_MESSAGE("request_memory_usage", on_request_memory_usage);
    REGISTER_IM_MESSAGE("report_memory_usage", on_report_memory_usage);
    REGISTER_IM_MESSAGE("get_ram_usage", on_get_ram_usage);
    REGISTER_IM_MESSAGE("_ui_activity", on_ui_activity);

    REGISTER_IM_MESSAGE("localpin/set", on_local_pin_set);
    REGISTER_IM_MESSAGE("localpin/entered", on_local_pin_entered);
    REGISTER_IM_MESSAGE("localpin/disable", on_local_pin_disable);

    REGISTER_IM_MESSAGE("idinfo/get", on_get_id_info);

    REGISTER_IM_MESSAGE("update/check", on_update_check);

    REGISTER_IM_MESSAGE("get_id_info", on_get_id_info);
    REGISTER_IM_MESSAGE("get_user_info", on_get_user_info);
    REGISTER_IM_MESSAGE("get_user_last_seen", on_get_user_last_seen);

    REGISTER_IM_MESSAGE("privacy_settings/set", on_set_privacy_settings);
    REGISTER_IM_MESSAGE("privacy_settings/get", on_get_privacy_settings);

    REGISTER_IM_MESSAGE("nickname/check", on_nickname_check);
    REGISTER_IM_MESSAGE("common_chats/get", on_get_common_chats);

    REGISTER_IM_MESSAGE("connection/reset", on_reset_connection);

    REGISTER_IM_MESSAGE("recent_avatars_size/update", on_update_recent_avatars_size);
}


im_container::~im_container()
{
}

void core::im_container::create()
{
    if (create_ims())
    {
        connect_ims();
    }
}

void im_container::connect_ims()
{
    for (const auto& x : ims_)
        x->connect();
}

bool im_container::update_login(im_login_id& _login)
{
    return logins_->update(_login);
}

void im_container::replace_uin_in_login(im_login_id& old_login, im_login_id& new_login)
{
    logins_->replace_uin(old_login, new_login);
}

bool core::im_container::create_ims()
{
    if (logins_->load())
    {
    }

    im_login_id login(std::string(), default_im_id);

    if (logins_->get_first_login(login))
    {
        // There has been a login at some point
        set_user_has_been_logged_in_ever(true);
    }
    else if (build::is_dit() || build::is_biz())
    {
        const auto app_ini_path = utils::get_app_ini_path();
        configuration::dump_app_config_to_disk(app_ini_path);
    }

    ims_.push_back(std::make_shared<wim::im>(login, voip_manager_, memory_stats_collector_));

    return !ims_.empty();
}

std::string core::im_container::get_first_login() const
{
    im_login_id login(std::string(), default_im_id);
    logins_->get_first_login(login);
    return login.get_login();
}

void core::im_container::on_message_from_gui(std::string_view _message, int64_t _seq, coll_helper& _params)
{
    if (const auto iter_handler = messages_map_.find(_message); iter_handler != messages_map_.end())
    {
        if (iter_handler->second)
            iter_handler->second(_seq, _params);
    }
    else
    {
        assert(!"unknown message type");
    }
}

void core::im_container::fromInternalProxySettings2Voip(const core::proxy_settings& proxySettings, voip_manager::VoipProxySettings& voipProxySettings) {
    using namespace voip_manager;

    if (!proxySettings.use_proxy_) {
        voipProxySettings.type = VoipProxySettings::kProxyType_None;
    }
    else {
        switch (proxySettings.proxy_type_) {
        case 0:
            voipProxySettings.type = VoipProxySettings::kProxyType_Http;
            break;

        case 4:
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks4;
            break;

        case 5:
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks5;
            break;

        case 6:
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks4a;
            break;

        default:
            voipProxySettings.type = VoipProxySettings::kProxyType_None;
        }
    }

    voipProxySettings.serverUrl = proxySettings.proxy_server_;
    voipProxySettings.userName = proxySettings.login_;
    voipProxySettings.userPassword = proxySettings.password_;
}

void core::im_container::on_voip_call_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    const std::string type = _params.get_value_as_string("type");
    if (!im && type != "voip_reset") {
        return;
    }

    if (type == "voip_call_start") {
        const std::string call_type = _params.get_value_as_string("call_type");
        const std::string mode = _params.get_value_as_string("mode");
        const std::string contact = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_start(contact, call_type == "video", mode == "attach");
        };
    }
    else if (type == "voip_add_window") {
        voip_manager::WindowParams windowParams = {};
        windowParams.hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        windowParams.isPrimary = _params.get_value_as_bool("mode");
        windowParams.isIncoming = _params.get_value_as_bool("incoming_mode");
        windowParams.scale = (float)_params.get_value_as_double("scale");

        std::vector<core::istream*> steramsForFree;

        // Function to get bitmap from params.
        auto getBitmapDescription = [&_params, &steramsForFree](const std::string& prefix, voip_manager::BitmapDescription& outBitmap) -> bool {
            bool res = false;
            if (_params.is_value_exist(prefix.c_str())) {
                core::istream* statusStream = _params.get_value_as_stream(prefix.c_str());
                const auto h = _params.get_value_as_int((prefix + "_height").c_str());
                const auto w = _params.get_value_as_int((prefix + "_width").c_str());
                assert(statusStream);
                if (statusStream) {
                    const auto stream_size = statusStream->size();
                    assert(stream_size);
                    if (stream_size > 0 && h > 0 && w > 0) {
                        outBitmap = get_bitmap_from_stream(statusStream, w, h);
                        res = true;
                    }

                    steramsForFree.push_back(statusStream);
                }
            }

            return res;
        };

        if (getBitmapDescription("camera_status", windowParams.cameraStatus.bitmap))
        {
            windowParams.cameraStatus.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("calling", windowParams.calling.bitmap))
        {
            windowParams.calling.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("closedByBusy", windowParams.closedByBusy.bitmap))
        {
            windowParams.calling.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("watermark", windowParams.watermark.bitmap))
        {
            windowParams.watermark.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("connecting_status", windowParams.connecting.bitmap))
        {
            windowParams.connecting.hwnd = windowParams.hwnd;
        }

        std::string buttonsParams[] = { "closeNormalButton" , "closeDisabledButton", "closePressedButton" , "closeHighlightedButton",
            "goToChatNormalButton", "goToChatHighlightedButton", "goToChatPressedButton", "goToChatDisabledButton" };

        voip_manager::BitmapDescription* buttons[] = { &windowParams.closeButton.normalButton, &windowParams.closeButton.disabledButton,
            &windowParams.closeButton.pressedButton,  &windowParams.closeButton.highlightedButton,

            &windowParams.goToChatButton.normalButton, &windowParams.goToChatButton.disabledButton,
            &windowParams.goToChatButton.pressedButton,  &windowParams.goToChatButton.highlightedButton };

        for (int i = 0; i < sizeof(buttonsParams) / sizeof(buttonsParams[0]); i++)
        {
            getBitmapDescription(buttonsParams[i], *buttons[i]);
        }

        im->on_voip_add_window(windowParams);

        for (auto stream : steramsForFree)
        {
            if (stream) {
                stream->reset();
            }
        }
    }
    else if (type == "voip_remove_window") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        im->on_voip_remove_window(hwnd);
    }
    else if (type == "voip_call_stop") {
        im->on_voip_call_stop();
    }
    else if (type == "voip_call_volume_change") {
        const int vol = _params.get_value_as_int("volume");
        im->on_voip_volume_change(vol);
    }
    else if (type == "audio_playback_mute_switch") {
        im->on_voip_mute_switch();
    }
    else if (type == "voip_call_media_switch") {
        const bool video = _params.get_value_as_bool("video");
        im->on_voip_switch_media(video);
    }
    else if (type == "voip_sounds_mute") {
        const bool mute = _params.get_value_as_bool("mute");
        im->on_voip_mute_incoming_call_sounds(mute);
    }
    else if (type == "voip_call_decline") {
        const std::string mode = _params.get_value_as_string("mode");
        im->on_voip_call_end(_params.get_value_as_string("contact"), mode == "busy");
    }
    else if (type == "converted_avatar") {
        on_voip_avatar_msg(im, _params);
    }
    else if (type == "backgroung_update") {
        on_voip_background_msg(im, _params);
    }
    else if (type == "voip_call_accept") {
        const std::string mode = _params.get_value_as_string("mode");
        const std::string contact = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_accept(contact, mode == "video");
        }
    }
    else if (type == "device_change") {
        const std::string dev_type = _params.get_value_as_string("dev_type");
        const std::string uid = _params.get_value_as_string("uid");
        im->on_voip_device_changed(dev_type, uid);
    }
    else if (type == "request_calls") {
        im->on_voip_call_request_calls();
    }
    else if (type == "update") {
        im->on_voip_update();
    }
    else if (type == "voip_set_window_offsets") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        auto l = _params.get_value_as_int("left");
        auto t = _params.get_value_as_int("top");
        auto r = _params.get_value_as_int("right");
        auto b = _params.get_value_as_int("bottom");

        im->on_voip_window_set_offsets(hwnd, l, t, r, b);
    }
    else if (type == "voip_reset") {
        if (!im) {
#ifndef STRIP_VOIP
            voip_manager_->get_voip_manager()->reset();
#endif
        }
        else {
            im->on_voip_reset();
        }
    }
    else if (type == "audio_playback_mute")
    {
        const std::string mode = _params.get_value_as_string("mute");
        im->on_voip_set_mute(mode == "on");
    }
    else if (type == "voip_minimal_bandwidth_switch")
    {
        im->on_voip_minimal_bandwidth_switch();
    }
    else if (type == "voip_load_mask")
    {
        const std::string maskPath = _params.get_value_as_string("path");
        im->on_voip_load_mask(maskPath);
    }
    else if (type == "window_set_primary")
    {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        const std::string contact = _params.get_value_as_string("contact");
        im->on_voip_window_set_primary(hwnd, contact);
    }
    else if (type == "voip_init_mask_engine")
    {
        im->on_voip_init_mask_engine();
    }
    else if (type == "voip_window_set_conference_layout")
    {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        const int vol = _params.get_value_as_int("layout");

        im->on_voip_window_set_conference_layout(hwnd, (voip_manager::VideoLayout)vol);
    }
    else if (type == "voip_window_set_hover")
    {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        const bool hover = _params.get_value_as_bool("hover");

        im->on_voip_window_set_hover(hwnd, hover);
    }
    else if (type == "voip_window_large_state")
    {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        const bool large = _params.get_value_as_bool("large");

        im->on_voip_window_set_large(hwnd, large);
    }
    else
    {
        assert(false);
    }
}

void core::im_container::on_voip_background_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    core::istream* stream = _params.get_value_as_stream("background");
    const auto h = _params.get_value_as_int("background_height");
    const auto w = _params.get_value_as_int("background_width");
    void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("window_handle");

    assert(stream);
    if (stream) {
        const auto stream_size = stream->size();

        assert(stream_size);
        if (stream_size > 0 && h > 0 && w > 0) {
            im->on_voip_window_update_background(hwnd, stream->read(stream->size()), stream_size, w, h);
            stream->reset();
        }
    }
}

void core::im_container::on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    typedef void (base_im::*__loader_func)(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);

    auto __load_avatar = [&_params, &im](const std::string& prefix, __loader_func func) {

        const voip_manager::AvatarThemeType theme = (voip_manager::AvatarThemeType)_params.get_value_as_int("theme");

        const std::string contact = _params.get_value_as_string("contact");
        const auto size = _params.get_value_as_int("size");// (!miniWindow ? _params.get_value_as_int("size") : _params.get_value_as_int("width"));

        assert(!contact.empty() && size);
        if (contact.empty() || !size) {
            return;
        }

        assert(!!im);
        if (!im) {
            return;
        }

        if (!_params.is_value_exist((prefix + "avatar").c_str())) {
            return;
        }

        core::istream* stream = _params.get_value_as_stream((prefix + "avatar").c_str());
        const auto h = _params.get_value_as_int((prefix + "avatar_height").c_str());
        const auto w = _params.get_value_as_int((prefix + "avatar_width").c_str());

        assert(stream);
        if (stream) {
            const auto stream_size = stream->size();

            assert(stream_size);
            if (stream_size > 0) {
                core::base_im& ptr = *im.get();
                (ptr.*func)(contact, stream->read(stream->size()), stream_size, h, w, theme);
                stream->reset();
            }
        }
    };

    __load_avatar("", &base_im::on_voip_user_update_avatar);
    __load_avatar("rem_camera_offline_", &base_im::on_voip_user_update_avatar_no_video);
    __load_avatar("sign_normal_", &base_im::on_voip_user_update_avatar_text);
    __load_avatar("sign_header_", &base_im::on_voip_user_update_avatar_text_header);
    //__load_avatar("loc_camera_offline_",  &base_im::on_voip_user_update_avatar_camera_off);
    //__load_avatar("loc_camera_disabled_", &base_im::on_voip_user_update_avatar_no_camera);
    __load_avatar("background_", &base_im::on_voip_user_update_avatar_background);
}

void im_container::on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::archive::msgids_list ids;
    if (auto idsArray = _params.get_value_as_array("ids"))
    {
        const auto size = idsArray->size();
        ids.reserve(size);
        for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
            ids.push_back(idsArray->get_at(i)->get_as_int64());
    }

    const auto is_updated = _params.get_value_as_bool("updated", false) ? core::wim::is_updated_messages::yes : core::wim::is_updated_messages::no;

    im->get_archive_messages_buddies(_seq, _params.get_value_as_string("contact"), std::make_shared<core::archive::msgids_list>(std::move(ids)), is_updated);
}


void im_container::on_get_archive_messages(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_archive_messages(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_int64("from"),
        _params.get_value_as_int64("count_early"),
        _params.get_value_as_int64("count_later"),
        _params.get_value_as_bool("need_prefetch"),
        _params.get_value_as_bool("is_first_request"),
        _params.get_value_as_bool("after_search"));
}

void core::im_container::on_get_archive_mentions(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mentions(
        _seq,
        _params.get_value_as_string("contact"));
}

void core::im_container::on_message_typing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto id = _params.get_value_as_string("id", "");
    im->send_message_typing(_seq, _params.get_value_as_string("contact"), (core::typing_status)_params.get_value_as_int("status"), id);
}

static core::archive::message_pack make_pack(const coll_helper& _params)
{
    std::string message;
    if (_params->is_value_exist("message"))
        message = _params.get_value_as_string("message");

    if (core::configuration::get_app_config().is_crash_enabled() && message == "!crash")
    {
        throw new std::runtime_error("artificial crash");
    }

    bool is_sms = false;
    auto type = message_type::base;
    if (_params->is_value_exist("is_sms"))
    {
        is_sms = _params.get_value_as_bool("is_sms");
        type = message_type::sms;
    }

    bool is_sticker = false;
    if (_params->is_value_exist("sticker"))
    {
        is_sticker = _params.get_value_as_bool("sticker");
        if (is_sticker)
        {
            std::stringstream ss_message;
            ss_message << "ext:" << _params.get_value_as_int("sticker/set_id") << ":sticker:" << _params.get_value_as_int("sticker/id");

            message = ss_message.str();
        }

        type = message_type::sticker;
    }

    core::archive::quotes_vec quotes;
    if (_params->is_value_exist("quotes"))
    {
        core::iarray* array = _params.get_value_as_array("quotes");
        const auto size = array->size();
        quotes.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::archive::quote q;
            q.unserialize(array->get_at(i)->get_as_collection());
            quotes.push_back(std::move(q));
        }
    }

    core::archive::mentions_map mentions;
    if (_params->is_value_exist("mentions"))
    {
        const core::iarray* array = _params.get_value_as_array("mentions");
        for (auto i = 0, size = array->size(); i < size; ++i)
        {
            auto coll = array->get_at(i)->get_as_collection();
            coll_helper helper(coll, false);
            mentions.emplace(helper.get_value_as_string("aimid"), helper.get_value_as_string("friendly"));
        }
    }

    std::string internal_id;
    if (_params->is_value_exist("internal_id"))
        internal_id = _params.get_value_as_string("internal_id");

    uint64_t time = 0;
    if (_params->is_value_exist("msg_time"))
        time = _params.get_value_as_int64("msg_time");

    assert(!(is_sms && is_sticker));

    std::string description;
    if (_params.is_value_exist("description"))
        description = _params.get_value_as_string("description");

    std::string url;
    if (_params.is_value_exist("url"))
        url = _params.get_value_as_string("url");

    common::tools::patch_version version;
    if (_params.is_value_exist("update_patch_version"))
        version.set_version(_params.get_value_as_string("update_patch_version"));

    if (_params.is_value_exist("offline_version"))
        version.set_offline_version(_params.get_value_as_int("offline_version"));

    core::archive::shared_contact shared_contact;
    if (_params.is_value_exist("shared_contact"))
    {
        core::archive::shared_contact_data contact;
        core::icollection* coll = _params.get_value_as_collection("shared_contact");
        if (contact.unserialize(coll))
            shared_contact = std::move(contact);
    }

    core::archive::message_pack pack;
    pack.message_ = std::move(message);
    pack.quotes_ = std::move(quotes);
    pack.mentions_ = std::move(mentions);
    pack.internal_id_ = std::move(internal_id);
    pack.type_ = type;
    pack.message_time_ = time;
    pack.description_ = std::move(description);
    pack.url_ = std::move(url);
    pack.version_ = std::move(version);
    pack.shared_contact_ = std::move(shared_contact);

    return pack;
}

void core::im_container::on_send_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (_params.is_value_exist("contact"))
    {
        im->send_message_to_contact(
            _seq,
            _params.get_value_as_string("contact"),
            make_pack(_params));
    }
    else if (_params->is_value_exist("contacts"))
    {
        std::vector<std::string> contacts_to_send;
        const core::iarray* array = _params.get_value_as_array("contacts");
        const auto size = array->size();
        contacts_to_send.reserve(size);
        for (auto i = 0; i < size; ++i)
            contacts_to_send.push_back(array->get_at(i)->get_as_string());
        im->send_message_to_contacts(
            _seq,
            contacts_to_send,
            make_pack(_params));
    }
}

void core::im_container::on_update_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string contact = _params.get_value_as_string("contact");
    const int64_t msgid = _params.get_value_as_int64("id", -1);

    auto pack = make_pack(_params);

    if (msgid != -1)
    {
        pack.internal_id_.clear();
    }

    im->update_message_to_contact(
        _seq,
        msgid,
        contact,
        pack);
}

void core::im_container::on_feedback(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::map<std::string, std::string> fields;
    std::vector<std::string> files;

    std::string url = _params.get_value_as_string("url");

    fields.insert(std::make_pair("fb.screen_resolution", _params.get_value_as_string("screen_resolution")));
    fields.insert(std::make_pair("fb.referrer", _params.get_value_as_string("referrer")));
    fields.insert(std::make_pair(build::get_product_variant("fb.question.3004", "fb.question.7780", "fb.question.53731", "fb.question.53742"), _params.get_value_as_string("version")));
    fields.insert(std::make_pair("fb.question.159", _params.get_value_as_string("os_version")));
    fields.insert(std::make_pair("fb.question.178", _params.get_value_as_string("build_type")));
    fields.insert(std::make_pair(build::get_product_variant("fb.question.3005", "fb.question.7782", "fb.question.53732", "fb.question.53743"), _params.get_value_as_string("platform")));
    fields.insert(std::make_pair("fb.user_name", _params.get_value_as_string("user_name")));
    fields.insert(std::make_pair("fb.message", _params.get_value_as_string("message")));
    fields.insert(std::make_pair("fb.communication_email", _params.get_value_as_string("communication_email")));
    fields.insert(std::make_pair("Lang", _params.get_value_as_string("language")));
    fields.insert(std::make_pair("attachements_count", _params.get_value_as_string("attachements_count")));

    if (build::is_icq() || build::is_agent())
    {
        const auto problem = build::is_icq() ? "fb.question.53496" : "fb.question.53490";
        fields.insert(std::make_pair(problem, _params.get_value_as_string(problem)));
    }

    if (_params.is_value_exist("attachement"))
    {
        core::iarray* array = _params.get_value_as_array("attachement");
        const auto size = array->size();
        files.reserve(size);
        for (auto i = 0; i < size; ++i)
            files.emplace_back(array->get_at(i)->get_as_string());
    }

    im->send_feedback(_seq, url, fields, files);
}

void core::im_container::on_phoneinfo(int64_t seq, coll_helper& _params)
{
    std::shared_ptr<base_im> im = get_im(_params);
    if (!im)
    {
        im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);
        ims_.push_back(im);
    }

    std::string phone = _params.get_value_as_string("phone");
    std::string gui_locale = _params.get_value_as_string("gui_locale");

    im->phoneinfo(seq, phone, gui_locale);
}

void core::im_container::on_set_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto state = profile_state::online;
    std::string sstate = _params.get_value_as_string("state");
    if (sstate == "dnd")
        state = profile_state::dnd;
    else if (sstate == "away")
        state = profile_state::away;
    else if (sstate == "invisible")
        state = profile_state::invisible;
    else if (sstate == "offline")
        state = profile_state::offline;

    im->set_state(_seq, state);
}

void core::im_container::on_remove_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_remove"));
}

void core::im_container::on_add_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_add"));
}

void core::im_container::on_add_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> members;

    if (const auto member_array = _params.get_value_as_array("m_chat_members"))
    {
        const auto size = member_array->size();
        members.reserve(size);
        for (core::iarray::size_type i = 0; i < size; ++i)
            members.emplace_back(member_array->get_at(i)->get_as_string());
    }


    im->add_chat(
        _seq,
        _params.get_value_as_string("m_chat_name"),
        std::move(members));
}

void core::im_container::on_modify_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->modify_chat(
        _seq,
        _params.get_value_as_string("aimid"), _params.get_value_as_string("m_chat_name"));
}

void core::im_container::on_mrim_get_key(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mrim_key(
        _seq,
        _params.get_value_as_string("email"));
}

void core::im_container::on_login_by_password(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));
    info.set_token_type(_params.get_value_as_enum<token_type>("token_type", token_type::basic));

    std::shared_ptr<base_im> im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);
    ims_.clear();
    im->login(_seq, info);
    ims_.push_back(im);
}

void core::im_container::on_login_by_password_for_attach_uin(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));

    auto from_im = get_im(_params);
    if (from_im)
        from_im->start_attach_uin(_seq, info, from_im->make_wim_params());
}

void core::im_container::on_login_get_sms_code(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));

    std::shared_ptr<base_im> im;

    auto is_login = _params.get_value_as_bool("is_login");
    if (is_login)
    {
        ims_.clear();
        im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);
    }
    else
    {
        im = get_im(_params);
    }

    im->login_normalize_phone(_seq, _params.get_value_as_string("country"), _params.get_value_as_string("phone"), _params.get_value_as_string("locale"), is_login);

    if (is_login)
        ims_.push_back(im);
}

void core::im_container::on_login_by_phone(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));
    info.set_sms_code(_params.get_value_as_string("sms_code"));

    if (ims_.empty())
    {
        assert(!"ims empty");
        return;
    }

    bool is_login = _params.get_value_as_bool("is_login");
    if (is_login)
        (*ims_.begin())->login_by_phone(_seq, info);
    else
        (*ims_.begin())->start_attach_phone(_seq, info);
}

void core::im_container::on_connect_after_migration(int64_t _seq, coll_helper& _params)
{
    ims_.clear();
    create();
}

void core::im_container::on_logout(int64_t _seq, coll_helper& _params)
{
    assert(!ims_.empty());
    if (ims_.empty())
        return;

    auto wr_this = weak_from_this();

    auto __onlogout = [wr_this]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->start_session_stats(false /* delayed */);

        (*ptr_this->ims_.begin())->erase_auth_data();
        g_core->set_local_pin_enabled(false);

        ptr_this->ims_.clear();

        if (!g_core->is_keep_local_data())
            g_core->remove_local_data();
    };

    g_core->post_message_to_gui("need_login", 0, nullptr);

#ifndef STRIP_VOIP
    if (voip_manager_->get_call_manager()->call_get_count())
#else
    if (0)
#endif
    {
        auto __doLogout = [wr_this, __onlogout]()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }
            (*ptr_this->ims_.begin())->logout(__onlogout);
        };
#ifndef STRIP_VOIP
        voip_manager_->get_call_manager()->call_stop_smart(__doLogout);
#endif //STRIP_VOIP
    }
    else
    {
        (*ims_.begin())->logout(__onlogout);
    }
}

void core::im_container::logout(const bool _is_auth_error)
{
    auto wr_this = weak_from_this();
    auto logout = [wr_this, _is_auth_error]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        assert(!ptr_this->ims_.empty());
        if (ptr_this->ims_.empty())
            return;

        ptr_this->ims_.front()->erase_auth_data();
        g_core->set_local_pin_enabled(false);

        coll_helper coll(g_core->create_collection(), true);

        coll.set_value_as_bool("is_auth_error", _is_auth_error);

        g_core->post_message_to_gui("need_login", 0, coll.get());

        ptr_this->ims_.clear();

        if (!ptr_this->logins_->empty() && !g_core->is_keep_local_data())
            g_core->remove_local_data();
    };

    voip_manager_->get_call_manager()->call_stop_smart(logout);
}

void core::im_container::on_sign_url(int64_t _seq, coll_helper& _params)
{
    (*ims_.begin())->sign_url(_seq, _params.get_value_as_string("url"));
}

void core::im_container::on_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    if (const auto prop_array = _params.get_value_as_array("props"))
    {
        const auto size = prop_array->size();
        props.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
            const auto prop_name = value.get_value_as_string("name");
            const auto prop_value = value.get_value_as_string("value");
            props.emplace_back(prop_name, prop_value);
        }
    }

    g_core->insert_event((core::stats::stats_event_names)_params.get_value_as_int("event"), props);
}

void core::im_container::on_im_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    if (const auto prop_array = _params.get_value_as_array("props"))
    {
        const auto size = prop_array->size();
        props.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
            const auto prop_name = value.get_value_as_string("name");
            const auto prop_value = value.get_value_as_string("value");
            props.emplace_back(prop_name, prop_value);
        }
    }

    g_core->insert_im_stats_event((core::stats::im_stat_event_names)_params.get_value_as_int("event"), std::move(props));
}

void core::im_container::on_url_played(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_played(_params.get_value_as_string("url"), _params.get_value_as_bool("played"));
}

void core::im_container::on_speech_to_text(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->speech_to_text(_seq, _params.get_value_as_string("url"), _params.get_value_as_string("locale"));
}

std::shared_ptr<base_im> core::im_container::get_im(coll_helper& _params) const
{
    // temporary, for many im
    return get_im_by_id(0);
}

std::shared_ptr<base_im> core::im_container::get_im_by_id(int32_t _id) const
{
    if (ims_.empty())
    {
        return std::shared_ptr<base_im>();
    }

    return (*ims_.begin());
}

void core::im_container::on_get_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto force = _params.get_value_as_bool("force", false);
    im->get_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"), force);
}

void core::im_container::on_show_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->show_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"));
}

void core::im_container::on_remove_contact_avatars(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact_avatars(_seq, _params.get_value_as_string("contact"));
}

void core::im_container::on_delete_archive_messages(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string contact_aimid = _params.get_value_as_string("contact_aimid");
    assert(!contact_aimid.empty());

    const auto messages_id = _params.get_value_as_int64("message_id");
    const auto internal_id = _params.get_value_as_string("internal_id");
    const auto for_all = _params.get_value_as_bool("for_all");

    im->delete_archive_message(_seq, contact_aimid, messages_id, internal_id, for_all);
}

void core::im_container::on_delete_archive_messages_from(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string contact_aimid = _params.get_value_as_string("contact_aimid");
    assert(!contact_aimid.empty());

    const auto from_id = _params.get_value_as_int64("from_id");
    assert(from_id >= 0);

    im->delete_archive_messages_from(_seq, contact_aimid, from_id);
}

void core::im_container::on_delete_archive_all_messages(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string_view contact_aimid = _params.get_value_as_string("contact_aimid");
    assert(!contact_aimid.empty());

    im->delete_archive_all_messages(_seq, contact_aimid);
}

void core::im_container::on_get_message_context(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_message_context(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int64("msgid"));
}

void core::im_container::on_archive_log_model(int64_t /*_seq*/, coll_helper& /*_params*/)
{
}

void core::im_container::on_add_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_remove_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_opened_dialog(_params.get_value_as_string("contact"));
}

void im_container::on_dialogs_search_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    if (!keyword.empty())
    {
        std::vector<std::string> aimids;
        if (_params.is_value_exist("contact"))
            aimids.push_back(_params.get_value_as_string("contact"));

        im->setup_search_dialogs_params(_seq);
        im->search_dialogs_local(keyword, aimids);
    }
}

void im_container::on_dialogs_search_local_ended(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clear_search_dialogs_params();
}

void im_container::on_dialogs_search_server(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string contact = _params.get_value_as_string("contact", "");
    if (_params.is_value_exist("cursor"))
    {
        const std::string cursor = _params.get_value_as_string("cursor");
        im->search_dialogs_continue(_seq, cursor, contact);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
        {
            if (!contact.empty())
                im->search_dialogs_one(_seq, keyword, contact);
            else
                im->search_dialogs_all(_seq, keyword);
        }
    }
}

void im_container::on_dialogs_search_add_pattern(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string_view contact = _params.get_value_as_string("contact");
    const std::string_view pattern = _params.get_value_as_string("pattern");

    if (!contact.empty() && !pattern.empty())
        im->add_search_pattern_to_history(pattern, contact);
}

void im_container::on_dialogs_search_remove_pattern(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string_view contact = _params.get_value_as_string("contact");
    const std::string_view pattern = _params.get_value_as_string("pattern");
    if (!contact.empty() && !pattern.empty())
        im->remove_search_pattern_from_history(pattern, contact);
}

void im_container::on_set_last_read(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto read_all = _params.get_value_as_bool("read_all", false);

    im->set_last_read(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"), read_all ? core::message_read_mode::read_all : core::message_read_mode::read_text);
}

void im_container::on_set_last_read_mention(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read_mention(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_set_last_read_partial(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read_partial(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_hide_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_chat(_params.get_value_as_string("contact"));
}

void im_container::on_mute_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mute_chat(_params.get_value_as_string("contact"), _params.get_value_as_bool("mute"));
}

void im_container::on_upload_file_sharing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::shared_ptr<core::tools::binary_stream> data = std::make_shared<core::tools::binary_stream>();
    if (_params.is_value_exist("file_stream"))
    {
        auto stream = _params.get_value_as_stream("file_stream");
        auto size = stream->size();
        data->write((char*)(stream->read(size)), size);
    }

    const auto file_name = _params.get_value_as_string("file", "");
    const auto extension = _params.get_value_as_string("ext", "");
    const auto description = _params.get_value_as_string("description", "");

    core::archive::quotes_vec quotes;
    if (_params->is_value_exist("quotes"))
    {
        core::iarray* array = _params.get_value_as_array("quotes");
        const auto size = array->size();
        quotes.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::archive::quote q;
            q.unserialize(array->get_at(i)->get_as_collection());
            quotes.push_back(std::move(q));
        }
    }

    core::archive::mentions_map mentions;
    if (_params->is_value_exist("mentions"))
    {
        const core::iarray* array = _params.get_value_as_array("mentions");
        for (auto i = 0, size = array->size(); i < size; ++i)
        {
            auto coll = array->get_at(i)->get_as_collection();
            coll_helper helper(coll, false);
            mentions.emplace(helper.get_value_as_string("aimid"), helper.get_value_as_string("friendly"));
        }
    }

    std::optional<int64_t> duration;
    if (_params.is_value_exist("duration"))
        duration = _params.get_value_as_int64("duration");

    im->upload_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        file_name,
        data,
        extension,
        quotes,
        description,
        mentions,
        duration);
}

void im_container::on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->abort_file_sharing_upload(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("process_seq"));
}

void im_container::on_get_file_sharing_preview_size(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_file_sharing_preview_size(
        _seq,
        _params.get_value_as_string("url"),
        _params.get_value_as_int("orig_size"));
}

void im_container::on_download_file_sharing_metainfo(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing_metainfo(
        _seq,
        _params.get_value_as_string("url"));
}

void im_container::on_download_file(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("url"),
        _params.get_value_as_bool("force_request_metainfo"),
        _params.get_value_as_string("filename"),
        _params.get_value_as_string("download_dir"),
        _params.get_value_as_bool("raise", false));
}

void im_container::on_download_image(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_image(
        _seq,
        _params.get_value_as_string("uri"),
        _params.get_value_as_string("destination", ""),
        _params.get_value_as_bool("is_preview"),
        _params.get_value_as_int("preview_width", 0),
        _params.get_value_as_int("preview_height", 0),
        _params.get_value_as_bool("ext_resource", true),
        _params.get_value_as_bool("raise", false),
        _params.get_value_as_bool("with_data", true));
}

void im_container::on_cancel_loader_task(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get_value_as_string("url");

    im->cancel_loader_task(url);
}

void im_container::on_download_link_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_link_preview(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("uri"),
        _params.get_value_as_int("preview_width", 0),
        _params.get_value_as_int("preview_height", 0));
}

void im_container::on_abort_file_downloading(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get_value_as_string("url");

    im->abort_file_sharing_download(url);
}

void im_container::on_download_raise_priority(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto proc_id = _params.get_value_as_int64("proc_id");
    assert(proc_id > 0);

    im->raise_download_priority(proc_id);
}

void im_container::on_contact_switched(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contact_aimid = _params.get_value_as_string("contact");

    im->contact_switched(contact_aimid);
}

void im_container::on_get_stickers_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_meta(_seq, _params.get_value_as_string("size"));
}

void im_container::on_check_theme_meta_updates(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->check_themes_meta_updates(_seq);
}

void im_container::on_get_theme_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme_wallpaper(_params.get_value_as_string("id"));
}

void im_container::on_get_theme_wallpaper_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme_wallpaper_preview(_params.get_value_as_string("id"));
}

void im_container::on_set_user_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::istream* stream = _params.get_value_as_stream("image");
    if (!stream)
    {
        assert(!"_params.get_value_as_stream(\"image\")");
        return;
    }

    tools::binary_stream bs_data;
    uint32_t size = stream->size();
    if (stream && size)
    {
        bs_data.write((const char*)stream->read(size), size);
        stream->reset();
    }

    const auto id = _params.get_value_as_string("id", "");
    im->set_user_wallpaper(id, std::move(bs_data));
}

void im_container::on_remove_user_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto id = _params.get_value_as_string("id", "");
    im->remove_user_wallpaper(id);
}

void im_container::on_get_sticker(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_sticker(_seq,
        _params.get_value_as_int("set_id", -1),
        _params.get_value_as_int("sticker_id", -1),
        _params.get_value_as_string("fs_id", ""),
        _params.get<core::sticker_size>("size"));
}

void im_container::on_get_fs_stickers_by_ids(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto stikers_ids = _params.get_value_as_array("ids");
    if (!stikers_ids)
    {
        assert(false);
        return;
    }
    const auto size = stikers_ids->size();
    std::vector<std::pair<int32_t, int32_t>> result;
    result.reserve(size_t(size) / 2);

    for (core::iarray::size_type i = 0; i < size; i += 2)
        result.emplace_back(stikers_ids->get_at(i)->get_as_int(), stikers_ids->get_at(i + 1)->get_as_int());

    im->get_fs_stickers_by_ids(_seq, std::move(result));
}

void im_container::on_get_stickers_pack_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_pack_info(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"),
        _params.get_value_as_string("file_id"));
}

void im_container::on_add_stickers_pack(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_stickers_pack(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"));
}

void im_container::on_remove_stickers_pack(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_stickers_pack(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"));
}

void im_container::on_get_stickers_store(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_store(_seq);
}

void im_container::on_search_stickers_store(int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->search_stickers_store(_seq, _params.get_value_as_string("term"));
}

void im_container::on_get_set_icon_big(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_set_icon_big(_seq, _params.get_value_as_int("set_id"));
}

void im_container::on_clean_set_icon_big(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clean_set_icon_big(_seq, _params.get_value_as_int("set_id"));
}

void im_container::on_set_sticker_order(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<int32_t> values_array;

    core::iarray* vals = _params.get_value_as_array("values");
    if (!vals)
    {
        assert(false);
        return;
    }

    values_array.reserve(vals->size());

    for (int32_t i = 0; i < vals->size(); ++i)
    {
        values_array.push_back(vals->get_at(i)->get_as_int());
    }

    im->set_sticker_order(_seq, values_array);
}

void im_container::on_get_chat_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string aimid, stamp;

    if (_params->is_value_exist("stamp"))
    {
        stamp = _params.get_value_as_string("stamp");
    }
    else if (_params->is_value_exist("aimid"))
    {
        aimid = _params.get_value_as_string("aimid");
    }
    else
    {
        assert(false);
    }

    im->get_chat_info(_seq, aimid, stamp, _params.get_value_as_int("limit"));
}

void im_container::on_get_chat_home(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto tag = _params.get_value_as_string("tag", "");
    im->get_chat_home(_seq, tag);
}

void im_container::on_resolve_pending(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> contacts;
    if (_params.is_value_exist("contacts"))
    {
        auto contacts_array = _params.get_value_as_array("contacts");
        for (auto i = 0; contacts_array && i < contacts_array->size(); ++i)
            contacts.push_back(contacts_array->get_at(i)->get_as_string());
    }

    im->resolve_pending(_seq, _params.get_value_as_string("aimid"), contacts, _params.get_value_as_bool("approve"));
}

void core::im_container::on_get_chat_member_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (auto arr = _params.get_value_as_array("members"))
    {
        std::vector<std::string> members;
        const auto size = arr->size();
        members.reserve(size);
        for (int i = 0; i < size; ++i)
            members.emplace_back(arr->get_at(i)->get_as_string());

        im->get_chat_member_info(_seq, _params.get_value_as_string("aimid"), members);
    }
}

void im_container::on_get_chat_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
        im->get_chat_members_next_page(_seq, aimid, _params.get_value_as_string("cursor"), page_size);
    else
        im->get_chat_members_page(_seq, aimid, _params.get_value_as_string("role"), page_size);
}

void core::im_container::on_search_chat_members(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
    {
        im->search_chat_members_next_page(_seq, aimid, _params.get_value_as_string("cursor"), page_size);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
            im->search_chat_members_page(_seq, aimid, _params.get_value_as_string("role", ""), keyword, page_size);
    }
}

void im_container::on_get_chat_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_chat_contacts(_seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("cursor", ""),
        _params.get_value_as_uint("page_size"));
}

void im_container::on_search_contacts_server(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    const std::string phone = _params.get_value_as_string("phoneNumber", "");

    if (!keyword.empty() || !phone.empty())
        im->search_contacts_server(_seq, keyword, phone);
}

void core::im_container::on_search_contacts_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    g_core->begin_cl_search();

    std::vector<std::vector<std::string>> searchSymbolsPatterns;
    std::string pattern;
    uint32_t fix_pat_count = 0;
    if (_params.is_value_exist("symbols_array"))
    {
        const auto searchSymbolsArray = _params.get_value_as_array("symbols_array");
        const auto search_symbol_size = searchSymbolsArray->size();
        searchSymbolsPatterns.reserve(search_symbol_size);
        for (auto i = 0; i < search_symbol_size; ++i)
        {
            std::vector<std::string> patternForSymbol;
            core::coll_helper symbol_value(searchSymbolsArray->get_at(i)->get_as_collection(), false);

            const auto symbols_patterns = symbol_value.get_value_as_array("symbols_patterns");
            const auto patterns_size = symbols_patterns->size();
            patternForSymbol.reserve(patterns_size);
            for (auto j = 0; j < patterns_size; ++j)
                patternForSymbol.emplace_back(symbols_patterns->get_at(j)->get_as_string());
            searchSymbolsPatterns.push_back(std::move(patternForSymbol));
        }

        fix_pat_count = _params.get_value_as_uint("fixed_patterns_count", 0);
    }
    else if (_params->is_value_exist("init_pattern"))
    {
        pattern = _params.get_value_as_string("init_pattern");
    }

    for (auto& symbol_iter : searchSymbolsPatterns)
    {
        for (auto& iter : symbol_iter)
            iter = ::core::tools::system::to_upper(iter);
    }

    im->search_contacts_local(searchSymbolsPatterns, _seq, fix_pat_count, pattern);
}

void core::im_container::on_syncronize_addressbook(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> phones;
    if (auto arr = _params.get_value_as_array("phones"))
    {
        const auto size = arr->size();
        phones.reserve(size);
        for (int i = 0; i < size; ++i)
            phones.emplace_back(arr->get_at(i)->get_as_string());
    }

    im->syncronize_address_book(_seq, _params.get_value_as_string("name"), phones);
}

void im_container::on_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_profile(_seq, _params.get_value_as_string("aimid"));
}

void im_container::on_hide_dlg_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (_params->is_value_exist("contacts"))
    {
        const auto contacts_array = _params.get_value_as_array("contacts");
        for (auto i = 0; i < contacts_array->size(); ++i)
            im->hide_dlg_state(contacts_array->get_at(i)->get_as_string());
    }
}

void im_container::on_set_attention_attribute(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->set_attention_attribute(_seq, _params.get_value_as_string("contact"), _params.get_value_as_bool("value"), false);
}

void im_container::on_add_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("group"),
        _params.get_value_as_string("message"));
}

void im_container::on_remove_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_rename_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->rename_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("friendly")
    );
}

void im_container::on_ignore_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->ignore_contact(
        _seq,
        _params.get_value_as_string("contact"), _params.get_value_as_bool("ignore"));
}

void im_container::on_get_ignore_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ignore_list(_seq);
}

void im_container::on_favorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->favorite(_params.get_value_as_string("contact"));
}

void im_container::on_unfavorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->unfavorite(_params.get_value_as_string("contact"));
}

void core::im_container::on_update_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::pair<std::string, std::string>> fields;

    if (const auto field_array = _params.get_value_as_array("fields"))
    {
        const auto size = field_array->size();
        fields.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(field_array->get_at(i)->get_as_collection(), false);
            auto field_name = value.get_value_as_string("field_name");
            auto field_value = value.get_value_as_string("field_value");
            fields.push_back(std::make_pair(field_name, field_value));
        }
    }

    im->update_profile(_seq, fields);
}

void core::im_container::on_join_livechat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string stamp = _params.get_value_as_string("stamp");

    im->join_live_chat(_seq, stamp);
}

void core::im_container::on_set_locale(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string locale = _params.get_value_as_string("locale");

    g_core->set_locale(locale);
}

void core::im_container::on_set_user_proxy(int64_t _seq, coll_helper& _params)
{
    proxy_settings user_proxy;

    user_proxy.proxy_type_ = (int32_t)_params.get_value_as_enum<proxy_type>("settings_proxy_type");

    assert(user_proxy.proxy_type_ >= 0 || user_proxy.proxy_type_ == -1);

    user_proxy.use_proxy_ = user_proxy.proxy_type_ != -1;
    user_proxy.proxy_server_ = tools::from_utf8(_params.get_value_as_string("settings_proxy_server"));
    user_proxy.proxy_port_ = _params.get_value_as_int("settings_proxy_port");
    user_proxy.login_ = tools::from_utf8(_params.get_value_as_string("settings_proxy_username"));
    user_proxy.password_ = tools::from_utf8(_params.get_value_as_string("settings_proxy_password"));
    user_proxy.need_auth_ = _params.get_value_as_bool("settings_proxy_need_auth");
    user_proxy.auth_type_ = _params.get_value_as_enum<core::proxy_auth>("settings_proxy_auth_type");

    g_core->set_user_proxy_settings(user_proxy);
}

void core::im_container::on_set_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    tools::binary_stream bs_data;

    core::istream* stream = _params.get_value_as_stream("avatar");
    if (!stream)
    {
        assert(!"_params.get_value_as_stream(\"avatar\")");
        return;
    }

    uint32_t size = stream->size();
    if (stream && size)
    {
        bs_data.write((const char*)stream->read(size), size);
        stream->reset();
    }

    im->set_avatar(_seq, std::move(bs_data), _params.get_value_as_string("aimid", ""), _params.get_value_as_bool("chat", false));
}

void core::im_container::on_create_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto params = core::wim::chat_params::create(_params);
    std::vector<std::string> members;
    if (const auto member_array = _params.get_value_as_array("members"))
    {
        const auto size = member_array->size();
        members.reserve(size);
        for (auto i = 0; i < size; ++i)
            members.push_back(member_array->get_at(i)->get_as_string());
    }
    im->create_chat(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"), members, std::move(params));
}

void core::im_container::on_mod_chat_params(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto params = core::wim::chat_params::create(_params);
    im->mod_chat_params(_seq, _params.get_value_as_string("aimid"), std::move(params));
}

void core::im_container::on_mod_chat_name(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_name(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"));
}

void core::im_container::on_mod_chat_about(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_about(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("about"));
}

void core::im_container::on_mod_chat_rules(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_rules(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("rules"));
}

void core::im_container::on_mod_chat_public(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_public(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("public"));
}

void core::im_container::on_mod_chat_join(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_join(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("approved"));
}

void core::im_container::on_mod_chat_link(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_link(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("link"));
}

void core::im_container::on_mod_chat_ro(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_ro(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("ro"));
}

void core::im_container::on_chat_pin_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->pin_chat_message(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_int64("msgId"), _params.get_value_as_bool("unpin"));
}

void core::im_container::on_block_chat_member(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->block_chat_member(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_bool("block"));
}

void core::im_container::on_set_chat_member_role(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_chat_member_role(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_string("role"));
}

void im_container::on_get_mask_id_list(int64_t _seq, coll_helper& _params)
{
#ifndef STRIP_VOIP
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_id_list(_seq);
#endif //STRIP_VOIP
}

void im_container::on_get_mask_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_preview(_seq, _params.get_value_as_string("mask_id"));
}

void im_container::on_get_mask_model(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_model(_seq);
}

void im_container::on_get_mask(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask(_seq, _params.get_value_as_string("mask_id"));
}

void im_container::on_get_existent_masks(int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_existent_masks(_seq);
}

bool im_container::has_valid_login() const
{
    auto im = get_im_by_id(0);
    if (!im)
        return false;

    return im->has_valid_login();
}

void im_container::on_merge_account(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::wim::auth_parameters auth;
    auth.unserialize(_params);

    return im->merge_exported_account(auth);
}


void im_container::on_get_mentions_suggests(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mentions_suggests(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("pattern"));
}

void im_container::on_get_code_by_phone_call(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_code_by_phone_call(_params.get_value_as_string("ivr_url"));
}

void im_container::on_get_attach_phone_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_attach_phone_info(_seq, _params.get_value_as_string("locale"), _params.get_value_as_string("lang"));
}

void im_container::on_get_logs_path(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_logs_path(_seq);
}

void im_container::on_change_app_config(const int64_t _seq, coll_helper& _params)
{
    // TODO: normal serialize / deserialize into a struct

    coll_helper cl_coll(g_core->create_collection(), true);

    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::full_log, _params.get_value_as_bool("fulllog"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::is_updateble, _params.get_value_as_bool("updateble"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::unlock_context_menu_features, _params.get_value_as_bool("dev.unlock_context_menu_features"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::show_msg_ids, _params.get_value_as_bool("dev.show_message_ids"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::save_rtp_dumps, _params.get_value_as_bool("dev.save_rtp_dumps"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::cache_history_pages_secs, _params.get_value_as_int("dev.cache_history_pages_secs"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::is_server_search_enabled, _params.get_value_as_bool("dev.server_search"));

    const auto app_ini_path = utils::get_app_ini_path();

    configuration::dump_app_config_to_disk(app_ini_path);

    cl_coll.set_value_as_int("error", 0);

    g_core->post_message_to_gui("app_config_changed", _seq, cl_coll.get());

}

void im_container::on_remove_content_cache(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_content_cache(_seq);
}

void im_container::on_clear_avatars(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clear_avatars(_seq);
}

void im_container::on_report_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->report_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("reason"),
        _params.get_value_as_bool("ignoreAndClose", true));
}

void im_container::on_report_stickerpack(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_stickerpack(_params.get_value_as_int("id"), _params.get_value_as_string("reason"));
}

void im_container::on_report_sticker(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_sticker(_params.get_value_as_string("id"), _params.get_value_as_string("reason"), _params.get_value_as_string("sn"), _params.get_value_as_string("chatId"));
}

void im_container::on_report_message(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_message(_params.get_value_as_int64("id"), _params.get_value_as_string("text"), _params.get_value_as_string("reason"), _params.get_value_as_string("sn"), _params.get_value_as_string("chatId"));
}

void im_container::on_request_memory_usage(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->request_memory_usage(_seq);
}

void im_container::on_report_memory_usage(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::memory_stats::partial_response part_response;
    if (!part_response.unserialize(_params))
        return;

    im->report_memory_usage(_seq,
                            _params.get_value_as_int64("request_id"),
                            part_response);
}

void im_container::on_get_ram_usage(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ram_usage(_seq);
}

void im_container::on_make_archive_holes(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->make_archive_holes(_seq, _params.get_value_as_string("archive"));
}

void im_container::on_invalidate_archive_data(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params); im)
    {
        if (_params.is_value_exist("ids"))
        {
            if (auto ids_array = _params.get_value_as_array("ids"))
            {
                std::vector<int64_t> ids;
                const auto size = ids_array->size();
                ids.reserve(size);
                for (auto i = 0; i < size; ++i)
                    ids.emplace_back(ids_array->get_at(i)->get_as_int64());
                im->invalidate_archive_data(_seq, _params.get_value_as_string("aimid"), std::move(ids));
            }
        }
        else
        {
            const int64_t from = _params.get_value_as_int64("from", 0);
            const int64_t before = _params.get_value_as_int64("before", 0);
            const int64_t after = _params.get_value_as_int64("after", 0);
            im->invalidate_archive_data(_seq, _params.get_value_as_string("aimid"), from, before, after);
        }
    }
}

void im_container::on_user_accept_gdpr(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);

    auto accept_flag = _params.get_value_as_int("accept_flag");
    auto reset_flag = _params.get_value_as_bool("reset");

    if (im)
        im->user_accept_gdpr(_seq, accept_agreement_info(static_cast<accept_agreement_info::agreement_action>(accept_flag),
            reset_flag));
}

void im_container::set_user_has_been_logged_in_ever(bool _hasEverLoggedIn)
{
    if (core::configuration::get_app_config().gdpr_user_has_logged_in_ever() == _hasEverLoggedIn)
        return;

    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::gdpr_user_has_logged_in_ever,
        std::move(_hasEverLoggedIn));

    const auto app_ini_path = utils::get_app_ini_path();
    configuration::dump_app_config_to_disk(app_ini_path);

    // Report new config to GUI
    core::coll_helper cl_coll(g_core->create_collection(), true);
    configuration::get_app_config().serialize(Out cl_coll);
    g_core->post_message_to_gui("app_config", 0, cl_coll.get());
}

void im_container::on_get_voip_calls_quality_popup_conf(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);

    if (im)
        im->get_voip_calls_quality_popup_conf(_seq, _params.get_value_as_string("locale"), _params.get_value_as_string("lang"));
}

void im_container::on_send_voip_calls_quality_report(const int64_t _seq, coll_helper &_params)
{
    UNUSED_ARG(_seq);

    auto im = get_im(_params);
    if (im)
    {
        int score = VOIP_RATING_STARS_SCORE_COEFFICIENT * _params.get_value_as_int("starsCount", -1);

        iarray* arr = _params.get_value_as_array("reasons");
        std::vector<std::string> reasons;
        reasons.reserve(arr->size());

        for (int i = 0; i < arr->size(); ++i)
            reasons.push_back(arr->get_at(i)->get_as_string());

        im->send_voip_calls_quality_report(score,
                                           _params.get_value_as_string("survey_id"),
                                           reasons,
                                           _params.get_value_as_string("aimid"));
    }
}

void im_container::on_get_dialog_gallery(const int64_t _seq, coll_helper &_params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_int("page_size");
    archive::gallery_entry_id after;
    if (_params.is_value_exist("after_msg"))
        after = archive::gallery_entry_id(_params.get_value_as_int64("after_msg"), _params.get_value_as_int64("after_seq"));

    std::vector<std::string> type;
    {
        const core::iarray* array = _params.get_value_as_array("type");
        const auto size = array->size();
        type.reserve(size);
        for (auto i = 0; i < size; ++i)
            type.emplace_back(array->get_at(i)->get_as_string());
    }

    auto im = get_im(_params);
    if (im)
        im->get_dialog_gallery(_seq, aimid, type, after, page_size);
}

void im_container::on_get_dialog_gallery_by_msg(const int64_t _seq, coll_helper &_params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    int64_t msg_id = _params.get_value_as_int64("msg");

    std::vector<std::string> type;
    {
        const core::iarray* array = _params.get_value_as_array("type");
        const auto size = array->size();
        type.reserve(size);
        for (auto i = 0; i < size; ++i)
            type.emplace_back(array->get_at(i)->get_as_string());
    }

    auto im = get_im(_params);
    if (im)
        im->get_dialog_gallery_by_msg(_seq, aimid, type, msg_id);
}

void im_container::on_request_gallery_state(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");

    auto im = get_im(_params);
    if (im)
        im->request_gallery_state(aimid);
}

void im_container::on_get_gallery_index(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto msg = _params.get_value_as_int64("msg");
    const auto seq = _params.get_value_as_int64("seq");
    std::vector<std::string> type;
    {
        const core::iarray* array = _params.get_value_as_array("type");
        const auto size = array->size();
        type.reserve(size);
        for (auto i = 0; i < size; ++i)
            type.emplace_back(array->get_at(i)->get_as_string());
    }

    auto im = get_im(_params);
    if (im)
        im->get_gallery_index(aimid, type, msg, seq);
}

void im_container::on_make_gallery_hole(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto from = _params.get_value_as_int64("from");
    const auto till = _params.get_value_as_int64("till");

    auto im = get_im(_params);
    if (im)
        im->make_gallery_hole(aimid, from, till);
}

void im_container::on_ui_activity(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_ui_activity(_params.get_value_as_int64("time"));
}

void im_container::on_close_stranger(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->close_stranger(_params.get_value_as_string("contact"));
}

void im_container::on_local_pin_set(const int64_t _seq, coll_helper& _params)
{
    const auto password = _params.get_value_as_string("password");
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_set(password);
}

void im_container::on_local_pin_entered(const int64_t _seq, coll_helper& _params)
{
    const auto password = _params.get_value_as_string("password");
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_entered(_seq, password);
}

void im_container::on_local_pin_disable(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_disable(_seq);
}

void im_container::on_update_check(const int64_t _seq, coll_helper &_params)
{
    g_core->check_update(_params.get_value_as_string("url"));
}

void im_container::on_get_id_info(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_id_info(_seq, _params.get_value_as_string("id"));
}

void im_container::on_get_user_info(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_user_info(_seq, _params.get_value_as_string("sn"));
}

void im_container::on_set_privacy_settings(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
    {
        wim::privacy_settings settings;
        settings.set_access_value(_params.get_value_as_string("group"), _params.get_value_as_enum<privacy_access_right>("value"));

        im->set_privacy_settings(_seq, std::move(settings));
    }
}

void im_container::on_get_privacy_settings(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_privacy_settings(_seq);
}

void im_container::on_get_user_last_seen(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);

    if (im)
    {
        std::vector<std::string> ids;
        if (auto arr = _params.get_value_as_array("ids"))
        {
            const auto size = arr->size();
            ids.reserve(size);
            for (int i = 0; i < size; ++i)
                ids.emplace_back(arr->get_at(i)->get_as_string());
        }

        im->get_user_last_seen(_seq, ids);
    }
}

void im_container::on_nickname_check(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (im)
        im->on_nickname_check(_seq, _params.get_value_as_string("nick"), _params.get_value_as_bool("set_nick"));
}

void im_container::on_get_common_chats(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_get_common_chats(_seq, _params.get_value_as_string("sn"));
}

void im_container::on_reset_connection(const int64_t _seq, coll_helper& _params)
{
    g_core->reset_connection();
}

void im_container::on_update_recent_avatars_size(const int64_t _seq, coll_helper& _params)
{
    if (_params.is_value_exist("size"))
        g_core->set_recent_avatars_size(_params.get_value_as_int("size"));
}

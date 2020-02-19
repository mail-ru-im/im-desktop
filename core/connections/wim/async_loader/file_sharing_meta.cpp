#include "stdafx.h"

#include "file_sharing_meta.h"
#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../tools/json_helper.h"

core::wim::file_sharing_meta::file_sharing_meta(std::string && _url)
    : file_url_(std::move(_url))
{
}

void core::wim::file_sharing_meta::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_status(rapidjson::Type::kObjectType);
    node_status.AddMember("code", status_code_, _a);
    _node.AddMember("status", std::move(node_status), _a);

    rapidjson::Value node_result(rapidjson::Type::kObjectType);

    rapidjson::Value node_info(rapidjson::Type::kObjectType);
    info_.serialize(node_info, _a);
    node_result.AddMember("info", std::move(node_info), _a);

    rapidjson::Value node_extra(rapidjson::Type::kObjectType);
    extra_.serialize(node_extra, _a);
    node_result.AddMember("extra", std::move(node_extra), _a);

    if (previews_)
    {
        rapidjson::Value node_previews(rapidjson::Type::kObjectType);
        previews_->serialize(node_previews, _a);
        node_result.AddMember("previews", std::move(node_previews), _a);
    }

    if (local_)
    {
        rapidjson::Value node_local(rapidjson::Type::kObjectType);
        local_->serialize(node_local, _a);
        _node.AddMember("local", std::move(node_local), _a);
    }

    _node.AddMember("result", std::move(node_result), _a);
}

core::wim::file_sharing_meta_uptr core::wim::file_sharing_meta::parse_json(InOut char* _json, std::string_view _uri)
{
    rapidjson::Document doc;

    auto meta = std::make_unique<file_sharing_meta>(std::string(_uri));

    if (doc.ParseInsitu(_json).HasParseError())
        return meta;

    if (const auto iter_status = doc.FindMember("status"); iter_status != doc.MemberEnd() && iter_status->value.IsObject())
    {
        if (!tools::unserialize_value(iter_status->value, "code", meta->status_code_))
            return meta;
    }
    else
    {
        return meta;
    }

    std::stringstream log;
    log << "file_sharing_meta: \n";
    log << "status: " << meta->status_code_ << '\n';

    if (meta->status_code_ != 200)
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    const auto iter_result = doc.FindMember("result");
    if (iter_result == doc.MemberEnd() || !iter_result->value.IsObject())
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    const auto iter_info = iter_result->value.FindMember("info");
    if (iter_info == iter_result->value.MemberEnd() || !iter_info->value.IsObject())
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    meta->info_.unserialize(iter_info->value);

    const auto iter_extra = iter_result->value.FindMember("extra");
    if (iter_extra == iter_result->value.MemberEnd() || !iter_extra->value.IsObject())
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    meta->extra_.unserialize(iter_extra->value);

    if (const auto iter_local = doc.FindMember("local"); iter_local != doc.MemberEnd() && iter_local->value.IsObject())
    {
        meta->local_ = fs_meta_local{};
        meta->local_->unserialize(iter_local->value);
    }


    if (const auto iter_previews = iter_result->value.FindMember("previews"); iter_previews != iter_result->value.MemberEnd() && iter_previews->value.IsObject())
    {
        meta->previews_ = fs_meta_previews{};
        meta->previews_->unserialize(iter_previews->value);
    }

    log << "mime: " << meta->info_.mime_ << '\n';
    log << "filename: " << meta->info_.file_name_ << '\n';
    log << "filesize: " << meta->info_.file_size_ << '\n';
    log << "is_previewable: " << meta->info_.is_previewable_ << '\n';
    log << "file_type: " << meta->extra_.file_type_ << '\n';

    if (meta->extra_.ptt_)
    {
        log << "is_recognized: " << meta->extra_.ptt_->is_recognized_ << '\n';
        log << "duration: " << meta->extra_.ptt_->duration_ << '\n';
        if (!meta->extra_.ptt_->language_.empty())
            log << "language: " << meta->extra_.ptt_->language_ << '\n';
    }
    else if (meta->extra_.video_)
    {
        log << "duration: " << meta->extra_.video_->duration_ << '\n';
        log << "has_audio: " << meta->extra_.video_->has_audio_ << '\n';
    }
    else if (meta->extra_.audio_)
    {
        log << "duration: " << meta->extra_.audio_->duration_ << '\n';
    }

    g_core->write_string_to_network_log(log.str());

    return meta;
}

void core::wim::fs_meta_info::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("has_previews", is_previewable_, _a);
    _node.AddMember("dlink", dlink_, _a);
    _node.AddMember("mime", mime_, _a);
    _node.AddMember("file_name", file_name_, _a);
    _node.AddMember("file_size", file_size_, _a);
    _node.AddMember("md5", md5_, _a);
}

void core::wim::fs_meta_info::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "has_previews", is_previewable_);
    core::tools::unserialize_value(_node, "dlink", dlink_);
    core::tools::unserialize_value(_node, "mime", mime_);
    core::tools::unserialize_value(_node, "file_name", file_name_);
    core::tools::unserialize_value(_node, "file_size", file_size_);
    core::tools::unserialize_value(_node, "md5", md5_);
}

void core::wim::fs_meta_ptt::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("duration", duration_, _a);
    _node.AddMember("is_recognized", is_recognized_, _a);
    _node.AddMember("language", language_, _a);
    _node.AddMember("text", text_, _a);
}

void core::wim::fs_meta_ptt::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "duration", duration_);
    core::tools::unserialize_value(_node, "is_recognized", is_recognized_);
    core::tools::unserialize_value(_node, "language", language_);
    core::tools::unserialize_value(_node, "text", text_);
}

void core::wim::fs_meta_video::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
     _node.AddMember("duration", duration_, _a);
     _node.AddMember("has_audio", has_audio_, _a);
}

void core::wim::fs_meta_video::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "duration", duration_);
    core::tools::unserialize_value(_node, "has_audio", has_audio_);
}

void core::wim::fs_meta_audio::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("duration", duration_, _a);
}

void core::wim::fs_meta_audio::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "duration", duration_);
}

void core::wim::fs_extra_meta::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("file_type", file_type_, _a);
    if (ptt_)
    {
        rapidjson::Value ptt(rapidjson::Type::kObjectType);
        ptt_->serialize(ptt, _a);
        _node.AddMember("ptt", std::move(ptt), _a);
    }
    else if (video_)
    {
        rapidjson::Value video(rapidjson::Type::kObjectType);
        video_->serialize(video, _a);
        _node.AddMember("video", std::move(video), _a);
    }
    else if (audio_)
    {
        rapidjson::Value audio(rapidjson::Type::kObjectType);
        audio_->serialize(audio, _a);
        _node.AddMember("audio", std::move(audio), _a);
    }
}

void core::wim::fs_extra_meta::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "file_type", file_type_);

    if (const auto iter_ptt = _node.FindMember("ptt"); iter_ptt != _node.MemberEnd() && iter_ptt->value.IsObject())
    {
        ptt_ = fs_meta_ptt{};
        ptt_->unserialize(iter_ptt->value);
    }
    else if (const auto iter_video = _node.FindMember("video"); iter_video != _node.MemberEnd() && iter_video->value.IsObject())
    {
        video_ = fs_meta_video{};
        video_->unserialize(iter_video->value);
    }
    else if (const auto iter_audio = _node.FindMember("audio"); iter_audio != _node.MemberEnd() && iter_audio->value.IsObject())
    {
        audio_ = fs_meta_audio{};
        audio_->unserialize(iter_audio->value);
    }
}

void core::wim::fs_meta_previews::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    //TODO
}

void core::wim::fs_meta_previews::unserialize(const rapidjson::Value& _node)
{
    //TODO
}

void core::wim::fs_meta_local::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("local_path", local_path_, _a);
    _node.AddMember("last_modified", last_modified_, _a);
}

void core::wim::fs_meta_local::unserialize(const rapidjson::Value& _node)
{
    core::tools::unserialize_value(_node, "local_path", local_path_);
    core::tools::unserialize_value(_node, "last_modified", last_modified_);
}

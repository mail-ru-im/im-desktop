#include "stdafx.h"

#include "file_sharing_meta.h"
#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../tools/json_helper.h"

core::wim::file_sharing_meta::file_sharing_meta(const std::string& _url)
    : status_code_(0)
    , file_url_(_url)
    , file_size_(0)
    , recognize_(false)
    , is_previewable_(false)
    , got_audio_(true)
    , duration_(0)
    , last_modified_(0)
{
}

void core::wim::file_sharing_meta::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("status", status_code_, _a);
    rapidjson::Value node_flist(rapidjson::Type::kArrayType);

    node_flist.Reserve(1, _a);
    rapidjson::Value node_file(rapidjson::Type::kObjectType);
    node_file.AddMember("dlink", file_download_url_, _a);
    node_file.AddMember("mime", mime_, _a);
    node_file.AddMember("filename", file_name_short_, _a);
    node_file.AddMember("iphone", file_mini_preview_url_, _a);
    node_file.AddMember("recognize", recognize_, _a);
    node_file.AddMember("is_previewable", is_previewable_, _a);
    node_file.AddMember("local_path", local_path_, _a);
    node_file.AddMember("last_modified", last_modified_, _a);
    node_file.AddMember("static800", file_full_preview_url_, _a);
    node_file.AddMember("filesize", std::to_string(file_size_), _a);
    node_file.AddMember("duration", duration_, _a);
    node_file.AddMember("language", language_, _a);
    node_file.AddMember("got_audio", got_audio_, _a);

    node_flist.PushBack(std::move(node_file), _a);
    _node.AddMember("file_list", std::move(node_flist), _a);
}

core::wim::file_sharing_meta_uptr core::wim::file_sharing_meta::parse_json(InOut char* _json, const std::string& _uri)
{
    rapidjson::Document doc;

    auto meta = std::make_unique<file_sharing_meta>(_uri);

    if (doc.ParseInsitu(_json).HasParseError())
        return meta;

    if (!tools::unserialize_value(doc, "status", meta->status_code_))
        return meta;

    std::stringstream log;
    log << "file_sharing_meta: \n";
    log << "status: " << meta->status_code_ << '\n';

    if (meta->status_code_ != 200)
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    auto iter_flist = doc.FindMember("file_list");
    if (iter_flist == doc.MemberEnd() || !iter_flist->value.IsArray())
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    if (iter_flist->value.Empty())
    {
        g_core->write_string_to_network_log(log.str());
        return meta;
    }

    auto iter_flist0 = iter_flist->value.Begin();
    tools::unserialize_value(*iter_flist0, "dlink", meta->file_download_url_);
    tools::unserialize_value(*iter_flist0, "mime", meta->mime_);
    tools::unserialize_value(*iter_flist0, "filename", meta->file_name_short_);
    tools::unserialize_value(*iter_flist0, "iphone", meta->file_mini_preview_url_);
    tools::unserialize_value(*iter_flist0, "recognize", meta->recognize_);
    tools::unserialize_value(*iter_flist0, "is_previewable", meta->is_previewable_);
    tools::unserialize_value(*iter_flist0, "local_path", meta->local_path_);
    tools::unserialize_value(*iter_flist0, "last_modified", meta->last_modified_);
    tools::unserialize_value(*iter_flist0, "language", meta->language_);
    if (!tools::unserialize_value(*iter_flist0, "duration", meta->duration_))
        meta->duration_ = 0;

    tools::unserialize_value(*iter_flist0, "got_audio", meta->got_audio_);

    if (!tools::unserialize_value(*iter_flist0, "static800", meta->file_full_preview_url_))
        if (!tools::unserialize_value(*iter_flist0, "static600", meta->file_full_preview_url_))
            tools::unserialize_value(*iter_flist0, "static194", meta->file_full_preview_url_);

    auto iter_file_size = iter_flist0->FindMember("filesize");
    if (iter_file_size != iter_flist0->MemberEnd())
        meta->file_size_ = strtoll(iter_file_size->value.GetString(), 0, 0);

    log << "mime: " << meta->mime_ << '\n';
    log << "filename: " << meta->file_name_short_ << '\n';
    log << "filesize: " << meta->file_size_ << '\n';
    log << "is_previewable: " << meta->is_previewable_ << '\n';
    log << "recognize: " << meta->recognize_ << '\n';
    log << "duration: " << meta->duration_ << '\n';
    log << "got_audio: " << meta->got_audio_ << '\n';
    if (!meta->language_.empty())
        log << "language: " << meta->language_ << '\n';

    g_core->write_string_to_network_log(log.str());

    return meta;
}

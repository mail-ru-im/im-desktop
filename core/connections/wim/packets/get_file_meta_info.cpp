#include "stdafx.h"
#include "get_file_meta_info.h"
#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../http_request.h"
#include "../loader/web_file_info.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


get_file_meta_info::get_file_meta_info(
    const wim_packet_params& _params,
    const web_file_info& _info)
    :    wim_packet(_params),
    info_(std::make_unique<web_file_info>(_info))
{
}


get_file_meta_info::~get_file_meta_info()
{
}


int32_t get_file_meta_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::files_host) << std::string_view("/getinfo?file_id=") << info_->get_file_id();
    ss_url << "&r=" <<  core::tools::system::generate_guid();

    _request->set_url(ss_url.str());
    _request->set_normalized_url("filesDownloadMetadata");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("file_id");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_file_meta_info::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(size)).HasParseError())
        return wpie_error_parse_response;

    if (const auto iter_status = doc.FindMember("status"); iter_status == doc.MemberEnd() || !iter_status->value.IsInt())
        return wpie_http_parse_response;
    else
        status_code_ = iter_status->value.GetInt();

    std::stringstream log;
    log << "file_meta_info: \n";
    log << "status: " << status_code_ << '\n';

    if (status_code_ == 200)
    {
        const auto iter_flist = doc.FindMember("file_list");
        if (iter_flist == doc.MemberEnd() || !iter_flist->value.IsArray())
            return wpie_error_parse_response;

        if (iter_flist->value.Empty())
            return wpie_error_parse_response;

        const auto iter_flist0 = iter_flist->value.Begin();

        if (const auto iter_is_previewable = iter_flist0->FindMember("is_previewable"); iter_is_previewable != iter_flist0->MemberEnd() && iter_is_previewable->value.IsInt())
            info_->set_is_previewable(!!iter_is_previewable->value.GetInt());

        if (const auto iter_dlink = iter_flist0->FindMember("dlink"); iter_dlink != iter_flist0->MemberEnd() && iter_dlink->value.IsString())
            info_->set_file_dlink(rapidjson_get_string(iter_dlink->value));

        if (const auto iter_mime = iter_flist0->FindMember("mime"); iter_mime != iter_flist0->MemberEnd() && iter_mime->value.IsString())
            info_->set_mime(rapidjson_get_string(iter_mime->value));

        if (const auto iter_md5 = iter_flist0->FindMember("md5"); iter_md5 != iter_flist0->MemberEnd() && iter_md5->value.IsString())
            info_->set_md5(rapidjson_get_string(iter_md5->value));

        if (const auto iter_file_size = iter_flist0->FindMember("filesize"); iter_file_size != iter_flist0->MemberEnd() && iter_file_size->value.IsString())
            info_->set_file_size(strtol(iter_file_size->value.GetString(), 0, 0));

        if (const auto iter_file_name = iter_flist0->FindMember("filename"); iter_file_name != iter_flist0->MemberEnd() && iter_file_name->value.IsString())
            info_->set_file_name_short(rapidjson_get_string(iter_file_name->value));

        if (const auto iter_iphone = iter_flist0->FindMember("iphone"); iter_iphone != iter_flist0->MemberEnd() && iter_iphone->value.IsString())
            info_->set_file_preview_2k(rapidjson_get_string(iter_iphone->value));

        if (const auto iter_file_preview800 = iter_flist0->FindMember("static800"); iter_file_preview800 != iter_flist0->MemberEnd() && iter_file_preview800->value.IsString())
        {
            info_->set_file_preview(rapidjson_get_string(iter_file_preview800->value));
        }
        else
        {

            if (const auto iter_file_preview600 = iter_flist0->FindMember("static600"); iter_file_preview600 != iter_flist0->MemberEnd() && iter_file_preview600->value.IsString())
            {
                info_->set_file_preview(rapidjson_get_string(iter_file_preview600->value));
            }
            else
            {
                if (const auto iter_file_preview194 = iter_flist0->FindMember("static194"); iter_file_preview194 != iter_flist0->MemberEnd() && iter_file_preview194->value.IsString())
                    info_->set_file_preview(rapidjson_get_string(iter_file_preview194->value));
            }
        }

        log << "mime: " << info_->get_mime() << '\n';
        log << "filename: " << info_->get_file_name_short() << '\n';
        log << "filesize: " << info_->get_file_size() << '\n';
        log << "is_previewable: " << info_->is_previewable() << '\n';
    }

    g_core->write_string_to_network_log(log.str());

    return 0;
}

int32_t get_file_meta_info::on_http_client_error()
{
    if (http_code_ == 404)
        return wpie_error_metainfo_not_found;

    return wpie_client_http_error;
}

const web_file_info& get_file_meta_info::get_info() const
{
    return *info_;
}

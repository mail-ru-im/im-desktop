#include "stdafx.h"
#include "web_file_info.h"

#include "../../../corelib/collection_helper.h"


using namespace core;
using namespace wim;

const std::string& web_file_info::get_file_url() const
{
    return file_url_;
}

void web_file_info::set_file_url(std::string _url)
{
    file_url_ = std::move(_url);
}

void web_file_info::set_file_size(int64_t _size)
{
    file_size_ = _size;
}

int64_t web_file_info::get_file_size() const
{
    return file_size_;
}

void web_file_info::set_file_name(std::wstring _name)
{
    file_name_ = std::move(_name);
}

const std::wstring& web_file_info::get_file_name() const
{
    return file_name_;
}

void web_file_info::set_bytes_transfer(int64_t _bytes)
{
    bytes_transfer_ = _bytes;
}

int64_t web_file_info::get_bytes_transfer() const
{
    return bytes_transfer_;
}


bool web_file_info::is_previewable() const
{
    return is_previewable_;
}

void web_file_info::set_is_previewable(bool _is_previewable)
{
    is_previewable_ = _is_previewable;
}

bool web_file_info::is_played() const
{
    return played_;
}

void web_file_info::set_played(bool _played)
{
    played_ = _played;
}

void web_file_info::set_file_dlink(std::string _dlink)
{
    file_dlink_ = std::move(_dlink);
}

const std::string& web_file_info::get_file_dlink() const
{
    return file_dlink_;
}

void web_file_info::set_mime(std::string _mime)
{
    mime_ = _mime;
}

const std::string& web_file_info::get_mime() const
{
    return mime_;
}

void web_file_info::set_md5(std::string _md5)
{
    md5_ = std::move(_md5);
}

const std::string& web_file_info::get_md5() const
{
    return md5_;
}

void web_file_info::set_file_preview(std::string _val)
{
    file_preview_ = std::move(_val);
}

const std::string& web_file_info::get_file_preview() const
{
    return file_preview_;
}

void web_file_info::set_file_preview_2k(std::string _val)
{
    file_preview_2k_ = std::move(_val);
}

const std::string& web_file_info::get_file_preview_2k() const
{
    return file_preview_2k_;
}

const std::string& web_file_info::get_file_id() const
{
    return file_id_;
}

void web_file_info::set_file_id(std::string _file_id)
{
    file_id_ = std::move(_file_id);
}

void web_file_info::set_file_name_short(std::string _file_name_short)
{
    file_name_short_ = std::move(_file_name_short);
}

const std::string& web_file_info::get_file_name_short() const
{
    return file_name_short_;
}

void web_file_info::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);

    coll.set_value_as_int64("file_size", file_size_);
    coll.set_value_as_string("file_name", core::tools::from_utf16(file_name_));
    coll.set_value_as_string("file_name_short", file_name_short_);
    coll.set_value_as_string("file_url", file_url_);
    coll.set_value_as_bool("is_previewable", is_previewable_);
    coll.set_value_as_bool("played", played_);
    coll.set_value_as_string("file_dlink", file_dlink_);
    coll.set_value_as_string("mime", mime_);
    coll.set_value_as_string("md5", md5_);
    coll.set_value_as_string("file_preview", file_preview_);
    coll.set_value_as_string("file_preview_2k", file_preview_2k_);
    coll.set_value_as_string("file_id", file_id_);
    coll.set_value_as_int64("bytes_transfer", bytes_transfer_);
}

enum info_fileds
{
    file_name            = 0,
    file_size            = 1,
    file_name_short        = 2,
    file_url            = 3,
    is_previewable        = 4,
    file_dlink            = 5,
    mime                = 6,
    md5                    = 7,
    file_preview        = 8,
    file_preview_2k        = 9,
    file_id                = 10,
    played              = 11,
};

void web_file_info::serialize(core::tools::binary_stream& _bs) const
{
    using namespace tools;

    tlvpack pack;

    pack.push_child(tlv(info_fileds::file_name, from_utf16(file_name_)));
    pack.push_child(tlv(info_fileds::file_size, file_size_));
    pack.push_child(tlv(info_fileds::file_name_short, file_name_short_));
    pack.push_child(tlv(info_fileds::file_url, file_url_));
    pack.push_child(tlv(info_fileds::is_previewable, is_previewable_));
    pack.push_child(tlv(info_fileds::file_dlink, file_dlink_));
    pack.push_child(tlv(info_fileds::mime, mime_));
    pack.push_child(tlv(info_fileds::md5, md5_));
    pack.push_child(tlv(info_fileds::file_preview, file_preview_));
    pack.push_child(tlv(info_fileds::file_preview_2k, file_preview_2k_));
    pack.push_child(tlv(info_fileds::file_id, file_id_));
    pack.push_child(tlv(info_fileds::played, played_));

    pack.serialize(_bs);
}

bool web_file_info::unserialize(const core::tools::binary_stream& _bs)
{
    using namespace tools;

    tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    auto tlv_file_name = pack.get_item(info_fileds::file_name);
    auto tlv_file_size = pack.get_item(info_fileds::file_size);
    auto tlv_file_name_short = pack.get_item(info_fileds::file_name_short);
    auto tlv_file_url = pack.get_item(info_fileds::file_url);
    auto tlv_is_previewable = pack.get_item(info_fileds::is_previewable);
    auto tlv_file_dlink = pack.get_item(info_fileds::file_dlink);
    auto tlv_mime = pack.get_item(info_fileds::mime);
    auto tlv_md5 = pack.get_item(info_fileds::md5);
    auto tlv_file_preview = pack.get_item(info_fileds::file_preview);
    auto tlv_file_preview_2k = pack.get_item(info_fileds::file_preview_2k);
    auto tlv_file_id = pack.get_item(info_fileds::file_id);
    auto tlv_played = pack.get_item(info_fileds::played);

    if (
        !tlv_file_name ||
        !tlv_file_size ||
        !tlv_file_name_short ||
        !tlv_file_url ||
        !tlv_is_previewable ||
        !tlv_file_dlink ||
        !tlv_mime ||
        !tlv_md5 ||
        !tlv_file_preview ||
        !tlv_file_preview_2k ||
        !tlv_file_id ||
        !tlv_played)
    {
        assert(false);
        return false;
    }

    file_name_ = from_utf8(tlv_file_name->get_value<std::string>());
    file_size_ = tlv_file_size->get_value<int64_t>();
    file_name_short_ = tlv_file_name_short->get_value<std::string>();
    file_url_ = tlv_file_url->get_value<std::string>();
    is_previewable_ = tlv_is_previewable->get_value<bool>();
    played_ = tlv_played->get_value<bool>();
    file_dlink_ = tlv_file_dlink->get_value<std::string>();
    mime_ = tlv_mime->get_value<std::string>();
    md5_ = tlv_md5->get_value<std::string>();
    file_preview_ = tlv_file_preview->get_value<std::string>();
    file_preview_2k_     = tlv_file_preview_2k->get_value<std::string>();
    file_id_ = tlv_file_id->get_value<std::string>();

    return true;
}

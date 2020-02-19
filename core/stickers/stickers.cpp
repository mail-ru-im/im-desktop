#include "stdafx.h"

#include "stickers.h"
#include "suggests.h"

#include "../../../corelib/enumerations.h"

#include "../tools/system.h"
#include "../tools/json_helper.h"

#include "../async_task.h"
#include "../../common.shared/loader_errors.h"
#include "../../common.shared/config/config.h"

#include "connections/urls_cache.h"

namespace core
{
    namespace stickers
    {
        sticker_size string_size_2_size(std::string_view _size)
        {
            if (_size == "small")
            {
                return sticker_size::small;
            }
            else if (_size == "medium")
            {
                return sticker_size::medium;
            }
            else if (_size == "large" || _size == "xlarge" || _size == "xxlarge")
            {
                return sticker_size::large;
            }
            else
            {
                assert(!"unknown type");
                return sticker_size::large;
            }
        }

        constexpr std::wstring_view stickers_meta_file_name() noexcept { return L"meta.js"; }


        //////////////////////////////////////////////////////////////////////////
        // class sticker
        //////////////////////////////////////////////////////////////////////////
        bool sticker::unserialize(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            file_sharing_id_ = rapidjson_get_string(_node);

            if (const auto it = _emojis.find(file_sharing_id_); it != _emojis.end())
                emojis_ = it->second;

            return true;
        }

        bool sticker::unserialize(const rapidjson::Value& _node)
        {
            if (!tools::unserialize_value(_node, "fileId", file_sharing_id_))
                return false;

            if (const auto it = _node.FindMember("emoji"); it != _node.MemberEnd() && it->value.IsArray())
            {
                emojis_.clear();
                emojis_.reserve(it->value.Size());
                for (const auto& e : it->value.GetArray())
                {
                    if (e.IsString())
                        emojis_.push_back(rapidjson_get_string(e));
                }
            }
            return true;
        }

        int32_t sticker::get_id() const
        {
            return id_;
        }

        const std::string& sticker::fs_id() const
        {
            return file_sharing_id_;
        }

        const size_map& sticker::get_sizes() const
        {
            return sizes_;
        }

        const emoji_vector &sticker::get_emojis() const
        {
            return emojis_;
        }

        //////////////////////////////////////////////////////////////////////////
        // class set_icon
        //////////////////////////////////////////////////////////////////////////
        set_icon::set_icon()
            : size_(set_icon_size::invalid)
        {

        }

        set_icon::set_icon(set_icon_size _size, std::string _url)
            :    size_(_size),
            url_(std::move(_url))
        {

        }

        set_icon_size set_icon::get_size() const
        {
            return size_;
        }

        const std::string& set_icon::get_url() const
        {
            return url_;
        }



        //////////////////////////////////////////////////////////////////////////
        // class set
        //////////////////////////////////////////////////////////////////////////

        int32_t set::get_id() const
        {
            return id_;
        }

        void set::set_id(int32_t _id)
        {
            id_ = _id;
        }

        const std::string& set::get_name() const
        {
            return name_;
        }

        void set::set_name(std::string&& _name)
        {
            name_ = std::move(_name);
        }

        bool set::is_show() const
        {
            return show_;
        }

        void set::set_show(bool _show)
        {
            show_ = _show;
        }

        bool set::is_purchased() const
        {
            return purchased_;
        }

        void set::set_purchased(const bool _purchased)
        {
            purchased_ = _purchased;
        }

        bool set::is_user() const
        {
            return user_;
        }

        void set::set_user(const bool _user)
        {
            user_ = _user;
        }

        const std::string& set::get_store_id() const
        {
            return store_id_;
        }

        void set::set_store_id(std::string&& _store_id)
        {
            store_id_ = std::move(_store_id);
        }

        void set::set_description(std::string&& _description)
        {
            description_ = std::move(_description);
        }

        const std::string& set::get_description() const
        {
            return description_;
        }

        const std::string& set::get_subtitle() const
        {
            return subtitle_;
        }

        void set::set_subtitle(std::string&& _subtitle)
        {
            subtitle_ = std::move(_subtitle);
        }

        const std::string& set::get_large_icon_url() const
        {
            return large_icon_url_;
        }

        void set::set_large_icon_url(std::string&& _url)
        {
            large_icon_url_ = std::move(_url);
        }

        void set::put_icon(set_icon&& _icon)
        {
            const auto size = _icon.get_size();
            icons_[size] = std::move(_icon);
        }

        const set_icon& set::get_icon(stickers::set_icon_size _size) const
        {
            if (const auto iter = icons_.find(_size); iter != icons_.end())
                return iter->second;

            assert(!"invalid icon size");
            static const auto empty = set_icon();
            return empty;
        }

        const icons_map& set::get_icons() const
        {
            return icons_;
        }

        const stickers_vector& set::get_stickers() const
        {
            return stickers_;
        }

        bool set::unserialize(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            if (int id = 0; tools::unserialize_value(_node, "id", id))
                set_id(id);
            else
                return false;

            if (std::string name; tools::unserialize_value(_node, "title", name))
                set_name(std::move(name));

            if (std::string name; tools::unserialize_value(_node, "name", name)) // for store/showcase
                set_name(std::move(name));

            if (bool show = false; tools::unserialize_value(_node, "is_enabled", show))
                set_show(show);

            if (bool purchased = true; tools::unserialize_value(_node, "purchased", purchased))
                set_purchased(purchased);

            if (bool usersticker = false; tools::unserialize_value(_node, "usersticker", usersticker))
                set_user(usersticker);

            if (std::string store_id; tools::unserialize_value(_node, "store_id", store_id))
                set_store_id(std::move(store_id));

            if (std::string description; tools::unserialize_value(_node, "description", description))
                set_description(std::move(description));

            if (std::string subtitle; tools::unserialize_value(_node, "subtitle", subtitle))
                set_subtitle(std::move(subtitle));

            if (std::string url; tools::unserialize_value(_node, "icons", url))
                set_large_icon_url(std::move(url));

            if (const auto iter_icons = _node.FindMember("contentlist_sticker_picker_icon"); iter_icons != _node.MemberEnd() && iter_icons->value.IsObject())
            {
                if (std::string xsmall; tools::unserialize_value(iter_icons->value, "xsmall", xsmall))
                    put_icon(set_icon(set_icon_size::_20, std::move(xsmall)));

                if (std::string small; tools::unserialize_value(iter_icons->value, "small", small))
                    put_icon(set_icon(set_icon_size::_32, std::move(small)));

                if (std::string medium; tools::unserialize_value(iter_icons->value, "medium", medium))
                    put_icon(set_icon(set_icon_size::_48, std::move(medium)));

                if (std::string large; tools::unserialize_value(iter_icons->value, "large", large))
                    put_icon(set_icon(set_icon_size::_64, std::move(large)));
            }

            if (std::string sp; tools::unserialize_value(_node, "stiker-picker", sp))
            {
                put_icon(set_icon(set_icon_size::_20, sp));
                put_icon(set_icon(set_icon_size::_32, sp));
                put_icon(set_icon(set_icon_size::_48, sp));
                put_icon(set_icon(set_icon_size::_64, sp));
            }

            if (const auto iter_stickers = _node.FindMember("stickers"); iter_stickers != _node.MemberEnd() && iter_stickers->value.IsArray())
            {
                for (const auto& sticker : iter_stickers->value.GetArray())
                {
                    auto new_sticker = std::make_shared<stickers::sticker>();

                    if (!new_sticker->unserialize(sticker, _emojis))
                        continue;

                    stickers_.push_back(std::move(new_sticker));
                }
            }
            else if (const auto iter_content = _node.FindMember("content"); iter_content != _node.MemberEnd() && iter_content->value.IsArray())
            {
                for (const auto& sticker : iter_content->value.GetArray())
                {
                    auto new_sticker = std::make_shared<stickers::sticker>();

                    if (!new_sticker->unserialize(sticker))
                        continue;

                    stickers_.push_back(std::move(new_sticker));
                }
            }

            return true;
        }

        bool set::unserialize(const rapidjson::Value& _node)
        {
            return unserialize(_node, {});
        }


        //////////////////////////////////////////////////////////////////////////
        // download_task
        //////////////////////////////////////////////////////////////////////////
        download_task::download_task(
            std::string _source_url,
            std::string _endpoint,
            std::wstring _dest_file,
            int32_t _set_id = -1,
            int32_t _sticker_id = -1,
            std::string _fs_id = std::string(),
            sticker_size _size = sticker_size::min)
            : source_url_(std::move(_source_url))
            , endpoint_(std::move(_endpoint))
            , dest_file_(std::move(_dest_file))
            , set_id_(_set_id)
            , sticker_id_(_sticker_id)
            , size_(_size)
            , fs_id_(std::move(_fs_id))
        {

        }

        const std::string& download_task::get_source_url() const
        {
            return source_url_;
        }

        const std::string& download_task::get_endpoint() const
        {
            return endpoint_;
        }

        const std::wstring& download_task::get_dest_file() const
        {
            return dest_file_;
        }

        int32_t download_task::get_set_id() const
        {
            return set_id_;
        }

        int32_t download_task::get_sticker_id() const
        {
            return sticker_id_;
        }

        const std::string& download_task::get_fs_id() const
        {
            return fs_id_;
        }

        sticker_size download_task::get_size() const
        {
            return size_;
        }

        std::wstring g_stickers_path;

        //////////////////////////////////////////////////////////////////////////
        // class cache
        //////////////////////////////////////////////////////////////////////////
        cache::cache(const std::wstring& _stickers_path)
            : suggests_(std::make_unique<suggests>())
        {
            g_stickers_path = _stickers_path;
        }

        cache::~cache()
        {
        }

        bool cache::parse(core::tools::binary_stream& _data, bool _insitu, bool& _up_todate)
        {
            _up_todate = false;

            rapidjson::Document doc;
            const auto& parse_result = (_insitu ? doc.ParseInsitu(_data.read(_data.available())) : doc.Parse(_data.read(_data.available())));
            if (parse_result.HasParseError())
                return false;

            auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd())
                return false;

            if (iter_status->value.IsInt())
            {
                auto status_code_ = iter_status->value.GetInt();
                if (status_code_ != 200)
                {
                    assert(false);
                    return false;
                }
            }
            else if (iter_status->value.IsString())
            {
                const auto status = rapidjson_get_string_view(iter_status->value);

                if (status == "UP_TO_DATE")
                {
                    _up_todate = true;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }


            if (!tools::unserialize_value(doc, "md5", md5_))
                return false;

            const auto data_iter = doc.FindMember("data");
            if (data_iter == doc.MemberEnd() || data_iter->value.IsNull() || !data_iter->value.IsObject())
                return true;

            std::string template_url_preview, template_url_original, template_url_send;
            tools::unserialize_value(data_iter->value, "template_url_preview", template_url_preview);
            tools::unserialize_value(data_iter->value, "template_url_original", template_url_original);
            tools::unserialize_value(data_iter->value, "template_url_send", template_url_send);
            update_template_urls(std::move(template_url_preview), std::move(template_url_original), std::move(template_url_send));

            const auto iter_packs = data_iter->value.FindMember("packs");
            if (iter_packs == data_iter->value.MemberEnd() || iter_packs->value.IsNull() || !iter_packs->value.IsArray())
                return true;

            suggests_->unserialize(data_iter->value);

            emoji_map emoji;

            for (const auto& suggest : suggests_->get_suggests())
                for (const auto& st : suggest.info_)
                    if (st.is_emoji())
                        emoji[st.fs_id_].push_back(suggest.emoji_);

            sets_.clear();

            for (const auto& set : iter_packs->value.GetArray())
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize(set, emoji))
                {
                    assert(!"error reading stickers set");
                    continue;
                }

                sets_.push_back(std::move(new_set));
            }

            return true;
        }

        bool cache::parse_store(core::tools::binary_stream& _data)
        {
            rapidjson::Document doc;
            const auto& parse_result = doc.ParseInsitu(_data.read(_data.available()));
            if (parse_result.HasParseError())
                return false;

            const auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd())
                return false;

            if (!iter_status->value.IsInt())
                return false;

            const auto status_code_ = iter_status->value.GetInt();
            if (status_code_ != 200)
            {
                assert(false);
                return false;
            }

            const auto iter_data = doc.FindMember("data");
            if (iter_data == doc.MemberEnd() || !iter_data->value.IsObject())
                return false;

            const auto iter_sets = iter_data->value.FindMember("top");
            if (iter_sets == iter_data->value.MemberEnd() || !iter_sets->value.IsArray())
                return false;

            store_.clear();

            for (const auto& set : iter_sets->value.GetArray())
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize(set, {}))
                {
                    assert(!"error reading stickers set");
                    continue;
                }

                store_.push_back(std::move(new_set));
            }

            return true;
        }

        bool cache::is_meta_icons_exist() const
        {
            for (const auto& set : sets_)
            {
                if (!set->is_show() || !set->is_purchased())
                    continue;

                for (const auto& [_, icon] : set->get_icons())
                {
                    if (std::wstring icon_file = get_set_icon_path(*set, icon); !core::tools::system::is_exist(icon_file))
                        return false;
                }
            }

            return true;
        }

        std::wstring cache::get_meta_file_name() const
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << stickers_meta_file_name();
            return ss_out.str();
        }

        std::wstring cache::get_set_icon_path(const set& _set, const set_icon& _icon)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << _set.get_id() << L"/icons/_icon_" << _icon.get_size() << L".png";

            return ss_out.str();
        }

        std::wstring cache::get_set_big_icon_path(const int32_t _set_id)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << _set_id << L"/icons/_icon_xset.png";

            return ss_out.str();
        }

        std::wstring cache::get_sticker_path(int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, sticker_size _size)
        {
            std::wstringstream ss_out;

            ss_out << g_stickers_path << L'/';
            if (_fs_id.empty())
                ss_out << _set_id << L'/' << _sticker_id;
            else
                ss_out << tools::from_utf8(_fs_id);

            ss_out << L'/' << _size << L".png";

            return ss_out.str();
        }

        std::wstring cache::get_sticker_path(const set& _set, const sticker& _sticker, sticker_size _size)
        {
            return get_sticker_path(_set.get_id(), _sticker.get_id(), _sticker.fs_id() , _size);
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const core::sticker_size _size) const
        {
            std::stringstream ss_size;

            ss_size << _size;

            return make_sticker_url(_set_id, _sticker_id, _fs_id, ss_size.str());
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const std::string& _size) const
        {
            std::stringstream ss_url;

            if (_fs_id.empty())
                ss_url << "https://" << config::get().url(config::urls::cicq_com) << "/store/stickers/" << _set_id << '/' << _sticker_id << '/' << _size << ".png";
            else
                ss_url << urls::get_url(urls::url_type::files_preview) << "/max/sticker_" << _size << '/' << _fs_id;

            return ss_url.str();
        }

        std::string cache::make_big_icon_url(const int32_t _set_id)
        {
            std::stringstream ss_url;

            const auto set_iter = std::find_if(store_.cbegin(), store_.cend(), [id = _set_id](const auto& set) { return set->get_id() == id; });
            if (set_iter != store_.cend())
                ss_url << (*set_iter)->get_large_icon_url();

            return ss_url.str();
        }

        std::vector<std::string> cache::make_download_tasks(const std::string& _size)
        {
            std::vector<std::string> res;
            for (const auto& set : sets_)
            {
                if (!set->is_show() || !set->is_purchased())
                    continue;

                for (const auto& [_, icon] : set->get_icons())
                {
                    std::wstring icon_file = get_set_icon_path(*set, icon);
                    if (!core::tools::system::is_exist(icon_file))
                        meta_tasks_.push_back(download_task(icon.get_url(), "stikersMeta", std::move(icon_file)));
                }

                for (const auto& sticker : set->get_stickers())
                    res.push_back(template_url_send_ + sticker->fs_id());
            }
            return res;
        }

        void cache::serialize_meta_set_sync(const stickers::set& _set, coll_helper _coll_set, const std::string& _size)
        {
            _coll_set.set_value_as_int("id", _set.get_id());
            _coll_set.set_value_as_string("name", _set.get_name());
            _coll_set.set_value_as_string("store_id", _set.get_store_id());
            _coll_set.set_value_as_bool("purchased", _set.is_purchased());
            _coll_set.set_value_as_bool("usersticker", _set.is_user());
            _coll_set.set_value_as_string("description", _set.get_description());
            _coll_set.set_value_as_string("subtitle", _set.get_subtitle());

            set_icon_size icon_size = set_icon_size::invalid;

            if (!_size.empty())
            {
                if (_size == "small")
                    icon_size = set_icon_size::_32;
                else if (_size == "medium")
                    icon_size = set_icon_size::_48;
                else if (_size == "large")
                    icon_size = set_icon_size::_64;

                std::wstring file_name = get_set_icon_path(_set, _set.get_icon(icon_size));

                core::tools::binary_stream bs_icon;

                if (bs_icon.load_from_file(file_name))
                {
                    ifptr<istream> icon(_coll_set->create_stream(), true);

                    uint32_t file_size = bs_icon.available();

                    icon->write((uint8_t*)bs_icon.read(file_size), file_size);

                    _coll_set.set_value_as_stream("icon", icon.get());
                }
            }

            // serialize stickers
            ifptr<iarray> stickers_array(_coll_set->create_array(), true);

            for (const auto& sticker : _set.get_stickers())
            {
                coll_helper coll_sticker(_coll_set->create_collection(), true);

                ifptr<ivalue> val_sticker(_coll_set->create_value(), true);

                val_sticker->set_as_collection(coll_sticker.get());

                stickers_array->push_back(val_sticker.get());

                coll_sticker.set_value_as_int("id", sticker->get_id());
                coll_sticker.set_value_as_string("fs_id", sticker->fs_id());
                coll_sticker.set_value_as_int("set_id", _set.get_id());

                ifptr<iarray> emoji_array(coll_sticker->create_array());
                const auto & emojis = sticker->get_emojis();
                emoji_array->reserve(emojis.size());
                for (const auto &emoji : emojis)
                {
                    ifptr<ivalue> emoji_value(coll_sticker->create_value());
                    emoji_value->set_as_string(emoji.c_str(), (int32_t)emoji.length());
                    emoji_array->push_back(emoji_value.get());
                }

                coll_sticker.set_value_as_array("emoji", emoji_array.get());
            }

            _coll_set.set_value_as_array("stickers", stickers_array.get());
        }

        void cache::serialize_meta_sync(coll_helper _coll, const std::string& _size)
        {
            _coll.set_value_as_string("template_url_preview", template_url_preview_);
            _coll.set_value_as_string("template_url_original", template_url_original_);
            _coll.set_value_as_string("template_url_send", template_url_send_);

            if (sets_.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);

            for (const auto& _set : sets_)
            {
                if (!_set->is_show())
                    continue;

                // serailize sets
                coll_helper coll_set(_coll->create_collection(), true);

                ifptr<ivalue> val_set(_coll->create_value(), true);

                val_set->set_as_collection(coll_set.get());

                sets_array->push_back(val_set.get());

                serialize_meta_set_sync(*_set, coll_set, _size);
            }

            _coll.set_value_as_array("sets", sets_array.get());
        }

        void cache::serialize_store_sync(coll_helper _coll)
        {
            if (store_.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);

            for (const auto& _set : store_)
            {
                // serailize sets
                coll_helper coll_set(_coll->create_collection(), true);

                ifptr<ivalue> val_set(_coll->create_value(), true);

                val_set->set_as_collection(coll_set.get());

                sets_array->push_back(val_set.get());

                serialize_meta_set_sync(*_set, coll_set, std::string());
            }

            _coll.set_value_as_array("sets", sets_array.get());
        }

        const std::string& cache::get_md5() const
        {
            return md5_;
        }


        bool cache::get_next_meta_task(download_task& _task)
        {
            if (meta_tasks_.empty())
                return false;

            _task = meta_tasks_.front();

            return true;
        }

        bool cache::get_next_sticker_task(download_task& _task)
        {
            if (stickers_tasks_.empty())
                return false;

            _task = stickers_tasks_.front();

            return true;
        }

        void cache::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string _fs_id, const sticker_size _size, tools::binary_stream& _data)
        {
            if (_fs_id.empty())
                gui_requests_[_set_id][_sticker_id].push_back(_seq);
            else
                gui_fs_requests_[_fs_id].push_back(_seq);

            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (((_set_id > 0 && iter->get_set_id() == _set_id && iter->get_sticker_id() == _sticker_id) || (!_fs_id.empty() && iter->get_fs_id() == _fs_id)) && iter->get_size() == _size)
                {
                    download_task task = std::move(*iter);
                    stickers_tasks_.erase(iter);
                    stickers_tasks_.push_front(std::move(task));

                    return;
                }
            }

            auto file_name = get_sticker_path(_set_id, _sticker_id, _fs_id, _size);

            if (!_data.load_from_file(file_name))
            {
                auto sticker_url = make_sticker_url(_set_id, _sticker_id, _fs_id, _size);

                stickers_tasks_.emplace_front(std::move(sticker_url), "stikersGetSticker", std::move(file_name), _set_id, _sticker_id, std::move(_fs_id), _size);

                return;
            }
        }

        void cache::get_set_icon_big(const int64_t _seq, const int32_t _set_id, tools::binary_stream& _data)
        {
            gui_requests_[_set_id][-1].push_back(_seq);

            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (iter->get_set_id() == _set_id && iter->get_sticker_id() == -1 && iter->get_fs_id().empty())
                {
                    download_task task = *iter;
                    stickers_tasks_.erase(iter);
                    stickers_tasks_.push_front(std::move(task));

                    return;
                }
            }

            if (auto file_name = get_set_big_icon_path(_set_id); !_data.load_from_file(file_name))
            {
                auto sticker_url = make_big_icon_url(_set_id);

                stickers_tasks_.emplace_front(std::move(sticker_url), "stikersBigIcon", std::move(file_name), _set_id, -1);

                return;
            }
        }

        void cache::clean_set_icon_big(const int32_t _set_id)
        {
            if (const auto file_name = get_set_big_icon_path(_set_id); core::tools::system::is_exist(file_name))
            {
                core::tools::system::delete_file(file_name);
            }
        }

        requests_list cache::get_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id, const std::string& _fs_id) const
        {
            if (_fs_id.empty())
            {
                if (const auto iter_set = gui_requests_.find(_set_id); iter_set != gui_requests_.end())
                {
                    if (const auto iter_sticker = iter_set->second.find(_sticker_id); iter_sticker != iter_set->second.end())
                        return iter_sticker->second;
                }
            }
            else
            {
                if (const auto iter_sticker = gui_fs_requests_.find(_fs_id); iter_sticker != gui_fs_requests_.end())
                    return iter_sticker->second;
            }


            return requests_list();
        }

        void cache::clear_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id, const std::string& _fs_id)
        {
            if (_fs_id.empty())
            {
                if (const auto iter_set = gui_requests_.find(_set_id); iter_set != gui_requests_.end())
                {
                    if (const auto iter_sticker = iter_set->second.find(_sticker_id); iter_sticker != iter_set->second.end())
                        iter_sticker->second.clear();
                }
            }
            else
            {
                if (const auto it = gui_fs_requests_.find(_fs_id); it != gui_fs_requests_.end())
                    it->second.clear();
            }
        }

        bool cache::has_gui_request(int32_t _set_id, int32_t _sticker_id, const std::string& _fs_id) const
        {
            if (_fs_id.empty())
            {
                if (const auto iter_set = gui_requests_.find(_set_id); iter_set != gui_requests_.end())
                {
                    if (const auto iter_sticker = iter_set->second.find(_sticker_id); iter_sticker != iter_set->second.end())
                        return !iter_sticker->second.empty();
                }
            }
            else
            {
                if (const auto it = gui_fs_requests_.find(_fs_id); it != gui_fs_requests_.end())
                    return !it->second.empty();
            }


            return false;
        }

        bool cache::sticker_loaded(const download_task& _task, /*out*/ requests_list& _requests)
        {
            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    _requests = get_sticker_gui_requests(_task.get_set_id(), _task.get_sticker_id(), _task.get_fs_id());

                    clear_sticker_gui_requests(_task.get_set_id(), _task.get_sticker_id(), _task.get_fs_id());

                    stickers_tasks_.erase(iter);

                    return true;
                }
            }
            return false;
        }

        bool cache::meta_loaded(const download_task& _task)
        {
            for (auto iter = meta_tasks_.begin(); iter != meta_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    meta_tasks_.erase(iter);
                    return true;
                }
            }
            return false;
        }

        void cache::serialize_suggests(coll_helper _coll)
        {
            suggests_->serialize(_coll);
        }

        void cache::update_template_urls(std::string _preview_url, std::string _original_url, std::string _send_url)
        {
            if (!_preview_url.empty() && _preview_url != template_url_preview_)
                template_url_preview_ = std::move(_preview_url);

            if (!_original_url.empty() && _original_url != template_url_original_)
                template_url_original_ = std::move(_original_url);

            if (!_send_url.empty() && _send_url != template_url_send_)
                template_url_send_ = std::move(_send_url);
        }



        //////////////////////////////////////////////////////////////////////////
        // stickers::face
        //////////////////////////////////////////////////////////////////////////
        face::face(const std::wstring& _stickers_path)
            : cache_(std::make_shared<cache>(_stickers_path))
            , thread_(std::make_shared<async_executer>("stickers"))
            , meta_requested_(false)
            , up_to_date_(false)
            , download_meta_in_progress_(false)
            , download_meta_error_(false)
            , download_stickers_in_progess_(false)
            , download_stickers_error_(false)
            , flag_meta_need_reload_(false)
        {
        }

        std::shared_ptr<result_handler<const parse_result&>> face::parse(std::shared_ptr<core::tools::binary_stream> _data, bool _insitu)
        {
            auto handler = std::make_shared<result_handler<const parse_result&>>();
            auto up_to_date = std::make_shared<bool>();

            thread_->run_async_function([stickers_cache = cache_, _data, _insitu, up_to_date]()->int32_t
            {
                return (stickers_cache->parse(*_data, _insitu, *up_to_date) ? 0 : -1);

            })->on_result_ = [handler, up_to_date](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(parse_result((_error == 0), *up_to_date));
            };

            return handler;
        }

        std::shared_ptr<result_handler<const bool>> face::parse_store(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<const bool>>();
            auto up_to_date = std::make_shared<bool>();

            thread_->run_async_function([stickers_cache = cache_, _data]()->int32_t
            {
                return (stickers_cache->parse_store(*_data) ? 0 : -1);

            })->on_result_ = [handler, up_to_date](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0));
            };

            return handler;
        }

        std::shared_ptr<result_handler<tools::binary_stream&>> face::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string _fs_id, const core::sticker_size _size)
        {
            assert(_size > core::sticker_size::min);
            assert(_size < core::sticker_size::max);

            auto handler = std::make_shared<result_handler<tools::binary_stream&>>();
            auto sticker_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([stickers_cache = cache_, sticker_data, _set_id, _sticker_id, _size, _seq, _fs_id = std::move(_fs_id)]
            {
                stickers_cache->get_sticker(_seq, _set_id, _sticker_id, std::move(_fs_id), _size, *sticker_data);

                return 0;

            })->on_result_ = [handler, sticker_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*sticker_data);
            };

            return handler;
        }

        std::shared_ptr<result_handler<tools::binary_stream&>> face::get_set_icon_big(const int64_t _seq, const int32_t _set_id)
        {
            auto handler = std::make_shared<result_handler<tools::binary_stream&>>();
            auto icon_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([stickers_cache = cache_, icon_data, _set_id, _seq]
            {
                stickers_cache->get_set_icon_big(_seq, _set_id, *icon_data);

                return 0;

            })->on_result_ = [handler, icon_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*icon_data);
            };

            return handler;
        }

        void face::clean_set_icon_big(const int64_t _seq, const int32_t _set_id)
        {
            thread_->run_async_function([stickers_cache = cache_, _set_id]
            {
                stickers_cache->clean_set_icon_big(_set_id);

                return 0;
            });
        }

        std::shared_ptr<result_handler<coll_helper>> face::serialize_meta(coll_helper _coll, const std::string& _size)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();

            thread_->run_async_function([stickers_cache = cache_, _coll, size = _size]()->int32_t
            {
                stickers_cache->serialize_meta_sync(_coll, size);

                return 0;

            })->on_result_ = [handler, _coll](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_coll);
            };

            return handler;
        }

        std::shared_ptr<result_handler<coll_helper>> face::serialize_store(coll_helper _coll)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();

            thread_->run_async_function([stickers_cache = cache_, _coll]()->int32_t
            {
                stickers_cache->serialize_store_sync(_coll);

                return 0;

            })->on_result_ = [handler, _coll](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_coll);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const std::vector<std::string>&>> face::make_download_tasks(const std::string& _size)
        {
            auto handler = std::make_shared<result_handler<const std::vector<std::string>&>>();

            auto res = std::make_shared<std::vector<std::string>>();

            thread_->run_async_function([stickers_cache = cache_, _size, res]()->int32_t
            {
                *res = stickers_cache->make_download_tasks(_size);
                return 0;

            })->on_result_ = [handler, res](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*res);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const load_result&>> face::load_meta_from_local()
        {
            auto handler = std::make_shared<result_handler<const load_result&>>();
            auto md5 = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache = cache_, md5]()->int32_t
            {
                core::tools::binary_stream bs;
                if (!bs.load_from_file(stickers_cache->get_meta_file_name()))
                    return -1;

                bs.write(0);

                bool up_todate = false;

                if (!stickers_cache->parse(bs, true, up_todate))
                    return -1;

                if (!stickers_cache->is_meta_icons_exist())
                    return -1;

                *md5 = stickers_cache->get_md5();

                return 0;

            })->on_result_ = [handler, md5](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(load_result((_error == 0), *md5));
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool>> face::save(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<bool>>();

            thread_->run_async_function([stickers_cache = cache_, _data]()->int32_t
            {
                return (_data->save_2_file(stickers_cache->get_meta_file_name()) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_meta_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto task = std::make_shared<download_task>(std::string(), std::string(), std::wstring());

            thread_->run_async_function([stickers_cache = cache_, task]()->int32_t
            {
                if (!stickers_cache->get_next_meta_task(*task))
                    return -1;

                return 0;

            })->on_result_ = [handler, task](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0, *task);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_sticker_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto task = std::make_shared<download_task>(std::string(), std::string(), std::wstring());

            thread_->run_async_function([stickers_cache = cache_, task]()->int32_t
            {
                if (!stickers_cache->get_next_sticker_task(*task))
                    return -1;

                return 0;

            })->on_result_ = [handler, task](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0, *task);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const std::list<int64_t>&>> face::on_sticker_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool, const std::list<int64_t>&>>();
            auto requests = std::make_shared<std::list<int64_t>>();

            thread_->run_async_function([stickers_cache = cache_, _task, requests]()->int32_t
            {
                return (stickers_cache->sticker_loaded(_task, *requests) ? 0 : -1);

            })->on_result_ = [handler, requests](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0), *requests);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool>> face::on_metadata_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool>>();

            thread_->run_async_function([stickers_cache = cache_, _task]()->int32_t
            {
                return (stickers_cache->meta_loaded(_task) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const std::string&>> face::get_md5()
        {
            auto handler = std::make_shared<result_handler<const std::string&>>();
            auto md5 = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache = cache_, md5]()->int32_t
            {
                *md5 = stickers_cache->get_md5();

                return 0;

            })->on_result_ = [handler, md5](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*md5);
            };

            return handler;
        }

        void face::set_gui_request_params(const gui_request_params& _params)
        {
            gui_request_params_ = _params;
        }

        const gui_request_params& face::get_gui_request_params()
        {
            return gui_request_params_;
        }

        void face::set_up_to_date(bool _up_to_date)
        {
            up_to_date_ = _up_to_date;
        }

        bool face::is_up_to_date() const
        {
            return up_to_date_;
        }

        void face::set_download_meta_error(const bool _is_error)
        {
            download_meta_error_ = _is_error;
        }

        bool face::is_download_meta_error() const
        {
            return download_meta_error_;
        }

        bool face::is_download_meta_in_progress()
        {
            return download_meta_in_progress_;
        }

        void face::set_download_meta_in_progress(const bool _in_progress)
        {
            download_meta_in_progress_ = _in_progress;
        }

        void face::set_flag_meta_need_reload(const bool _need_reload)
        {
            flag_meta_need_reload_ = _need_reload;
        }

        bool face::is_flag_meta_need_reload() const
        {
            return flag_meta_need_reload_;
        }

        void face::set_download_stickers_error(const bool _is_error)
        {
            download_stickers_error_ = _is_error;
        }

        bool face::is_download_stickers_error() const
        {
            return download_stickers_error_;
        }

        bool face::is_download_stickers_in_progress() const
        {
            return download_stickers_in_progess_;
        }

        void face::set_download_stickers_in_progress(const bool _in_progress)
        {
            download_stickers_in_progess_ = _in_progress;
        }

        std::shared_ptr<result_handler<const bool>> face::serialize_suggests(coll_helper _coll)
        {
            auto handler = std::make_shared<result_handler<const bool>>();
            auto stickers_cache = cache_;

            thread_->run_async_function([stickers_cache, _coll]()->int32_t
            {
                stickers_cache->serialize_suggests(_coll);

                return 0;

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        void face::update_template_urls(const rapidjson::Value& _value)
        {
            std::string template_url_preview, template_url_original, template_url_send;

            tools::unserialize_value(_value, "template_url_preview", template_url_preview);
            tools::unserialize_value(_value, "template_url_original", template_url_original);
            tools::unserialize_value(_value, "template_url_send", template_url_send);

            thread_->run_async_function([stickers_cache = cache_,
                                         url_preview = std::move(template_url_preview),
                                         url_original = std::move(template_url_original),
                                         url_send = std::move(template_url_send)]()
            {
                stickers_cache->update_template_urls(std::move(url_preview), std::move(url_original), std::move(url_send));
                return 0;
            });
        }
    }
}



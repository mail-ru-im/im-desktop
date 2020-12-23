#include "stdafx.h"

#include "stickers.h"
#include "suggests.h"

#include "../../../corelib/enumerations.h"

#include "../tools/system.h"
#include "../tools/json_helper.h"
#include "../tools/file_sharing.h"
#include "../tools/features.h"

#include "../async_task.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/loader_errors.h"
#include "../../common.shared/config/config.h"

#include "connections/urls_cache.h"

namespace
{
    bool is_lottie_id(std::string_view _id)
    {
        const auto content_type = core::tools::get_content_type_from_file_sharing_id(_id);
        return content_type && content_type->is_lottie();
    }

    bool is_lottie_url(std::string_view _uri)
    {
        return is_lottie_id(core::tools::get_file_id(_uri));
    }

    constexpr std::wstring_view image_ext() { return L".png"; }
    constexpr std::wstring_view lottie_ext() { return L".lottie"; }
}

namespace core
{
    namespace stickers
    {
        set_icon_size icon_size_str_to_size(std::string_view _size)
        {
            if (_size == "small")
                return set_icon_size::_32;
            else if (_size == "medium")
                return set_icon_size::_48;
            else if (_size == "large" || _size == "xlarge" || _size == "xxlarge")
                return set_icon_size::_64;

            assert(!"unknown type");
            return set_icon_size::_64;
        }

        sets_list parse_sets(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            sets_list res;

            if (!_node.IsArray())
                return res;

            for (const auto& set : _node.GetArray())
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize(set, _emojis))
                {
                    assert(!"error reading stickers set");
                    continue;
                }

                res.push_back(std::move(new_set));
            }

            return res;
        }

        void serialize_sets(coll_helper& _coll, const sets_list& _sets, const std::string& _size = std::string())
        {
            if (_sets.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);
            sets_array->reserve(_sets.size());
            for (const auto& _set : _sets)
            {
                if (!_set->is_show())
                    continue;

                coll_helper coll_set(_coll->create_collection(), true);
                ifptr<ivalue> val_set(_coll->create_value(), true);
                val_set->set_as_collection(coll_set.get());
                sets_array->push_back(val_set.get());

                cache::serialize_meta_set_sync(*_set, coll_set, _size);
            }

            _coll.set_value_as_array("sets", sets_array.get());
        }

        void remove_from_tasks(download_tasks& _tasks, const download_task& _task)
        {
            const auto it = std::find_if(_tasks.begin(), _tasks.end(), [&_task](const auto& t) { return t.get_source_url() == _task.get_source_url(); });
            if (it != _tasks.end())
                _tasks.erase(it);
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

        const set_icon& set::get_big_icon() const
        {
            return big_icon_;
        }

        void set::set_big_icon(set_icon&& _icon)
        {
            big_icon_ = std::move(_icon);
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

            if (icons_.size() == 1)
                if (const auto iter = icons_.cbegin(); iter->first == stickers::set_icon_size::scalable)
                    return iter->second;

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
            {
                const auto size = is_lottie_url(url) ? set_icon_size::scalable : set_icon_size::big;
                set_big_icon(set_icon(size, url));
            }

            auto unserialize_stickerpicker_icon = [this, &_node](std::string_view _name)
            {
                if (std::string sp; tools::unserialize_value(_node, _name, sp) && !sp.empty())
                {
                    if (is_lottie_url(sp))
                    {
                        put_icon(set_icon(set_icon_size::scalable, sp));
                    }
                    else
                    {
                        put_icon(set_icon(set_icon_size::_20, sp));
                        put_icon(set_icon(set_icon_size::_32, sp));
                        put_icon(set_icon(set_icon_size::_48, sp));
                        put_icon(set_icon(set_icon_size::_64, sp));
                    }
                }
            };

            if (const auto iter_icons = _node.FindMember("contentlist_sticker_picker_icon"); iter_icons != _node.MemberEnd())
            {
                if (iter_icons->value.IsString())
                {
                    unserialize_stickerpicker_icon("contentlist_sticker_picker_icon");
                }
                else if (iter_icons->value.IsObject())
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
            }

            unserialize_stickerpicker_icon("stiker-picker");

            const auto unserialize_stickers = [this, &_node, &_emojis](const auto _node_name)
            {
                if (const auto iter = _node.FindMember(_node_name); iter != _node.MemberEnd() && iter->value.IsArray())
                {
                    for (const auto& sticker : iter->value.GetArray())
                    {
                        auto new_sticker = std::make_shared<stickers::sticker>();

                        auto success = false;
                        if (sticker.IsString())
                            success = new_sticker->unserialize(sticker, _emojis);
                        else
                            success = new_sticker->unserialize(sticker);

                        if (success)
                            stickers_.push_back(std::move(new_sticker));
                    }
                    return true;
                }
                return false;
            };

            if (const auto res = unserialize_stickers("stickers"); !res)
                unserialize_stickers("content");

            return true;
        }

        bool set::unserialize(const rapidjson::Value& _node)
        {
            return unserialize(_node, {});
        }

        bool set::is_lottie_pack() const
        {
            return
                big_icon_.get_size() == set_icon_size::scalable ||
                icons_.find(set_icon_size::scalable) != icons_.end() ||
                std::any_of(stickers_.begin(), stickers_.end(), [](const auto& s) { return is_lottie_id(s->fs_id()); });
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

            sets_ = parse_sets(iter_packs->value, emoji);
            return true;
        }

        bool cache::parse_store(core::tools::binary_stream& _data)
        {
            rapidjson::Document doc;
            const auto& parse_result = doc.ParseInsitu(_data.read(_data.available()));
            if (parse_result.HasParseError())
                return false;

            int status_code = 0;
            if (!tools::unserialize_value(doc, "status", status_code))
                return false;

            if (status_code != 200)
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

            store_ = parse_sets(iter_sets->value, {});
            return true;
        }

        std::wstring cache::get_meta_file_name() const
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << stickers_meta_file_name();
            return ss_out.str();
        }

        std::wstring cache::get_set_icon_path(int32_t _set_id, const set_icon& _icon)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << _set_id << L"/icons/_icon";

            if (_icon.get_size() == set_icon_size::scalable)
                ss_out << lottie_ext();
            else
                ss_out << "_" << (int)_icon.get_size() << image_ext();

            return ss_out.str();
        }

        std::pair<std::wstring, set_icon_size> cache::get_set_big_icon_path(int32_t _set_id)
        {
            if (const auto& set = get_set(_set_id))
            {
                if (set->get_big_icon().get_size() == set_icon_size::scalable)
                    return { get_set_icon_path(_set_id, set->get_big_icon()), set_icon_size::scalable };
            }

            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/' << _set_id << L"/icons/_icon_xset" << image_ext();
            return { ss_out.str(), set_icon_size::big };
        }

        std::wstring cache::get_sticker_path(int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, sticker_size _size)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/';

            if (is_lottie_id(_fs_id))
            {
                ss_out << tools::from_utf8(_fs_id) << L'/' << "sticker" << lottie_ext();
            }
            else
            {
                if (_fs_id.empty())
                    ss_out << _set_id << L'/' << _sticker_id;
                else
                    ss_out << tools::from_utf8(_fs_id);

                ss_out << L'/' << _size << image_ext();
            }

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
            if (!_fs_id.empty())
            {
                if (is_lottie_id(_fs_id))
                    return su::concat(urls::get_url(urls::url_type::sticker), '/', _fs_id);

                return su::concat(urls::get_url(urls::url_type::files_preview), "/max/sticker_", _size, '/', _fs_id);
            }

            std::stringstream ss_url;
            ss_url << "https://" << config::get().url(config::urls::cicq_com) << "/store/stickers/" << _set_id << '/' << _sticker_id << '/' << _size << ".png";
            return ss_url.str();
        }

        const std::shared_ptr<stickers::set>& cache::get_set(int32_t _set_id) const
        {
            const auto it = std::find_if(store_.cbegin(), store_.cend(), [id = _set_id](const auto& set) { return set->get_id() == id; });
            if (it != store_.cend())
                return *it;

            static const std::shared_ptr<stickers::set> empty;
            return empty;
        }

        void cache::on_download_tasks_added(int _tasks_count)
        {
            if (_tasks_count > 0)
            {
                if (stat_dl_tasks_count_ == 0)
                    stat_dl_start_time_ = std::chrono::steady_clock::now();
                stat_dl_tasks_count_ += _tasks_count;
            }
            else
            {
                if (stat_dl_tasks_count_ > 0)
                {
                    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stat_dl_start_time_).count();

                    if (size_t(stat_dl_tasks_count_) >= features::get_max_parallel_sticker_downloads() || time_diff > 1000)
                    {
                        core::stats::event_props_type props;
                        props.emplace_back("count", std::to_string(stat_dl_tasks_count_));

                        props.emplace_back("time", std::to_string(time_diff));

                        if (time_diff < 1000)
                            time_diff = 1000;

                        const auto per_sec = (double)stat_dl_tasks_count_ / (time_diff / 1000.);
                        const auto speed_str = per_sec <= 1. ? std::string("0.") + core::stats::round_value(per_sec * 10, 1, 10) : core::stats::round_value(per_sec, 5, 100);
                        props.emplace_back("speed", speed_str);
                        g_core->insert_im_stats_event(stats::im_stat_event_names::stickers_download_per_second, std::move(props));
                    }
                }

                stat_dl_tasks_count_ = 0;
            }
        }


        std::string cache::make_big_icon_url(const int32_t _set_id)
        {
            if (const auto& set = get_set(_set_id))
                return set->get_big_icon().get_url();
            return {};
        }

        int cache::make_set_icons_tasks(const std::string& _size)
        {
            for (const auto& set : sets_)
            {
                if (!set->is_show() || !set->is_purchased())
                    continue;

                const auto& icon = set->get_icon(icon_size_str_to_size(_size));
                auto icon_file = get_set_icon_path(set->get_id(), icon);
                if (!core::tools::system::is_exist(icon_file))
                {
                    auto dl_task = download_task(icon.get_url(), "stikersMeta", std::move(icon_file), set->get_id());
                    dl_task.set_need_decompress(icon.get_size() == stickers::set_icon_size::scalable);
                    dl_task.set_type(download_task::type::icon);
                    pending_tasks_.emplace_back(std::move(dl_task));
                }
            }

            return pending_tasks_.size();
        }

        void cache::serialize_meta_set_sync(const stickers::set& _set, coll_helper _coll_set, const std::string& _size)
        {
            _coll_set.set_value_as_int("set_id", _set.get_id());
            _coll_set.set_value_as_string("name", _set.get_name());
            _coll_set.set_value_as_string("store_id", _set.get_store_id());
            _coll_set.set_value_as_bool("purchased", _set.is_purchased());
            _coll_set.set_value_as_bool("usersticker", _set.is_user());
            _coll_set.set_value_as_string("description", _set.get_description());
            _coll_set.set_value_as_string("subtitle", _set.get_subtitle());
            _coll_set.set_value_as_bool("lottie", _set.is_lottie_pack());

            if (!_size.empty())
            {
                if (const auto& icon = _set.get_icon(icon_size_str_to_size(_size)); icon.is_valid())
                {
                    std::wstring file_name = get_set_icon_path(_set.get_id(), icon);
                    if (tools::system::is_exist(file_name))
                        _coll_set.set_value_as_string("icon_path", tools::from_utf16(file_name));
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

            serialize_sets(_coll, sets_, _size);
        }

        void cache::serialize_store_sync(coll_helper _coll)
        {
            serialize_sets(_coll, store_);
        }

        const std::string& cache::get_md5() const
        {
            return md5_;
        }

        download_tasks cache::take_download_tasks()
        {
            download_tasks res;
            if (!have_tasks_to_download())
            {
                on_download_tasks_added(res.size());
                return res;
            }
            else if (active_tasks_.size() >= features::get_max_parallel_sticker_downloads())
            {
                return res;
            }

            const auto dl_count = active_tasks_.size();
            if (const auto max_dl = features::get_max_parallel_sticker_downloads(); dl_count < max_dl)
            {
                active_tasks_.splice(
                    active_tasks_.end(),
                    pending_tasks_,
                    pending_tasks_.begin(),
                    std::next(pending_tasks_.begin(), std::min(max_dl - dl_count, pending_tasks_.size())));
            }

            res = download_tasks(std::next(active_tasks_.begin(), dl_count), active_tasks_.end());
            on_download_tasks_added(res.size());
            return res;
        }

        bool cache::have_tasks_to_download() const noexcept
        {
            return !pending_tasks_.empty();
        }

        int32_t cache::on_task_loaded(const download_task& _task)
        {
            remove_from_tasks(active_tasks_, _task);
            remove_from_tasks(pending_tasks_, _task);
            return pending_tasks_.size();
        }

        int cache::cancel_download_tasks(std::vector<std::string> _fs_ids, sticker_size _size)
        {
            const auto prevSize = pending_tasks_.size();
            pending_tasks_.remove_if([&_fs_ids, _size](const auto& _task)
            {
                return std::any_of(
                    _fs_ids.begin(),
                    _fs_ids.end(),
                    [&_task, _size](const auto& fs_id) { return _task.get_size() == _size && _task.get_fs_id() == fs_id; });
            });
            return pending_tasks_.size() - prevSize;
        }

        void cache::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string _fs_id, const sticker_size _size, std::wstring& _path)
        {
            auto get_iter = [_set_id, _sticker_id, _size, &_fs_id](auto& _list)
            {
                return std::find_if(_list.begin(), _list.end(), [_set_id, _sticker_id, _size, &_fs_id](const auto& _task)
                {
                    const auto same_size = _task.get_size() == _size;
                    const auto same_set_and_id = _set_id > 0 && _task.get_set_id() == _set_id && _task.get_sticker_id() == _sticker_id;
                    const auto same_fs_id = !_fs_id.empty() && _task.get_fs_id() == _fs_id;

                    return same_size && same_set_and_id && same_fs_id;
                });
            };

            if (get_iter(active_tasks_) != active_tasks_.end())
                return;

            if (auto iter = get_iter(pending_tasks_); iter != pending_tasks_.end())
            {
                download_task task = std::move(*iter);
                pending_tasks_.erase(iter);
                pending_tasks_.emplace_front(std::move(task));
                return;
            }

            _path = get_sticker_path(_set_id, _sticker_id, _fs_id, _size);
            if (!tools::system::is_exist(_path))
            {
                auto sticker_url = make_sticker_url(_set_id, _sticker_id, _fs_id, _size);

                auto dl_task = download_task(std::move(sticker_url), "stikersGetSticker", _path, _set_id, _sticker_id, std::move(_fs_id), _size);
                dl_task.set_need_decompress(is_lottie_id(dl_task.get_fs_id()));
                pending_tasks_.emplace_back(std::move(dl_task));
            }
        }

        void cache::get_set_icon_big(const int64_t _seq, const int32_t _set_id, std::wstring& _path)
        {
            auto get_iter = [_set_id](auto& _list)
            {
                return std::find_if(_list.begin(), _list.end(), [_set_id](const auto& _task)
                {
                    return _task.get_set_id() == _set_id && _task.get_sticker_id() == -1 && _task.get_fs_id().empty();
                });
            };

            if (get_iter(active_tasks_) != active_tasks_.end())
                return;

            if (auto iter = get_iter(pending_tasks_); iter != pending_tasks_.end())
            {
                download_task task = std::move(*iter);
                pending_tasks_.erase(iter);
                pending_tasks_.emplace_front(std::move(task));
                return;
            }

            auto [path, size] = get_set_big_icon_path(_set_id);
            _path = std::move(path);
            if (!tools::system::is_exist(_path))
            {
                auto sticker_url = make_big_icon_url(_set_id);
                auto dl_task = download_task(std::move(sticker_url), "stikersBigIcon", _path, _set_id, -1);
                dl_task.set_need_decompress(size == set_icon_size::scalable);
                pending_tasks_.emplace_back(std::move(dl_task));
            }
        }

        void cache::clean_set_icon_big(const int32_t _set_id)
        {
            if (const auto path = get_set_big_icon_path(_set_id); core::tools::system::is_exist(path.first))
                core::tools::system::delete_file(path.first);
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
        {
        }

        std::shared_ptr<result_handler<const parse_result&>> face::parse(const std::shared_ptr<core::tools::binary_stream>& _data, bool _insitu)
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

            thread_->run_async_function([stickers_cache = cache_, _data = std::move(_data)]()->int32_t
            {
                return (stickers_cache->parse_store(*_data) ? 0 : -1);

            })->on_result_ = [handler, up_to_date](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0));
            };

            return handler;
        }

        std::shared_ptr<result_handler<std::wstring_view>> face::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string _fs_id, const core::sticker_size _size)
        {
            assert(_size > core::sticker_size::min);
            assert(_size < core::sticker_size::max);

            auto handler = std::make_shared<result_handler<std::wstring_view>>();
            auto sticker_path = std::make_shared<std::wstring>();

            thread_->run_async_function([stickers_cache = cache_, sticker_path, _set_id, _sticker_id, _size, _seq, _fs_id = std::move(_fs_id)]
            {
                stickers_cache->get_sticker(_seq, _set_id, _sticker_id, std::move(_fs_id), _size, *sticker_path);

                return 0;

            })->on_result_ = [handler, sticker_path](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*sticker_path);
            };

            return handler;
        }

        std::shared_ptr<result_handler<int>> face::cancel_download_tasks(std::vector<std::string> _fs_ids, core::sticker_size _size)
        {
            auto handler = std::make_shared<result_handler<int>>();

            thread_->run_async_function([stickers_cache = cache_, fs_ids = std::move(_fs_ids), _size]()->int32_t
            {
                return stickers_cache->cancel_download_tasks(std::move(fs_ids), _size);

            })->on_result_ = [handler](int32_t _count_deleted)
            {
                if (handler->on_result_)
                    handler->on_result_(_count_deleted);
            };

            return handler;
        }

        std::shared_ptr<result_handler<std::wstring_view>> face::get_set_icon_big(const int64_t _seq, const int32_t _set_id)
        {
            auto handler = std::make_shared<result_handler<std::wstring_view>>();
            auto icon_path = std::make_shared<std::wstring>();

            thread_->run_async_function([stickers_cache = cache_, icon_path, _set_id, _seq]
            {
                stickers_cache->get_set_icon_big(_seq, _set_id, *icon_path);

                return 0;

            })->on_result_ = [handler, icon_path](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*icon_path);
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

        std::shared_ptr<result_handler<int>> face::make_set_icons_tasks(const std::string& _size)
        {
            auto handler = std::make_shared<result_handler<int>>();
            thread_->run_async_function([stickers_cache = cache_, _size]()->int32_t
            {
                return stickers_cache->make_set_icons_tasks(_size);
            })->on_result_ = [handler](int32_t _count)
            {
                if (handler->on_result_)
                    handler->on_result_(_count);
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

            thread_->run_async_function([stickers_cache = cache_, _data = std::move(_data)]()->int32_t
            {
                return (_data->save_2_file(stickers_cache->get_meta_file_name()) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const download_tasks&>> face::take_download_tasks()
        {
            auto handler = std::make_shared<result_handler<const download_tasks&>>();
            auto tasks = std::make_shared<download_tasks>();

            thread_->run_async_function([stickers_cache = cache_, tasks]()->int32_t
            {
                *tasks = stickers_cache->take_download_tasks();
                return 0;

            })->on_result_ = [handler, tasks](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*tasks);
            };

            return handler;
        }

        std::shared_ptr<result_handler<int>> core::stickers::face::on_task_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<int>>();

            thread_->run_async_function([stickers_cache = cache_, _task]()->int32_t
            {
                return stickers_cache->on_task_loaded(_task);

            })->on_result_ = [handler](int32_t _count)
            {
                if (handler->on_result_)
                    handler->on_result_(_count);
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

        static void serialize_image(coll_helper& _coll, const tools::binary_stream& _data, std::string_view _id)
        {
            if (const auto data_size = _data.available(); data_size > 0)
            {
                ifptr<istream> sticker_data(_coll->create_stream(), true);
                sticker_data->write((const uint8_t*)_data.read(data_size), data_size);
                _coll.set_value_as_stream(_id, sticker_data.get());
            }
        }

        static void post_sticker_impl(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, core::sticker_size _size, int _error, std::wstring_view _path)
        {
            assert(_size > sticker_size::min);
            assert(_size < sticker_size::max);

            coll_helper coll(g_core->create_collection(), true);

            if (_set_id >= 0)
                coll.set_value_as_int("set_id", _set_id);
            coll.set_value_as_int("sticker_id", _sticker_id);
            coll.set_value_as_string("fs_id", _fs_id);
            coll.set_value_as_int("error", _error);

            assert(!to_string(_size).empty());
            coll.set_value_as_string(to_string(_size), core::tools::from_utf16(_path));

            g_core->post_message_to_gui("stickers/sticker/get/result", _seq, coll.get());
        }

        void post_sticker_fail_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, loader_errors _error)
        {
            post_sticker_impl(_seq, _set_id, _sticker_id, _fs_id, core::sticker_size::small, (int)_error, {});
        }

        void post_sticker_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, core::sticker_size _size, std::wstring_view _path)
        {
            post_sticker_impl(_seq, _set_id, _sticker_id, _fs_id, _size, 0, _path);
        }

        void post_set_icon_2_gui(int32_t _set_id, std::string_view _message, std::wstring_view _path, int32_t _error)
        {
            assert(_set_id >= 0);

            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_int("error", _error);
            coll.set_value_as_int("set_id", _set_id);
            coll.set_value_as_string("icon_path", core::tools::from_utf16(_path));

            g_core->post_message_to_gui(_message, 0, coll.get());
        }
    }
}



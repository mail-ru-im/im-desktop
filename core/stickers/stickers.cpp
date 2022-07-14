#include "stdafx.h"

#include "stickers.h"
#include "suggests.h"

#include "../../../corelib/enumerations.h"

#include "../tools/system.h"
#include "../tools/file_sharing.h"
#include "../tools/features.h"

#include "../async_task.h"
#include "../../common.shared/json_helper.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/loader_errors.h"
#include "../../common.shared/config/config.h"

#include "connections/urls_cache.h"
#include "connections/wim/robusto_packet.h"

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

    constexpr std::wstring_view image_ext() noexcept { return L".png"; }
    constexpr std::wstring_view lottie_ext() noexcept { return L".lottie"; }
    constexpr std::string_view big_size() noexcept { return "big"; }
    constexpr std::wstring_view meta_file_name() noexcept { return L"meta.js"; }
    constexpr std::wstring_view meta_etag_file_name() noexcept { return L"etag"; }

    std::wstring get_icon_file_name(const core::tools::filesharing_id& _fs_id, std::string_view _size)
    {
        constexpr std::wstring_view icon = L"_icon";

        if (is_lottie_id(_fs_id.file_id()))
            return su::wconcat(icon, lottie_ext());

        std::wstring_view suffix = L"64";
        if (_size == "small")
            suffix = L"32";
        else if (_size == "medium")
            suffix = L"48";
        else if (_size == big_size())
            suffix = L"xset";

        return su::wconcat(icon, L'_', suffix, image_ext());
    }

    std::wstring g_stickers_path;
}

namespace core
{
    namespace stickers
    {
        sets_list parse_sets(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            sets_list res;

            if (!_node.IsArray())
                return res;

            for (const auto& set : _node.GetArray())
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize2(set, _emojis))
                {
                    im_assert(!"error reading stickers set");
                    continue;
                }

                res.push_back(std::move(new_set));
            }

            return res;
        }

        constexpr set_type string_to_type(std::string_view _str) noexcept
        {
            if (_str == "animated")
                return set_type::animated;
            return set_type::image;
        }

        void serialize_sets(coll_helper& _coll, const sets_list& _sets, std::string_view _size = {})
        {
            if (_sets.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);
            sets_array->reserve(_sets.size());
            for (const auto& _set : _sets)
            {
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




        //////////////////////////////////////////////////////////////////////////
        // class sticker
        //////////////////////////////////////////////////////////////////////////
        sticker::sticker(const core::tools::filesharing_id& _id)
            : file_sharing_id_(_id)
        {
        }

        sticker::sticker(std::string_view _file_id)
            : file_sharing_id_(_file_id)
        {
        }

        bool sticker::unserialize(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            file_sharing_id_ = core::tools::filesharing_id(rapidjson_get_string(_node));

            if (const auto it = _emojis.find(std::string(file_sharing_id_.file_id())); it != _emojis.end())
                emojis_ = it->second;

            return true;
        }

        bool sticker::unserialize(const rapidjson::Value& _node)
        {
            std::string_view file_id;
            if (!tools::unserialize_value(_node, "fileId", file_id))
                return false;
            file_sharing_id_ = core::tools::filesharing_id(file_id);
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

        void sticker::update_fideration_id(const std::string& _federation_id)
        {
            if (!_federation_id.empty())
                file_sharing_id_.set_source_id(_federation_id);
        }

        int32_t sticker::get_id() const
        {
            return id_;
        }

        const core::tools::filesharing_id& sticker::fs_id() const
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

        const std::string& set::get_title() const
        {
            return title_;
        }

        void set::set_title(std::string&& _title)
        {
            title_ = std::move(_title);
        }

        bool set::is_added() const
        {
            return is_added_;
        }

        void set::set_added(bool _added)
        {
            is_added_ = _added;
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

        void set::set_federation_id(const std::string& _federation_id)
        {
            for (auto sticker : stickers_)
                sticker->update_fideration_id(_federation_id);
        }

        const std::string& set::get_description() const
        {
            return description_;
        }

        const sticker& set::get_main_sticker() const
        {
            return main_sticker_;
        }

        void set::set_main_sticker(sticker&& _sticker)
        {
            main_sticker_ = std::move(_sticker);
        }

        const stickers_vector& set::get_stickers() const
        {
            return stickers_;
        }

        bool set::unserialize(const rapidjson::Value& _node)
        {
            if (int id = 0; tools::unserialize_value(_node, "id", id))
                set_id(id);
            else
                return false;

            if (std::string name; tools::unserialize_value(_node, "title", name))
                set_title(std::move(name));

            if (std::string name; tools::unserialize_value(_node, "name", name)) // for store/showcase
                set_title(std::move(name));

            if (bool purchased = true; tools::unserialize_value(_node, "purchased", purchased))
                set_added(purchased);

            if (std::string store_id; tools::unserialize_value(_node, "store_id", store_id))
                set_store_id(std::move(store_id));

            if (std::string description; tools::unserialize_value(_node, "description", description))
                set_description(std::move(description));

            if (std::string_view url; tools::unserialize_value(_node, "icons", url))
                set_main_sticker(sticker::from_file_id(core::tools::get_file_id(url)));

            auto unserialize_stickerpicker_icon = [this, &_node](std::string_view _name)
            {
                if (std::string sp; tools::unserialize_value(_node, _name, sp) && !sp.empty())
                    set_main_sticker(sticker::from_file_id(core::tools::get_file_id(sp)));
            };

            if (const auto iter_icons = _node.FindMember("contentlist_sticker_picker_icon"); iter_icons != _node.MemberEnd())
            {
                if (iter_icons->value.IsString())
                {
                    unserialize_stickerpicker_icon("contentlist_sticker_picker_icon");
                }
                else if (iter_icons->value.IsObject())
                {
                    for (std::string_view s : { "xsmall", "small", "medium", "large" })
                    {
                        if (std::string_view url; tools::unserialize_value(iter_icons->value, s, url))
                        {
                            set_main_sticker(sticker::from_file_id(core::tools::get_file_id(url)));
                            break;
                        }
                    }
                }
            }

            unserialize_stickerpicker_icon("stiker-picker");

            const auto unserialize_stickers = [this, &_node](const auto _node_name)
            {
                if (const auto iter = _node.FindMember(_node_name); iter != _node.MemberEnd() && iter->value.IsArray())
                {
                    for (const auto& sticker : iter->value.GetArray())
                    {
                        auto new_sticker = std::make_shared<stickers::sticker>();

                        auto success = false;
                        if (sticker.IsString())
                            success = new_sticker->unserialize(sticker, {});
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

        bool set::unserialize2(const rapidjson::Value& _node, const emoji_map& _emojis)
        {
            if (int id = 0; tools::unserialize_value(_node, "id", id))
                set_id(id);
            else
                return false;

            if (std::string store_id; tools::unserialize_value(_node, "store_id", store_id))
                set_store_id(std::move(store_id));

            if (std::string title; tools::unserialize_value(_node, "title", title))
                set_title(std::move(title));

            if (std::string description; tools::unserialize_value(_node, "description", description))
                set_description(std::move(description));

            if (bool added = true; tools::unserialize_value(_node, "is_added", added))
                set_added(added);

            if (std::string_view id; tools::unserialize_value(_node, "main_sticker", id))
                set_main_sticker(sticker::from_file_id(id));

            if (std::string_view type; tools::unserialize_value(_node, "type", type))
                type_ = string_to_type(type);

            if (const auto iter = _node.FindMember("stickers"); iter != _node.MemberEnd() && iter->value.IsArray())
            {
                auto stickers_array = iter->value.GetArray();
                stickers_.reserve(stickers_array.Size());
                for (const auto& sticker : stickers_array)
                {
                    auto new_sticker = std::make_shared<stickers::sticker>();
                    if (new_sticker->unserialize(sticker, _emojis))
                        stickers_.push_back(std::move(new_sticker));
                }
            }

            return true;
        }

        std::wstring set::get_main_sticker_path(std::string_view _size) const
        {
            return su::wconcat(g_stickers_path, L'/', std::to_wstring(id_), L"/icons/", get_icon_file_name(main_sticker_.fs_id(), _size));
        }

        std::string set::get_main_sticker_url(std::string_view _size) const
        {
            const auto& id = main_sticker_.fs_id().file_id();
            if (is_lottie_id(id))
                return su::concat(urls::get_url(urls::url_type::sticker), '/', id);

            if (_size == big_size())
                _size = "large";
            return su::concat(urls::get_url(urls::url_type::files_preview), "/max/sticker_", _size, '/', id);
        }

        //////////////////////////////////////////////////////////////////////////
        // download_task
        //////////////////////////////////////////////////////////////////////////
        download_task::download_task(
            std::string _source_url,
            std::string _endpoint,
            std::wstring _dest_file,
            int32_t _set_id,
            int32_t _sticker_id,
            core::tools::filesharing_id _fs_id,
            sticker_size _size)
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

        const core::tools::filesharing_id& download_task::get_fs_id() const
        {
            return fs_id_;
        }

        sticker_size download_task::get_size() const
        {
            return size_;
        }

        //////////////////////////////////////////////////////////////////////////
        // class cache
        //////////////////////////////////////////////////////////////////////////
        cache::cache(const std::wstring& _stickers_path)
            : suggests_(std::make_unique<suggests>())
        {
            g_stickers_path = _stickers_path;
        }

        cache::~cache() = default;

        bool cache::parse(core::tools::binary_stream& _data, bool _insitu)
        {
            rapidjson::Document doc;
            const auto& parse_result = (_insitu ? doc.ParseInsitu(_data.read(_data.available())) : doc.Parse(_data.read(_data.available())));
            if (parse_result.HasParseError())
                return false;

            const auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd() || !iter_status->value.IsObject())
                return false;

            if (int code = 0; !tools::unserialize_value(iter_status->value, "code", code) || code != wim::robusto_protocol_error::ok)
                return false;

            const auto data_iter = doc.FindMember("result");
            if (data_iter == doc.MemberEnd() || data_iter->value.IsNull() || !data_iter->value.IsObject())
                return true;

            const auto iter_packs = data_iter->value.FindMember("sticker_packs");
            if (iter_packs == data_iter->value.MemberEnd() || iter_packs->value.IsNull() || !iter_packs->value.IsArray())
                return true;

            suggests_->unserialize(data_iter->value);

            emoji_map emoji;

            for (const auto& suggest : suggests_->get_suggests())
                for (const auto& st : suggest.info_)
                    if (st.is_emoji())
                        emoji[st.fs_id_].push_back(suggest.emoji_);

            sets_ = parse_sets(iter_packs->value, emoji);

            for (auto& s : sets_)
                s->set_added(true);

            return true;
        }

        bool cache::parse_store(core::tools::binary_stream& _data)
        {
            rapidjson::Document doc;
            const auto& parse_result = doc.ParseInsitu(_data.read(_data.available()));
            if (parse_result.HasParseError())
                return false;

            const auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd() || !iter_status->value.IsObject())
                return false;

            if (int code = 0; !tools::unserialize_value(iter_status->value, "code", code) || code != wim::robusto_protocol_error::ok)
                return false;

            const auto iter_data = doc.FindMember("result");
            if (iter_data == doc.MemberEnd() || !iter_data->value.IsObject())
                return false;

            const auto iter_sets = iter_data->value.FindMember("showcase");
            if (iter_sets == iter_data->value.MemberEnd() || !iter_sets->value.IsArray())
                return false;

            store_ = parse_sets(iter_sets->value, {});
            return true;
        }

        std::wstring cache::get_meta_file_name() const
        {
            return su::wconcat(g_stickers_path, L'/', meta_file_name());
        }

        std::wstring cache::get_meta_etag_file_name() const
        {
            return su::wconcat(g_stickers_path, L'/', meta_etag_file_name());
        }

        std::wstring cache::get_sticker_path(int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, sticker_size _size)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L'/';

            auto write_filesharing_id = [&ss_out, &_fs_id]()
            {
                ss_out << tools::from_utf8(_fs_id.file_id());
                if (const auto source = _fs_id.source_id())
                    ss_out << L'_' << tools::from_utf8(*source);
            };

            if (is_lottie_id(_fs_id.file_id()))
            {
                write_filesharing_id();
                ss_out << L'/' << "sticker" << lottie_ext();
            }
            else
            {
                if (_fs_id.file_id().empty())
                    ss_out << _set_id << L'/' << _sticker_id;
                else
                    write_filesharing_id();

                ss_out << L'/' << _size << image_ext();
            }

            return ss_out.str();
        }

        std::wstring cache::get_sticker_path(const set& _set, const sticker& _sticker, sticker_size _size)
        {
            return get_sticker_path(_set.get_id(), _sticker.get_id(), _sticker.fs_id() , _size);
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, const core::sticker_size _size) const
        {
            std::stringstream ss_size;

            ss_size << _size;

            return make_sticker_url(_set_id, _sticker_id, _fs_id, ss_size.str());
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, const std::string& _size) const
        {
            const auto& file_id = _fs_id.file_id();
            if (!file_id.empty())
            {
                const auto source_id = _fs_id.source_id();
                if (is_lottie_id(file_id))
                    return su::concat(urls::get_url(urls::url_type::sticker), '/', file_id, source_id ? su::concat("?source=", *source_id) : std::string());

                return su::concat(urls::get_url(urls::url_type::files_preview), "/max/sticker_", _size, '/', file_id, source_id ? su::concat("?source=", *source_id) : std::string());
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

        int cache::make_set_icons_tasks(std::string_view _size)
        {
            for (const auto& set : sets_)
            {
                if (!set->is_added())
                    continue;

                auto path = set->get_main_sticker_path(_size);
                if (!path.empty() && !core::tools::system::is_exist(path))
                {
                    auto dl_task = download_task(set->get_main_sticker_url(_size), "stikersMeta", std::move(path), set->get_id());
                    dl_task.set_need_decompress(is_lottie_id(set->get_main_sticker().fs_id().file_id()));
                    dl_task.set_type(download_task::type::icon);
                    pending_tasks_.emplace_back(std::move(dl_task));
                }
            }

            return pending_tasks_.size();
        }

        void cache::serialize_meta_set_sync(const stickers::set& _set, coll_helper _coll_set, std::string_view _size)
        {
            _coll_set.set_value_as_int("set_id", _set.get_id());
            _coll_set.set_value_as_string("name", _set.get_title());
            _coll_set.set_value_as_string("store_id", _set.get_store_id());
            _coll_set.set_value_as_bool("purchased", _set.is_added());
            _coll_set.set_value_as_string("description", _set.get_description());
            _coll_set.set_value_as_bool("lottie", _set.is_lottie_pack());

            if (!_size.empty())
            {
                const auto path = _set.get_main_sticker_path(_size);
                if (tools::system::is_exist(path))
                    _coll_set.set_value_as_string("icon_path", tools::from_utf16(path));
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
                const auto& filesharing_id = sticker->fs_id();
                coll_sticker.set_value_as_string("file_id", filesharing_id.file_id());
                if (const auto source = filesharing_id.source_id())
                    coll_sticker.set_value_as_string("source_id", *source);
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

        void cache::serialize_meta_sync(coll_helper _coll, std::string_view _size)
        {
            serialize_sets(_coll, sets_, _size);
        }

        void cache::serialize_store_sync(coll_helper _coll)
        {
            serialize_sets(_coll, store_);
        }

        const std::string& cache::get_etag() const
        {
            return etag_;
        }

        void cache::set_etag(std::string&& _etag)
        {
            etag_ = std::move(_etag);
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

        int cache::cancel_download_tasks(std::vector<core::tools::filesharing_id> _fs_ids, sticker_size _size)
        {
            const auto prevSize = pending_tasks_.size();
            pending_tasks_.remove_if([&_fs_ids, _size](const auto& _task)
            {
                return std::any_of(
                    _fs_ids.begin(),
                    _fs_ids.end(),
                    [&_task, _size](const auto& fs_id) { return _task.get_size() == _size && fs_id == _task.get_fs_id(); });
            });
            return pending_tasks_.size() - prevSize;
        }

        void cache::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, const sticker_size _size, std::wstring& _path)
        {
            auto get_iter = [_set_id, _sticker_id, _size, &_fs_id](auto& _list)
            {
                return std::find_if(_list.begin(), _list.end(), [_set_id, _sticker_id, _size, &_fs_id](const auto& _task)
                {
                    const auto same_size = _task.get_size() == _size;
                    const auto same_set_and_id = _set_id > 0 && _task.get_set_id() == _set_id && _task.get_sticker_id() == _sticker_id;
                    const auto same_fs_id = !_fs_id.is_empty() && _task.get_fs_id() == _fs_id;

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

                auto dl_task = download_task(std::move(sticker_url), "stikersGetSticker", _path, _set_id, _sticker_id, _fs_id, _size);
                dl_task.set_need_decompress(is_lottie_id(dl_task.get_fs_id().file_id()));
                pending_tasks_.emplace_back(std::move(dl_task));
            }
        }

        void cache::get_set_icon_big(const int64_t _seq, const int32_t _set_id, std::wstring& _path)
        {
            auto get_iter = [_set_id](auto& _list)
            {
                return std::find_if(_list.begin(), _list.end(), [_set_id](const auto& _task)
                {
                    return _task.get_set_id() == _set_id && _task.get_sticker_id() == -1 && _task.get_fs_id().is_empty();
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

            if (const auto& set = get_set(_set_id))
            {
                _path = set->get_main_sticker_path(big_size());
                if (!tools::system::is_exist(_path))
                {
                    auto dl_task = download_task(set->get_main_sticker_url(big_size()), "stikersBigIcon", _path, _set_id, -1);
                    dl_task.set_need_decompress(is_lottie_id(set->get_main_sticker().fs_id().file_id()));
                    pending_tasks_.emplace_back(std::move(dl_task));
                }
            }
        }

        void cache::clean_set_icon_big(const int32_t _set_id)
        {
            if (const auto& set = get_set(_set_id))
            {
                if (const auto path = set->get_main_sticker_path(big_size()); core::tools::system::is_exist(path))
                    core::tools::system::delete_file(path);
            }
        }

        void cache::serialize_suggests(coll_helper _coll)
        {
            suggests_->serialize(_coll);
        }


        //////////////////////////////////////////////////////////////////////////
        // stickers::face
        //////////////////////////////////////////////////////////////////////////
        face::face(const std::wstring& _stickers_path)
            : cache_(std::make_shared<cache>(_stickers_path))
            , thread_(std::make_shared<async_executer>("stickers"))
        {
        }

        std::shared_ptr<result_handler<bool>> face::parse(const std::shared_ptr<core::tools::binary_stream>& _data, bool _insitu)
        {
            auto handler = std::make_shared<result_handler<bool>>();

            thread_->run_async_function([stickers_cache = cache_, _data, _insitu]()->int32_t
            {
                return (stickers_cache->parse(*_data, _insitu) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const bool>> face::parse_store(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<const bool>>();

            thread_->run_async_function([stickers_cache = cache_, _data = std::move(_data)]()->int32_t
            {
                return (stickers_cache->parse_store(*_data) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0));
            };

            return handler;
        }

        std::shared_ptr<result_handler<std::wstring_view>> face::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, const core::sticker_size _size)
        {
            im_assert(_size > core::sticker_size::min);
            im_assert(_size < core::sticker_size::max);

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

        std::shared_ptr<result_handler<int>> face::cancel_download_tasks(std::vector<core::tools::filesharing_id> _fs_ids, core::sticker_size _size)
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

        std::shared_ptr<result_handler<coll_helper>> face::serialize_meta(coll_helper _coll, std::string_view _size)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();

            thread_->run_async_function([stickers_cache = cache_, _coll, size = std::string(_size)]()->int32_t
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

        std::shared_ptr<result_handler<int>> face::make_set_icons_tasks(std::string_view _size)
        {
            auto handler = std::make_shared<result_handler<int>>();
            thread_->run_async_function([stickers_cache = cache_, size = std::string(_size)]()->int32_t
            {
                return stickers_cache->make_set_icons_tasks(size);
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
            auto etag = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache = cache_, etag]()->int32_t
            {
                core::tools::binary_stream bs;
                if (!bs.load_from_file(stickers_cache->get_meta_file_name()))
                    return -1;

                bs.write(0);

                if (!stickers_cache->parse(bs, true))
                    return -1;

                core::tools::binary_stream etag_bs;
                if (auto loaded = etag_bs.load_from_file(stickers_cache->get_meta_etag_file_name()); loaded && etag_bs.available())
                {
                    stickers_cache->set_etag(std::string(etag_bs.get_data(), etag_bs.available()));
                    *etag = stickers_cache->get_etag();
                }

                return 0;

            })->on_result_ = [handler, etag](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(load_result((_error == 0), std::move(*etag)));
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

        std::shared_ptr<result_handler<bool>> face::set_etag(std::string _etag)
        {
            auto handler = std::make_shared<result_handler<bool>>();

            thread_->run_async_function([stickers_cache = cache_, _etag = std::move(_etag)]() mutable
            {
                int32_t res = 0;
                const auto path = stickers_cache->get_meta_etag_file_name();
                if (!_etag.empty())
                {
                    if (!core::tools::binary_stream::save_2_file(_etag, path))
                        res = -1;
                }
                else
                {
                    core::tools::system::delete_file(path);
                }

                stickers_cache->set_etag(std::move(_etag));

                return res;

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

        std::shared_ptr<result_handler<const std::string&>> face::get_etag()
        {
            auto handler = std::make_shared<result_handler<const std::string&>>();
            auto etag = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache = cache_, etag]()->int32_t
            {
                *etag = stickers_cache->get_etag();

                return 0;

            })->on_result_ = [handler, etag](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*etag);
            };

            return handler;
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

        static void serialize_image(coll_helper& _coll, const tools::binary_stream& _data, std::string_view _id)
        {
            if (const auto data_size = _data.available(); data_size > 0)
            {
                ifptr<istream> sticker_data(_coll->create_stream(), true);
                sticker_data->write((const uint8_t*)_data.read(data_size), data_size);
                _coll.set_value_as_stream(_id, sticker_data.get());
            }
        }

        static void post_sticker_impl(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, core::sticker_size _size, int _error, std::wstring_view _path)
        {
            im_assert(_size > sticker_size::min);
            im_assert(_size < sticker_size::max);

            coll_helper coll(g_core->create_collection(), true);

            if (_set_id >= 0)
                coll.set_value_as_int("set_id", _set_id);
            coll.set_value_as_int("sticker_id", _sticker_id);
            coll.set_value_as_string("file_id", _fs_id.file_id());
            if (const auto source = _fs_id.source_id())
                coll.set_value_as_string("source_id", *source);
            coll.set_value_as_int("error", _error);

            im_assert(!to_string(_size).empty());
            coll.set_value_as_string(to_string(_size), core::tools::from_utf16(_path));

            g_core->post_message_to_gui("stickers/sticker/get/result", _seq, coll.get());
        }

        void post_sticker_fail_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, loader_errors _error)
        {
            post_sticker_impl(_seq, _set_id, _sticker_id, _fs_id, core::sticker_size::small, (int)_error, {});
        }

        void post_sticker_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, core::sticker_size _size, std::wstring_view _path)
        {
            post_sticker_impl(_seq, _set_id, _sticker_id, _fs_id, _size, 0, _path);
        }

        void post_set_icon_2_gui(int32_t _set_id, std::string_view _message, std::wstring_view _path, int32_t _error)
        {
            im_assert(_set_id >= 0);

            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_int("error", _error);
            coll.set_value_as_int("set_id", _set_id);
            coll.set_value_as_string("icon_path", core::tools::from_utf16(_path));

            g_core->post_message_to_gui(_message, 0, coll.get());
        }
    }
}



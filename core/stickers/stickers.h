#pragma once

#include "../../corelib/collection_helper.h"
enum class loader_errors;

namespace core
{
    class async_executer;
    struct icollection;
    enum class sticker_size;

    namespace tools
    {
        class binary_stream;
    }

    namespace stickers
    {
        class suggests;

        class sticker_params
        {
            sticker_size size_;
            int32_t width_;
            int32_t height_;

        public:

            sticker_params(sticker_size _size, int32_t _width, int32_t _height);

            sticker_size get_size() const;

        };

        using size_map = std::map<sticker_size, sticker_params>;

        using emoji_vector = std::vector<std::string>;
        using emoji_map = std::unordered_map<std::string, emoji_vector>;

        class sticker
        {
            int32_t id_ = 0;
            size_map sizes_;
            std::string file_sharing_id_;
            emoji_vector emojis_;

        public:

            int32_t get_id() const;
            const std::string& fs_id() const;

            const size_map& get_sizes() const;

            const emoji_vector& get_emojis() const;

            bool unserialize(const rapidjson::Value& _node, const emoji_map& _emojis);
            bool unserialize(const rapidjson::Value& _node);
        };


        enum class set_icon_size
        {
            invalid = 0,

            scalable = 1,
            big = 2,

            _20	= 20,
            _32 = 32,
            _48 = 48,
            _64 = 64,
        };

        class set_icon
        {
            set_icon_size size_ = set_icon_size::invalid;
            std::string url_;

        public:
            set_icon() = default;
            set_icon(set_icon_size _size, std::string _url)
                : size_(_size)
                , url_(std::move(_url))
            {}

            set_icon_size get_size() const noexcept { return size_; }
            const std::string& get_url() const noexcept { return url_; }
            bool is_valid() const noexcept { return size_ != set_icon_size::invalid; }
        };

        using icons_map = std::map<stickers::set_icon_size, set_icon>;
        using stickers_vector = std::vector<std::shared_ptr<sticker>>;

        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class set
        {
            int32_t id_ = -1;

            bool show_ = true;

            bool purchased_ = true;

            bool user_ = false;

            std::string name_;

            std::string store_id_;

            std::string description_;

            std::string subtitle_;

            icons_map icons_;

            stickers_vector stickers_;

            set_icon big_icon_;

        public:

            int32_t get_id() const;
            void set_id(int32_t _id);

            const std::string& get_name() const;
            void set_name(std::string&& _name);

            void put_icon(set_icon&& _icon);
            const set_icon& get_icon(stickers::set_icon_size _size) const;

            bool is_show() const;
            void set_show(const bool _show);

            bool is_purchased() const;
            void set_purchased(const bool _purchased);

            bool is_user() const;
            void set_user(const bool _user);

            const std::string& get_store_id() const;
            void set_store_id(std::string&& _store_id);

            const std::string& get_description() const;
            void set_description(std::string&& _description);

            const std::string& get_subtitle() const;
            void set_subtitle(std::string&& _subtitle);

            const icons_map& get_icons() const;
            const stickers_vector& get_stickers() const;

            const set_icon& get_big_icon() const;
            void set_big_icon(set_icon&& _icon);

            bool unserialize(const rapidjson::Value& _node, const emoji_map& _emojis);
            bool unserialize(const rapidjson::Value& _node);

            bool is_lottie_pack() const;
        };

        using sets_list = std::list<std::shared_ptr<stickers::set>>;

        template<class t0_, class t1_ = void, class t2_ = void>
        class result_handler
        {
        public:
            std::function<void(t0_, t1_, t2_)>	on_result_;
        };

        template<class t0_>
        class result_handler<t0_, void, void>
        {
        public:
            std::function<void(t0_)>	on_result_;
        };

        template<class t0_, class t1_>
        class result_handler<t0_, t1_, void>
        {
        public:
            std::function<void(t0_, t1_)>	on_result_;
        };

        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class parse_result
        {
            bool result_;
            bool up_to_date_;

        public:

            parse_result(bool _result, bool _up_to_date) :	result_(_result), up_to_date_(_up_to_date) {}

            bool is_success() const { return result_; }
            bool is_up_to_date() const { return up_to_date_; }
        };

        class load_result
        {
            bool result_;
            const std::string md5_;

        public:

            load_result(bool _result, std::string _md5) :	result_(_result), md5_(std::move(_md5)) {}

            bool get_result() const { return result_; }
            const std::string& get_md5() const { return md5_; }
        };


        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class download_task
        {
        public:
            download_task(
                std::string _source_url,
                std::string _endpoint,
                std::wstring _dest_file,
                int32_t _set_id,
                int32_t _sticker_id,
                std::string _fs_id,
                sticker_size _size);

            const std::string& get_source_url() const;
            const std::string& get_endpoint() const;
            const std::wstring& get_dest_file() const;
            int32_t get_set_id() const;
            int32_t get_sticker_id() const;
            const std::string& get_fs_id() const;
            sticker_size get_size() const;

            void set_need_decompress(bool _decompress) noexcept { decompress_ = _decompress; }
            bool need_decompress() const noexcept { return decompress_; }

            enum class type
            {
                sticker,
                icon,
            };
            void set_type(type _type) noexcept { type_ = _type; }
            type get_type() const noexcept { return type_; }
            bool is_icon_task() const noexcept { return type_ == type::icon; }

        private:
            std::string source_url_;
            std::string endpoint_;
            std::wstring dest_file_;

            int32_t set_id_;
            int32_t sticker_id_;
            sticker_size size_;
            std::string fs_id_;
            bool decompress_ = false;
            type type_ = type::sticker;
        };

        using download_tasks = std::list<download_task>;

        //////////////////////////////////////////////////////////////////////////
        // class cache
        //////////////////////////////////////////////////////////////////////////
        class cache
        {
            std::string md5_;
            sets_list sets_;
            sets_list store_;

            std::string template_url_preview_;
            std::string template_url_original_;
            std::string template_url_send_;

            download_tasks pending_tasks_;
            download_tasks active_tasks_;

            std::unique_ptr<suggests> suggests_;

            int32_t stat_dl_tasks_count_ = 0;
            std::chrono::steady_clock::time_point stat_dl_start_time_;

            std::string make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const core::sticker_size _size) const;
            std::string make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const std::string& _size) const;

            const std::shared_ptr<stickers::set>& get_set(int32_t _set_id) const;

            void on_download_tasks_added(int _tasks_count);

        public:
            int make_set_icons_tasks(const std::string& _size);

            cache(const std::wstring& _stickers_path);
            virtual ~cache();

            std::wstring get_meta_file_name() const;
            bool parse(core::tools::binary_stream& _data, bool _insitu, bool& _up_todate);
            bool parse_store(core::tools::binary_stream& _data);

            static void serialize_meta_set_sync(const stickers::set& _set, coll_helper _coll_set, const std::string& _size);
            void serialize_meta_sync(coll_helper _coll, const std::string& _size);
            void serialize_store_sync(coll_helper _coll);

            static std::wstring get_set_icon_path(int32_t _set_id, const set_icon& _icon);
            std::pair<std::wstring, set_icon_size> get_set_big_icon_path(int32_t _set_id);
            static std::wstring get_sticker_path(const set& _set, const sticker& _sticker, sticker_size _size);
            static std::wstring get_sticker_path(int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, sticker_size _size);

            std::string make_big_icon_url(const int32_t _set_id);

            download_tasks take_download_tasks();
            bool have_tasks_to_download() const noexcept;
            int32_t on_task_loaded(const download_task& _task);
            int cancel_download_tasks(std::vector<std::string> _fs_ids, sticker_size _size);

            void get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string _fs_id, const sticker_size _size, std::wstring& _path);
            void get_set_icon_big(const int64_t _seq, const int32_t _set_id, std::wstring& _path);
            void clean_set_icon_big(const int32_t _set_id);
            const std::string& get_md5() const;

            void serialize_suggests(coll_helper _coll);

            void update_template_urls(std::string _preview_url, std::string _original_url, std::string _send_url);
        };

        struct gui_request_params
        {
            std::string size_;
            int64_t seq_ = -1;
            std::string md5_;

            gui_request_params(const std::string& _size, int64_t _seq, const std::string& _md5)
                :   size_(_size), seq_(_seq), md5_(_md5) {}

            gui_request_params() = default;
        };

        class face
        {
            std::shared_ptr<cache> cache_;
            std::shared_ptr<async_executer> thread_;

            bool meta_requested_ = false;
            bool up_to_date_ = false;
            bool download_meta_in_progress_ = false;
            bool download_meta_error_ = false;
            bool download_stickers_error_ = false;
            bool flag_meta_need_reload_ = false;

            gui_request_params gui_request_params_;

        public:

            face(const std::wstring& _stickers_path);

            std::shared_ptr<result_handler<const parse_result&>> parse(
                const std::shared_ptr<core::tools::binary_stream>& _data,
                bool _insitu);

            std::shared_ptr<result_handler<const bool>> parse_store(std::shared_ptr<core::tools::binary_stream> _data);

            std::shared_ptr<result_handler<const load_result&>> load_meta_from_local();
            std::shared_ptr<result_handler<bool>> save(std::shared_ptr<core::tools::binary_stream> _data);
            std::shared_ptr<result_handler<int>> make_set_icons_tasks(const std::string& _size);
            std::shared_ptr<result_handler<coll_helper>> serialize_meta(coll_helper _coll, const std::string& _size);
            std::shared_ptr<result_handler<coll_helper>> serialize_store(coll_helper _coll);
            std::shared_ptr<result_handler<std::wstring_view>> get_sticker(
                int64_t _seq,
                int32_t _set_id,
                int32_t _sticker_id,
                std::string _fs_id,
                const core::sticker_size _size);

            std::shared_ptr<result_handler<int>> cancel_download_tasks(std::vector<std::string> _fs_ids, core::sticker_size _size);

            std::shared_ptr<result_handler<std::wstring_view>> get_set_icon_big(const int64_t _seq, const int32_t _set_id);
            void clean_set_icon_big(const int64_t _seq, const int32_t _set_id);

            std::shared_ptr<result_handler<const download_tasks&>> take_download_tasks();
            std::shared_ptr<result_handler<int>> on_task_loaded(const download_task& _task);

            std::shared_ptr<result_handler<const std::string&>> get_md5();

            void set_gui_request_params(const gui_request_params& _params);
            const gui_request_params& get_gui_request_params();

            void set_up_to_date(bool _up_to_date);
            bool is_up_to_date() const;

            void set_download_meta_error(const bool _is_error);
            bool is_download_meta_error() const;

            bool is_download_meta_in_progress();
            void set_download_meta_in_progress(const bool _in_progress);

            void set_flag_meta_need_reload(const bool _need_reload);
            bool is_flag_meta_need_reload() const;

            void set_download_stickers_error(const bool _is_error);
            bool is_download_stickers_error() const;

            std::shared_ptr<result_handler<const bool>> serialize_suggests(coll_helper _coll);

            void update_template_urls(const rapidjson::Value& _value);
        };

        void post_sticker_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, core::sticker_size _size, std::wstring_view _data);
        void post_sticker_fail_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, std::string_view _fs_id, loader_errors _error);
        void post_set_icon_2_gui(int32_t _set_id, std::string_view _message, std::wstring_view _path, int32_t _error = 0);
    }
}

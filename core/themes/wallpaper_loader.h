#pragma once

#include "../../corelib/collection_helper.h"

namespace core
{
    class async_executer;

    namespace tools
    {
        class binary_stream;
    }

    namespace themes
    {
        void post_wallpaper_2_gui(const std::string_view _id, const tools::binary_stream& _data);
        void post_preview_2_gui(const std::string_view _id, const tools::binary_stream& _data);

        struct wallpaper_info
        {
            std::string id_;
            std::string full_image_url_;
            std::string preview_url_;
        };

        using wallpaper_info_v = std::vector<wallpaper_info>;

        class download_task
        {
            std::string source_url_;
            std::string endpoint_;
            std::wstring dest_file_;

            time_t modified_time_;

            std::string id_;

            bool is_requested_;
            bool is_preview_;

        public:
            download_task(const std::string_view _source_url, const std::string_view _endpoint, const std::wstring& _dest_file, time_t _modified_time, const std::string_view _id)
                : source_url_(_source_url)
                , endpoint_(_endpoint)
                , dest_file_(_dest_file)
                , modified_time_(_modified_time)
                , id_(_id)
                , is_requested_(false)
                , is_preview_(false)
            {}

            const std::string& get_source_url() const noexcept { return source_url_; }
            const std::string& get_endpoint() const noexcept { return endpoint_; }
            const std::wstring& get_dest_file() const noexcept { return dest_file_; }
            time_t get_last_modified_time() const noexcept { return modified_time_; }
            const std::string& get_id() const noexcept { return id_; }
            void set_is_preview(const bool _is_preview) { is_preview_ = _is_preview; }
            bool is_preview() const noexcept { return is_preview_; }

            bool is_requested() const noexcept { return is_requested_; }
            void set_requested(const bool _is_requested) { is_requested_ = _is_requested; }
        };
        using download_tasks = std::list<download_task>;
        using possible_dl_task = std::optional<download_task>;

        template<class T0, class T1 = void>
        class result_handler
        {
        public:
            std::function<void(T0, T1)>	on_result_;
        };

        template<class T0>
        class result_handler<T0, void>
        {
        public:
            std::function<void(T0)>	on_result_;
        };

        using next_dl_task_handler = result_handler<const possible_dl_task&>;
        using image_data_handler = result_handler<const tools::binary_stream&>;

        class wallpaper_loader : public std::enable_shared_from_this<wallpaper_loader>
        {
        public:
            wallpaper_loader();

            std::shared_ptr<next_dl_task_handler> get_next_download_task();
            void on_task_downloaded(const std::string_view _url);

            std::shared_ptr<image_data_handler> get_wallpaper_full(const std::string_view _id);
            std::shared_ptr<image_data_handler> get_wallpaper_preview(const std::string_view _id);
            std::shared_ptr<result_handler<bool>> save(std::shared_ptr<core::tools::binary_stream> _data);

            void save_wallpaper(const std::string_view _wallpaper_id, std::shared_ptr<core::tools::binary_stream> _data);
            void remove_wallpaper(const std::string_view _wallpaper_id);

            void clear_missing_wallpapers();

        private:
            void set_wallpaper_urls();
            void get_wallpaper_full(const std::string_view _wp_id, tools::binary_stream& _data);
            void get_wallpaper_preview(const std::string_view _wp_id, tools::binary_stream& _data);

            void create_download_task_full(const wallpaper_info& _wall, const bool _is_requested = false);
            void create_download_task_preview(const wallpaper_info& _wall, const bool _is_requested = false);

            void clear_all();
            void clear_missing();
            void clear_wallpaper(const std::string_view _wallpaper_id);

            std::shared_ptr<async_executer> thread_;

            wallpaper_info_v wallpapers_;

            download_tasks download_tasks_;
        };
    }
}

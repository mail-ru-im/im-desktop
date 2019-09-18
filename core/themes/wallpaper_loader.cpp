#include "stdafx.h"

#include "wallpaper_loader.h"
#include "../utils.h"
#include "../async_task.h"
#include "../tools/binary_stream.h"
#include "../tools/strings.h"

namespace fs = boost::filesystem;

namespace
{
    std::wstring get_folder_id(const std::string_view _wall_id)
    {
        return core::tools::from_utf8(_wall_id);
    }

    std::wstring get_folder(const std::string_view _wall_id)
    {
        return core::utils::get_themes_path() + L"/" + get_folder_id(_wall_id);
    }

    std::wstring get_preview_path(const std::string_view _wall_id)
    {
        return get_folder(_wall_id) + L"/thumb.jpg";
    }

    std::wstring get_wp_path(const std::string_view _wall_id)
    {
        return get_folder(_wall_id) + L"/image.jpg";
    }

    time_t get_file_time(const fs::path& _path)
    {
        boost::system::error_code e;
        if (fs::exists(_path, e))
            return fs::last_write_time(_path, e);
        return 0;
    }

    time_t get_preview_file_time(const std::string_view _wall_id)
    {
        return get_file_time(get_preview_path(_wall_id));
    }

    time_t get_wp_file_time(const std::string_view _wall_id)
    {
        return get_file_time(get_wp_path(_wall_id));
    }

    void post_data_2_gui(const std::string_view _id, const core::tools::binary_stream& _data, const std::string_view _message)
    {
        core::coll_helper coll(core::g_core->create_collection(), true);

        coll.set_value_as_string("id", _id);

        core::ifptr<core::istream> theme_image_data(coll->create_stream(), true);
        if (const auto data_size = _data.available(); data_size > 0)
        {
            theme_image_data->write((const uint8_t*)_data.read(data_size), data_size);
            coll.set_value_as_stream("data", theme_image_data.get());
        }
        else
        {
            coll.set_value_as_int("error", 1);
        }

        core::g_core->post_message_to_gui(_message, 0, coll.get());
    }
}

namespace core
{
    namespace themes
    {
        void post_wallpaper_2_gui(const std::string_view _id, const tools::binary_stream& _data)
        {
            post_data_2_gui(_id, _data, "themes/wallpaper/get/result");
        }

        void post_preview_2_gui(const std::string_view _id, const tools::binary_stream& _data)
        {
            post_data_2_gui(_id, _data, "themes/wallpaper/preview/get/result");
        }

        wallpaper_loader::wallpaper_loader()
          :  thread_(std::make_shared<async_executer>("themes"))
        {
            set_wallpaper_urls();
        }

        void wallpaper_loader::set_wallpaper_urls()
        {
            wallpapers_ = g_core->get_wallpaper_urls();
            assert(!wallpapers_.empty());
        }

        std::shared_ptr<next_dl_task_handler> wallpaper_loader::get_next_download_task()
        {
            auto handler = std::make_shared<next_dl_task_handler>();
            auto task_data = std::make_shared<possible_dl_task>();

            thread_->run_async_function([wr_this = weak_from_this(), task_data]()->int32_t
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return 0;

                if (!ptr_this->download_tasks_.empty())
                {
                    *task_data = std::make_optional(ptr_this->download_tasks_.front());
                    ptr_this->download_tasks_.pop_front();
                }

                return 0;

            })->on_result_ = [handler, task_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*task_data);
            };

            return handler;
        }

        void wallpaper_loader::on_task_downloaded(const std::string_view _url)
        {
            thread_->run_async_function([wr_this = weak_from_this(), task_url = std::string(_url)]()->int32_t
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return 0;

                auto& tasks = ptr_this->download_tasks_;
                const auto it = std::find_if(tasks.begin(), tasks.end(), [&task_url](const auto& _task) { return _task.get_source_url() == task_url; });
                if (it != tasks.end())
                    tasks.erase(it);

                return 0;
            });
        }

        std::shared_ptr<image_data_handler> wallpaper_loader::get_wallpaper_full(const std::string_view _id)
        {
            auto handler = std::make_shared<image_data_handler>();
            auto image_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([wr_this = weak_from_this(), image_data, id = std::string(_id)]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return -1;

                ptr_this->get_wallpaper_full(id, *image_data);
                return 0;

            })->on_result_ = [handler, image_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*image_data);
            };

            return handler;
        }

        std::shared_ptr<image_data_handler> wallpaper_loader::get_wallpaper_preview(const std::string_view _id)
        {
            auto handler = std::make_shared<image_data_handler>();
            auto image_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([wr_this = weak_from_this(), image_data, id = std::string(_id)]
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return -1;

                    ptr_this->get_wallpaper_preview(id, *image_data);
                    return 0;

                })->on_result_ = [handler, image_data](int32_t _error)
                {
                    if (handler->on_result_)
                        handler->on_result_(*image_data);
                };

                return handler;
        }

        void wallpaper_loader::get_wallpaper_full(const std::string_view _id, tools::binary_stream& _data)
        {
            const auto dl_it = std::find_if(download_tasks_.begin(), download_tasks_.end(), [_id](const auto& _task)
            {
                return _task.get_id() == _id;
            });

            if (dl_it != download_tasks_.end())
            {
                download_task task = *dl_it;
                download_tasks_.erase(dl_it);

                task.set_requested(true);
                download_tasks_.push_front(task);
            }
            else if (!_data.load_from_file(get_wp_path(_id)))
            {
                const auto it = std::find_if(wallpapers_.begin(), wallpapers_.end(), [_id](const auto& _w)
                {
                    return _w.id_ == _id;
                });

                if (it != wallpapers_.end())
                    create_download_task_full(*it, true);
            }
        }

        void wallpaper_loader::get_wallpaper_preview(const std::string_view _id, tools::binary_stream& _data)
        {
            const auto dl_it = std::find_if(download_tasks_.begin(), download_tasks_.end(), [_id](const auto& _task)
            {
                return _task.get_id() == _id && _task.is_preview();
            });

            if (dl_it != download_tasks_.end())
            {
                download_task task = *dl_it;
                download_tasks_.erase(dl_it);

                task.set_requested(true);
                download_tasks_.push_front(task);
            }
            else if (!_data.load_from_file(get_preview_path(_id)))
            {
                const auto it = std::find_if(wallpapers_.begin(), wallpapers_.end(), [_id](const auto& _w)
                {
                    return _w.id_ == _id;
                });

                if (it != wallpapers_.end())
                    create_download_task_preview(*it, true);
            }
        }

        void wallpaper_loader::create_download_task_full(const wallpaper_info& _wall, const bool _is_requested)
        {
            assert(std::none_of(download_tasks_.begin(), download_tasks_.end(), [&_wall](const auto& _task) { return _task.get_source_url() == _wall.preview_url_; }));

            const auto image_file_name = get_wp_path(_wall.id_);
            const auto image_time = get_wp_file_time(_wall.id_);
            download_tasks_.emplace_back(_wall.full_image_url_, "wallpapersImage", image_file_name, image_time, _wall.id_);
            download_tasks_.back().set_requested(_is_requested);
        }

        void wallpaper_loader::create_download_task_preview(const wallpaper_info& _wall, const bool _is_requested)
        {
            assert(std::none_of(download_tasks_.begin(), download_tasks_.end(), [&_wall](const auto& _task) { return _task.get_source_url() == _wall.preview_url_; }));

            const auto thumb_file_name = get_preview_path(_wall.id_);
            const auto thumb_time = get_preview_file_time(_wall.id_);
            download_tasks_.emplace_back(_wall.preview_url_, "wallpapersThumb", thumb_file_name, thumb_time, _wall.id_);
            download_tasks_.back().set_is_preview(true);
            download_tasks_.back().set_requested(_is_requested);
        }

        std::shared_ptr<result_handler<bool>> wallpaper_loader::save(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<bool>>();

            thread_->run_async_function([_data]()->int32_t
            {
                return (_data->save_2_file(core::utils::get_themes_meta_path()) ? 0 : -1);
            }
            )->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        void wallpaper_loader::save_wallpaper(const std::string_view _wallpaper_id, std::shared_ptr<core::tools::binary_stream> _data)
        {
            thread_->run_async_function([_data, id = std::string(_wallpaper_id)]()->int32_t
            {
                return (_data->save_2_file(get_wp_path(id)) ? 0 : -1);
            });
        }

        void wallpaper_loader::remove_wallpaper(const std::string_view _wallpaper_id)
        {
            thread_->run_async_function([wr_this = weak_from_this(), id = std::string(_wallpaper_id)]()->int32_t
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return -1;

                ptr_this->clear_wallpaper(id);
                return 0;
            });
        }

        void wallpaper_loader::clear_missing_wallpapers()
        {
            thread_->run_async_function([wr_this = weak_from_this()]()->int32_t
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return 0;

                ptr_this->clear_missing();
                return 0;
            });
        }

        void wallpaper_loader::clear_all()
        {
            boost::system::error_code e;
            fs::remove_all(core::utils::get_themes_path(), e);
        }

        void wallpaper_loader::clear_missing()
        {
            boost::system::error_code e;

            fs::path themes_root(core::utils::get_themes_path());
            if (!fs::exists(themes_root, e))
                return;

            std::list<fs::path> to_delete;

            try
            {
                fs::directory_iterator end;

                for (fs::directory_iterator dir_it(themes_root, e); dir_it != end && !e; dir_it.increment(e))
                {
                    fs::path path = (*dir_it);
                    const auto dir_name = path.leaf().string();
                    if (!dir_name.empty() && core::tools::is_digit(dir_name.front()))
                    {
                        if (std::none_of(wallpapers_.begin(), wallpapers_.end(), [&dir_name](const auto& _wallpaper) { return _wallpaper.id_ == dir_name; }))
                            to_delete.emplace_back(path);
                    }
                }
            }
            catch (const std::exception&)
            {

            }

            for (auto& path : to_delete)
                fs::remove_all(path, e);
        }

        void wallpaper_loader::clear_wallpaper(const std::string_view _wallpaper_id)
        {
            boost::system::error_code e;

            fs::path path(get_folder(_wallpaper_id));
            if (fs::exists(path, e))
                fs::remove_all(path, e);
        }
    }
}

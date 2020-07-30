#pragma once

#include "../../../namespaces.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    struct icollection;

    namespace wim
    {
        class web_file_info;

        namespace PREVIEW_PROXY_NS
        {
            class link_meta;

            typedef std::unique_ptr<link_meta> link_meta_uptr;
        }

        struct upload_progress_handler
        {
            std::function<void(int32_t, const web_file_info& _info)> on_result;
            std::function<void(const web_file_info& _info)> on_progress;
        };

        struct download_progress_handler
        {
            std::function<void(int32_t, const web_file_info& _info)> on_result;
            std::function<void(const web_file_info& _info)> on_progress;
        };

        struct download_image_handler
        {
            typedef std::function<
                void(
                    int32_t _error,
                    std::shared_ptr<core::tools::binary_stream> _image_data,
                    const std::string &_image_uri,
                    const std::string &_local_path)
            > image_callback_t;

            typedef std::function<
                void(
                    int32_t _preview_width,
                    int32_t _preview_height,
                    const std::string &_download_uri,
                    const int64_t _file_size)
            > meta_callback_t;

            image_callback_t on_image_result;

            meta_callback_t on_meta_result;
        };

        struct download_link_metainfo_handler
        {
            std::function<void(int32_t _error, const preview_proxy::link_meta_uptr &_meta)> on_result;
        };

        struct get_file_direct_uri_handler
        {
            std::function<void(int32_t _error, const std::string& _uri)> on_result;
        };
    }
}
#pragma once

namespace core
{
    namespace wim
    {
        struct file_sharing_meta;

        typedef std::unique_ptr<file_sharing_meta> file_sharing_meta_uptr;

        struct file_sharing_meta
        {
            explicit file_sharing_meta(const std::string& _url);

            int             status_code_;
            std::string     file_url_;
            std::wstring    file_name_;
            int64_t         file_size_;
            std::string     file_name_short_;
            std::string     file_download_url_; // dlink
            std::string     mime_;
            std::string     file_mini_preview_url_;
            std::string     file_full_preview_url_;
            bool            recognize_; //ptt only
            bool            is_previewable_;
            bool            got_audio_;
            int32_t         duration_;
            uint64_t        last_modified_;
            std::string     local_path_;
            std::string     language_;

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            static file_sharing_meta_uptr parse_json(InOut char* _json, const std::string& _uri);
        };
    }
}

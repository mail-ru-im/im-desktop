#pragma once

namespace core
{
    namespace wim
    {
        struct fs_meta_info
        {
            bool is_previewable_ = false;
            int64_t file_size_ = 0;
            std::string file_name_;
            std::string dlink_;
            std::string mime_;
            std::string md5_;

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_meta_ptt
        {
            uint64_t duration_ = 0;
            bool is_recognized_ = false;
            std::string language_;
            std::string text_;

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_meta_video
        {
            uint64_t duration_ = 0;
            bool has_audio_ = false;

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_meta_audio
        {
            uint64_t duration_ = 0;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_extra_meta
        {
            std::string file_type_;
            std::optional<fs_meta_ptt> ptt_;
            std::optional<fs_meta_video> video_;
            std::optional<fs_meta_audio> audio_;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_meta_previews
        {
            std::string file_mini_preview_url_;
            std::string file_full_preview_url_;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct fs_meta_local
        {
            uint64_t last_modified_ = 0;
            std::string local_path_;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct file_sharing_meta;

        using file_sharing_meta_uptr = std::unique_ptr<file_sharing_meta>;

        struct file_sharing_meta
        {
            explicit file_sharing_meta(std::string&& _url);

            int status_code_ = 0;
            std::string file_url_;

            fs_meta_info info_;
            fs_extra_meta extra_;

            std::optional<fs_meta_previews> previews_;

            std::optional<fs_meta_local> local_;

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            static file_sharing_meta_uptr parse_json(InOut char* _json, std::string_view _uri);

            int64_t get_status_code() const { return status_code_; }
        };
    }
}

#pragma once

namespace core
{
    struct icollection;

    namespace wim
    {
        class cached_chat
        {
        public:
            cached_chat() = default;
            cached_chat(std::string_view _aimid, int64_t _msg_id, std::string_view _thread_id);

            const std::string& get_aimid() const noexcept { return aimid_; }
            const std::unordered_map<int64_t, std::string>& get_threads() const noexcept { return threads_; }

            void add_thread(int64_t _msg_id, std::string_view _thread_id);
            bool remove_thread(std::string_view _thread_id);
            void remove_thread(int64_t _msg_id);

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

        private:
            std::string	aimid_;
            std::unordered_map<int64_t, std::string> threads_;
        };

        class chats_threads_cache
        {
        public:
            chats_threads_cache() = default;

            void add_thread(std::string_view _aimid, int64_t _msg_id, std::string_view _thread_id);
            void remove_thread(std::string_view _aimid, std::string_view _thread_id);
            void remove_thread(std::string_view _thread_id);
            void remove_thread(std::string_view _aimid, int64_t _msg_id);
            void remove_chat(std::string_view _aimid);
            void clear();

            size_t size() const noexcept { return chats_.size(); }

            std::set<std::string> threads(std::string_view _aimid = {});

            const std::unordered_map<std::string, cached_chat>& contacts() const { return chats_; }

            bool contains(std::string_view _aimid) const;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            bool is_changed() const { return true; }
            void set_changed(bool) {}

        private:
            std::unordered_map<std::string, cached_chat> chats_;
        };
    }
}

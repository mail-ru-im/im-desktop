#ifndef __MENTIONS_ME_H_
#define __MENTIONS_ME_H_

namespace core
{
    namespace archive
    {
        class archive_index;
        class contact_archive;
        class history_message;
        class storage;

        using history_block = std::vector<std::shared_ptr<history_message>>;

        class mentions_me
        {
        private:
            using mentions_map = std::map<int64_t, std::shared_ptr<archive::history_message>>;

        public:

            mentions_me(std::wstring _file_name);
            ~mentions_me();

            void serialize(const std::shared_ptr<archive::history_message>& _mention, core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);

            history_block get_messages() const;

            bool load_from_local();

            void delete_up_to(int64_t _id);

            std::optional<int64_t> first_id() const;

            void add(const std::shared_ptr<archive::history_message>& _message);

            bool save_all();

        private:
            storage::result_type save(const std::shared_ptr<archive::history_message>& _message);

        private:
            bool loaded_from_local_ = false;
            std::unique_ptr<storage> storage_;

            mentions_map messages_;
        };
    }
}

#endif//__MENTIONS_ME_H_

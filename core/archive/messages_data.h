#pragma once

#include "history_message.h"
#include "dlg_state.h"
#include "storage.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    namespace search
    {
        using found_messages = std::unordered_map<std::string, std::vector<archive::history_message_sptr>>;
    }

    namespace archive
    {
        class message_header;
        class headers_block;

        using history_block = std::vector< std::shared_ptr<history_message> >;
        using headers_list = std::list<message_header>;
        using contact_and_msgs = std::vector<std::pair<std::string, int64_t>>;

        using contact_and_offset = std::tuple<std::string, std::shared_ptr<int64_t>, std::shared_ptr<int64_t>>;
        using contact_and_offsets_v = std::vector<contact_and_offset>;

        struct coded_term
        {
            std::string lower_term;
            std::vector<std::pair<std::string, int32_t>> symb_table;
            std::string symbs;
            std::vector<int32_t> coded_string;
            std::vector<int32_t> prefix;
            std::vector<int32_t> symb_indexes;
        };

        class messages_data
        {
            std::unique_ptr<storage> storage_;

            history_block get_message_modifications(const message_header& _header) const;

        public:

            messages_data(std::wstring _file_name);
            ~messages_data();

            storage::result_type update(const history_block& _data);

            using error_vector = std::vector<std::pair<int64_t, int32_t>>;

            error_vector get_messages(const headers_list& _headers, history_block& _messages) const;

            void drop();

            static void search_in_archive(std::shared_ptr<contact_and_offsets_v> _contacts, std::shared_ptr<coded_term> _cterm
                , std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data
                , search::found_messages& found_messages
                , int64_t _min_id);

            static bool get_history_archive(const std::wstring& _file_name, core::tools::binary_stream& _buffer
                , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode);
        };

    }
}

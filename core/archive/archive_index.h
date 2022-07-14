#ifndef __CONTACT_ARCHIVE_INDEX_H_
#define __CONTACT_ARCHIVE_INDEX_H_

#pragma once

#include "history_message.h"
#include "dlg_state.h"
#include "message_flags.h"
#include "errors.h"
#include "storage.h"

namespace core
{
    namespace archive
    {
        typedef std::vector<std::shared_ptr<history_message>> history_block;
        typedef std::shared_ptr<history_block> history_block_sptr;

        typedef std::list<message_header> headers_list;
        typedef std::shared_ptr<headers_list> headers_list_sptr;

        typedef std::map<int64_t, message_header> headers_map;

        class archive_hole
        {
            int64_t from_ = -1;
            int64_t to_ = -1;
            int64_t depth_ = 0;

        public:
            int64_t get_from() const noexcept { return from_; }
            void set_from(int64_t _value) noexcept { from_ = _value; }
            bool has_from() const noexcept { return (from_ > 0); }

            int64_t get_to() const noexcept { return to_; }
            void set_to(int64_t _value) noexcept { to_ = _value; }
            bool has_to() const noexcept { return (to_ > 0); }

            void set_depth(int64_t _depth) noexcept { depth_ = _depth; }
            int64_t get_depth() const noexcept { return depth_; }
        };

        enum class archive_hole_error
        {
            ok,
            last_reached,
            depth_reached,
            index_not_found,
            no_hole
        };


        //////////////////////////////////////////////////////////////////////////
        // archive_index class
        //////////////////////////////////////////////////////////////////////////
        class archive_index
        {
            archive::error last_error_;
            headers_map headers_index_;
            std::unique_ptr<storage> storage_;
            int32_t outgoing_count_;
            bool loaded_from_local_;
            std::string aimid_;

            int32_t merged_count_;

            template <typename R>
            void serialize_block(const R& _headers, core::tools::binary_stream& _data) const;
            bool unserialize_block(core::tools::binary_stream& _data, const std::chrono::seconds _current_time);
            void insert_block(archive::headers_list& _headers);
            void insert_header(archive::message_header& header, const std::chrono::seconds _current_time);

            void notify_core_outgoing_msg_count();

            int32_t get_outgoing_count() const;
            bool is_outgoing_header(const archive::message_header& _header, const std::chrono::seconds _current_time) const;

            void drop_outdated_headers(const std::chrono::seconds _current_time);

        public:

            enum class include_from_id
            {
                no,
                yes
            };

            enum class mark_as_updated
            {
                no,
                yes
            };

            bool get_header(int64_t _msgid, message_header& _header) const;

            bool has_header(const int64_t _msgid) const;

            bool has_hole_in_range(int64_t _msgid, int32_t _count_before, int32_t _count_after) const;
            archive::archive_hole_error get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const;
            std::vector<int64_t> get_messages_for_update()const;
            int64_t validate_hole_request(const archive_hole& _hole, const int32_t _count) const;

            bool save_all();
            archive::storage::result_type save_block(const archive::headers_list& _block);

            void filter_deleted(std::vector<int64_t>& _ids) const;

            void optimize();
            bool need_optimize() const;
            void free();

            bool load_from_local();

            void drop_header(int64_t _msgid);

            bool serialize_from(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _list, include_from_id _mode = include_from_id::no) const;
            archive::storage::result_type update(const archive::history_block& _data, /*out*/ headers_list& _headers);

            void delete_up_to(const int64_t _to);

            int64_t get_last_msgid() const;
            int64_t get_first_msgid() const;

            archive::error get_last_error() const { return last_error_; }

            void make_holes();
            void invalidate();
            void invalidate_message_data(int64_t _msgid, mark_as_updated _mark_as_updated = mark_as_updated::no);
            void invalidate_message_data(int64_t _from, int64_t _before_count, int64_t _after_count);
            int64_t get_memory_usage() const;

            archive_index(std::wstring _file_name, std::string _aimid);
            ~archive_index();
        };

    }
}


#endif //__CONTACT_ARCHIVE_INDEX_H_
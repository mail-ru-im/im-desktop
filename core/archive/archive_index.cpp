#include "stdafx.h"

#include "archive_index.h"
#include "storage.h"
#include "options.h"
#include "core.h"
#include "network_log.h"

#include <limits>

using namespace core;
using namespace archive;

namespace
{
    template<typename T>
    T skip_patches_forward(T first, T last)
    {
        while (first != last)
        {
            const auto &header = first->second;
            if (!header.is_patch())
                return first;
            ++first;
        }
        return last;
    }

    constexpr auto outgoing_count_time_span = std::chrono::hours(24 * 21); // 21 days
    constexpr auto max_merged_percent = 0.1; // 10%
}

//////////////////////////////////////////////////////////////////////////
// archive_index class
//////////////////////////////////////////////////////////////////////////

constexpr int32_t max_index_size            = 25000;
constexpr int32_t index_size_need_optimize    = 30000;

enum archive_index_types : uint32_t
{
    header        = 1,
    msgid        = 2,
    msgflags    = 3,
};

archive_index::archive_index(std::wstring _file_name, std::string _aimid)
    : last_error_(archive::error::ok)
    , storage_(std::make_unique<storage>(std::move(_file_name)))
    , outgoing_count_(0)
    , loaded_from_local_(false)
    , aimid_(std::move(_aimid))
    , merged_count_(0)
{
}


archive_index::~archive_index() = default;

bool archive_index::serialize_from(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _list, include_from_id _mode) const
{
    if (headers_index_.empty())
        return true;
    using size_type = decltype(headers_index_)::size_type;

    assert(_count_early == -1 || size_type(_count_early) < std::numeric_limits<size_type>::max());
    assert(_count_later == -1 || size_type(_count_later) < std::numeric_limits<size_type>::max());
    assert(size_type(std::max(_count_later, int64_t(0)) + std::max(_count_early, int64_t(0))) < std::numeric_limits<size_type>::max());

    const auto headers_end = headers_index_.end();
    const auto headers_begin = headers_index_.begin();
    auto iter_from = headers_end;

    if (_from != -1)
    {
        iter_from = headers_index_.lower_bound(_from);
        if (iter_from == headers_end)
        {
            assert(!"invalid index number");
            return false;
        }
    }

    if (_count_early > 0)
    {
        size_t not_deleted = 0;
        auto iter_header = iter_from;
        if (_mode == include_from_id::yes && _count_later <= 0 && iter_header != headers_end)
            ++iter_header; // to include fromId

        while (iter_header != headers_begin)
        {
            --iter_header;

            if (iter_header->second.is_patch())
                continue;

            if (not_deleted >= size_type(_count_early))
                break;

            if (!iter_header->second.is_deleted())
                ++not_deleted;

            _list.emplace_front(iter_header->second);
        };
    }
    if (_count_later > 0)
    {
        size_t not_deleted = 0;
        auto iter_header = iter_from;
        while (iter_header != headers_end)
        {
            if (iter_header->second.is_patch() || iter_header->second.is_deleted())
            {
                ++iter_header;
                continue;
            }
            if (!iter_header->second.is_deleted())
                ++not_deleted;
            _list.emplace_back(iter_header->second);
            ++iter_header;


            if (not_deleted >= _count_later)
                break;
        };
    }

    return true;
}

void archive_index::insert_block(archive::headers_list& _inserted_headers)
{
    const auto prev_outgoing_count = get_outgoing_count();

    const auto now = std::chrono::seconds(std::time(nullptr));

    for (auto &header : _inserted_headers)
        insert_header(header, now);

    if (get_outgoing_count() != prev_outgoing_count)
        notify_core_outgoing_msg_count();
}

void archive_index::insert_header(archive::message_header& header, const std::chrono::seconds _current_time)
{
    const auto msg_id = header.get_id();
    assert(msg_id > 0);

    const auto existing_iter = headers_index_.find(msg_id);

    if (existing_iter == headers_index_.end())
    {
        if (header.is_outgoing() && _current_time - std::chrono::seconds(header.get_time()) <= outgoing_count_time_span)
            ++outgoing_count_;

        headers_index_.emplace_hint(
            headers_index_.end(),
            std::make_pair(msg_id, std::cref(header))
            );

        return;
    }

    auto &existing_header = existing_iter->second;

    existing_header.merge_with(header);
    if (!existing_header.is_deleted() && !existing_header.is_modified())
        header = existing_header;

    if (!(header.is_patch() && header.is_modified()))
        ++merged_count_;
}

void archive_index::notify_core_outgoing_msg_count()
{
    g_core->update_outgoing_msg_count(aimid_, get_outgoing_count());
}

bool archive_index::get_header(int64_t _msgid, message_header& _header) const
{
    if (const auto iter_header = headers_index_.find(_msgid); iter_header != headers_index_.end())
    {
        _header = iter_header->second;
        return true;
    }
    return false;
}

bool archive_index::has_header(const int64_t _msgid) const
{
    return headers_index_.find(_msgid) != headers_index_.end();
}

archive::storage::result_type archive_index::update(const archive::history_block& _data, /*out*/ headers_list& _headers)
{
    _headers.clear();

    for (const auto& hm: _data)
    {
        _headers.emplace_back(
            hm->get_flags(),
            hm->get_time(),
            hm->get_msgid(),
            hm->get_prev_msgid(),
            hm->get_data_offset(),
            hm->get_data_size(),
            hm->get_update_patch_version(),
            hm->has_shared_contact_with_sn(),
            hm->has_poll_with_id()
        );
    }

    insert_block(_headers);

    return save_block(_headers);
}

template <typename R>
void archive_index::serialize_block(const R& _headers, core::tools::binary_stream& _data) const
{
    core::tools::tlvpack tlv_headers;

    core::tools::binary_stream header_data;

    for (const auto& hdr : _headers)
    {
        header_data.reset();
        hdr.serialize(header_data);

        tlv_headers.push_child(core::tools::tlv(archive_index_types::header, header_data));
    }

    tlv_headers.serialize(_data);
}

bool archive_index::unserialize_block(core::tools::binary_stream& _data, const std::chrono::seconds _current_time)
{
    uint32_t tlv_type = 0;
    uint32_t tlv_length = 0;

    while (uint32_t available = _data.available())
    {
        if (available < sizeof(uint32_t)*2)
            return false;

        tlv_type = _data.read<uint32_t>();
        tlv_length = _data.read<uint32_t>();

        if (available < tlv_length)
            return false;

        archive::message_header msg_header;
        if (!msg_header.unserialize(_data))
            return false;

        insert_header(msg_header, _current_time);
    }
    return true;
}

archive::storage::result_type archive_index::save_block(const archive::headers_list& _block)
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return { false, 0 };

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream block_data;
    serialize_block(_block, block_data);

    int64_t offset = 0;
    return storage_->write_data_block(block_data, offset);
}

bool archive_index::save_all()
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage_->open(mode))
        return false;

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    if (headers_index_.empty())
        return true;

    std::vector<message_header> headers;

    headers.reserve(history_block_size * 1.5);

    const auto end = headers_index_.cend();
    const auto iter_last = std::prev(end);

    core::tools::binary_stream block_data;
    for (auto iter_header = headers_index_.cbegin(); iter_header != end; ++iter_header)
    {
        headers.emplace_back(iter_header->second);

        if (iter_header->second.is_modified())
        {
            for (const auto& h : iter_header->second.get_modifications())
                headers.emplace_back(h);
        }

        if (headers.size() >= history_block_size || iter_header == iter_last)
        {
            serialize_block(headers, block_data);

            int64_t offset = 0;
            if (!storage_->write_data_block(block_data, offset))
                return false;

            headers.clear();
            block_data.reset();
        }
    }

    return true;
}

bool archive_index::load_from_local()
{
    last_error_ = archive::error::ok;

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage_->open(mode))
    {
        last_error_ = storage_->get_last_error();
        return false;
    }

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    auto tmp_headers = std::move(headers_index_);
    headers_index_.clear();
    outgoing_count_ = 0;
    merged_count_ = 0;

    const auto now = std::chrono::seconds(std::time(nullptr));

    core::tools::binary_stream data_stream;
    while (storage_->read_data_block(-1, data_stream))
    {
        if (!unserialize_block(data_stream, now))
        {
            for (auto &x : tmp_headers)
                insert_header(x.second, now);

            last_error_ = archive::error::parse_headers;

            return false;
        }

        data_stream.reset();
    }

    for (auto &x : tmp_headers)
        insert_header(x.second, now);

    if (storage_->get_last_error() != archive::error::end_of_file)
    {
        last_error_ = storage_->get_last_error();
        return false;
    }

    loaded_from_local_ = true;

    notify_core_outgoing_msg_count();

    return true;
}

void archive_index::drop_header(int64_t _msgid)
{
    if (auto it = headers_index_.find(_msgid); it != headers_index_.end())
        headers_index_.erase(it);
}

void archive_index::delete_up_to(const int64_t _to)
{
    assert(_to > -1);

    auto iter = headers_index_.lower_bound(_to);

    const auto delete_all = (iter == headers_index_.end());
    if (delete_all)
    {
        headers_index_.clear();

        save_all();

        return;
    }

    auto &header = iter->second;

    const auto is_del_up_to_found = (header.get_id() == _to);

    if (is_del_up_to_found)
    {
        const auto iter_after_deleted = std::next(iter);

        headers_index_.erase(headers_index_.begin(), iter_after_deleted);

        if (iter_after_deleted != headers_index_.end())
        {
            auto &header_after_deleted = iter_after_deleted->second;
            if (header_after_deleted.get_prev_msgid() == _to)
            {
                header_after_deleted.set_prev_msgid(-1);
            }
        }

        save_all();
    }
    else
    {
        if (headers_index_.begin() != iter)
        {
            headers_index_.erase(headers_index_.begin(), iter);

            save_all();
        }
    }

}

int64_t archive_index::get_last_msgid() const
{
    if (headers_index_.empty())
        return -1;

    return headers_index_.rbegin()->first;
}

int64_t archive_index::get_first_msgid() const
{
    if (headers_index_.empty())
        return -1;

    return headers_index_.begin()->first;
}

int32_t archive_index::get_outgoing_count() const
{
    if (!loaded_from_local_)
        return -1;
    return outgoing_count_;
}

bool archive_index::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const
{
    if (headers_index_.empty())
        return true;

    int64_t current_depth = 0;

    // 1. position search cursor

    auto iter_cursor = headers_index_.crbegin();

    const auto is_from_specified = (_from != -1);
    if (is_from_specified)
    {
        const auto last_header_key = iter_cursor->first;

        const auto is_hole_at_the_end = (last_header_key < _from);
        if (is_hole_at_the_end)
        {
            // if "from" from dlg_state (still not in index obviously)

            assert(iter_cursor->second.get_id() == last_header_key);

            _hole.set_from(-1);
            _hole.set_to(last_header_key);

            return true;
        }

        auto from_iter = headers_index_.find(_from);
        if (from_iter == headers_index_.cend())
        {
            assert(!"index not found");
            return false;
        }

        iter_cursor = std::make_reverse_iterator(++from_iter);
    }

    iter_cursor = skip_patches_forward(iter_cursor, headers_index_.crend());

    const auto only_patches_in_index = (iter_cursor == headers_index_.crend());
    if (only_patches_in_index)
        return true;

    // 2. search for holes

    while (iter_cursor != headers_index_.crend())
    {
        ++current_depth;

        const auto &current_header = iter_cursor->second;
        assert(!current_header.is_patch() || current_header.is_updated_message());

        const auto iter_next = skip_patches_forward(std::next(iter_cursor), headers_index_.crend());

        const auto reached_last_header = (iter_next == headers_index_.crend());
        if (reached_last_header)
        {
            if (current_header.get_prev_msgid() != -1)
            {
                _hole.set_from(current_header.get_id());
                return true;
            }

            return false;
        }

        const auto &prev_header = iter_next->second;
        assert(!prev_header.is_patch() || prev_header.is_updated_message());

        if (current_header.get_prev_msgid() != prev_header.get_id())
        {
            _hole.set_from(current_header.get_id());
            _hole.set_to(prev_header.get_id());
            _hole.set_depth(current_depth);

            return true;
        }

        if (_depth != -1 && current_depth >= _depth)
            return false;

        ++iter_cursor;
        iter_cursor = skip_patches_forward(iter_cursor, headers_index_.crend());
    }

    return false;
}

std::vector<int64_t> core::archive::archive_index::get_messages_for_update() const
{
    std::vector<int64_t> ids;
    for (const auto& [id, header] : headers_index_)
        if (header.is_updated_message())
            ids.push_back(id);
    return ids;
}

int64_t archive_index::validate_hole_request(const archive_hole& _hole, const int32_t _count) const
{
    int64_t ret_from = _hole.get_from();

    do
    {
        if (_hole.get_from() <= 0 || _hole.get_to() <= 0 || abs(_count) <= 1)
            break;

        auto from_iter = headers_index_.find(_hole.get_from());
        if (from_iter == headers_index_.end())
            break;

        auto iter_cursor = std::make_reverse_iterator(++from_iter);
        if (iter_cursor->second.is_patch())
            break;

        const auto iter_prev = skip_patches_forward(std::next(iter_cursor), headers_index_.rend());

        if (iter_prev == headers_index_.rend())
            break;

        if (iter_prev->first != iter_cursor->second.get_prev_msgid())
        {
            ret_from = iter_prev->first;
        }
    }
    while (false);

    return ret_from;
}

bool archive_index::need_optimize() const
{
    return
        headers_index_.size() > index_size_need_optimize ||
        (loaded_from_local_ && !headers_index_.empty() && merged_count_ / (double)headers_index_.size() >= max_merged_percent);
}

void archive_index::filter_deleted(std::vector<int64_t>& _ids) const
{
    auto pred = [this](auto id)
    {
        if (const auto it = headers_index_.find(id); it != headers_index_.end())
            return it->second.is_patch() || it->second.is_deleted() || it->second.is_modified();
        return true;
    };
    _ids.erase(std::remove_if(_ids.begin(), _ids.end(), pred), _ids.end());
}

void archive_index::optimize()
{
    if (!need_optimize())
        return;

    std::stringstream s;
    s << "optimized archive index for " << aimid_ << " loaded_from_local=" << loaded_from_local_ << "\r\n";
    s << "merged/total: " << merged_count_ << '/' << headers_index_.size() << "\r\n";

    if (const int to_remove = headers_index_.size() - max_index_size; to_remove > 0)
    {
        s << "overflow" << to_remove << ", erased" << headers_index_.size() << "->" << headers_index_.size() - to_remove << "\r\n";

        headers_index_.erase(headers_index_.begin(), std::next(headers_index_.begin(), to_remove));

        assert(!headers_index_.empty());
        if (!headers_index_.empty())
            headers_index_.begin()->second.set_prev_msgid(-1);
    }

    g_core->write_string_to_network_log(s.str());

    merged_count_ = 0;

    save_all();
}

void archive_index::make_holes()
{
    srand(time(nullptr));

    for (auto iter = std::begin(headers_index_), end = std::end(headers_index_); iter != end;)
    {
        int32_t cnt = rand() % 2;

        auto iter_erase = iter;

        ++iter;

        if (cnt == 0)
            headers_index_.erase(iter_erase);
    }
}

void archive_index::invalidate_message_data(int64_t _msgid, mark_as_updated _mark_as_updated)
{
    if (auto it = headers_index_.find(_msgid); it != headers_index_.end())
    {
        auto& header = it->second;
        header.invalidate_data_offset();
        if (_mark_as_updated == mark_as_updated::yes && !header.is_patch())
            header.set_updated(true); // to request messages from server
    }
}

void archive_index::invalidate_message_data(int64_t _from, int64_t _before_count, int64_t _after_count)
{
    const auto headers_end = headers_index_.end();
    const auto iter_from = headers_index_.lower_bound(_from);
    if (iter_from == headers_end)
    {
        assert(!"invalid index number");
        return;
    }

    const auto headers_begin = headers_index_.begin();
    if (_before_count > 0)
    {
        auto iter_header = iter_from;
        if (_after_count <= 0 && iter_header != headers_end)
            ++iter_header; // to include fromId

        auto remaining = _before_count;
        while (iter_header != headers_begin)
        {
            --iter_header;

            if (iter_header->second.is_patch())
                continue;

            if (remaining-- <= 0)
                break;

            iter_header->second.invalidate_data_offset();
        };
    }
    if (_after_count > 0)
    {
        auto iter_header = iter_from;
        auto remaining = _after_count;
        while (iter_header != headers_end)
        {
            if (iter_header->second.is_patch())
            {
                ++iter_header;
                continue;
            }
            if (remaining-- <= 0)
                break;
            iter_header->second.invalidate_data_offset();
            ++iter_header;
        };
    }
}

void archive_index::free()
{
    headers_index_.clear();

    loaded_from_local_ = false;
    merged_count_ = 0;
}

int64_t archive_index::get_memory_usage() const
{
    const int32_t map_node_size = 40;

    return (headers_index_.size() * (sizeof(message_header) + sizeof(int64_t) + map_node_size));
}

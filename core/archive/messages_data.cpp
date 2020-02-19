#include "stdafx.h"
#include "messages_data.h"
#include "storage.h"
#include "archive_index.h"
#include "search_dialog_result.h"
#include "../tools/system.h"
#include "../../common.shared/common_defs.h"
#include "../../common.shared/constants.h"

using namespace core;
using namespace archive;

messages_data::messages_data(std::wstring _file_name)
    : storage_(std::make_unique<storage>(std::move(_file_name)))
{
}


messages_data::~messages_data()
{
}


messages_data::error_vector messages_data::get_messages(const headers_list& _headers, history_block& _messages) const
{
    error_vector res;
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.read_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
    {
        res.emplace_back(-1, -1);
        return res;
    }
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    _messages.reserve(_headers.size());

    core::tools::binary_stream message_data;

    for (const auto &header : _headers)
    {
        message_data.reset();
        assert(!header.is_patch() || header.is_updated_message());

        auto make_fake_mesage = [](const auto& header)
        {
            auto msg = std::make_shared<history_message>();
            msg->apply_header_flags(header);

            msg->set_msgid(header.get_id());
            msg->set_prev_msgid(header.get_prev_msgid());
            msg->set_time(header.get_time());
            msg->set_text("Invalid message data. Will be updated soon");
            return msg;
        };

        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid message data");
            res.emplace_back(header.get_id(), 1);
            _messages.push_back(make_fake_mesage(header));
            continue;
        }

        auto msg = std::make_shared<history_message>();
        if (msg->unserialize(message_data) != 0)
        {
            assert(!"unserialize message error");
            res.emplace_back(header.get_id(), 2);
            _messages.push_back(make_fake_mesage(header));
            continue;
        }

        if (msg->get_msgid() != header.get_id())
        {
            assert(!"message data invalid");
            res.emplace_back(header.get_id(), 3);
            _messages.push_back(make_fake_mesage(header));
            continue;
        }

        msg->apply_header_flags(header);

        const auto modifications = get_message_modifications(header);
        msg->apply_modifications(modifications);

        _messages.push_back(std::move(msg));
    }

    if (_messages.size() != _headers.size())
        res.emplace_back(-1, 4);

    return res;
}

void messages_data::drop()
{
    archive::storage_mode mode;
    mode.flags_.write_ = mode.flags_.truncate_ = true;
    storage_->open(mode);
    storage_->close();
}

bool is_equal(const char* _str1, const char* _str2, int _b, int _l)
{
    if (_l == 1)
        return *_str1 == *(_str2 + _b);

    return std::memcmp(_str1, _str2 + _b, _l) == 0;
}

int32_t kmp_strstr(const char* _str, uint32_t _str_sz, const std::vector<int32_t>& _term
               , const std::vector<int32_t>& _prefix, const std::string& _symbs, const std::vector<int32_t>& _symb_indexes)
{
    if (!_str)
        return -1;

    auto j = 0u;

    for (auto i = 0u; i < _str_sz; )
    {
        auto len = tools::utf8_char_size(*(_str + i));

        while (j > 0 && (!is_equal(_symbs.c_str() + _symb_indexes[2 * j], _str, i, len) && !is_equal(_symbs.c_str() + _symb_indexes[2 * j + 1], _str, i, len)))
            j = _prefix[j - 1];

        if (is_equal(_symbs.c_str() + _symb_indexes[2 * j], _str, i, len) || is_equal(_symbs.c_str() + _symb_indexes[2 * j + 1], _str, i, len))
            j += 1;

        if (j == _term.size())
        {
            return i - j + 1;
        }
        i += len;
    }
    return -1;
}

bool messages_data::get_history_archive(const std::wstring& _file_name, core::tools::binary_stream& _buffer
    , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode)
{
    auto file = tools::system::open_file_for_read(_file_name, std::ios::binary | std::ios::ate);

    auto init_size = static_cast<int64_t>(file.tellg());
    std::streamsize size = std::min<int64_t>(*_remaining_size, init_size - *_offset);

    if (size <= 0 || init_size == -1)
    {
        *_offset = -1;
        return false;
    }

    if (*_mode == 0)
    {
        constexpr int64_t limit = search::archive_block_size();

        if (init_size > limit  && *_offset + limit < init_size)
        {
            *_offset = init_size - limit;
        }
        else
        {
            *_mode = 1;
        }
    }

    file.seekg(*_offset, std::ios::beg);

    if (!file.read(_buffer.get_data_for_write() + _cur_index, size))
    {
        return false;
    }

    *_remaining_size -= size;
    _cur_index += size;

    if (size == init_size - *_offset )
    {
        *_offset = -1;
    }

    return true;
}

void messages_data::search_in_archive(std::shared_ptr<contact_and_offsets_v> _contacts_and_offsets, std::shared_ptr<coded_term> _cterm
                , std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data
                , search::found_messages& found_messages
                , int64_t _min_id)
{
    std::set<int64_t, std::greater<int64_t>> top_ids;


    auto find_by_id = [](auto term_id, auto id)
    {
        if constexpr (build::is_debug())
        {
            if (term_id > 0 && term_id == id)
                return true;
        }
        return false;
    };

    int64_t term_id = -1;
    if constexpr (build::is_debug())
    {
        try
        {
            term_id = std::stoll(_cterm->lower_term);
        }
        catch (...)
        {
        }
    }

    for (auto contact_i = 0u; contact_i < _archive->size() - 1; ++contact_i)
    {
        auto current_pos = (*_archive)[contact_i].second;
        auto end_pos = (*_archive)[contact_i + 1].second;
        auto _offset = std::get<2>((*_contacts_and_offsets)[contact_i]);
        auto _contact = (*_archive)[contact_i].first;

        int64_t begin_of_block;

        while (storage::fast_read_data_block((*_data), current_pos, begin_of_block, end_pos))
        {
            _data->set_output(begin_of_block);
            auto mess_id = history_message::get_id_field(*_data);

            if (mess_id == -1 || mess_id <= _min_id)
                continue;

            if (top_ids.size() > ::common::get_limit_search_results())
            {
                auto greater = top_ids.upper_bound(mess_id);

                if (greater != top_ids.end())
                {
                    auto min_id = (*top_ids.rbegin());
                    top_ids.erase(min_id);
                    top_ids.insert(mess_id);
                }
                else
                {
                    continue;
                }
            }

            uint32_t text_length = 0;

            _data->set_output(begin_of_block);

            if (history_message::is_sticker(*_data))
                continue;

            _data->set_output(begin_of_block);

            if (history_message::is_chat_event(*_data))
                continue;

            _data->set_output(begin_of_block);

            history_message::jump_to_text_field(*_data, text_length);

            if (!text_length)
                continue;

            char* pointer = nullptr;
            if (_data->available())
                pointer = _data->read_available();

            if (find_by_id(term_id, mess_id)
                || kmp_strstr(pointer, text_length, _cterm->coded_string, _cterm->prefix, _cterm->symbs, _cterm->symb_indexes) != -1)
            {
                _data->set_output(begin_of_block);
                auto msg = std::make_shared<history_message>();
                if (msg->unserialize(*_data) == 0)
                {
                    top_ids.insert(mess_id);
                    auto& messages = found_messages[_contact];
                    const auto msg_it = std::find_if(messages.begin(), messages.end(), [mess_id](const auto& msg) { return msg->get_msgid() == mess_id; });
                    if (msg_it != messages.end())
                        *msg_it = std::move(msg);
                    else
                        messages.push_back(std::move(msg));
                }
            }
            else
            {
                if (const auto it = found_messages.find(_contact); it != found_messages.end())
                {
                    auto& messages = it->second;
                    const auto msg_it = std::find_if(messages.begin(), messages.end(), [mess_id](const auto& msg) { return msg->get_msgid() == mess_id; });
                    if (msg_it != messages.end())
                        messages.erase(msg_it);
                }
            }
        }

        if (current_pos == (*_archive)[contact_i].second && !((*_archive)[contact_i].first.empty()))
            *_offset = -1;

        if (*_offset != -1)
            *_offset += current_pos - (*_archive)[contact_i].second;
    }
}

history_block messages_data::get_message_modifications(const message_header& _header) const
{
    if (!_header.is_modified())
    {
        return history_block();
    }

    history_block modifications;

    core::tools::binary_stream message_data;

    const auto &modification_headers = _header.get_modifications();
    modifications.reserve(modification_headers.size());
    for (const auto &header : modification_headers)
    {
        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid modification data");
            continue;
        }

        auto modification = std::make_shared<history_message>();
        if (modification->unserialize(message_data) != 0)
        {
            assert(!"unserialize modification error");
            continue;
        }

        if (modification->get_msgid() != header.get_id())
        {
            assert(!"modification data invalid");
            continue;
        }

        modifications.push_back(std::move(modification));
    }

    return modifications;
}

storage::result_type messages_data::update(const archive::history_block& _data)
{
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.write_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return { false, 0 };
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream message_data;

    for (const auto& msg : _data)
    {
        message_data.reset();
        msg->serialize(message_data);

        const auto data_size = message_data.available();

        int64_t offset = 0;
        if (auto res = storage_->write_data_block(message_data, offset); !res)
            return res;

        msg->set_data_offset(offset);
        msg->set_data_size(data_size);
    }

    return { true, 0 };
}

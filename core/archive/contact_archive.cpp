#include "stdafx.h"

#include "../log/log.h"

#include "contact_archive.h"
#include "messages_data.h"
#include "archive_index.h"
#include "history_message.h"
#include "mentions_me.h"
#include "gallery_cache.h"
#include "reactions.h"

#include "../tools/time_measure.h"

#include "../common.shared/string_utils.h"

#include "re2/re2.h"

using namespace core;
using namespace archive;

contact_archive::contact_archive(std::wstring _archive_path, const std::string& _contact_id)
    : path_(std::move(_archive_path))
    , index_(std::make_unique<archive_index>(su::wconcat(path_, L'/', index_filename()), _contact_id))
    , data_(std::make_unique<messages_data>(su::wconcat(path_, L'/', db_filename())))
    , state_(std::make_unique<archive_state>(su::wconcat(path_, L'/', dlg_state_filename()), _contact_id))
    , mentions_(std::make_unique<mentions_me>(su::wconcat(path_, L'/', mentions_filename())))
    , gallery_(std::make_unique<gallery_storage>(su::wconcat(path_, L'/', gallery_cache_filename()), su::wconcat(path_, L'/', gallery_state_filename())))
    , reactions_(std::make_unique<reactions_storage>(su::wconcat(path_, L'/', reactions_index_filename()), su::wconcat(path_, L'/', reactions_data_filename())))
    , local_loaded_(false)
    , load_metrics_({0})
{
}


contact_archive::~contact_archive() = default;

void contact_archive::get_messages(int64_t _from, int64_t _count_early, int64_t _count_later, history_block& _messages, get_message_policy policy) const
{
    _messages.clear();

    headers_list headers;
    while (true)
    {
        index_->serialize_from(_from, _count_early, _count_later, headers);
        if (headers.empty())
            return;

        _from = headers.begin()->get_id();

        if (policy == get_message_policy::skip_patches_and_deleted)
        {
            headers.remove_if([](const message_header& h) { return h.is_patch() || h.is_deleted(); });
            if (headers.empty())
                continue;
        }

        data_->get_messages(headers, _messages);
        return;
    }
}

void contact_archive::get_messages_index(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _headers) const
{
    index_->serialize_from(_from, _count_early, _count_later, _headers, archive_index::include_from_id::yes);
}

bool contact_archive::get_history_file(const std::wstring& _file_name, core::tools::binary_stream& _data
    , const std::shared_ptr<int64_t>& _offset, const std::shared_ptr<int64_t>& _remaining_size, int64_t& _cur_index, const std::shared_ptr<int64_t>& _mode)
{
    return messages_data::get_history_archive(_file_name, _data, _offset, _remaining_size, _cur_index, _mode);
}

history_block contact_archive::get_mentions() const
{
    return mentions_->get_messages();
}

void contact_archive::filter_deleted(std::vector<int64_t>& _ids) const
{
    index_->filter_deleted(_ids);
}

bool contact_archive::get_messages_buddies_from_ids(const archive::msgids_list& _ids, history_block& _messages, error_vector& _errors) const
{
    headers_list _headers;

    for (auto id : _ids)
    {
        message_header msg_header;

        if (!index_->get_header(id, Out msg_header))
        {
            assert(!"message header not found");
            continue;
        }

        if (msg_header.is_patch())
            continue;

        _headers.emplace_back(std::move(msg_header));
    }

    return get_messages_buddies_from_headers(_headers, _messages, _errors);
}

bool contact_archive::get_messages_buddies_from_headers(const headers_list& _headers, history_block& _messages, error_vector& _errors) const
{
    if (_headers.empty())
        return false;

    _errors = data_->get_messages(_headers, _messages);

    for (auto id : boost::adaptors::keys(std::as_const(_errors)))
    {
        if (id > 0)
            index_->invalidate_message_data(id, archive_index::mark_as_updated::yes);
    }

    return !_messages.empty();
}

bool contact_archive::get_messages_buddies(const std::shared_ptr<archive::msgids_list>& _ids, const std::shared_ptr<history_block>& _messages, const std::shared_ptr<error_vector>& _errors) const
{
    return get_messages_buddies_from_ids(*_ids, *_messages, *_errors);
}

bool contact_archive::get_messages_buddies(const headers_list& _headers, const std::shared_ptr<history_block>& _messages, const std::shared_ptr<error_vector>& _errors) const
{
    return get_messages_buddies_from_headers(_headers, *_messages, *_errors);
}

const dlg_state& contact_archive::get_dlg_state() const
{
    return state_->get_state();
}

void contact_archive::set_dlg_state(const dlg_state& _state, Out dlg_state_changes& _changes)
{
    state_->set_state(_state, Out _changes);
}

void contact_archive::update_attention_attribute(const bool _value)
{
    state_->update_attention_attribute(_value);
}

void contact_archive::merge_server_gallery(const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till, std::vector<archive::gallery_item>& _changes)
{
    gallery_->merge_from_server(_gallery, _from, _till, _changes);
}

archive::gallery_state contact_archive::get_gallery_state() const
{
    return gallery_->get_gallery_state();
}

void contact_archive::set_gallery_state(const archive::gallery_state& _state, bool _store_patch_version)
{
    gallery_->set_state(_state, _store_patch_version);
}

bool contact_archive::get_gallery_holes(archive::gallery_entry_id& _from, archive::gallery_entry_id& _till) const
{
    return gallery_->get_next_hole(_from, _till);
}

std::vector<archive::gallery_item> contact_archive::get_gallery_entries(const archive::gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, bool& _exhausted)
{
    return gallery_->get_items(_from, _types, _page_size, _exhausted);
}

std::vector<archive::gallery_item> contact_archive::get_gallery_entries_by_msg(int64_t _msg_id, const std::vector<std::string>& _types, int& _index, int& _total)
{
    return gallery_->get_items(_msg_id, _types, _index, _total);
}

void contact_archive::clear_hole_request()
{
    gallery_->clear_hole_request();
}

void contact_archive::make_gallery_hole(int64_t _from, int64_t _till)
{
    gallery_->make_hole(_from, _till);
}

void contact_archive::make_holes()
{
    bool first_load = false;
    load_from_local(first_load);

    index_->make_holes();
    index_->save_all();
}

bool contact_archive::is_gallery_hole_requested() const
{
    return gallery_->is_hole_requested();
}

void contact_archive::invalidate_message_data(const std::vector<int64_t>& _ids)
{
    bool first_load = false;
    load_from_local(first_load);

    for (auto id : _ids)
        index_->invalidate_message_data(id);
    index_->save_all();
}

void contact_archive::invalidate_message_data(int64_t _from, int64_t _before_count, int64_t _after_count)
{
    bool first_load = false;
    load_from_local(first_load);
    index_->invalidate_message_data(_from, _before_count, _after_count);
    index_->save_all();
}

void contact_archive::clear_dlg_state()
{
    state_->clear_state();
}

void contact_archive::insert_history_block(
    history_block_sptr _data,
    Out headers_list& _inserted_messages,
    Out dlg_state& _updated_state,
    Out dlg_state_changes& _state_changes,
    Out archive::storage::result_type& _result,
    int64_t _from,
    bool _has_older_message_id
)
{
    Out _updated_state = state_->get_state();

    archive::history_block insert_data;
    insert_data.reserve(_data->size());

    const auto last_msgid = state_->get_state().get_last_msgid();
    auto last_message_updated = false;

    for (const auto &message : *_data)
    {
        assert(message);
        assert(message->has_msgid());

        message_header existing_header;
        if (index_->get_header(message->get_msgid(), Out existing_header))
        {
            const bool is_patch_operation = message->is_patch() != existing_header.is_patch();

            const bool is_version_updated = message->get_update_patch_version() > existing_header.get_update_patch_version();

            const bool is_valid_data = existing_header.is_message_data_valid();

            const auto& quotes = message->get_quotes();
            const bool quote_with_chat_name = !quotes.empty() && !quotes.front().get_chat_name().empty();

            const bool is_prev_id_changed = message->get_prev_msgid() != existing_header.get_prev_msgid();

            const bool shared_contact_sn_added = message->has_shared_contact_with_sn() && !existing_header.has_shared_contact_with_sn();

            const bool poll_id_added = message->has_poll_with_id() && !existing_header.has_poll_with_id();

            const bool reactions_changed = message->has_reactions() != existing_header.has_reactions();

            const bool skip_duplicate = !is_patch_operation && !quote_with_chat_name &&
                                        !is_prev_id_changed && !is_version_updated &&
                                        is_valid_data && !shared_contact_sn_added &&
                                        !poll_id_added && !reactions_changed;

            if (skip_duplicate)
            {
                // message is already in the db, no need in quote header replacement,
                // it's not a patch,
                // prev_ids are not changed, no need to fix linked list,
                // no shared contact sn added,
                // no poll id added,
                // and no reactions changed,
                // thus just skip it
                continue;
            }
        }

        const auto is_message_obsolete = (
            message->has_msgid() &&
            (message->get_msgid() <= _updated_state.get_del_up_to())
        );
        if (is_message_obsolete)
        {
            assert(message->is_patch());

            __INFO(
                "delete_history",
                "skipped obsolete message\n"
                "    id=<%1%>\n"
                "    is_patch=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % logutils::yn(message->is_patch()) % _updated_state.get_del_up_to()
            );

            continue;
        }

        const auto is_prev_id_obsolete = (
            message->has_prev_msgid() &&
            (message->get_prev_msgid() <= _updated_state.get_del_up_to())
        );

        if (is_prev_id_obsolete)
        {
            __INFO(
                "delete_history",
                "fixed message prev_msgid\n"
                "    id=<%1%>\n"
                "    prev_id=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % message->get_prev_msgid() % _updated_state.get_del_up_to()
            );

            message->set_prev_msgid(-1);
        }

        last_message_updated = (
            last_message_updated ||
            (message->get_msgid() == last_msgid)
        );

        insert_data.push_back(message);
    }

    if (insert_data.empty())
    {
        error_vector v;
        if (_from != -1 && !_has_older_message_id && get_messages_buddies_from_ids({ _from }, insert_data, v))
        {
            assert(insert_data.size() < 2);
            if (!insert_data.empty())
                insert_data.front()->set_prev_msgid(-1);
        }
        else
        {
            return;
        }
    }

    _result = data_->update(insert_data);
    if (!_result)
    {
        assert(!"update data error");
        return;
    }

    _result = index_->update(insert_data, _inserted_messages);
    if (!_result)
    {
        assert(!"update index error");
        return;
    }
}

void contact_archive::update_message_data(const history_message& _message)
{
    message_header existing_header;

    if (!index_->get_header(_message.get_msgid(), Out existing_header))
        return;

    history_block block;

    data_->get_messages({ existing_header }, block);

    if (block.size())
    {
        (*block.begin())->merge(_message);
    }

    auto _result = data_->update(block);
    if (!_result)
    {
        assert(!"update data error");
        return;
    }

    headers_list headers = { existing_header };

    _result = index_->update(block, headers);
    if (!_result)
    {
        assert(!"update index error");
        return;
    }

}

void contact_archive::drop_history()
{
    index_->free();
    index_->save_all();
    data_->drop();
}

int32_t contact_archive::load_from_local(/*out*/ bool& _first_load)
{
    if (local_loaded_)
    {
        _first_load = false;
        return 0;
    }

    local_loaded_ = true;
    _first_load = true;

    load_metrics_.index_ms_ = tools::time_measure<>::execution([this]()
    {
        if (!index_->load_from_local())
        {
            if (index_->get_last_error() != archive::error::file_not_exist)
            {
                assert(!"index file crash, need repair");
                index_->save_all();
            }
        }
    });

    load_metrics_.mentions_ms_ = tools::time_measure<>::execution([this]()
    {
        if (!mentions_->load_from_local())
            mentions_->save_all();
    });

    load_metrics_.gallery_ms_ = tools::time_measure<>::execution([this]()
    {
        gallery_->load_from_local();
    });

    if (const auto first_id = index_->get_first_msgid(); first_id > 0)
    {
        if (const auto dlg_state = state_->get_state(); dlg_state.has_del_up_to() && dlg_state.get_del_up_to() > first_id)
            delete_messages_up_to(dlg_state.get_del_up_to());
    }

    if (const auto first_id = mentions_->first_id(); first_id)
    {
        const auto dlg_state = state_->get_state();
        const auto del_up = dlg_state.has_del_up_to() ? dlg_state.get_del_up_to() : -1;
        const auto last_read = dlg_state.has_last_read_mention() ? dlg_state.get_last_read_mention() : -1;
        if (const auto res = std::max(del_up, last_read); res > 0)
            mentions_->delete_up_to(res);
    }

    return 0;
}

void contact_archive::load_gallery_state_from_local()
{
    gallery_->load_state_from_local();
}

std::string core::archive::contact_archive::get_load_metrics_for_log() const
{
    std::stringstream info;
    info << "index: " << load_metrics_.index_ms_ << " ms\r\n";
    info << "mentions: " << load_metrics_.mentions_ms_ << " ms\r\n";
    info << "gallery: " << load_metrics_.gallery_ms_ << " ms\r\n";

    return info.str();
}

std::vector<int64_t> core::archive::contact_archive::get_messages_for_update() const
{
    return index_->get_messages_for_update();
}

bool contact_archive::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const
{
    return index_->get_next_hole(_from, _hole, _depth);
}

int64_t contact_archive::validate_hole_request(const archive_hole& _hole, const int32_t _count) const
{
    return index_->validate_hole_request(_hole, _count);
}

static constexpr const char* russian_letters() noexcept
{
    return "[\xd0\xb0\xd0\xb1\xd0\xb2\xd0\xb3\xd0\xb4\xd0\xb5\xd1\x91\xd0\xb6\xd0\xb7\xd0\xb8\xd0\xb9\xd0\xba\xd0\xbb\xd0\xbc\xd0\xbd\xd0\xbe\xd0\xbf\xd1\x80\xd1\x81\xd1\x82\xd1\x83\xd1\x84\xd1\x85\xd1\x86\xd1\x87\xd1\x88\xd1\x89\xd1\x8a\xd1\x8b\xd1\x8c\xd1\x8d\xd1\x8e\xd1\x8f\xd0\x90\xd0\x91\xd0\x92\xd0\x93\xd0\x94\xd0\x95\xd0\x81\xd0\x96\xd0\x97\xd0\x98\xd0\x99\xd0\x9a\xd0\x9b\xd0\x9c\xd0\x9d\xd0\x9e\xd0\x9f\xd0\xa0\xd0\xa1\xd0\xa2\xd0\xa3\xd0\xa4\xd0\xa5\xd0\xa6\xd0\xa7\xd0\xa8\xd0\xa9\xd0\xaa\xd0\xab\xd0\xac\xd0\xad\xd0\xae\xd0\xaf]";
}

std::optional<std::string> contact_archive::get_locale() const
{
    history_block messages;
    constexpr int64_t count = 10;

    headers_list headers;
    get_messages_index(-1, count, 0, headers);
    error_vector errors;
    get_messages_buddies_from_headers(headers, messages, errors);

    const static RE2 reg(russian_letters());
    auto contains_ru = [](const auto& m)
    {
        return RE2::PartialMatch(m->get_text(), reg);
    };

    if (std::any_of(messages.cbegin(), messages.cend(), contains_ru))
        return "ru_RU";

    return {};
}

bool contact_archive::need_optimize() const
{
    return index_->need_optimize();
}

void contact_archive::optimize()
{
    if (index_->need_optimize())
        index_->optimize();
}

void contact_archive::free()
{
    index_->free();
    gallery_->free();

    local_loaded_ = false;
}

void contact_archive::delete_messages_up_to(const int64_t _up_to)
{
    assert(_up_to > -1);

    auto dlg_state = state_->get_state();

    const auto &last_message = dlg_state.get_last_message();
    const auto is_last_message_obsolete = (last_message.has_msgid() && (_up_to >= last_message.get_msgid()));

    const auto is_last_msgid_obsolete = (dlg_state.has_last_msgid() && (_up_to >= dlg_state.get_last_msgid()));

    if (is_last_message_obsolete || is_last_msgid_obsolete)
    {
        dlg_state.clear_last_message();
        dlg_state.clear_last_msgid();

        dlg_state_changes changes;
        state_->set_state(dlg_state, Out changes);
    }

    index_->delete_up_to(_up_to);
}

void contact_archive::add_mention(const std::shared_ptr<archive::history_message>& _message)
{
    mentions_->add(_message);
}

void contact_archive::get_memory_usage(int64_t& _index_size, int64_t& _gallery_size) const
{
    _index_size = index_->get_memory_usage();
    _gallery_size = gallery_->get_memory_usage();
}

void contact_archive::get_reactions(const msgids_list& _ids_to_load, std::vector<reactions_data>& _reactions, msgids_list& _missing)
{
    reactions_->get_reactions(_ids_to_load, _reactions, _missing);
}

void contact_archive::insert_reactions(std::vector<reactions_data>& _reactions)
{
    reactions_->insert_reactions(_reactions);
}

std::wstring_view archive::db_filename() noexcept
{
    return L"_db2";
}

std::wstring_view archive::index_filename() noexcept
{
    return L"_idx2";
}

std::wstring_view archive::dlg_state_filename() noexcept
{
    return L"_ste2";
}

std::wstring_view archive::cache_filename() noexcept
{
    return L"cache2";
}

std::wstring_view archive::mentions_filename() noexcept
{
    return L"_mentions";
}

std::wstring_view archive::gallery_cache_filename() noexcept
{
    return L"_gc3";
}

std::wstring_view archive::gallery_state_filename() noexcept
{
    return L"_gs3";
}

std::wstring_view archive::reactions_index_filename() noexcept
{
    return L"_reactions_idx";
}

std::wstring_view archive::reactions_data_filename() noexcept
{
    return L"_reactions_db";
}

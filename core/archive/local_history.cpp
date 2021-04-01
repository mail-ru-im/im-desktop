#include "stdafx.h"

#include "../../common.shared/url_parser/url_parser.h"
#include "../../common.shared/string_utils.h"

#include "../../corelib/collection_helper.h"

#include "../log/log.h"

#include "../configuration/app_config.h"

#include "history_message.h"
#include "contact_archive.h"
#include "archive_index.h"
#include "not_sent_messages.h"
#include "messages_data.h"
#include "gallery_cache.h"
#include "../../corelib/enumerations.h"

#include "local_history.h"

namespace
{
    bool is_chat(std::string_view _contact) noexcept
    {
        return _contact.find("@chat.agent") != _contact.npos;
    }

    void insert_message_time_events(const core::stats::stats_event_names _event_flurry, const core::stats::im_stat_event_names _event_imstat, const int32_t _duration_msec)
    {
        assert(_duration_msec >= 0);
        if (core::g_core && _duration_msec >= 0)
        {
            core::stats::event_props_type props;
            props.emplace_back("time", core::stats::round_value(_duration_msec, 50, 1000));
            core::g_core->insert_event(_event_flurry, core::stats::event_props_type{ props });
            core::g_core->insert_im_stats_event(_event_imstat, std::move(props));
        }
    }

    constexpr auto msg_stat_delivered_timeout = std::chrono::hours(1);
}

using namespace core;
using namespace archive;

local_history::local_history(const std::wstring& _archive_path)
    : archive_path_(_archive_path)
    , po_(std::make_unique<pending_operations>(
        _archive_path + L"/pending.db",
        _archive_path + L"/pending.delete.db"))
{
}


local_history::~local_history() = default;

std::shared_ptr<contact_archive> local_history::get_contact_archive(const std::string& _contact)
{
    // load contact archive, insert to map

    if (const auto it = archives_.find(_contact); it != archives_.end())
        return it->second;

    std::wstring contact_folder = core::tools::from_utf8(_contact);
    std::replace(contact_folder.begin(), contact_folder.end(), L'|', L'_');
    auto contact_arch = std::make_shared<contact_archive>(su::wconcat(archive_path_, L'/', contact_folder), _contact);

    archives_.insert(std::make_pair(_contact, contact_arch));

    return contact_arch;
}

void local_history::update_history(
    const std::string& _contact,
    archive::history_block_sptr _data,
    Out headers_list& _inserted_messages,
    Out dlg_state& _state,
    Out dlg_state_changes& _state_changes,
    Out archive::storage::result_type& _result,
    int64_t _from,
    has_older_message_id _has_older_msdid)
{
    get_contact_archive(_contact)->insert_history_block(_data, Out _inserted_messages, Out _state, Out _state_changes, _result, _from, _has_older_msdid == has_older_message_id::yes);
}

void local_history::update_message_data(const std::string& _contact, const history_message& _message)
{
    get_contact_archive(_contact)->update_message_data(_message);
}

void local_history::drop_history(const std::string& _contact)
{
    get_contact_archive(_contact)->drop_history();
    get_pending_operations().drop(_contact);
    stat_clear_message_delivered(_contact);
}

bool local_history::get_messages(const std::string& _contact,
    int64_t _from,
    int64_t _count_early,
    int64_t _count_later,
    /*out*/ archive::history_block_sptr _messages,
    /*out*/ bool& _first_load,
    /*out*/ std::shared_ptr<error_vector> _errors)
{
    auto archive = get_contact_archive(_contact);
    if (!archive)
        return false;

    archive->load_from_local(_first_load);

    headers_list headers;
    const auto start_get_messages = std::chrono::steady_clock::now();
    archive->get_messages_index(_from, _count_early, _count_later, headers);
    archive->get_messages_buddies(headers, _messages, _errors);

    if (_first_load && configuration::get_app_config().is_full_log_enabled())
    {
        const auto get_messages_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_get_messages).count();
        std::stringstream info;
        info << "Local archive loading metrics for " << _contact << "\r\n";
        info << archive->get_load_metrics_for_log();
        info << "messages headers and bodies: " << get_messages_ms << " ms\r\n";
        g_core->write_string_to_network_log(info.str());
    }

    return true;
}

bool local_history::get_history_file(const std::string& _contact, /*out*/ core::tools::binary_stream& _history_archive
    , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode)
{
    std::wstring contact_folder = core::tools::from_utf8(_contact);
    std::replace(contact_folder.begin(), contact_folder.end(), L'|', L'_');
    std::wstring file_name = su::wconcat(archive_path_, L'/', contact_folder, L'/', db_filename());

    contact_archive::get_history_file(file_name, _history_archive, _offset, _remaining_size, _cur_index, _mode);
    return true;
}

void local_history::get_messages_buddies(
    const std::string& _contact,
    std::shared_ptr<archive::msgids_list> _ids,
    /*out*/ archive::history_block_sptr _messages,
    /*out*/ bool& _first_load,
    /*out*/ std::shared_ptr<error_vector> _errors)
{
    auto archive = get_contact_archive(_contact);
    if (!archive)
        return;
    archive->load_from_local(_first_load);
    archive->get_messages_buddies(_ids, _messages, _errors);
}

dlg_state local_history::get_dlg_state(const std::string& _contact)
{
    return get_contact_archive(_contact)->get_dlg_state();
}

std::vector<dlg_state> local_history::get_dlg_states(const std::vector<std::string>& _contacts)
{
    std::vector<dlg_state> states;
    states.reserve(_contacts.size());
    for (const auto& contact : _contacts)
        states.emplace_back(get_dlg_state(contact));
    return states;
}

void local_history::set_dlg_state(const std::string& _contact, const dlg_state& _state, const std::chrono::steady_clock::time_point& _correct_time, Out dlg_state& _result, Out dlg_state_changes& _changes)
{
    get_contact_archive(_contact)->set_dlg_state(_state, Out _changes);

    if (!stat_message_times_.empty() && !is_chat(_contact))
    {
        if (const auto last_id = _state.get_theirs_last_delivered(); last_id != -1)
            on_dlgstate_check_message_delivered(_contact, last_id, _correct_time);
    }

    Out _result = get_dlg_state(_contact);
}


bool local_history::clear_dlg_state(const std::string& _contact)
{
    get_contact_archive(_contact)->clear_dlg_state();
    stat_clear_message_delivered(_contact);

    return true;
}

std::vector<int64_t> local_history::get_messages_for_update(const std::string& _contact)
{
    return get_contact_archive(_contact)->get_messages_for_update();
}

void local_history::filter_deleted(const std::string & _contact, std::vector<int64_t>& _ids, bool& _first_load)
{
    if(auto archive = get_contact_archive(_contact))
    {
        archive->load_from_local(_first_load);
        archive->filter_deleted(_ids);
    }
}

history_block local_history::get_mentions(const std::string& _contact, bool& _first_load)
{
    auto archive = get_contact_archive(_contact);
    if (!archive)
        return {};
    archive->load_from_local(_first_load);
    return archive->get_mentions();
}

local_history::hole_result local_history::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto hole = std::make_shared<archive_hole>();
    auto error = get_contact_archive(_contact)->get_next_hole(_from, *hole, _depth);
    if (error == archive_hole_error::ok)
        return { std::move(hole), error };
    return { nullptr, error };
}


int64_t local_history::validate_hole_request(
    const std::string& _contact,
    const archive_hole& _hole_request,
    const int32_t _count)
{
    return get_contact_archive(_contact)->validate_hole_request(_hole_request, _count);
}

std::optional<std::string> local_history::get_locale(const std::string& _contact)
{
    return get_contact_archive(_contact)->get_locale();
}

void local_history::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (const auto& header : *_headers)
    {
        auto hh = std::make_unique<hheader>();
        hh->id_ = header.get_id();
        hh->prev_id_ = header.get_prev_msgid();
        hh->time_ = header.get_time();
        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders("headers", val_headers.get());
}


void local_history::serialize_headers(archive::history_block_sptr _data, coll_helper& _coll)
{
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (const auto& hist_block : *_data)
    {
        auto hh = std::make_unique<hheader>();
        hh->id_ = hist_block->get_msgid();
        hh->prev_id_ = hist_block->get_prev_msgid();
        hh->time_ = hist_block->get_time();

        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders("headers", val_headers.get());
}

const pending_operations& local_history::get_pending_operations() const
{
    return *po_;
}

pending_operations& local_history::get_pending_operations()
{
    return *po_;
}

int32_t local_history::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    get_pending_operations().insert_message(_contact, _msg);
    return 0;
}


bool local_history::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    return get_pending_operations().update_message_if_exist(_contact, _msg);
}


not_sent_message_sptr local_history::get_first_message_to_send()
{
    return get_pending_operations().get_first_ready_to_send();
}

bool local_history::get_first_message_to_delete(std::string& _contact, delete_message& _message) const
{
    return get_pending_operations().get_first_pending_delete_message(_contact, _message);
}

bool local_history::get_messages_to_delete(std::string& _contact, std::vector<delete_message>& _messages) const
{
    return get_pending_operations().get_pending_delete_messages(_contact, _messages);
}

int32_t local_history::insert_pending_delete_message(const std::string& _contact, delete_message _message)
{
    return get_pending_operations().insert_deleted_message(_contact, std::move(_message));
}

int32_t local_history::remove_pending_delete_message(const std::string& _contact, const delete_message& _message)
{
    return get_pending_operations().remove_deleted_message(_contact, _message);
}

not_sent_message_sptr local_history::get_not_sent_message_by_iid(const std::string& _iid)
{
    return get_pending_operations().get_message_by_internal_id(_iid);
}

void local_history::get_pending_file_sharing(std::list<not_sent_message_sptr>& _messages)
{
    return get_pending_operations().get_pending_file_sharing_messages(_messages);
}

void local_history::stat_write_sent_time(const message_stat_time& _stime, const std::chrono::steady_clock::time_point& _correct_time)
{
    if (_stime.need_stat_)
    {
        assert(!_stime.contact_.empty());
        assert(_stime.msg_id_ != -1);

        const auto current_time = std::chrono::steady_clock::now();
        const auto sent_duration = (current_time - _stime.create_time_ - (current_time - _correct_time)) / std::chrono::milliseconds(1);
        insert_message_time_events(stats::stats_event_names::messages_from_created_2_sent, stats::im_stat_event_names::messages_from_created_to_sent, sent_duration);
    }
}

void local_history::stat_write_delivered_time(message_stat_time& _stime, const std::chrono::steady_clock::time_point& _correct_time)
{
    if (_stime.need_stat_)
    {
        assert(!_stime.contact_.empty());
        assert(_stime.msg_id_ != -1);

        const auto current_time = std::chrono::steady_clock::now();
        const auto deliver_duration = (current_time - _stime.create_time_ - (current_time - _correct_time)) / std::chrono::milliseconds(1);
        insert_message_time_events(stats::stats_event_names::messages_from_created_2_delivered, stats::im_stat_event_names::messages_from_created_to_delivered, deliver_duration);

        _stime.need_stat_ = false;
    }
}

void local_history::on_dlgstate_check_message_delivered(const std::string_view _contact, const int64_t _last_delivered_msgid, const std::chrono::steady_clock::time_point& _correct_time)
{
    bool need_cleanup = false;
    for (auto& st : stat_message_times_)
    {
        if (st.contact_ == _contact && _last_delivered_msgid >= st.msg_id_)
        {
            stat_write_delivered_time(st, _correct_time);
            need_cleanup = true;
        }
    }

    if (need_cleanup)
        cleanup_stat_message_delivered();
}

void local_history::stat_clear_message_delivered(const std::string_view _contact)
{
    bool need_cleanup = false;
    for (auto& st : stat_message_times_)
    {
        if (st.contact_ == _contact)
        {
            st.need_stat_ = false;
            need_cleanup = true;
        }
    }

    if (need_cleanup)
        cleanup_stat_message_delivered();
}

void local_history::cleanup_stat_message_delivered()
{
    const auto cur_time = std::chrono::steady_clock::now();
    const auto check = [&cur_time](const auto& _st) { return !_st.need_stat_ || cur_time - _st.create_time_ > msg_stat_delivered_timeout; };
    stat_message_times_.erase(std::remove_if(stat_message_times_.begin(), stat_message_times_.end(), check), stat_message_times_.end());
}

not_sent_message_sptr local_history::update_pending_with_imstate(
    const std::string& _message_internal_id,
    const int64_t& _hist_msg_id,
    const int64_t& _before_hist_msg_id,
    const std::chrono::steady_clock::time_point& _correct_time)
{
    auto pending_message = get_pending_operations().update_with_imstate(
        _message_internal_id,
        _hist_msg_id,
        _before_hist_msg_id);

    if (pending_message)
    {
        message_stat_time st(
            pending_message->is_calc_stat(),
            pending_message->get_create_time(),
            pending_message->get_aimid(),
            _hist_msg_id
        );

        if (st.need_stat_)
        {
            stat_write_sent_time(st, _correct_time);

            if (!is_chat(st.contact_))
            {
                 if (std::none_of(stat_message_times_.begin(), stat_message_times_.end(), [&st](const auto& x) { return x == st; }))
                    stat_message_times_.push_back(std::move(st));
            }
        }
    }
    return pending_message;
}


void local_history::failed_pending_message(const std::string& _message_internal_id)
{
    return get_pending_operations().failed_pending_message(_message_internal_id);
}

void local_history::delete_messages_up_to(const std::string& _contact, const int64_t _id)
{
    assert(!_contact.empty());
    assert(_id > -1);

    __INFO(
        "delete_history",
        "deleting history\n"
        "    contact=<%1%>\n"
        "    up-to=<%2%>",
        _contact % _id
    );

    return get_contact_archive(_contact)->delete_messages_up_to(_id);
}

int32_t local_history::remove_messages_from_not_sent(
    const std::string& _contact,
    const bool _remove_if_modified,
    const archive::history_block_sptr& _data,
    const std::chrono::steady_clock::time_point& _correct_time)
{
    auto stat_info = get_pending_operations().remove(_contact, _remove_if_modified, _data);

    const auto last_delivered = is_chat(_contact) ? -1: get_dlg_state(_contact).get_theirs_last_delivered();
    for (auto& st : stat_info)
    {
        if (st.need_stat_)
        {
            stat_write_sent_time(st, _correct_time);

            if (last_delivered != -1)
            {
                if (last_delivered >= st.msg_id_)
                    stat_write_delivered_time(st, _correct_time);
                else if (std::none_of(stat_message_times_.begin(), stat_message_times_.end(), [&st](const auto& x) { return x == st; }))
                    stat_message_times_.push_back(std::move(st));
            }
        }
    }

    return 0;
}

int32_t local_history::remove_messages_from_not_sent(
    const std::string& _contact,
    const bool _remove_if_modified,
    const archive::history_block_sptr& _data1,
    const archive::history_block_sptr& _data2,
    const std::chrono::steady_clock::time_point& _correct_time)
{
    for (const auto& data : { _data1, _data2 })
    {
        if (data && !data->empty())
            remove_messages_from_not_sent(_contact, _remove_if_modified, data, _correct_time);
    }
    return 0;
}

void local_history::mark_message_duplicated(const std::string& _message_internal_id)
{
    get_pending_operations().mark_duplicated(_message_internal_id);
}

void local_history::update_message_post_time(
    const std::string& _message_internal_id,
    const std::chrono::system_clock::time_point& _time_point)
{
    get_pending_operations().update_message_post_time(_message_internal_id, _time_point);
}

bool local_history::has_not_sent_messages(const std::string& _contact)
{
    return get_pending_operations().exist_message(_contact);
}

void local_history::get_not_sent_messages(const std::string& _contact, /*out*/ archive::history_block_sptr _messages)
{
    get_pending_operations().get_messages(_contact, *_messages);
}

void local_history::optimize_contact_archive(const std::string& _contact)
{
    get_contact_archive(_contact)->optimize();
}

void local_history::free_dialog(const std::string& _contact)
{
    get_contact_archive(_contact)->free();
}

void local_history::add_mention(const std::string& _contact, const std::shared_ptr<archive::history_message>& _message)
{
    get_contact_archive(_contact)->add_mention(_message);
}

void local_history::update_attention_attribute(const std::string& _contact, const bool _value)
{
    get_contact_archive(_contact)->update_attention_attribute(_value);
}

void local_history::merge_server_gallery(const std::string& _constact, const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till, std::vector<archive::gallery_item>& _changes)
{
    const auto archive = get_contact_archive(_constact);
    bool first = false;
    archive->load_from_local(first);
    archive->merge_server_gallery(_gallery, _from, _till, _changes);
}

void local_history::get_gallery_state(const std::string& _aimid, archive::gallery_state& _state)
{
    const auto archive = get_contact_archive(_aimid);
    archive->load_gallery_state_from_local();
    _state = archive->get_gallery_state();
}

void local_history::set_gallery_state(const std::string& _aimid, const archive::gallery_state& _state, bool _store_patch_version)
{
    const auto archive = get_contact_archive(_aimid);
    archive->load_gallery_state_from_local();
    archive->set_gallery_state(_state, _store_patch_version);
}

void local_history::get_gallery_holes(const std::string& _aimid, bool& result, archive::gallery_entry_id& _from, archive::gallery_entry_id& _till)
{
    const auto archive = get_contact_archive(_aimid);
    bool first = false;
    archive->load_from_local(first);
    result = archive->get_gallery_holes(_from, _till);
}

bool local_history::get_gallery_entries(const std::string& _aimid, const archive::gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, std::vector<archive::gallery_item>& _entries)
{
    const auto archive = get_contact_archive(_aimid);
    bool first = false;
    archive->load_from_local(first);
    auto exhausted = false;
    _entries = archive->get_gallery_entries(_from, _types, _page_size, exhausted);
    return exhausted;
}

void local_history::get_gallery_entries_by_msg(const std::string& _aimid, const std::vector<std::string>& _types, int64_t _msg_id, std::vector<archive::gallery_item>& _entries, int& _index, int& _total)
{
    const auto archive = get_contact_archive(_aimid);
    bool first = false;
    archive->load_from_local(first);
    _entries = archive->get_gallery_entries_by_msg(_msg_id, _types, _index, _total);
}

void local_history::clear_hole_request(const std::string& _aimId)
{
    get_contact_archive(_aimId)->clear_hole_request();
}

void local_history::make_gallery_hole(const std::string& _aimid, int64_t _from, int64_t _till)
{
    const auto archive = get_contact_archive(_aimid);
    bool first = false;
    archive->load_from_local(first);
    get_contact_archive(_aimid)->make_gallery_hole(_from, _till);
}

void local_history::make_holes(const std::string& _aimId)
{
    get_contact_archive(_aimId)->make_holes();
}

void local_history::is_gallery_hole_requested(const std::string& _aimId, bool& _is_hole_requsted)
{
    _is_hole_requsted = get_contact_archive(_aimId)->is_gallery_hole_requested();
}

void local_history::invalidate_message_data(const std::string& _aimId, const std::vector<int64_t>& _ids)
{
    get_contact_archive(_aimId)->invalidate_message_data(_ids);
}

void local_history::invalidate_message_data(const std::string& _aimId, int64_t _from, int64_t _before_count, int64_t _after_count)
{
    get_contact_archive(_aimId)->invalidate_message_data(_from, _before_count, _after_count);
}

void local_history::get_memory_usage(int64_t& _index_size, int64_t& _gallery_size)
{
    _index_size = 0;
    _gallery_size = 0;

    for (const auto& [_contact, _index] : archives_)
    {
        int64_t _i_index_size = 0;
        int64_t _i_gallery_size = 0;

        _index->get_memory_usage(_i_index_size, _i_gallery_size);

        _index_size += _i_index_size;
        _gallery_size += _i_gallery_size;
    }
}

void local_history::insert_reactions(const std::string& _contact, const reactions_vector_sptr& _reactions)
{
    get_contact_archive(_contact)->insert_reactions(*_reactions);
}

void local_history::get_reactions(const std::string& _contact, const std::shared_ptr<msgids_list>& _msg_ids, /*out*/ reactions_vector_sptr _reactions, /*out*/ std::shared_ptr<msgids_list> _missing)
{
    get_contact_archive(_contact)->get_reactions(*_msg_ids, *_reactions, *_missing);
}




face::face(const std::wstring& _archive_path)
    : history_cache_(std::make_shared<local_history>(_archive_path))
    , thread_(std::make_shared<core::async_executer>("face", 1, configuration::get_app_config().is_task_trace_enabled()))
{
}

std::shared_ptr<update_history_handler> face::update_history(const std::string& _contact, const archive::history_block_sptr& _data, int64_t _from, archive::local_history::has_older_message_id _has_older_msgid)
{
    assert(!_contact.empty());

    __LOG(core::log::info("archive", boost::format("update_history, contact=%1%") % _contact);)

    auto handler = std::make_shared<update_history_handler>();

    auto ids = std::make_shared<headers_list>();
    auto state = std::make_shared<dlg_state>();
    auto state_changes = std::make_shared<dlg_state_changes>();
    auto result = std::make_shared<core::archive::storage::result_type>();

    static constexpr char task_name[] = "face::update_history";

    thread_->run_async_function(
        [history_cache = history_cache_, _data, _contact, ids, state, state_changes, _from, _has_older_msgid, result]
        {
            history_cache->update_history(_contact, _data, Out *ids, Out *state, Out *state_changes, Out *result, _from, _has_older_msgid);
            return 0;
        }, task_name
    )->on_result_ =
        [handler, ids, state, state_changes, result](int32_t _error)
        {
            if (handler->on_result)
            {
                handler->on_result(
                    ids,
                    *state,
                    *state_changes,
                    *result
                );
            }
        };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_message_data(const std::string& _contact, const history_message& _message)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::update_message_data";

    thread_->run_async_function([history_cache = history_cache_, _contact, _message]() -> int32_t
    {
        history_cache->update_message_data(_contact, _message);

        return 0;
    }, task_name)->on_result_ = [wr_this = weak_from_this(), handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::drop_history(const std::string& _contact)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::drop_history";

    thread_->run_async_function([history_cache = history_cache_, _contact]() -> int32_t
    {
        history_cache->drop_history(_contact);

        return 0;
    }, task_name)->on_result_ = [wr_this = weak_from_this(), handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids)
{
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();
    auto first_load = std::make_shared<bool>(false);
    auto errors = std::make_shared<error_vector>();

    static constexpr char task_name[] = "face::get_messages_buddies";

    thread_->run_async_function([history_cache = history_cache_, _contact, _ids, out_messages, first_load, errors]()->int32_t
    {
        history_cache->get_messages_buddies(_contact, _ids, out_messages, *first_load, errors);
        return 0;

    }, task_name)->on_result_ = [handler, out_messages, first_load, errors](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages, *first_load ? archive::first_load::yes : archive::first_load::no, errors);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages(const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later)
{
    assert(!_contact.empty());

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();
    auto first_load = std::make_shared<bool>(false);
    auto errors = std::make_shared<error_vector>();

    constexpr char task_name[] = "face::get_messages";

    thread_->run_async_function([_contact, out_messages, _from, _count_early, _count_later, history_cache, first_load, errors]()->int32_t
    {
        return (history_cache->get_messages(_contact, _from, _count_early, _count_later, out_messages, *first_load, errors) ? 0 : -1);
    }, task_name)->on_result_ = [wr_this = weak_from_this(), handler, out_messages, _contact, history_cache, first_load, errors](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        constexpr char task_name[] = "face::get_messages_on_result";

        ptr_this->thread_->run_async_function([history_cache, _contact]()->int32_t
        {
            history_cache->optimize_contact_archive(_contact);
            return 0;
        }, task_name);

        if (handler->on_result)
            handler->on_result(out_messages, *first_load ? archive::first_load::yes : archive::first_load::no, errors);
    };

    return handler;
}

std::shared_ptr<request_history_file_handler> face::get_history_block(std::shared_ptr<contact_and_offsets_v> _contacts,
                                                                      std::shared_ptr<contact_and_msgs> _archive,
                                                                      std::shared_ptr<tools::binary_stream> _data,
                                                                      std::function<bool()> _cancel)
{
    assert(!_contacts->empty());

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_history_file_handler>();

    auto remaining = std::make_shared<contact_and_offsets_v>();

    static constexpr char task_name[] = "face::get_history_block";

    thread_->run_async_function([_contacts, _archive, history_cache, remaining, _data, _cancel]() -> int32_t
    {
        auto remain_size = std::make_shared<int64_t>(search::archive_block_size());
        auto index = 0u;

        _archive->clear();
        auto has_result = false;
        int64_t cur_index = 0;

        while (*remain_size > 0 && index < _contacts->size())
        {
            if (_cancel())
                return -1;

            auto [contact, regim, offset] = (*_contacts)[index];
            ++index;

            auto prev_cur_index = cur_index;
            auto cur_result = history_cache->get_history_file(contact, *_data, offset, remain_size, cur_index, regim);
            has_result |= cur_result;

            if (!cur_result)
            {
                *offset = -2;
            }

            //if (cur_result && cur_index != prev_cur_index)
            {
                _archive->push_back(std::make_pair(contact, prev_cur_index));
            }
        }
        _archive->push_back(std::make_pair(std::string(), cur_index));

        const auto last_index = index;
        const auto contacts_size = _contacts->size();
        remaining->reserve(contacts_size - last_index);
        for (; index < contacts_size; ++index)
        {
            remaining->push_back((*_contacts)[index]);
        }
        _contacts->resize(last_index);

        return (has_result ? 0 : -1);
    }, task_name, _cancel)->on_result_ = [wr_this = weak_from_this(), handler, _archive, history_cache, remaining, _data](int32_t _error)
    {
        // we calc count of finished threads
        // if (_error == -1)
        //    return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(_archive, remaining, _data);
    };

    return handler;
}

std::shared_ptr<request_dlg_state_handler> face::get_dlg_state(std::string_view _contact, bool _debug)
{
    auto handler = std::make_shared<request_dlg_state_handler>();
    auto dialog = std::make_shared<dlg_state>();

    static constexpr char task_name[] = "face::get_dlg_state";

    thread_->run_async_function([history_cache = history_cache_, dialog, _contact = std::string(_contact), _debug]()->int32_t
    {
        if (_debug)
        {
            std::stringstream info;
            info << " -> get_dlg_state(" << _contact << ")\r\n";
            g_core->write_string_to_network_log(info.str());
        }

        *dialog = history_cache->get_dlg_state(_contact);

        return 0;

    }, task_name)->on_result_ = [handler, dialog](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*dialog);
    };

    return handler;
}

std::shared_ptr<request_dlg_states_handler> face::get_dlg_states(const std::vector<std::string>& _contacts)
{
    auto handler = std::make_shared<request_dlg_states_handler>();
    auto dialogs = std::make_shared<std::vector<dlg_state>>();

    static constexpr char task_name[] = "face::get_dlg_states";

    thread_->run_async_function([history_cache = history_cache_, _contacts, dialogs]()->int32_t
    {
        *dialogs = history_cache->get_dlg_states(_contacts);

        return 0;

    }, task_name)->on_result_ = [handler, dialogs](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*dialogs);
    };

    return handler;
}

std::shared_ptr<set_dlg_state_handler> face::set_dlg_state(const std::string& _contact, const dlg_state& _state)
{
    auto handler = std::make_shared<set_dlg_state_handler>();
    auto result_state = std::make_shared<dlg_state>();
    auto state_changes = std::make_shared<dlg_state_changes>();
    const auto correct_time = std::chrono::steady_clock::now();

    static constexpr char task_name[] = "face::set_dlg_state";

    thread_->run_async_function(
        [history_cache = history_cache_, _state, _contact, result_state, state_changes, correct_time]
        {
            history_cache->set_dlg_state(_contact, _state, correct_time, Out *result_state, Out *state_changes);

            return 0;
        }, task_name)
    ->on_result_ =
        [handler, result_state, state_changes]
        (int32_t _error)
        {
            if (handler->on_result)
                handler->on_result(*result_state, *state_changes);
        };

    return handler;
}


std::shared_ptr<async_task_handlers> face::clear_dlg_state(const std::string& _contact)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::clear_dlg_state";

    thread_->run_async_function([history_cache = history_cache_, _contact]()->int32_t
    {
        return (history_cache->clear_dlg_state(_contact) ? 0 : -1);

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_msg_ids_handler> face::get_messages_for_update(const std::string& _contact)
{
    auto handler = std::make_shared<request_msg_ids_handler>();
    auto ids = std::make_shared<std::vector<int64_t>>();

    static constexpr char task_name[] = "face::get_messages_for_update";

    thread_->run_async_function([history_cache = history_cache_, _contact, ids]() -> int32_t
    {
        *ids = history_cache->get_messages_for_update(_contact);

        return 0;

    }, task_name)->on_result_ = [handler, ids](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(ids);
    };

    return handler;
}

std::shared_ptr<get_mentions_handler> face::get_mentions(std::string_view _contact)
{
    auto handler = std::make_shared<get_mentions_handler>();
    auto mentions = std::make_shared<history_block>();
    auto first_load = std::make_shared<bool>(false);

    static constexpr char task_name[] = "face::get_mentions";

    thread_->run_async_function([history_cache = history_cache_, _contact = std::string(_contact), mentions, first_load]() -> int32_t
    {
        *mentions = history_cache->get_mentions(_contact, *first_load);

        return 0;

    }, task_name)->on_result_ = [handler, mentions, first_load](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(mentions, *first_load ? archive::first_load::yes : archive::first_load::no);
    };

    return handler;
}

std::shared_ptr<filter_deleted_handler> face::filter_deleted_messages(const std::string & _contact, std::vector<int64_t>&& _ids)
{
    auto handler = std::make_shared<filter_deleted_handler>();
    auto first_load = std::make_shared<bool>(false);
    auto result = std::make_shared<std::vector<int64_t>>(std::move(_ids));

    static constexpr char task_name[] = "face::filter_deleted_messages";

    thread_->run_async_function([history_cache = history_cache_, _contact = std::string(_contact), result, first_load]()->int32_t
    {
        history_cache->filter_deleted(_contact, *result, *first_load);

        return 0;

    }, task_name)->on_result_ = [handler, result, first_load](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*result, *first_load ? archive::first_load::yes : archive::first_load::no);
    };

    return handler;
}

std::shared_ptr<request_next_hole_handler> face::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto handler = std::make_shared<request_next_hole_handler>();
    auto hole = std::make_shared<archive_hole>();
    auto hole_error = std::make_shared<archive_hole_error>(archive_hole_error::ok);

    static constexpr char task_name[] = "face::get_next_hole";

    thread_->run_async_function([history_cache = history_cache_, hole, hole_error, _contact, _from, _depth]()->int32_t
    {
        auto result = history_cache->get_next_hole(_contact, _from, _depth);
        *hole_error = result.error;
        if (result.hole)
        {
            *hole = *result.hole;
            return 0;
        }

        return -1;

    }, task_name)->on_result_ = [handler, hole, hole_error](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result( (_error == 0) ? hole : nullptr, *hole_error);
    };

    return handler;
}

std::shared_ptr<validate_hole_request_handler> face::validate_hole_request(const std::string& _contact, const archive_hole& _hole_request, const int32_t _count)
{
    auto handler = std::make_shared<validate_hole_request_handler>();
    auto from_result = std::make_shared<int64_t>();

    static constexpr char task_name[] = "face::validate_hole_request";

    thread_->run_async_function([history_cache = history_cache_, _contact, _hole_request, from_result, _count]()->int32_t
    {
        *from_result = history_cache->validate_hole_request(_contact, _hole_request, _count);

        return 0;

    }, task_name)->on_result_ = [handler, from_result](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*from_result);
    };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_pending_message()
{
    auto handler = std::make_shared<not_sent_messages_handler>();
    auto message = std::make_shared<not_sent_message_sptr>();

    static constexpr char task_name[] = "face::get_pending_message";

   thread_->run_async_function(
        [history_cache = history_cache_, message]
    {
        *message = history_cache->get_first_message_to_send();
        return (*message) ? 0 : -1;
    }, task_name)->on_result_ = [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

void face::get_pending_delete_message(std::function<void(const bool _empty, const std::string& _contact, const delete_message& _message)> _callback)
{
    auto message = std::make_shared<delete_message>();
    auto contact = std::make_shared<std::string>();

    static constexpr char task_name[] = "face::get_pending_delete_message";

    thread_->run_async_function([history_cache = history_cache_, message, contact]
    {
        return (history_cache->get_first_message_to_delete(*contact, *message) ? 0 : -1);

    }, task_name)->on_result_ = [_callback, message, contact](int32_t _error)
    {
        _callback(_error != 0, *contact, *message);
    };
}

void face::get_pending_delete_messages(std::function<void(const bool _empty, const std::string& _contact, const std::vector<delete_message>& _messages)> _callback)
{
    auto messages = std::make_shared<std::vector<delete_message>>();
    auto contact = std::make_shared<std::string>();

    thread_->run_async_function([history_cache = history_cache_, messages, contact]
    {
        return (history_cache->get_messages_to_delete(*contact, *messages) ? 0 : -1);

    })->on_result_ = [_callback, messages, contact](int32_t _error)
    {
        _callback(_error != 0, *contact, *messages);
    };
}

std::shared_ptr<insert_pending_delete_message_handler> face::insert_pending_delete_message(const std::string& _contact, delete_message _message)
{
    auto handler = std::make_shared<insert_pending_delete_message_handler>();

    static constexpr char task_name[] = "face::insert_pending_delete_message";

    thread_->run_async_function([history_cache = history_cache_, _contact, _message]
    {
        return history_cache->insert_pending_delete_message(_contact, _message);
    }, task_name)->on_result_ = [handler, _message](int32_t)
    {
        if (handler->on_result)
            handler->on_result(_message.get_message_id(), _message.get_internal_id());
    };

    return handler;
}

void face::remove_pending_delete_message(const std::string& _contact, const delete_message& _message)
{
    static constexpr char task_name[] = "face::remove_pending_delete_message";

    thread_->run_async_function([history_cache = history_cache_, _contact, _message]
    {
        return history_cache->remove_pending_delete_message(_contact, _message);
    }, task_name);
}

std::shared_ptr<get_locale_handler> face::get_locale(std::string_view _contact)
{
    auto handler = std::make_shared<get_locale_handler>();
    auto locale = std::make_shared<std::optional<std::string>>();

    static constexpr char task_name[] = "face::get_locale";

    thread_->run_async_function([_contact = std::string(_contact), history_cache = history_cache_, locale]()->int32_t
    {
        *locale = history_cache->get_locale(_contact);
        return 0;

    }, task_name)->on_result_ = [handler, locale](int32_t /*_error*/)
    {
        if (handler->on_result)
            handler->on_result(std::move(*locale));
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::delete_messages_up_to(const std::string& _contact, const int64_t _id)
{
    assert(!_contact.empty());
    assert(_id > -1);

    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::delete_messages_up_to";

    thread_->run_async_function(
        [history_cache = history_cache_, _contact, _id]
        {
            history_cache->delete_messages_up_to(_contact, _id);
            return 0;
        }, task_name
    )->on_result_ =
        [handler](int32_t _error)
        {
            if (handler->on_result_)
                handler->on_result_(_error);
        };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_not_sent_message_by_iid(const std::string& _iid)
{
    auto handler = std::make_shared<not_sent_messages_handler>();
    auto message = std::make_shared<not_sent_message_sptr>();

    static constexpr char task_name[] = "face::get_not_sent_message_by_iid";

    auto task = thread_->run_async_function(
        [history_cache = history_cache_, message, _iid]
    {
        *message = history_cache->get_not_sent_message_by_iid(_iid);
        return (*message) ? 0 : -1;
    }, task_name
    );

    task->on_result_ =
        [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::insert_not_sent_message";

    thread_->run_async_function([_contact, _msg, history_cache = history_cache_]()->int32_t
    {
        return history_cache->insert_not_sent_message(_contact, _msg);

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::update_if_exist_not_sent_message";

    thread_->run_async_function([_contact, _msg, history_cache = history_cache_]()->int32_t
    {
        return history_cache->update_if_exist_not_sent_message(_contact, _msg);

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::remove_messages_from_not_sent(
    const std::string& _contact,
    const bool _remove_if_modified,
    const archive::history_block_sptr& _data)
{
    auto handler = std::make_shared<async_task_handlers>();
    const auto correct_time = std::chrono::steady_clock::now();

    static constexpr char task_name[] = "face::remove_messages_from_not_sent";

    thread_->run_async_function([_contact, _data, history_cache = history_cache_, correct_time, _remove_if_modified]()->int32_t
    {
        return history_cache->remove_messages_from_not_sent(_contact, _remove_if_modified, _data, correct_time);

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::remove_messages_from_not_sent(
    const std::string& _contact,
    const bool _remove_if_modified,
    const archive::history_block_sptr& _data1,
    const archive::history_block_sptr& _data2)
{
    auto handler = std::make_shared<async_task_handlers>();
    const auto correct_time = std::chrono::steady_clock::now();

    static constexpr char task_name[] = "face::remove_messages_from_not_sent2";

    thread_->run_async_function([_contact, _data1, _data2, history_cache = history_cache_, correct_time, _remove_if_modified]()->int32_t
    {
        return history_cache->remove_messages_from_not_sent(_contact, _remove_if_modified, _data1, _data2, correct_time);

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::remove_message_from_not_sent(
    const std::string& _contact,
    const bool _remove_if_modified,
    const history_message_sptr _data)
{
    auto block = std::make_shared<history_block>();
    block->push_back(std::const_pointer_cast<history_message>(_data));

    return remove_messages_from_not_sent(_contact, _remove_if_modified, block);
}

std::shared_ptr<async_task_handlers> face::mark_message_duplicated(const std::string& _message_internal_id)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::mark_message_duplicated";

    thread_->run_async_function([history_cache = history_cache_, _message_internal_id]()->int32_t
    {
        history_cache->mark_message_duplicated(_message_internal_id);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

void face::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    local_history::serialize(_headers, _coll);
}

void face::serialize_headers(archive::history_block_sptr _data, coll_helper& _coll)
{
    local_history::serialize_headers(_data, _coll);
}

std::shared_ptr<has_not_sent_handler> face::has_not_sent_messages(const std::string& _contact)
{
    auto handler = std::make_shared<has_not_sent_handler>();
    auto res = std::make_shared<bool>(false);

    static constexpr char task_name[] = "face::has_not_sent_messages";

    thread_->run_async_function([_contact, history_cache = history_cache_, res]()->int32_t
    {
        *res = history_cache->has_not_sent_messages(_contact);

        return 0;

    }, task_name)->on_result_ = [handler, res](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*res);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_not_sent_messages(const std::string& _contact, bool _debug)
{
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();

    static constexpr char task_name[] = "face::get_not_sent_messages";

    thread_->run_async_function([_contact, out_messages, history_cache = history_cache_, _debug]()->int32_t
    {
        if (_debug)
        {
            std::string info;
            info += "-> get_not_sent_messages(";
            info += _contact;
            info +=  ")\r\n";
            g_core->write_string_to_network_log(info);
        }

        history_cache->get_not_sent_messages(_contact, out_messages);

        return 0;

    }, task_name)->on_result_ = [handler, out_messages](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages, archive::first_load::no, std::make_shared<error_vector>()); // now there is no need to check first load for pendings
    };

    return handler;
}


std::shared_ptr<pending_messages_handler> face::get_pending_file_sharing()
{
    auto handler = std::make_shared<pending_messages_handler>();
    auto messages_list = std::make_shared<std::list<not_sent_message_sptr>>();

    static constexpr char task_name[] = "face::get_pending_file_sharing";

    thread_->run_async_function([messages_list, history_cache = history_cache_]()->int32_t
    {
        history_cache->get_pending_file_sharing(*messages_list);

        return 0;

    }, task_name)->on_result_ = [handler, messages_list](int32_t _error)
    {
        handler->on_result(*messages_list);
    };

    return handler;
}


std::shared_ptr<not_sent_messages_handler> face::update_pending_messages_by_imstate(const std::string& _message_internal_id,
                                                                                    const int64_t& _hist_msg_id,
                                                                                    const int64_t& _before_hist_msg_id)
{
    auto handler = std::make_shared<not_sent_messages_handler>();
    auto message = std::make_shared<not_sent_message_sptr>();

    const auto correct_time = std::chrono::steady_clock::now();

    static constexpr char task_name[] = "face::update_pending_messages_by_imstate";

    auto task = thread_->run_async_function([history_cache = history_cache_, message, _message_internal_id, _hist_msg_id, _before_hist_msg_id, correct_time]
    {
        *message = history_cache->update_pending_with_imstate(_message_internal_id, _hist_msg_id, _before_hist_msg_id, correct_time);

        return (*message) ? 0 : -1;
    }, task_name);

    task->on_result_ = [handler, message](int32_t _error)
    {
        const auto succeed = (_error == 0);

        handler->on_result(succeed ? *message : nullptr);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_message_post_time(
    const std::string& _message_internal_id,
    const std::chrono::system_clock::time_point& _time_point)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::update_message_post_time";

    auto task = thread_->run_async_function([history_cache = history_cache_, _message_internal_id, _time_point]
    {
        history_cache->update_message_post_time(_message_internal_id, _time_point);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(0);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::failed_pending_message(const std::string& _message_internal_id)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::failed_pending_message";

    auto task = thread_->run_async_function([history_cache = history_cache_, _message_internal_id]
    {
        history_cache->failed_pending_message(_message_internal_id);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(0);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::sync_with_history()
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::sync_with_history";

    thread_->run_async_function([]()->int32_t
    {
        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::add_mention(const std::string& _contact, const std::shared_ptr<archive::history_message>& _message)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::add_mention";

    thread_->run_async_function([_contact, _message, history_cache = history_cache_]()->int32_t
    {
        history_cache->add_mention(_contact, _message);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_attention_attribute(const std::string& _aimid, const bool _value)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::update_attention_attribute";

    thread_->run_async_function([history_cache = history_cache_, _aimid, _value]()->int32_t
    {
        history_cache->update_attention_attribute(_aimid, _value);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_merge_gallery_from_server> face::merge_gallery_from_server(const std::string& _aimid, const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till)
{
    auto handler = std::make_shared<request_merge_gallery_from_server>();
    auto changes = std::make_shared<std::vector<archive::gallery_item>>();

    static constexpr char task_name[] = "face::merge_gallery_from_server";

    thread_->run_async_function([history_cache = history_cache_, _aimid, _gallery, _from, _till, changes]()->int32_t
    {
        history_cache->merge_server_gallery(_aimid, _gallery, _from, _till, *changes);

        return 0;

    }, task_name)->on_result_ = [handler, changes](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*changes);
    };

    return handler;
}

std::shared_ptr<request_gallery_state_handler> face::get_gallery_state(const std::string& _aimid)
{
    auto handler = std::make_shared<request_gallery_state_handler>();
    auto _state = std::make_shared<archive::gallery_state>();

    static constexpr char task_name[] = "face::get_gallery_state";

    thread_->run_async_function([history_cache = history_cache_, _state, _aimid]()->int32_t
    {
        history_cache->get_gallery_state(_aimid, *_state);

        return 0;

    }, task_name)->on_result_ = [handler, _state](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*_state);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::set_gallery_state(const std::string& _aimid, const archive::gallery_state& _state, bool _store_patch_version)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::set_gallery_state";

    thread_->run_async_function([history_cache = history_cache_, _state, _aimid, _store_patch_version]()->int32_t
    {
        history_cache->set_gallery_state(_aimid, _state, _store_patch_version);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_gallery_holes> face::get_gallery_holes(const std::string& _aimid)
{
    auto handler = std::make_shared<request_gallery_holes>();
    auto result = std::make_shared<bool>();
    auto from = std::make_shared<archive::gallery_entry_id>();
    auto till = std::make_shared<archive::gallery_entry_id>();

    static constexpr char task_name[] = "face::get_gallery_holes";

    thread_->run_async_function([history_cache = history_cache_, result, from, till, _aimid]()->int32_t
    {
        history_cache->get_gallery_holes(_aimid, *result, *from, *till);

        return 0;

    }, task_name)->on_result_ = [handler, result, from, till](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*result, *from, *till);
    };

    return handler;
}

std::shared_ptr<request_gallery_entries_page> face::get_gallery_entries(const std::string& _aimid, const archive::gallery_entry_id& _from, const std::vector<std::string> _types, int _page_size)
{
    auto handler = std::make_shared<request_gallery_entries_page>();
    auto entries = std::make_shared<std::vector<archive::gallery_item>>();
    auto exhausted = std::make_shared<bool>();

    static constexpr char task_name[] = "face::get_gallery_entries";

    thread_->run_async_function([history_cache = history_cache_, entries, exhausted, _aimid, _from, _types, _page_size]()->int32_t
    {
        *exhausted = history_cache->get_gallery_entries(_aimid, _from, _types, _page_size, *entries);

        return 0;

    }, task_name)->on_result_ = [handler, entries, exhausted](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*entries, *exhausted);
    };

    return handler;
}

std::shared_ptr<request_gallery_entries> face::get_gallery_entries_by_msg(const std::string& _aimid, const std::vector<std::string> _types, int64_t _msg_id)
{
    auto handler = std::make_shared<request_gallery_entries>();
    auto entries = std::make_shared<std::vector<archive::gallery_item>>();
    auto index = std::make_shared<int>();
    auto total = std::make_shared<int>();

    static constexpr char task_name[] = "face::get_gallery_entries_by_msg";

    thread_->run_async_function([history_cache = history_cache_, entries, _aimid, _types, _msg_id, index, total]()->int32_t
    {
        history_cache->get_gallery_entries_by_msg(_aimid, _types, _msg_id, *entries, *index, *total);
        return 0;

    }, task_name)->on_result_ = [handler, entries, index, total](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*entries, *index, *total);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::clear_hole_request(const std::string& _aimid)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::clear_hole_request";

    thread_->run_async_function([history_cache = history_cache_, _aimid]()->int32_t
    {
        history_cache->clear_hole_request(_aimid);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::make_gallery_hole(const std::string& _aimId, int64_t _from, int64_t _till)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::make_gallery_hole";

    thread_->run_async_function([history_cache = history_cache_, _aimId, _from, _till]()->int32_t
    {
        history_cache->make_gallery_hole(_aimId, _from, _till);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::make_holes(const std::string& _aimid)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::make_holes";

    thread_->run_async_function([history_cache = history_cache_, _aimid]()->int32_t
    {
        history_cache->make_holes(_aimid);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_gallery_is_hole_requested> face::is_gallery_hole_requested(const std::string& _aimid)
{
    auto handler = std::make_shared<request_gallery_is_hole_requested>();
    auto is_hole_requsted = std::make_shared<bool>();


    static constexpr char task_name[] = "face::is_gallery_hole_requested";

    thread_->run_async_function([history_cache = history_cache_, _aimid, is_hole_requsted]()->int32_t
    {
        history_cache->is_gallery_hole_requested(_aimid, *is_hole_requsted);
        return 0;

    }, task_name)->on_result_ = [handler, is_hole_requsted](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*is_hole_requsted);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::invalidate_message_data(const std::string& _aimid, std::vector<int64_t> _ids)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::invalidate_message_data";

    thread_->run_async_function([history_cache = history_cache_, _aimid, ids = std::move(_ids)]()->int32_t
    {
        history_cache->invalidate_message_data(_aimid, ids);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::invalidate_message_data(const std::string& _aimid, int64_t _from, int64_t _before_count, int64_t _after_count)
{
    auto handler = std::make_shared<async_task_handlers>();

    static constexpr char task_name[] = "face::mark_message_duplicated2";

    thread_->run_async_function([history_cache = history_cache_, _aimid, _from, _before_count, _after_count]()->int32_t
    {
        history_cache->invalidate_message_data(_aimid, _from, _before_count, _after_count);

        return 0;

    }, task_name)->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

void face::free_dialog(const std::string& _contact)
{
    static constexpr char task_name[] = "face::free_dialog";

    thread_->run_async_function([history_cache = history_cache_, _contact]()->int32_t
    {
        history_cache->free_dialog(_contact);

        return 0;

    }, task_name);
}

std::shared_ptr<memory_usage> face::get_memory_usage()
{
    auto handler = std::make_shared<memory_usage>();

    auto index_size = std::make_shared<int64_t>(0);
    auto gallery_size = std::make_shared<int64_t>(0);

    static constexpr char task_name[] = "face::get_memory_usage";

    thread_->run_async_function([history_cache = history_cache_, index_size, gallery_size]()->int32_t
    {
        history_cache->get_memory_usage(*index_size, *gallery_size);

        return 0;

    }, task_name)->on_result_ = [handler, index_size, gallery_size](int32_t _error)
    {
        handler->on_result(*index_size, *gallery_size);
    };

    return handler;
}

void face::insert_reactions(const std::string& _contact, const reactions_vector_sptr& _reactions)
{
    static constexpr char task_name[] = "face::insert_reactions";

    thread_->run_async_function([history_cache = history_cache_, _contact, _reactions]()->int32_t
    {
        history_cache->insert_reactions(_contact, _reactions);

        return 0;

    }, task_name);
}

std::shared_ptr<get_reactions_handler> face::get_reactions(const std::string& _contact, const std::shared_ptr<msgids_list>& _msg_ids)
{
    auto handler = std::make_shared<get_reactions_handler>();

    auto result = std::make_shared<reactions_vector>();
    auto missing = std::make_shared<msgids_list>();

    static constexpr char task_name[] = "face::get_reactions";

    thread_->run_async_function([history_cache = history_cache_, _contact, _msg_ids, result, missing]()->int32_t
    {
        history_cache->get_reactions(_contact, _msg_ids, result, missing);

        return 0;

    }, task_name)->on_result_ = [handler, result, missing](int32_t _error)
    {
        handler->on_result(result, missing);
    };

    return handler;
}

#include "stdafx.h"

#include "../../corelib/collection.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"

#include "../log/log.h"

#include "../tools/file_sharing.h"
#include "../tools/system.h"

#include "../core.h"
#include "../network_log.h"

#include "history_message.h"
#include "storage.h"

#include "not_sent_messages.h"

using namespace core;
using namespace archive;

namespace
{
    enum not_sent_message_fields
    {
        min = 0,

        contact = 1,
        message = 2,
        duplicated = 3,
        modified = 4,
        network_request_id = 5,
        deleted = 6,
        delete_op = 7,

        max
    };
}

not_sent_message_sptr not_sent_message::make(const core::tools::tlvpack& _pack)
{
    const not_sent_message_sptr msg(new not_sent_message);
    if (msg->unserialize(_pack))
    {
        return msg;
    }

    return not_sent_message_sptr();
}

not_sent_message_sptr not_sent_message::make(const not_sent_message_sptr& _message, const uint64_t _message_time)
{
    return not_sent_message_sptr(new not_sent_message(_message, _message_time));
}

not_sent_message_sptr not_sent_message::make(
    const std::string& _aimid,
    std::string _message,
    const message_type _type,
    const uint64_t _message_time,
    std::string _internal_id)
{
    return not_sent_message_sptr(new not_sent_message(_aimid, std::move(_message), _type, _message_time, std::move(_internal_id)));
}

not_sent_message_sptr not_sent_message::make_outgoing_file_sharing(
    const std::string& _aimid,
    const uint64_t _message_time,
    const std::string& _local_path,
    const core::archive::quotes_vec& _quotes,
    const std::string& _description,
    const core::archive::mentions_map& _mentions,
    const std::optional<int64_t>& _duration)
{
    assert(!_aimid.empty());
    assert(_message_time > 0);
    assert(!_local_path.empty());

    not_sent_message_sptr not_sent(
        new not_sent_message(
            _aimid,
            std::string(),
            message_type::file_sharing,
            _message_time,
            std::string()));

    if (!not_sent->get_message()->init_file_sharing_from_local_path(_local_path, _duration,
        _duration.has_value() ? std::make_optional(file_sharing_base_content_type::ptt) : std::nullopt))
        return {};

    not_sent->set_description(_description);
    not_sent->attach_quotes(_quotes);
    not_sent->set_mentions(_mentions);

    return not_sent;
}

not_sent_message_sptr not_sent_message::make_incoming_file_sharing(
    const std::string& _aimid,
    const uint64_t _message_time,
    const std::string& _uri,
    std::string _internal_id)
{
    assert(!_aimid.empty());
    assert(_message_time > 0);
    assert(!_internal_id.empty());

    not_sent_message_sptr not_sent(
        new not_sent_message(_aimid, _uri, message_type::file_sharing, _message_time, std::move(_internal_id))
        );

    not_sent->get_message()->init_file_sharing_from_link(_uri);

    return not_sent;
}

not_sent_message::not_sent_message()
    : message_(std::make_shared<history_message>())
    , duplicated_(false)
    , calc_sent_stat_(false)
    , create_time_(std::chrono::steady_clock::now())
    , updated_id_(-1)
    , modified_(false)
    , deleted_(false)
    , delete_operation_(delete_operation::del_for_all)
{
}

not_sent_message::not_sent_message(
    const std::string& _aimid,
    std::string _message,
    const message_type _type,
    const uint64_t _message_time,
    std::string&& _internal_id)
    : not_sent_message()
{
    aimid_ = _aimid;

    message_->set_internal_id(_internal_id.empty() ? core::tools::system::generate_internal_id() : std::move(_internal_id));

    message_->set_outgoing(true);
    message_->set_time(_message_time);

    if (_type == message_type::sticker)
    {
        message_->init_sticker_from_text(std::move(_message));
    }
    else
    {
        message_->set_text(std::move(_message));
    }

    if (_aimid.find("@chat.agent") != _aimid.npos)
        message_->set_chat_data(archive::chat_data());
}

not_sent_message::not_sent_message(const not_sent_message_sptr& _message, const uint64_t _message_time)
    : not_sent_message()
{
    copy_from(_message);
    message_->set_time(_message_time);
}

not_sent_message::~not_sent_message()
{
}

void not_sent_message::copy_from(const not_sent_message_sptr& _message)
{
    assert(_message);

    aimid_ = _message->aimid_;
    duplicated_ = _message->duplicated_;
    post_time_ = _message->post_time_;
    message_ = _message->message_;
    modified_ = _message->modified_;
    network_request_id_ = _message->network_request_id_;
    deleted_ = _message->deleted_;
    delete_operation_ = _message->delete_operation_;
}

const history_message_sptr& not_sent_message::get_message() const
{
    return message_;
}

const std::string& not_sent_message::get_aimid() const
{
    assert(!aimid_.empty());
    return aimid_;
}

const std::string& not_sent_message::get_internal_id() const
{
    return message_->get_internal_id();
}

const std::string& not_sent_message::get_file_sharing_local_path() const
{
    static const std::string empty;
    assert(message_);
    return message_ && message_->get_file_sharing_data() ? message_->get_file_sharing_data()->get_local_path() : empty;
}

const core::archive::quotes_vec& not_sent_message::get_quotes() const
{
    return message_->get_quotes();
}

void not_sent_message::attach_quotes(core::archive::quotes_vec _quotes)
{
    message_->attach_quotes(std::move(_quotes));
}

const core::archive::mentions_map& core::archive::not_sent_message::get_mentions() const
{
    return message_->get_mentions();
}

void core::archive::not_sent_message::set_mentions(core::archive::mentions_map _mentions)
{
    message_->set_mentions(std::move(_mentions));
}

void core::archive::not_sent_message::set_updated_id(int64_t _id)
{
    message_->set_msgid(_id);
}

int64_t core::archive::not_sent_message::get_updated_id() const
{
    return message_->get_msgid();
}

bool not_sent_message::is_ready_to_send() const
{
    const auto &message = get_message();
    assert(message);

    if (duplicated_)
    {
        return false;
    }

    const auto &fs_data = message->get_file_sharing_data();
    if (fs_data && !fs_data->get_local_path().empty())
    {
        return false;
    }

    return true;
}

void not_sent_message::serialize(core::tools::tlvpack& _pack) const
{
    _pack.push_child(tools::tlv(not_sent_message_fields::contact, aimid_));
    _pack.push_child(tools::tlv(not_sent_message_fields::duplicated, duplicated_));
    _pack.push_child(tools::tlv(not_sent_message_fields::modified, modified_));
    _pack.push_child(tools::tlv(not_sent_message_fields::network_request_id, network_request_id_));
    _pack.push_child(tools::tlv(not_sent_message_fields::deleted, deleted_));
    _pack.push_child(tools::tlv(not_sent_message_fields::delete_op, (int32_t) delete_operation_));

    core::tools::binary_stream bs_message;
    message_->serialize(bs_message);
    _pack.push_child(tools::tlv(not_sent_message_fields::message, bs_message));
}

void not_sent_message::serialize(core::coll_helper& _coll, const time_t _offset) const
{
    get_message()->serialize(_coll.get(), _offset);
}

bool not_sent_message::unserialize(const core::tools::tlvpack& _pack)
{
    auto tlv_aimid = _pack.get_item(not_sent_message_fields::contact);
    auto tlv_message = _pack.get_item(not_sent_message_fields::message);
    auto tlv_duplicated = _pack.get_item(not_sent_message_fields::duplicated);
    auto tlv_modified = _pack.get_item(not_sent_message_fields::modified);
    auto tlv_network_request_id = _pack.get_item(not_sent_message_fields::network_request_id);
    auto tlv_deleted = _pack.get_item(not_sent_message_fields::deleted);
    auto tlv_delete_operation = _pack.get_item(not_sent_message_fields::delete_op);

    if (!tlv_message || !tlv_aimid)
        return false;

    aimid_ = tlv_aimid->get_value<std::string>();

    if (tlv_duplicated)
    {
        duplicated_ = tlv_duplicated->get_value<bool>();
    }

    if (tlv_modified)
    {
        modified_ = tlv_modified->get_value<bool>();
    }

    if (tlv_network_request_id)
    {
        network_request_id_ = tlv_network_request_id->get_value<std::string>();
    }

    if (tlv_deleted)
    {
        deleted_ = tlv_deleted->get_value<bool>();
    }

    if (tlv_delete_operation)
    {
        delete_operation_ = (delete_operation) tlv_delete_operation->get_value<int32_t>();
    }

    core::tools::binary_stream stream = tlv_message->get_value<core::tools::binary_stream>();
    return !message_->unserialize(stream);
}

void not_sent_message::mark_duplicated()
{
    duplicated_ = true;
}



void not_sent_message::set_failed()
{
    duplicated_ = false;
}

void not_sent_message::update_post_time(const std::chrono::system_clock::time_point& _time_point)
{
    post_time_ = _time_point;
}

const std::chrono::system_clock::time_point& not_sent_message::get_post_time() const
{
    return post_time_;
}

const std::chrono::steady_clock::time_point& not_sent_message::get_create_time() const
{
    return create_time_;
}

void not_sent_message::set_calc_stat(const bool _calc)
{
    calc_sent_stat_ = _calc;
}

bool not_sent_message::is_calc_stat() const
{
    return calc_sent_stat_;
}

bool not_sent_message::is_modified() const
{
    return modified_;
}

void not_sent_message::set_modified(const bool _modified)
{
    modified_ = _modified;
}

void not_sent_message::set_network_request_id(const std::string& _id)
{
    network_request_id_ = _id;
}

const std::string& not_sent_message::get_network_request_id() const
{
    return network_request_id_.empty() ? get_internal_id() : network_request_id_;
}

bool not_sent_message::is_deleted() const
{
    return deleted_;
}

void not_sent_message::set_deleted(const bool _deleted)
{
    deleted_ = _deleted;
}


void not_sent_message::set_delete_operation(const delete_operation _delete_operation)
{
    delete_operation_ = _delete_operation;
    if (_delete_operation == delete_operation::del_for_all)
        history_message::make_modified(message_);
    else if (_delete_operation == delete_operation::del_for_me)
        history_message::make_deleted(message_);
}

const delete_operation& not_sent_message::get_delete_operation() const
{
    return delete_operation_;
}

void not_sent_message::set_description(const std::string& _description)
{
    message_->set_description(_description);
}

std::string not_sent_message::get_description() const
{
    return message_->description();
}

void not_sent_message::set_url(const std::string& _url)
{
    message_->set_url(_url);
}

std::string not_sent_message::get_url() const
{
    return message_->url();
}

void not_sent_message::set_shared_contact(const core::archive::shared_contact& _contact)
{
    message_->set_shared_contact(_contact);
}

const core::archive::shared_contact& not_sent_message::get_shared_contact() const
{
    return message_->get_shared_contact();
}

std::optional<int64_t> core::archive::not_sent_message::get_duration() const
{
    assert(message_);
    return message_ && message_->get_file_sharing_data() ? message_->get_file_sharing_data()->get_duration() : std::nullopt;
}

std::optional<file_sharing_base_content_type> core::archive::not_sent_message::get_base_content_type() const
{
    assert(message_);
    return message_ && message_->get_file_sharing_data() ? message_->get_file_sharing_data()->get_base_content_type() : std::nullopt;
}

void not_sent_message::set_geo(const core::archive::geo& _geo)
{
    message_->set_geo(_geo);
}

const core::archive::geo&not_sent_message::get_geo() const
{
    return message_->get_geo();
}

void not_sent_message::set_poll(const core::archive::poll& _poll)
{
    message_->set_poll(_poll);
}

const core::archive::poll not_sent_message::get_poll() const
{
    return message_->get_poll();
}

//////////////////////////////////////////////////////////////////////////
// delete_message
//////////////////////////////////////////////////////////////////////////
delete_message::delete_message()
    : message_id_(-1)
    , operation_(delete_operation::min)
{

}

delete_message::delete_message(const std::string& _aimid, const int64_t _message_id, const std::string& _internal_id, const delete_operation _operation)
    : aimid_(_aimid)
    , message_id_(_message_id)
    , internal_id_(_internal_id)
    , operation_(_operation)
{

}

int64_t delete_message::get_message_id() const
{
    return message_id_;
}

const std::string& delete_message::get_internal_id() const
{
    return internal_id_;
}

delete_operation delete_message::get_operation() const
{
    return operation_;
}

const std::string& delete_message::get_aimid() const
{
    return aimid_;
}

bool delete_message::operator==(const delete_message& _other) const
{
    return (message_id_ == _other.message_id_ && operation_ == _other.operation_);
}

void delete_message::serialize(core::tools::tlvpack& _pack) const
{
    _pack.push_child(tools::tlv(delete_message_fields::contact, aimid_));
    _pack.push_child(tools::tlv(delete_message_fields::message_id, get_message_id()));
    _pack.push_child(tools::tlv(delete_message_fields::internal_id, get_internal_id()));
    _pack.push_child(tools::tlv(delete_message_fields::operation, (int32_t) get_operation()));
}

std::unique_ptr<delete_message> delete_message::make(const core::tools::tlvpack& _pack)
{
    auto tlv_aimid = _pack.get_item(delete_message_fields::contact);
    auto tlv_message_id = _pack.get_item(delete_message_fields::message_id);
    auto tlv_internal_id = _pack.get_item(delete_message_fields::internal_id);
    auto tlv_operation = _pack.get_item(delete_message_fields::operation);

    if (!tlv_aimid || !tlv_message_id || !tlv_operation || !tlv_internal_id)
        return nullptr;

    const std::string aimid = tlv_aimid->get_value<std::string>();
    const int64_t message_id = tlv_message_id->get_value<int64_t>();
    const std::string internal_id = tlv_internal_id->get_value<std::string>();
    const delete_operation operation = (delete_operation) tlv_operation->get_value<int32_t>();

    if (operation <= delete_operation::min || operation >= delete_operation::max)
    {
        assert(false);
        return nullptr;
    }

    return std::make_unique<delete_message>(aimid, message_id, internal_id, operation);
}


//////////////////////////////////////////////////////////////////////////
// pending_operations class
//////////////////////////////////////////////////////////////////////////
pending_operations::pending_operations(
    const std::wstring& _file_pending_sent,
    const std::wstring& _file_pending_delete)
    : pending_sent_storage_(std::make_unique<storage>(_file_pending_sent))
    , pending_delete_storage_(std::make_unique<storage>(_file_pending_delete))
    , is_loaded_(false)
{
    load_if_need();
}

pending_operations::~pending_operations() = default;

void pending_operations::insert_message(const std::string& _aimid, const not_sent_message_sptr &_message)
{
    assert(_message);

    auto &contact_messages = pending_messages_[_aimid];

    bool was_updated = false;
    for (auto &existing_message : contact_messages)
    {
        const auto have_same_internal_id = (_message->get_message()->get_internal_id() == existing_message->get_message()->get_internal_id());
        if (!have_same_internal_id)
        {
            continue;
        }

        assert(!"not sent message already exist");
        existing_message = _message;
        was_updated = true;
        break;
    }

    if (!was_updated)
    {
        contact_messages.push_back(_message);

        std::stringstream log;
        log << "CORE: push pending\r\n";
        log << "for: " << _aimid << "\r\n";
        log << "reqId: " << _message->get_message()->get_internal_id() << "\r\n";
        g_core->write_string_to_network_log(log.str());
    }

    save_pending_sent_messages();
}

bool pending_operations::update_message_if_exist(const std::string &_aimid, const not_sent_message_sptr &_message)
{
    assert(!_aimid.empty());
    assert(_message);

    auto iter_messages = pending_messages_.find(_aimid);
    if (iter_messages == pending_messages_.end())
    {
        return false;
    }

    auto &messages = iter_messages->second;

    for (auto &message : messages)
    {
        if (_message->get_internal_id() == message->get_internal_id())
        {
            auto fs = message->get_message()->get_file_sharing_data() ? *(message->get_message()->get_file_sharing_data()) : file_sharing_data{};
            message = _message;
            if (!fs.is_empty())
                message->get_message()->init_file_sharing(std::move(fs));

            save_pending_sent_messages();

            return true;
        }
    }

    return false;
}

void pending_operations::load_if_need()
{
    if (!is_loaded_)
    {
        load_pending_sent_messages();
        load_pending_delete_messages();
    }

    is_loaded_ = true;
}



void pending_operations::remove(const std::string& _internal_id)
{
    for (auto& _messages_pair : pending_messages_)
    {
        for (auto iter = _messages_pair.second.begin(); iter != _messages_pair.second.end();)
        {
            const auto &not_sent_message = **iter;

            const auto &message = *not_sent_message.get_message();

            const auto same_id = (message.get_internal_id() == _internal_id || not_sent_message.get_network_request_id() == _internal_id);

            if (same_id)
            {
                iter = _messages_pair.second.erase(iter);
                save_pending_sent_messages();

                std::stringstream log;
                log << "CORE: remove pending\r\n";
                log << "reqId: " << _internal_id << "\r\n";
                g_core->write_string_to_network_log(log.str());

                break;
            }

            ++iter;
        }
    }
}

void pending_operations::drop(const std::string& _aimid)
{
    if (auto it = pending_messages_.find(_aimid); it != pending_messages_.end())
    {
        pending_messages_.erase(it);
        save_pending_sent_messages();

        std::stringstream log;
        log << "CORE: drop pendings\r\n";
        log << "for: " << _aimid << "\r\n";
        g_core->write_string_to_network_log(log.str());
    }
    if (auto it = pending_delete_messages_.find(_aimid); it != pending_delete_messages_.end())
    {
        pending_delete_messages_.erase(it);
        save_pending_delete_messages();
    }
}

message_stat_time_v pending_operations::remove(const std::string &_aimid, const bool _remove_if_modified, const history_block_sptr &_data)
{
    assert(!_aimid.empty());
    assert(_data);

    message_stat_time_v stats;

    auto &messages = pending_messages_[_aimid];
    if (messages.empty())
        return stats;

    bool save_on_exit = false;

    tools::auto_scope as([&save_on_exit, this]
    {
        if (save_on_exit)
            save_pending_sent_messages();
    });

    stats.reserve((*_data).size());
    for (const auto &block : *_data)
    {
        const auto &block_internal_id = block->get_internal_id();
        const auto block_msgid = block->get_msgid();

        std::stringstream log;
        log << "CORE: remove pending\r\n";
        log << "reqId: " << block_internal_id << "\r\n";
        log << "id: " << block_msgid << "\r\n";
        g_core->write_string_to_network_log(log.str());

        for (auto iter = messages.begin(); iter != messages.end();)
        {
            const auto &not_sent_message = **iter;

            const auto &message = *not_sent_message.get_message();

            const auto same_id = (message.get_internal_id() == block_internal_id || not_sent_message.get_network_request_id() == block_internal_id);

            if (same_id)
            {
                message_stat_time st;
                st.create_time_ = not_sent_message.get_create_time();
                st.need_stat_ = not_sent_message.is_calc_stat();
                st.contact_ = not_sent_message.get_aimid();
                st.msg_id_ = block_msgid;
                stats.emplace_back(std::move(st));

                save_on_exit = true;

                if (not_sent_message.is_deleted())
                {
                    insert_deleted_message(_aimid, delete_message(_aimid, block_msgid, message.get_internal_id(), not_sent_message.get_delete_operation()));

                    iter = messages.erase(iter);
                    continue;
                }
                else if (not_sent_message.is_modified() && !_remove_if_modified)
                {
                    (*iter)->set_updated_id(block_msgid);
                    (*iter)->set_modified(false);
                    (*iter)->set_network_request_id(core::tools::system::generate_internal_id());
                }
                else
                {
                    iter = messages.erase(iter);
                    continue;
                }
            }

            ++iter;
        }
    }

    if (messages.empty())
        pending_messages_.erase(_aimid);

    return stats;
}

void pending_operations::mark_duplicated(const std::string& _message_internal_id)
{
    auto msg = get_message_by_internal_id(_message_internal_id);

    if (msg)
    {
        msg->mark_duplicated();

        save_pending_sent_messages();
    }
}

void pending_operations::update_message_post_time(
    const std::string& _message_internal_id,
    const std::chrono::system_clock::time_point& _time_point)
{
    auto msg = get_message_by_internal_id(_message_internal_id);

    if (msg)
    {
        msg->update_post_time(_time_point);
    }
}

not_sent_message_sptr pending_operations::update_with_imstate(
    const std::string& _message_internal_id,
    const int64_t& _hist_msg_id,
    const int64_t& _before_hist_msg_id)
{
    auto msg = get_message_by_internal_id(_message_internal_id);

    if (!msg)
    {
        return nullptr;
    }

    if (_hist_msg_id > 0)
    {
        if (msg->is_deleted())
        {
            insert_deleted_message(msg->get_aimid(), delete_message(msg->get_aimid(), _hist_msg_id, _message_internal_id, msg->get_delete_operation()));

            remove(_message_internal_id);
        }
        else if (msg->is_modified())
        {

        }
        else
        {
            remove(_message_internal_id);
        }

        save_pending_sent_messages();

        return msg;
    }

    return nullptr;
}

void pending_operations::failed_pending_message(const std::string& _message_internal_id)
{
    auto msg = get_message_by_internal_id(_message_internal_id);

    if (!msg)
    {
        return;
    }

    msg->set_failed();

    save_pending_sent_messages();
}

bool pending_operations::exist_message(const std::string& _aimid) const
{
    auto iter_c = pending_messages_.find(_aimid);
    if (iter_c == pending_messages_.end())
        return false;

    return (!iter_c->second.empty());
}

void pending_operations::get_messages(const std::string& _aimid, Out history_block& _messages)
{
    auto iter_c = pending_messages_.find(_aimid);
    if (iter_c == pending_messages_.end())
        return;

    const not_sent_messages_list& messages_list = iter_c->second;

    _messages.reserve(messages_list.size());

    for (const auto& msg : messages_list)
    {
        if (msg->get_message()->is_file_sharing())
        {
            const auto& data = msg->get_message()->get_file_sharing_data();
            if (data && data->get_uri().empty() && !core::tools::system::is_exist(data->get_local_path()))
                continue;
        }
        _messages.push_back(std::make_shared<history_message>(*msg->get_message()));
    }
}

void pending_operations::get_pending_file_sharing_messages(not_sent_messages_list& _messages) const
{
    for (const auto &pair : pending_messages_)
    {
        const auto &messages = pair.second;

        for (const auto& msg : messages)
        {
            if (msg->get_message()->is_file_sharing() && !msg->is_ready_to_send())
                _messages.push_back(msg);
        }
    }
}

not_sent_message_sptr pending_operations::get_first_ready_to_send() const
{
    for (const auto &pair : pending_messages_)
    {
        const auto &messages = pair.second;

        const auto found = std::find_if(
            messages.begin(),
            messages.end(),
            [](const not_sent_message_sptr &_message)
        {
            return _message->is_ready_to_send();
        }
        );

        if (found != messages.end())
        {
            std::stringstream log;
            log << "CORE: get pending for send\r\n";
            log << "reqId: " << (*found)->get_internal_id() << "\r\n";
            g_core->write_string_to_network_log(log.str());

            return *found;
        }
    }

    return not_sent_message_sptr();
}

not_sent_message_sptr pending_operations::get_message_by_internal_id(const std::string& _iid) const
{
    for (const auto &pair : pending_messages_)
    {
        const auto &messages = pair.second;

        const auto found = std::find_if(
            messages.begin(),
            messages.end(),
            [&_iid](const not_sent_message_sptr &_message)
        {
            return (_message->get_internal_id() == _iid);
        }
        );

        if (found != messages.end())
        {
            return *found;
        }
    }

    return not_sent_message_sptr();
}


int32_t pending_operations::insert_deleted_message(const std::string& _contact, delete_message _message)
{
    bool pending_changed = false;

    auto iter_contact = pending_messages_.find(_contact);

    if (iter_contact != pending_messages_.end())
    {
        auto& messages = iter_contact->second;

        for (auto iter_message =  messages.begin(); iter_message != messages.end();)
        {
            if (
                (_message.get_message_id() > 0 && _message.get_message_id() == (*iter_message)->get_message()->get_msgid()) ||
                (!_message.get_internal_id().empty() && _message.get_internal_id() == (*iter_message)->get_internal_id()))
            {
                (*iter_message)->set_deleted(true);
                (*iter_message)->set_delete_operation(_message.get_operation());

                pending_changed = true;
            }

            ++iter_message;
        }
    }

    if (pending_changed)
    {
        save_pending_sent_messages();
    }

    if (_message.get_message_id() > 0)
    {
        pending_delete_messages_[_contact].emplace_back(std::make_unique<delete_message>(std::move(_message)));

        save_pending_delete_messages();
    }

    return 0;
}

int32_t pending_operations::remove_deleted_message(const std::string& _contact, const delete_message& _message)
{
    int32_t res = -1;

    if (auto iter_contact = pending_delete_messages_.find(_contact);  iter_contact != pending_delete_messages_.end())
    {
        auto& messages = iter_contact->second;

        for (auto iter_msg = messages.begin(); iter_msg != messages.end();)
        {
            if (_message == *(*iter_msg))
            {
                res = 0;

                iter_msg = messages.erase(iter_msg);
                continue;
            }

            ++iter_msg;
        }

        if (messages.empty())
            pending_delete_messages_.erase(iter_contact);
    }

    save_pending_delete_messages();

    return res;
}

bool pending_operations::get_first_pending_delete_message(std::string& _contact, delete_message& _message) const
{
    if (pending_delete_messages_.empty())
        return false;

    const auto& contact_messages = *pending_delete_messages_.begin();

    if (contact_messages.second.empty())
        return false;

    _contact = contact_messages.first;
    _message = *(*(contact_messages.second.begin()));

    return true;
}

bool pending_operations::get_pending_delete_messages(std::string& _contact, std::vector<delete_message>& _messages) const
{
    if (pending_delete_messages_.empty())
        return false;

    const auto& contact_messages = *pending_delete_messages_.begin();

    if (contact_messages.second.empty())
        return false;

    _contact = contact_messages.first;
    for (const auto& msg : contact_messages.second)
        _messages.push_back(*msg);

    return true;
}

template <class container_type_>
bool pending_operations::save(storage& _storage, const container_type_& _pendings) const
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!_storage.open(mode))
    {
        return false;
    }

    core::tools::auto_scope lb([&_storage] { _storage.close(); });

    core::tools::binary_stream block_data;

    core::tools::tlvpack pack_root;

    for (const auto &pair : _pendings)
    {
        const auto &messages = pair.second;

        for (const auto &message : messages)
        {
            core::tools::tlvpack pack_message;
            message->serialize(pack_message);

            core::tools::binary_stream bs_message;
            pack_message.serialize(bs_message);

            pack_root.push_child(core::tools::tlv(0, bs_message));
        }
    }

    pack_root.serialize(block_data);

    int64_t offset = 0;
    return _storage.write_data_block(block_data, offset);
}

template <class container_type_, class entry_type_>
bool pending_operations::load(storage& _storage, container_type_& _pendings)
{
    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!_storage.open(mode))
    {
        return false;
    }

    core::tools::auto_scope lbs([&_storage] { _storage.close(); });

    core::tools::binary_stream bs_data;
    if (!_storage.read_data_block(-1, bs_data))
    {
        return false;
    }

    core::tools::tlvpack pack_root;
    if (!pack_root.unserialize(bs_data))
    {
        return false;
    }

    auto tlv_msg = pack_root.get_first();
    while (tlv_msg)
    {
        auto bs_message = tlv_msg->get_value<core::tools::binary_stream>();

        core::tools::tlvpack pack_message;

        if (pack_message.unserialize(bs_message))
        {
            auto msg = entry_type_::make(pack_message);
            if (msg)
            {
                _pendings[msg->get_aimid()].emplace_back(std::move(msg));
            }
        }

        tlv_msg = pack_root.get_next();
    }

    return true;
}

bool pending_operations::save_pending_sent_messages()
{
    return save(*pending_sent_storage_, pending_messages_);
}

bool pending_operations::load_pending_sent_messages()
{
    return load<std::map<std::string, not_sent_messages_list>, not_sent_message>(*pending_sent_storage_, pending_messages_);
}

bool pending_operations::save_pending_delete_messages()
{
    return save(*pending_delete_storage_, pending_delete_messages_);
}

bool pending_operations::load_pending_delete_messages()
{
    return load<std::map<std::string, delete_messages_list>, delete_message>(*pending_delete_storage_, pending_delete_messages_);
}


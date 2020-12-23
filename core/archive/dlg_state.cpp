#include "stdafx.h"

#include "../../corelib/core_face.h"
#include "../../corelib/collection_helper.h"

#include "../../core/core.h"
#include "tools/coretime.h"

#include "../log/log.h"

#include "history_message.h"
#include "storage.h"

#include "dlg_state.h"

#include "../../common.shared/patch_version.h"

using namespace core;
using namespace archive;

namespace
{
    constexpr std::chrono::seconds offline_timeout = std::chrono::minutes(30);
}

enum dlg_state_fields
{
    unreads_count = 1,
    last_msg_id = 2,
    yours_last_read = 3,
    theirs_last_read = 4,
    theirs_last_delivered = 5,
    last_message = 7,
    visible = 8,
    last_message_friendly = 9, // unused
    patch_version = 10,
    del_up_to = 11,
    friendly = 12,
    official = 13,
    fake = 14,
    hidden_msg_id = 15,
    unread_mentions_count = 16,
    pinned_message = 17,
    attention = 18,
    suspicious = 19,
    heads = 20,
    head_aimid = 21,
    head_friendly = 22,
    last_read_mention = 23,
    stranger = 24,
    info_version = 25,
    no_recents_update = 26,
    members_version = 27
};

dlg_state::dlg_state()
    : unread_count_(0)
    , unread_mentions_count_(0)
    , last_read_mention_(-1)
    , last_msgid_(-1)
    , yours_last_read_(-1)
    , del_up_to_(-1)
    , theirs_last_read_(-1)
    , theirs_last_delivered_(-1)
    , hidden_msg_id_(-1)
    , visible_(true)
    , fake_(false)
    , official_(false)
    , suspicious_(false)
    , attention_(false)
    , stranger_(false)
    , no_recents_update_(false)
    , last_message_(std::make_unique<history_message>())
    , pinned_message_(std::make_unique<history_message>())
{
}

dlg_state::~dlg_state()
{

}

dlg_state::dlg_state(const dlg_state& _state)
{
    last_message_ = std::make_unique<history_message>();
    pinned_message_ = std::make_unique<history_message>();
    copy_from(_state);
}

void dlg_state::copy_from(const dlg_state& _state)
{
    unread_count_ = _state.unread_count_;
    unread_mentions_count_ = _state.unread_mentions_count_;
    last_read_mention_ = _state.last_read_mention_;
    set_last_msgid(_state.get_last_msgid());
    yours_last_read_ = _state.yours_last_read_;
    theirs_last_read_ = _state.theirs_last_read_;
    theirs_last_delivered_ = _state.theirs_last_delivered_;
    visible_ = _state.visible_;
    fake_ = _state.fake_;
    *last_message_ = *_state.last_message_;
    *pinned_message_ = *_state.pinned_message_;
    dlg_state_patch_version_ = _state.dlg_state_patch_version_;
    history_patch_version_ = _state.history_patch_version_;
    hidden_msg_id_ = _state.hidden_msg_id_;
    del_up_to_ = _state.del_up_to_;
    friendly_ = _state.friendly_;
    official_ = _state.official_;
    suspicious_ = _state.suspicious_;
    attention_ = _state.attention_;
    stranger_ = _state.stranger_;
    no_recents_update_ = _state.no_recents_update_;
    heads_ = _state.heads_;
    info_version_ = _state.info_version_;
    members_version_ = _state.members_version_;
}

void dlg_state::set_last_msgid(const int64_t _value)
{
    assert(_value >= -1);

    last_msgid_ = (_value <= 0) ? -1 : _value;
}

dlg_state& dlg_state::operator=(const dlg_state& _state)
{
    copy_from(_state);

    return *this;
}

const history_message& dlg_state::get_last_message() const
{
    return *last_message_;
}

void dlg_state::set_last_message(const history_message& _message)
{
    assert(!_message.is_patch());

    *last_message_ = _message;
}

bool dlg_state::has_last_message() const
{
    assert(last_message_);

    return (last_message_->get_msgid() != -1);
}

void dlg_state::clear_last_message()
{
    last_message_ = std::make_unique<history_message>();
}

const history_message& dlg_state::get_pinned_message() const
{
    return *pinned_message_;
}

void dlg_state::set_pinned_message(const history_message& _message)
{
    assert(!_message.is_patch());

    *pinned_message_ = _message;
}

bool dlg_state::has_pinned_message() const
{
    return pinned_message_ && pinned_message_->has_msgid();
}

void dlg_state::clear_pinned_message()
{
    pinned_message_ = std::make_unique<history_message>();
}

const common::tools::patch_version& dlg_state::get_history_patch_version() const
{
    return history_patch_version_;
}

common::tools::patch_version dlg_state::get_history_patch_version(std::string_view _default) const
{
    if (has_history_patch_version())
    {
        return history_patch_version_;
    }

    return common::tools::patch_version(_default);
}

bool dlg_state::has_history_patch_version() const
{
    return !history_patch_version_.is_empty();
}

void dlg_state::set_history_patch_version(common::tools::patch_version _patch_version)
{
    assert(!_patch_version.is_empty());

    history_patch_version_ = std::move(_patch_version);
}

void dlg_state::set_history_patch_version(std::string_view _patch_version)
{
    assert(!_patch_version.empty());

    history_patch_version_ = common::tools::patch_version(_patch_version);
}

void dlg_state::reset_history_patch_version()
{
    history_patch_version_ = common::tools::patch_version();
}

const std::string& dlg_state::get_dlg_state_patch_version() const
{
    return dlg_state_patch_version_;
}

void dlg_state::set_dlg_state_patch_version(std::string _patch_version)
{
    dlg_state_patch_version_ = std::move(_patch_version);
}

int64_t dlg_state::get_del_up_to() const
{
    assert(del_up_to_ >= -1);

    return del_up_to_;
}

void dlg_state::set_del_up_to(const int64_t _msg_id)
{
    assert(_msg_id >= -1);

    del_up_to_ = _msg_id;
}

bool dlg_state::has_del_up_to() const
{
    return (del_up_to_ > 0);
}

bool dlg_state::is_empty() const
{
    return (!has_last_msgid() && !last_message_->has_msgid());
}

void dlg_state::set_unread_mentions_count(const int32_t _count)
{
    unread_mentions_count_ = _count;
}

void dlg_state::set_unread_mentions_count(std::optional<int32_t> _count)
{
    unread_mentions_count_ = _count;
}

int32_t dlg_state::get_unread_mentions_count() const
{
    return unread_mentions_count_.value_or(0);
}

bool dlg_state::has_unread_mentions_count() const
{
    return unread_mentions_count_.has_value();
}

void dlg_state::serialize(icollection* _collection, const time_t _offset, const time_t _last_successful_fetch, const bool _serialize_message) const
{
    coll_helper coll(_collection, false);

    coll.set<int64_t>("unreads", get_unread_count());
    coll.set<int32_t>("unread_mention_count", get_unread_mentions_count());
    coll.set<int64_t>("last_read_mention", get_last_read_mention());
    coll.set<int64_t>("last_msg_id", get_last_msgid());
    coll.set<int64_t>("yours_last_read", get_yours_last_read());
    coll.set<int64_t>("theirs_last_read", get_theirs_last_read());
    coll.set<int64_t>("theirs_last_delivered", get_theirs_last_delivered());
    coll.set<int64_t>("del_up_to", get_del_up_to());
    coll.set<std::string>("friendly", get_friendly());
    coll.set<bool>("serialize_message", _serialize_message);
    coll.set<bool>("official", get_official());
    coll.set<bool>("suspicious", get_suspicious());
    coll.set<bool>("attention", get_attention());
    coll.set<bool>("stranger", get_stranger());
    coll.set<bool>("no_recents_update", get_no_recents_update());

    if (info_version_)
        coll.set<std::string>("info_version", *info_version_);

    if (members_version_)
        coll.set<std::string>("members_version", *members_version_);

    auto cur_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + _offset;
    bool timed_out = cur_time - _last_successful_fetch >= offline_timeout.count();
    coll.set_value_as_bool("visible", get_visible() && !timed_out);

    coll_helper msg_coll(g_core->create_collection(), true);
    get_last_message().serialize(msg_coll.get(), _offset, _serialize_message);
    coll.set_value_as_collection("message", msg_coll.get());

    if (has_pinned_message())
    {
        coll_helper pin_coll(g_core->create_collection(), true);
        get_pinned_message().serialize(pin_coll.get(), _offset, true);
        coll.set_value_as_collection("pinned", pin_coll.get());
    }

    if (!heads_.empty())
    {
        ifptr<iarray> heads_array(coll->create_array());
        heads_array->reserve(heads_.size());

        int cnt = 0;
        for (const auto& _head : heads_)
        {
            if (++cnt > 3)
                break;

            ifptr<ivalue> friendly_value(coll->create_value());

            coll_helper coll_head(coll->create_collection(), true);

            coll_head.set_value_as_string("aimid", _head.first);

            friendly_value->set_as_collection(coll_head.get());

            heads_array->push_back(friendly_value.get());
        }

        coll.set_value_as_array("heads", heads_array.get());
    }
}

void dlg_state::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack state_pack;

    state_pack.push_child(core::tools::tlv(dlg_state_fields::unreads_count, get_unread_count()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::last_msg_id, get_last_msgid()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::yours_last_read, get_yours_last_read()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::theirs_last_read, get_theirs_last_read()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::theirs_last_delivered, get_theirs_last_delivered()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::visible, get_visible()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::friendly, get_friendly()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::official, get_official()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::suspicious, get_suspicious()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::fake, get_fake()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::hidden_msg_id, get_hidden_msg_id()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::unread_mentions_count, get_unread_mentions_count()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::attention, get_attention()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::last_read_mention, get_last_read_mention()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::stranger, get_stranger()));
    state_pack.push_child(core::tools::tlv(dlg_state_fields::no_recents_update, get_no_recents_update()));

    if (info_version_)
        state_pack.push_child(core::tools::tlv(dlg_state_fields::info_version, *info_version_));

    if (members_version_)
        state_pack.push_child(core::tools::tlv(dlg_state_fields::members_version, *members_version_));

    if (has_history_patch_version())
    {
        state_pack.push_child(core::tools::tlv(
            dlg_state_fields::patch_version,
            get_history_patch_version().as_string()
            ));
    }

    if (has_del_up_to())
    {
        state_pack.push_child(core::tools::tlv(
            dlg_state_fields::del_up_to,
            get_del_up_to()
            ));
    }

    core::tools::binary_stream bs_message;
    get_last_message().serialize(bs_message);
    state_pack.push_child(core::tools::tlv(dlg_state_fields::last_message, bs_message));

    if (has_pinned_message())
    {
        core::tools::binary_stream bs_pinned;
        get_pinned_message().serialize(bs_pinned);
        state_pack.push_child(core::tools::tlv(dlg_state_fields::pinned_message, bs_pinned));
    }

    if (!heads_.empty())
    {
        core::tools::tlvpack heads_pack;

        for (const auto& _head : heads_)
        {
            core::tools::tlvpack head_pack;

            head_pack.push_child(core::tools::tlv(dlg_state_fields::head_aimid, _head.first));
            head_pack.push_child(core::tools::tlv(dlg_state_fields::head_friendly, _head.second));

            heads_pack.push_child(core::tools::tlv(0, head_pack));
        }

        state_pack.push_child(core::tools::tlv(dlg_state_fields::heads, heads_pack));
    }

    state_pack.serialize(_data);
}

bool dlg_state::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack state_pack;
    if (!state_pack.unserialize(_data))
        return false;

    auto tlv_unreads_count = state_pack.get_item(dlg_state_fields::unreads_count);
    auto tlv_last_msg_id = state_pack.get_item(dlg_state_fields::last_msg_id);
    auto tlv_yours_last_read = state_pack.get_item(dlg_state_fields::yours_last_read);
    auto tlv_theirs_last_read = state_pack.get_item(dlg_state_fields::theirs_last_read);
    auto tlv_theirs_last_delivered = state_pack.get_item(dlg_state_fields::theirs_last_delivered);
    auto tlv_last_message = state_pack.get_item(dlg_state_fields::last_message);
    auto tlv_pinned_message = state_pack.get_item(dlg_state_fields::pinned_message);
    auto tlv_visible = state_pack.get_item(dlg_state_fields::visible);
    auto tlv_friendly = state_pack.get_item(dlg_state_fields::friendly);
    auto tlv_official = state_pack.get_item(dlg_state_fields::official);
    auto tlv_suspicious = state_pack.get_item(dlg_state_fields::suspicious);
    auto tlv_fake = state_pack.get_item(dlg_state_fields::fake);
    auto tlv_hidden_msg_id = state_pack.get_item(dlg_state_fields::hidden_msg_id);
    auto tlv_mention_me_count = state_pack.get_item(dlg_state_fields::unread_mentions_count);
    auto tlv_attention = state_pack.get_item(dlg_state_fields::attention);
    auto tlv_heads = state_pack.get_item(dlg_state_fields::heads);
    auto tlv_last_read_mention = state_pack.get_item(dlg_state_fields::last_read_mention);
    auto tlv_stranger = state_pack.get_item(dlg_state_fields::stranger);
    auto tlv_info_version = state_pack.get_item(dlg_state_fields::info_version);
    auto tlv_members_version = state_pack.get_item(dlg_state_fields::members_version);
    auto tlv_no_recents_update = state_pack.get_item(dlg_state_fields::no_recents_update);

    if (!tlv_unreads_count || !tlv_last_msg_id || !tlv_yours_last_read || !tlv_theirs_last_read ||
        !tlv_theirs_last_delivered || !tlv_last_message || !tlv_visible)
    {
        return false;
    }

    set_unread_count(tlv_unreads_count->get_value<uint32_t>(0));
    set_last_msgid(tlv_last_msg_id->get_value<int64_t>(0));
    set_yours_last_read(tlv_yours_last_read->get_value<int64_t>(0));
    set_theirs_last_read(tlv_theirs_last_read->get_value<int64_t>(0));
    set_theirs_last_delivered(tlv_theirs_last_delivered->get_value<int64_t>(0));
    set_visible(tlv_visible->get_value<bool>(true));

    if (tlv_info_version)
        set_info_version(tlv_info_version->get_value<std::string>());

    if (tlv_members_version)
        set_members_version(tlv_members_version->get_value<std::string>());

    if (tlv_last_read_mention)
        set_last_read_mention(tlv_last_read_mention->get_value<int64_t>(-1));

    if (tlv_friendly)
        set_friendly(tlv_friendly->get_value<std::string>());

    auto tlv_patch_version = state_pack.get_item(dlg_state_fields::patch_version);
    if (tlv_patch_version)
        set_history_patch_version(tlv_patch_version->get_value<std::string>());

    auto tlv_del_up_to = state_pack.get_item(dlg_state_fields::del_up_to);
    if (tlv_del_up_to)
        set_del_up_to(tlv_del_up_to->get_value<int64_t>());

    if (tlv_official)
        set_official(tlv_official->get_value<bool>());

    if (tlv_suspicious)
        set_suspicious(tlv_suspicious->get_value<bool>());

    if (tlv_fake)
        set_fake(tlv_fake->get_value<bool>());

    if (tlv_hidden_msg_id)
        set_hidden_msg_id(tlv_hidden_msg_id->get_value<int64_t>());

    if (tlv_mention_me_count)
        set_unread_mentions_count(tlv_mention_me_count->get_value<int32_t>(0));

    if (tlv_stranger)
        set_stranger(tlv_stranger->get_value<bool>());

    if (tlv_no_recents_update)
        set_no_recents_update(tlv_no_recents_update->get_value<bool>());

    core::tools::binary_stream bs_message = tlv_last_message->get_value<core::tools::binary_stream>();
    last_message_->unserialize(bs_message);

    if (tlv_pinned_message)
    {
        core::tools::binary_stream bs_pinned = tlv_pinned_message->get_value<core::tools::binary_stream>();
        pinned_message_->unserialize(bs_pinned);
    }

    if (tlv_attention)
        set_attention(tlv_attention->get_value<bool>());

    if (tlv_heads)
    {
        core::tools::tlvpack tlv_pack_heads;
        if (tlv_pack_heads.unserialize(tlv_heads->get_value<core::tools::binary_stream>()))
        {
            heads_.reserve(tlv_pack_heads.size());

            auto val = tlv_pack_heads.get_first();

            while (val)
            {
                core::tools::tlvpack tlv_pack_head;

                tlv_pack_head.unserialize(val->get_value<core::tools::binary_stream>());

                auto tlv_head_aimid = tlv_pack_head.get_item(dlg_state_fields::head_aimid);
                auto tlv_head_friendly = tlv_pack_head.get_item(dlg_state_fields::head_friendly);

                if (tlv_head_aimid && tlv_head_friendly)
                {
                    heads_.emplace_back(
                        tlv_head_aimid->get_value<std::string>(),
                        tlv_head_friendly->get_value<std::string>());
                }

                val = tlv_pack_heads.get_next();
            }
        }
    }

    return true;
}

dlg_state_changes::dlg_state_changes()
    : del_up_to_changed_(false)
    , history_patch_version_changed_(false)
    , initial_fill_(false)
    , last_message_changed_(false)
{
}

archive_state::archive_state(std::wstring _file_name, std::string _contact_id)
    : storage_(std::make_unique<storage>(std::move(_file_name)))
    , contact_id_(std::move(_contact_id))
{
    assert(!contact_id_.empty());
}

archive_state::~archive_state()
{

}

bool archive_state::save()
{
    if (!state_)
    {
        assert(!"dlg_state not intialized");
        return false;
    }

    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage_->open(mode))
        return false;

    __INFO(
        "delete_history",
        "serializing dialog state\n"
        "    contact=<%1%>\n"
        "    history-patch=<%2%>\n"
        "    del-up-to=<%3%>",
        contact_id_ % state_->get_history_patch_version().as_string() % state_->get_del_up_to());

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream block_data;
    state_->serialize(block_data);

    int64_t offset = 0;
    return storage_->write_data_block(block_data, offset);
}

bool archive_state::load()
{
    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage_->open(mode))
        return false;

    auto p_state_storage = storage_.get();
    core::tools::auto_scope lbs([p_state_storage]{p_state_storage->close();});

    core::tools::binary_stream state_stream;
    if (!storage_->read_data_block(-1, state_stream))
        return false;

    return state_->unserialize(state_stream);
}

void dlg_state::set_heads(std::vector<dlg_state_head>&& _heads)
{
    heads_ = std::move(_heads);
}

void dlg_state::set_heads(const std::vector<dlg_state_head>& _heads)
{
    heads_ = _heads;
}

const std::vector<dlg_state_head>& dlg_state::get_heads() const
{
    return heads_;
}






const dlg_state& archive_state::get_state()
{
    if (!state_)
    {
        state_ = std::make_unique<dlg_state>();

        load();

        __INFO(
            "delete_history",
            "dialog state deserialized\n"
            "    contact=<%1%>\n"
            "    history-patch=<%2%>\n"
            "    del-up-to=<%3%>",
            contact_id_ % state_->get_history_patch_version().as_string() % state_->get_del_up_to()
            );
    }

    return *state_;
}

bool archive_state::merge_state(const dlg_state& _new_state, Out dlg_state_changes& _changes)
{
    Out _changes.initial_fill_ = state_->is_empty();

    const auto update_history_patch = (
                                       _new_state.has_history_patch_version() &&
                                       (_new_state.get_history_patch_version() != state_->get_history_patch_version())
                                       );

    const auto patch_version_changed = (!_new_state.get_dlg_state_patch_version().empty() && state_->get_history_patch_version() < common::tools::patch_version(_new_state.get_dlg_state_patch_version()));


    if (!update_history_patch && !patch_version_changed && _new_state.has_last_msgid() && state_->has_last_msgid() && _new_state.get_last_msgid() < state_->get_last_msgid())
        return false;

    const auto update_del_up_to = (
        _new_state.get_del_up_to() > state_->get_del_up_to()
        );

    Out _changes.del_up_to_changed_ = update_del_up_to;
    if (update_del_up_to)
    {
        state_->set_del_up_to(_new_state.get_del_up_to());
    }

    if (state_->has_last_msgid() && update_del_up_to && state_->get_last_msgid() == _new_state.get_del_up_to())
    {
        state_->set_unread_count(0);
        state_->set_attention(false);
        auto message = state_->get_last_message();
        message.merge(archive::history_message());
        message.set_outgoing(false);
        time_t now = time(nullptr);
        tm now_tm = { 0 };
        if (tools::time::localtime(&now, &now_tm))
            message.set_time(mktime(&now_tm));
        state_->set_last_message(message);
        Out _changes.last_message_changed_ = true;
        return true;
    }

    state_->set_last_msgid(_new_state.get_last_msgid());
    state_->set_visible(_new_state.get_visible());
    state_->set_fake(_new_state.get_fake());
    state_->set_official(_new_state.get_official());
    state_->set_suspicious(_new_state.get_suspicious());
    state_->set_attention(_new_state.get_attention());
    state_->set_theirs_last_read(_new_state.get_theirs_last_read());
    state_->set_theirs_last_delivered(_new_state.get_theirs_last_delivered());
    state_->set_last_read_mention(_new_state.get_last_read_mention());
    state_->set_stranger(_new_state.get_stranger());
    state_->set_no_recents_update(_new_state.get_no_recents_update());

    if (_new_state.has_info_version())
        state_->set_info_version(_new_state.get_info_version());

    if (_new_state.has_members_version())
        state_->set_members_version(_new_state.get_members_version());

    if (_new_state.get_last_read_mention() == _new_state.get_last_msgid())
        state_->set_unread_mentions_count(0);
    else if (_new_state.has_unread_mentions_count())
        state_->set_unread_mentions_count(_new_state.get_unread_mentions_count());

    state_->set_pinned_message(_new_state.get_pinned_message());

    auto hidden_msg_id = _new_state.get_hidden_msg_id();
    if (hidden_msg_id != -1)
        state_->set_hidden_msg_id(hidden_msg_id);

    if (!_new_state.get_friendly().empty())
        state_->set_friendly(_new_state.get_friendly());

    if (_new_state.get_yours_last_read() == _new_state.get_last_msgid())
        state_->set_unread_count(0);
    else if (_new_state.has_unread_count())
        state_->set_unread_count(_new_state.get_unread_count());

    state_->set_yours_last_read(_new_state.get_yours_last_read());

    const auto &new_last_message = _new_state.get_last_message();

    const auto &this_last_message = state_->get_last_message();

    const auto new_has_last_message = (new_last_message.has_msgid() || new_last_message.has_internal_id());
    const auto new_last_message_emptied = (!new_last_message.has_msgid() && !_new_state.has_last_msgid());
    const auto update_last_message = (new_has_last_message || new_last_message_emptied);
    if (update_last_message)
    {
        const auto last_message_changed = (
            (this_last_message.get_msgid() != new_last_message.get_msgid()) ||
            !this_last_message.contents_equal(new_last_message)
        );

        Out _changes.last_message_changed_= last_message_changed;
    }

    Out _changes.history_patch_version_changed_ = update_history_patch;

    __INFO(
        "delete_history",
        "merging history patch/del version\n"
        "    contact=<%1%>\n"
        "    patch-updated=<%2%>\n"
        "    patch-before=<%3%>\n"
        "    patch-after=<%4%>\n"
        "    del-up-to-updated=<%5%>\n"
        "    del-up-to-before=<%6%>\n"
        "    del-up-to-after=<%7%>\n"
        "    update-last-message=<%8%>\n"
        "    last-message-changed=<%9%>\n"
        "    last-message-msgid=<%10%>\n"
        "    their-last-message-msgid=<%11%>",
        contact_id_ %
        logutils::yn(update_history_patch) %
        state_->get_history_patch_version().as_string() %
        _new_state.get_history_patch_version().as_string() %
        logutils::yn(update_del_up_to) %
        state_->get_del_up_to() %
        _new_state.get_del_up_to() %
        logutils::yn(update_last_message) %
        logutils::yn(_changes.last_message_changed_) %
        this_last_message.get_msgid() %
        new_last_message.get_msgid());

    if (update_last_message)
    {
        state_->set_last_message(new_last_message);
    }

    if (update_history_patch)
    {
        state_->set_history_patch_version(_new_state.get_history_patch_version());
    }

    state_->set_heads(_new_state.get_heads());
    return true;
}

void archive_state::set_state(const dlg_state& _state, Out archive::dlg_state_changes& _changes)
{
    if (!state_)
        state_ = std::make_unique<dlg_state>();

    if (merge_state(_state, Out _changes))
        save();
}

void archive_state::update_attention_attribute(const bool _value)
{
    if (state_)
        state_->set_attention(_value);

    save();
}

void archive_state::clear_state()
{
    if (state_)
        *state_ = dlg_state();

    save();
}

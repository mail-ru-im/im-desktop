#include "stdafx.h"

#include "../../archive/history_message.h"
#include "../../archive/history_patch.h"
#include "../../archive/dlg_state.h"
#include "../../tools/json_helper.h"

#include "../../log/log.h"

#include "wim_history.h"


using namespace core;
using namespace wim;

namespace
{
    void apply_persons(InOut archive::history_message_sptr &_message, const archive::persons_map &_persons)
    {
        assert(_message);

        if (!_persons.empty())
        {
            _message->set_sender_friendly(_persons.begin()->second.friendly_);
        }

        auto chat_info = _message->get_chat_data();
        if (chat_info)
        {
            chat_info->apply_persons(_persons);
        }

        auto &chat_event = _message->get_chat_event_data();
        if (chat_event)
        {
            chat_event->apply_persons(_persons);
        }

        auto &voip = _message->get_voip_data();
        if (voip)
        {
            voip->apply_persons(_persons);
        }

        _message->apply_persons_to_quotes(_persons);
        _message->apply_persons_to_mentions(_persons);
    }
}

bool core::wim::parse_history_messages_json(
    const rapidjson::Value &_node,
    const int64_t _older_msg_id,
    const std::string &_sender_aimid,
    Out archive::history_block &_block,
    const archive::persons_map& _persons,
    message_order _order,
    const char* _node_member /* = "messages" */)
{
    assert(!_sender_aimid.empty());

    const auto iter_messages = _node.FindMember(_node_member);
    if (iter_messages == _node.MemberEnd())
        return true;

    if (!iter_messages->value.IsArray())
        return false;

    if (iter_messages->value.Empty())
        return true;

    auto prev_msg_id = _older_msg_id;

    _block.reserve(iter_messages->value.Size());

    const bool is_reverse = message_order::reverse == _order;

    auto iter_message = is_reverse ? iter_messages->value.End() : iter_messages->value.Begin();
    const auto end = is_reverse ? iter_messages->value.Begin() : iter_messages->value.End();
    do
    {
        if (is_reverse)
            --iter_message;

        auto msg = std::make_shared<archive::history_message>();

        if (0 != msg->unserialize(*iter_message, _sender_aimid))
        {
            assert(!"parse message error");
        }
        else
        {
            assert(!msg->is_patch());

            const auto is_same_as_prev = (prev_msg_id == msg->get_msgid());
            assert(!is_same_as_prev);

            if (is_same_as_prev)
            {
                // workaround for the server issue

                __INFO(
                    "delete_history",
                    "server issue detected, message skipped\n"
                    "    older_msg_id=<%1%>\n"
                    "    prev_msg_id=<%2%>\n"
                    "    msg_id=<%3%>",
                    _older_msg_id % prev_msg_id % msg->get_msgid()
                );

                continue;
            }

            msg->set_prev_msgid(prev_msg_id);
            _block.push_back(msg);

            prev_msg_id = msg->get_msgid();
        }

        apply_persons(msg, _persons);
        if (!is_reverse)
            ++iter_message;
    }
    while (iter_message != end);

    return true;
}

patch_container core::wim::parse_patches_json(const rapidjson::Value& _node_patch)
{
    patch_container result;

    if (!_node_patch.IsArray())
    {
        assert(!"unexpected patches node format");
        return result;
    }

    if (_node_patch.Empty())
        return result;

    std::map<int64_t, archive::history_patch_uptr> content_related_pathes;
    std::map<int64_t, archive::history_patch_uptr> pin_related_pathes;

    for (const auto &patch_node : _node_patch.GetArray())
    {
        const auto iter_msg_id = patch_node.FindMember("msgId");
        const auto iter_type = patch_node.FindMember("type");

        if (iter_msg_id == patch_node.MemberEnd() || iter_type == patch_node.MemberEnd())
        {
            assert(!"unexpected patch node format");
            continue;
        }

        const auto msg_id = iter_msg_id->value.GetInt64();
        assert(msg_id > 0);

        const auto type = rapidjson_get_string_view(iter_type->value);
        assert(!type.empty());

        __INFO(
            "delete_history",
            "parsed patch\n"
            "    type=<%1%>\n"
            "    msg_id=<%2%>",
            type % msg_id
        );

        using namespace archive;

        auto patch_type = history_patch::type::min;

        if (type == "delete")
            patch_type = history_patch::type::deleted;
        else if (type == "modify")
            patch_type = history_patch::type::modified;
        else if (type == "update")
            patch_type = history_patch::type::updated;
        else if (type == "pinned")
            patch_type = history_patch::type::pinned;
        else if (type == "unpinned")
            patch_type = history_patch::type::unpinned;
        else if (type == "clear")
            patch_type = history_patch::type::clear;

        assert(history_patch::is_valid_type(patch_type));

        if (history_patch::is_valid_type(patch_type))
        {
            __INFO(
                "delete_history",
                "emplaced patch\n"
                "    type=<%1%>\n"
                "    msg_id=<%2%>",
                type % msg_id
            );

            if (patch_type == history_patch::type::unpinned || patch_type == history_patch::type::pinned)
            {
                pin_related_pathes[msg_id] = history_patch::make(patch_type, msg_id);
            }
            else
            {
                if (const auto it = content_related_pathes.find(msg_id); it != content_related_pathes.end())
                {
                    if (it->second->get_type() != history_patch::type::deleted) // don't override 'deleted' patch
                        it->second = history_patch::make(patch_type, msg_id);
                }
                else
                {
                    content_related_pathes[msg_id] = history_patch::make(patch_type, msg_id);
                }
            }
        }
    }
    result.reserve(pin_related_pathes.size() + content_related_pathes.size());
    std::move(content_related_pathes.begin(), content_related_pathes.end(), std::back_inserter(result));
    std::move(pin_related_pathes.begin(), pin_related_pathes.end(), std::back_inserter(result));
    return result;
}

void core::wim::apply_patches(const std::vector<std::pair<int64_t, archive::history_patch_uptr>>& _patches, InOut archive::history_block& _block, Out bool& _unpinned, Out archive::dlg_state& _dlg_state)
{
    using namespace archive;

    for (const auto& [message_id, patch] : _patches)
    {
        switch (patch->get_type())
        {
        case history_patch::type::deleted:
             _block.emplace_back(history_message::make_deleted_patch(message_id, std::string()));
            break;
        case history_patch::type::modified:
            _block.emplace_back(history_message::make_modified_patch(message_id, std::string()));
            break;
        case history_patch::type::updated:
            _block.emplace_back(history_message::make_updated_patch(message_id, std::string()));
            break;
        case history_patch::type::pinned:
            break;
        case history_patch::type::unpinned:
            if (_dlg_state.has_pinned_message() && _dlg_state.get_pinned_message().get_msgid() == message_id)
                _dlg_state.clear_pinned_message();
            _unpinned = true;
            break;
        case history_patch::type::clear:
            _block.emplace_back(history_message::make_clear_patch(message_id, std::string()));
            break;
        default:
            break;
        }
    }
}

void core::wim::set_last_message(const archive::history_block& _block, InOut archive::dlg_state& _dlg_state)
{
    if (_block.empty())
        return;

    if (!_dlg_state.has_last_msgid())
        return;

    const auto last_msgid = _dlg_state.get_last_msgid();
    assert(last_msgid > 0);

    for (const auto& message_ptr : boost::adaptors::reverse(_block))
    {
        const auto msg_id = message_ptr->get_msgid();
        assert(msg_id > 0);

        if (last_msgid != msg_id)
        {
            continue;
        }

        _dlg_state.set_last_message(*message_ptr);

        break;
    }
}

std::vector<core::archive::dlg_state_head> core::wim::parse_heads(const rapidjson::Value& _node_heads, const archive::persons_map& _persons)
{
    std::vector<archive::dlg_state_head> heads;
    heads.reserve(_node_heads.Size());

    for (const auto& pos : _node_heads.GetArray())
    {
        const auto iter_aimid = pos.FindMember("sn");
        if (iter_aimid == pos.MemberEnd() || !iter_aimid->value.IsString())
        {
            assert(false);
            continue;
        }

        std::string aimid = rapidjson_get_string(iter_aimid->value);
        const auto iter_persons = _persons.find(aimid);

        heads.emplace_back(
            std::move(aimid), (iter_persons == _persons.end() ? std::string() : iter_persons->second.friendly_));
    }
    return heads;
}

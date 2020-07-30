#ifndef __WIM_HISTORY_H_
#define __WIM_HISTORY_H_

#pragma once

#include "persons.h"

namespace core
{
    enum class message_order
    {
        direct,
        reverse
    };

    namespace archive
    {
        class history_message;
        class dlg_state;
        class history_patch;

        using history_patch_uptr = std::unique_ptr<history_patch>;
        using dlg_state_head = std::pair<std::string, std::string>;
    }

    namespace wim
    {
        enum class message_order
        {
            direct,
            reverse
        };

        bool parse_history_messages_json(
            const rapidjson::Value &_node,
            const int64_t _older_msg_id,
            const std::string &_sender_aimid,
            Out archive::history_block &_block,
            const archive::persons_map& _persons,
            message_order _order,
            const char* _node_member = "messages");

        using patch_container = std::vector<std::pair<int64_t, archive::history_patch_uptr>>;

        patch_container parse_patches_json(const rapidjson::Value& _node_patch);
        void apply_patches(const std::vector<std::pair<int64_t, archive::history_patch_uptr>>& _patches, InOut archive::history_block& _block, Out bool& _unpinned, Out archive::dlg_state& _dlg_state);
        void set_last_message(const archive::history_block& _block, InOut archive::dlg_state& _dlg_state);

        std::vector<archive::dlg_state_head> parse_heads(const rapidjson::Value& _node_heads, const archive::persons_map& _persons);
    }
}
#endif //__WIM_HISTORY_H_

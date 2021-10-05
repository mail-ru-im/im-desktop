#pragma once

#include "history_message.h"

namespace core::archive
{
    struct draft
    {
        enum class state { local, syncing, synced };

        state state_ = state::local;
        int32_t timestamp_ = 0;
        int64_t local_timestamp_ = 0;
        history_message_sptr message_;
        std::string friendly_;

        bool empty() const;

        void serialize(coll_helper& _coll, time_t _time_offset) const;
        void serialize(core::tools::binary_stream& _data) const;
        bool unserialize(const core::tools::binary_stream& _data);
        bool unserialize(const rapidjson::Value& _node, const std::string& _contact);
    };

    class draft_storage
    {
    public:
        draft_storage(std::wstring _data_path);

        const draft& get_draft() noexcept;
        void set_draft(const draft& _draft);

    private:
        void load();
        void write_file();

        draft draft_;
        std::wstring path_;
    };
}


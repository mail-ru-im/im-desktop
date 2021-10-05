#pragma once

#include "storage.h"

namespace core::archive
{
    class pending_drafts
    {
    public:
        pending_drafts(std::wstring _data_path);

        std::string get_next_pending_draft_contact() const;
        void remove_pending_draft_contact(const std::string& _contact, int64_t _timestamp);
        void add_pending_draft_contact(const std::string& _contact, int64_t _timestamp);

    private:
        void load();
        void write_file();

        std::vector<std::pair<int64_t, std::string>> prev_pending_draft_contacts_;
        std::unordered_map<std::string, int64_t> current_pending_draft_contacts_;
        std::unique_ptr<storage> storage_;
    };
}


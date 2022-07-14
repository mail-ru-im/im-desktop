#pragma once

#include "fetch_event.h"
#include "../../../tools/file_sharing.h"
#include "../../../../common.shared/antivirus/antivirus_types.h"

namespace core::wim
{
    class fetch_event_antivirus : public fetch_event
    {
        core::antivirus::check::result result_;
        core::tools::filesharing_id file_hash_;

    public:
        virtual ~fetch_event_antivirus() = default;

        virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
        virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

        const core::tools::filesharing_id& get_file_hash() const noexcept { return file_hash_; }
        core::antivirus::check::result get_check_result() const noexcept { return result_; }
    };
}

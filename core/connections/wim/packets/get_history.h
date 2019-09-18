#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace archive
    {
        class history_message;
        class dlg_state;
        class history_patch;

        typedef std::unique_ptr<history_patch> history_patch_uptr;
    }

    namespace wim
    {
        struct get_history_params
        {
            const std::string aimid_;
            const int64_t till_msg_id_;
            const int64_t from_msg_id_;
            const int64_t seq_;
            const int32_t count_;
            const std::string patch_version_;
            bool init_;
            bool from_deleted_;
            bool from_editing_;
            bool from_search_;

            get_history_params(
                const std::string &_aimid,
                const int64_t _from_msg_id,
                const int64_t _till_msg_id,
                const int32_t _count,
                const std::string &_patch_version,
                bool _init = false,
                bool _from_deleted = false,
                bool _from_editing = false,
                bool _from_search = false,
                int64_t _seq = 0
                );
        };


        class get_history : public robusto_packet, public persons_packet
        {
            int64_t older_msgid_;

            get_history_params hist_params_;

            std::shared_ptr<archive::history_block> messages_;
            std::shared_ptr<archive::dlg_state> dlg_state_;
            std::vector<std::pair<int64_t, archive::history_patch_uptr>> history_patches_;
            std::string patch_version_;
            std::string locale_;
            bool unpinned_;
            std::shared_ptr<core::archive::persons_map> persons_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

        public:
            get_history(
                wim_packet_params _params,
                const get_history_params& _hist_params,
                const std::string& _locale
                );

            virtual ~get_history();

            std::shared_ptr<archive::history_block> get_messages() const { return messages_; }
            std::shared_ptr<archive::dlg_state> get_dlg_state() const { return dlg_state_; }
            bool get_unpinned() const noexcept { return unpinned_; }
            const get_history_params& get_hist_params() const { return hist_params_; }
            bool has_older_msgid() const noexcept { return older_msgid_ != -1; }
            int64_t get_older_msgid() const { return older_msgid_; }
            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
        };
    }
}

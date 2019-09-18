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

        using history_block = std::vector<std::shared_ptr<history_message>>;

        using history_patch_uptr = std::unique_ptr<history_patch>;
    }

    namespace wim
    {
        struct get_history_batch_params
        {
            const std::string aimid_;
            const std::vector<int64_t> ids_;
            const std::string patch_version_;

            const int32_t count_early_;
            const int32_t count_after_;

            const int64_t seq_;
            const bool is_context_request_;

            get_history_batch_params(const std::string_view _aimid, std::vector<int64_t>&& _ids, const std::string_view _patch_version);
            get_history_batch_params(const std::string_view _aimid, const std::string_view _patch_version, const int64_t _id, const int32_t _count_early, const int32_t _count_after, int64_t _seq = 0);

            bool is_message_context_params() const noexcept { return is_context_request_; }
        };


        class get_history_batch : public robusto_packet, public persons_packet
        {
            get_history_batch_params hist_params_;

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
            get_history_batch(
                wim_packet_params _params,
                get_history_batch_params&& _hist_params,
                std::string&& _locale
                );

            virtual ~get_history_batch();

            std::shared_ptr<archive::history_block> get_messages() const { return messages_; }
            std::shared_ptr<archive::dlg_state> get_dlg_state() const { return dlg_state_; }
            bool get_unpinned() const noexcept { return unpinned_; }
            const get_history_batch_params& get_hist_params() const { return hist_params_; }
            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
        };
    }
}

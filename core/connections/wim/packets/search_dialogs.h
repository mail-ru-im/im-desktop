#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"

#include "../../../archive/search_dialog_result.h"

namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class search_dialogs : public robusto_packet, public persons_packet
    {
    public:
        search_dialogs(wim_packet_params _packet_params, const std::string_view _keyword, const std::string_view _contact, const std::string_view _cursor = std::string_view());

        const std::shared_ptr<core::archive::persons_map>& get_persons() const override;

        search::search_dialog_results results_;

        void set_dates_range(const std::string_view _start_date, const std::string_view _end_date);
        void reset_dates();

        bool support_async_execution() const override { return true; }

    private:
        int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

        bool parse_entries(const rapidjson::Value& _root_node, const std::string_view _contact, const std::string_view _cursor = std::string_view(), const int _entry_count = -1);
        bool parse_single_entry(const rapidjson::Value& _entry_node, const std::string_view _contact, const std::string_view _cursor, const int _entry_count);

        std::string get_method_name() const;
        bool is_searching_all_dialogs() const;

        std::string keyword_;
        std::string contact_;
        std::string cursor_;

        std::string start_date_;
        std::string end_date_;

        std::shared_ptr<core::archive::persons_map> persons_;
    };
}

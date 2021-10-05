#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"

#include "archive/thread_parent_topic.h"

namespace core::wim
{
    class thread_add : public robusto_packet, public persons_packet
    {
    public:
        thread_add(wim_packet_params _params, archive::thread_parent_topic _parent_topic);
        std::string_view get_method() const override;

        const std::string& get_thread_id() const { return thread_id_; }
        const archive::thread_parent_topic& get_parent_topic() const { return topic_; }
        const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

    private:
        std::string thread_id_;
        archive::thread_parent_topic topic_;
        std::shared_ptr<core::archive::persons_map> persons_;
    };
}
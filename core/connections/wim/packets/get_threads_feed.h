#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "archive/thread_parent_topic.h"
#include "archive/history_message.h"
#include "archive/thread_update.h"

namespace core::wim
{

struct thread_feed_data
{
    archive::history_message parent_message_;
    archive::history_block messages_;
    archive::thread_parent_topic parent_topic_;
    std::string thread_id_;
    int64_t older_msg_id_ = 0;
    int64_t replies_count_ = 0;
    int64_t unread_count_ = 0;

    void unserialize(const rapidjson::Value& _thread_node, const archive::persons_map& _persons);
    void serialize(coll_helper& _coll, int _time_offset) const;
};

class get_threads_feed : public robusto_packet, public persons_packet
{
public:
    get_threads_feed(wim_packet_params _params, const std::string& _cursor);

    std::string_view get_method() const override;
    bool auto_resend_on_fail() const override { return true; }
    const std::vector<thread_feed_data>& get_data() const;
    const std::string& get_cursor() const;
    const std::vector<archive::thread_update>& get_updates() const;
    bool get_reset_page() const;
    const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    std::string cursor_;
    std::vector<thread_feed_data> data_;
    std::vector<archive::thread_update> updates_;
    std::shared_ptr<core::archive::persons_map> persons_;
    bool reset_page_ = false;
};

}


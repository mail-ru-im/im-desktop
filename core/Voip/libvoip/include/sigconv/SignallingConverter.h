#pragma once

#include <voip/voip3.h>
#include <string>
#include <list>

namespace voip {

class WIMSignallingConverter {
public:
    bool    PackAllocRequest(const std::string &user_id,
                                   int         &wim_msg_id,
                                   std::string &out_wim_msg );

    bool    PackSignalingMsg(const voip::SignalingMsg &msg,
                             const std::string        &user_id,
                                   int                &wim_msg_id,
                                   std::string        &out_wim_msg,
                                   bool enableVoip2Compatibility = true);


    bool    IsAllocMessageId(int wim_msg_id);

    bool    UnpackAllocResponse(      int         wim_msg_id,
                                const std::string wim_msg,
                                      std::string &call_guid,
                                      std::string &ice_params,
                                      std::string &peer_id);

    using MessagePair = std::pair<std::string /*from*/, voip::SignalingMsg /*msg*/>;

    bool    UnpackSignalingMsg(       int                     wim_msg_id,
                                const std::string             wim_msg,
                                      std::list< MessagePair> &msg_list_out,
                                      bool enableVoip2Compatibility = true);

    explicit WIMSignallingConverter(int out_alloc_msg_id, int out_signall_msg_id, int in_alloc_msg_id, int in_signal_msg_id);

    ~WIMSignallingConverter() = default;

private:
    int _outgoing_alloc_msg_id;
    int _outgoing_signal_msg_id;
    int _incoming_alloc_msg_id;
    int _incoming_signal_msg_id;
};

}
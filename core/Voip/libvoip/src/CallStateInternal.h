#pragma once
#include <voip/voip3.h>

namespace voip {


// CallStateInternal contains all info about the call that should be shared between modules.
// This struct intended to be easy-copyable, it should not contain pointers or other not-owned objects.
    
struct CallStateInternal
{
    struct CallAllocationState
    {
        bool            alloc_sent = false;
        bool            alloc_received = false;
        std::string     ice_params;
    };

    enum InvitingState {
        INIT,           // only valid for just-created calls (or after allocate is complete)
        ALLOCATING,     // for outgoing calls: waiting for app to provide ICE config, etc. 
                        // After alloc is complete, call switched to INIT state again.
        CALLING,        // invite sent or received
        ACCEPTED,       // locally/remotely accepted. Call is active (but maybe not yet established. See MediaStatus.connected)
        TERMINATED
    };

    struct MediaStatus
    {
        bool                connected = false;  // Media is connected
        MediaSenderState    sender_state;       // Configured media sender state

        std::string         ToPackedString() const;     // adds "C" to sender_state.ToPackedString() if connected
        //MediaSenderState    actual_state;     // Actual sender state;
    };

    struct PeerStatus
    {
        enum RoomState {
            P2P_ONLY,           // peer is not in room, it is P2P connection with him
            PENDING_INVITE,     // peer is invited by us, but we haven't received confirmation from room (e.g. we are not connected to the room yet)
            INVITING,           // Room is inviting him (don't depend on who exactly invited him)
            JOINING,            // He is connecting to room
            JOINED,             // He belongs to the room
            HANGUP,             // He leaved the room. Reason is |hangup_reason|
        };

        PeerId              peerId;
        std::string         instance_id;                // Only valid when it is known (e.g. outgoing call was accepted)
        MediaStatus         media_status;               // actual_state is equal to sender_state
        RoomState           room_state = P2P_ONLY;
        TerminateReason     hangup_reason = TR_HANGUP;  // only valid for room_state==HANGUP
        bool                ringing_received = false;   // Only valid for outgoing P2P calls.
        std::string         zrtp_sas;                   // ZRTP 'emoji' check string (UTF-8). Empty if not received yet or peer doesn't support ZRTP.
    };



    CallId                      id;                     // Local call id
    std::string                 call_guid;              // Empty, if not known
    bool                        is_outgoing = false;    // Direction of a call
    InvitingState               inviting_state = INIT;  // Call state
    TerminateReason             terminate_reason = TR_HANGUP; // Valid when inviting_state==TERMINATED
    bool                        terminated_locally = true;    // Valid when inviting_state==TERMINATED
    CallAllocationState         alloc_state;            // Contains ice_params.
    MediaStatus                 local_media_status;     // Our media status
    bool                        is_conference = false;  
    PeerId                      room_peer_id;           // <guid>@focus. only valid for conf call mode
    bool                        joined_to_room = false; // we've received successful ROOM_JOIN_ACK

    const PeerStatus           &p2p_peer() const;
    
    PeerStatus                 &p2p_peer() { return const_cast<PeerStatus &>(static_cast<const CallStateInternal &>(*this).p2p_peer()); }

    const std::vector<PeerStatus>   &peers() const {
        return _peers;
    }

    std::vector<PeerStatus>         &peers() { return const_cast<std::vector<PeerStatus> &>(static_cast<const CallStateInternal &>(*this).peers()); }

    std::vector<PeerStatus>::const_iterator     find_peer(const PeerId &peer_id) const;
    std::vector<PeerStatus>::iterator           find_peer(const PeerId &peer_id);

    std::string     ToString() const;
public:
    bool            is_on_hold() const  { 
        return inviting_state == ACCEPTED && (local_media_status.sender_state.onhold || (!is_conference && p2p_peer().media_status.sender_state.onhold));
    }

private:
    mutable std::vector<PeerStatus>     _peers;
};


std::string  InvitingStateToString(CallStateInternal::InvitingState     state);
std::string  RoomStateToString(CallStateInternal::PeerStatus::RoomState     state);
std::string  PeerStatusToString(const CallStateInternal::PeerStatus &peer);

} // namespace voip
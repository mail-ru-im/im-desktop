#pragma once
#include <string>
#include <vector>
#include <memory>

namespace voip {

// PeerId is unique identifier of a peer.
using PeerId = std::string;
// There are special peer ids:
const PeerId    kFocusId = { "@focus" };                    // FOCUS (group call server id).


// CallId is local call id. It is NOT equal to SignalingMsg::call_guid, because this ID may be unknown at the time of call creation
using CallId = std::string;

// Arbitrary string, that can hold app-specific data that needs to be transferred to peer during invite/accept phase.
// App _should_ use JSON to format this string, but it is not required
using CallAppData = std::string;

struct SignalingMsg {
    enum Type {
        INVITE = 0,
        RINGING,
        ACCEPT,
        DECLINE,
        TRANSPORT_MSG,
        INVALID = -1
    };

    Type            type = INVALID;     // required
    std::string     subtype;            // optional, type-specific
    std::string     call_guid;          // required. Note: this is NOT CallId
    std::string     payload;            // optional. stringitized JSON

    //helper functions
    static std::string  TypeToString(SignalingMsg::Type type);
    std::string         ToString() const;
};

enum TerminateReason {
    TR_HANGUP = 0,          // Use Call_Terminate(TR_HANGUP) to hangup active call (after it was accepted).
                            // OnCallTerminated(TR_HANGUP) signaled when remote/local side hang up the call.

    TR_REJECT,              // Use Call_Terminate(TR_REJECT) to reject incoming call.
                            // OnCallTerminated(TR_REJECT) signaled when remote/local side rejects the call.

    TR_BUSY,                // Use Call_Terminate(TR_BUSY) to terminate call by BUSY reason (e.g. GSM call is active)
                            // OnCallTerminated(TR_BUSY) signaled when local/remote side terminated the call using BUSY
    TR_HANDLED_BY_ANOTHER_INSTANCE, // Call was handled by another device of logged-in user.
                                    // OnCallTerminated(TR_HANDLED_BY_ANOTHER_INSTANCE) is signaled when another device accepts the call.
                                    // Note: this status requires signaling server's support.

    TR_UNAUTHORIZED,        // Call was rejected due to security errors

    TR_ALLOCATE_FAILED,     // OnCallTerminated(TR_ALLOCATE_FAILED) signaled if app didn't provide ALLOCATE data or it is invalid.

    TR_ANSWER_TIMEOUT,      // OnCallTerminated(TR_ANSWER_TIMEOUT) signaled when:
                            //      - remote side hasn't answered (accepted or rejected) for outgoing call
                            //      - local user hasn't answered (so neither AcceptIncomingCall nor TerminateCall were called)

    TR_CONNECT_TIMEOUT,     // OnCallTerminated(TR_CONNECT_TIMEOUT) signaled when connection cannot be established or lost

    TR_INTERNAL_ERROR       // OnCallTerminated(TR_INTERNAL_ERROR) signaled on internal error (todo: more info about that)
};

// Helper functions
namespace util {
    std::string TerminateReasonToString(TerminateReason tr);
    std::string TerminateReasonToString(bool terminated_locally, TerminateReason tr);
}

enum SoundEvent {
    OutgoingStarted,
    WaitingForAccept,
    WaitingForAccept_Confirmed,  // ringing
    IncomingInvite,
    Connected,
    Connecting,
    Reconnecting,
    Hold,

    HangupLocal,
    HangupRemote,
    HangupRejectLocal,
    HangupRejectRemote,
    HangupLocalBusy,
    HangupRemoteBusy,
    HangupUnauthorized,
    HangupHandledByAnotherInstance, // silent
    HangupTimeout,
    HangupLocalAnswerTimeout,
    HangupRemoteAnswerTimeout,
    HangupByError,

    SoundEvent_Max,
};




struct MediaSenderState {
    bool  sending_audio = false;
    bool  sending_video = false;
    bool  sending_desktop = false;
    bool  onhold = false;

    bool  supports_audio = false;
    bool  supports_video = false;
    bool  supports_desktop = false;

    bool operator == (const MediaSenderState& to) const;
    bool operator != (const MediaSenderState& to) const { return !operator==(to); }
    //helper functions
    std::string  ToPackedString() const;      // [A|a|][V|v|][D|d|][H|]
                                              // A - sending_audio & supports_audio
                                              // a - supports_audio
                                              // V - sending_video & supports_video
                                              // v - supports_video
                                              // D - sending_desktop & supports_desktop
                                              // d - supports_desktop
                                              // H - onhold
                                              // example: "Av" - voice only, but video is supported
    static MediaSenderState FromPackedString(const std::string &packed_mss);
};

struct CallParticipantInfo
{
    enum State {
        UNKNOWN = 0,      // Used in incoming call info, we are unaware of current state of participants
        INVITING,
        CONNECTING,
        ACTIVE,
        HANGUP
    };

    PeerId            user_id;
    State             state = UNKNOWN;
    MediaSenderState  media_sender;
    std::string       zrtp_sas;

    //helper functions
    static std::string  StateToString(CallParticipantInfo::State  state);
    std::string  ToString() const;
};


class IVoipVideoWindowRenderer;         // declared in voip_video_window_renderer.h
class MediaManager;
struct CallStateInternal;

// C-like API for easy wrapping

class VoipEngine {
public:

    /////////////////////////////////////////////////////////////////////////////
    // Observers (delegates) interface declaration

    class   ISignalingObserver
    {
    public:
        // Called when starting new outgoing call. Application must respond by calling 
        // VoipEngine::Call_Allocated() with call guid and ice parameters.
        // App _may_ request these params from some server API or prepare them in advance.
        // |new_call_guid| _may_ be equal to call_id, but not required (e.g. when it is Signaling server's
        // responsibility to generate call guids).
        // This call differs from OnSendSignalingMsg() because voip engine don't know call guid at this stage.
        virtual void    OnAllocateCall(const CallId &call_id) = 0;

        // Called by VoipEngine when it requires to send signaling message to a peer or special-peer
        virtual void    OnSendSignalingMsg(const CallId &call_id, const PeerId &to, const SignalingMsg &message) = 0;
        virtual         ~ISignalingObserver() = default;
    };


    class IVoipEngineObserver
    {
    public:
        virtual void  OnIncomingCall(const CallStateInternal &call_state,
                                    const PeerId &from,
                                    bool is_video,
                                    const std::vector<CallParticipantInfo> &participants,
                                    const CallAppData &app_data_from_caller) = 0;

        virtual void  OnRinging(const CallStateInternal &call_state) = 0;

        virtual void  OnOutgoingCallAccepted(const CallStateInternal &call_state, const CallAppData &app_data_from_callee) = 0;

        virtual void  OnConnecting(const CallStateInternal &call_state) = 0;

        virtual void  OnCallActive(const CallStateInternal &call_state) = 0;

        // You should not use |call_id| after the call
        virtual void  OnCallTerminated(const CallStateInternal &call_state, bool terminated_locally, TerminateReason reason) = 0;

        virtual void  OnCallParticipantsUpdate(const CallStateInternal &call_state, const std::vector<CallParticipantInfo> &participants) = 0;

        virtual void  OnStateUpdated(const CallStateInternal &call_state) {};


        virtual ~IVoipEngineObserver() = default;
    };



    /////////////////////////////////////////////////////////////////////////////
    // initialization/destruction 

    // return nullptr if failed, filling up |err_out| with error information.
    static VoipEngine  *Create(std::shared_ptr<MediaManager> mediaManager);
    virtual ~VoipEngine() = default;

    // To unset observer, pass nullptr
    virtual void    SetSignalingObserver(ISignalingObserver   *signaling_observer) = 0;
    virtual void    SetVoipEngineObserver(IVoipEngineObserver *voip_observer) = 0;


    /////////////////////////////////////////////////////////////////////////////
    // Signaling API

    // See ISignalingObserver::OnAllocateCall() description
    virtual void    Call_Allocated(const CallId &call_id, const std::string &new_call_guid, const std::string &ice_parameters) = 0;

    // Push new signaling message from signaling server to engine. This may cause new incoming call (OnIncomingCall called)
    virtual void    SignalingMsgReceived(const PeerId &from, const SignalingMsg &message) = 0;

    /////////////////////////////////////////////////////////////////////////////
    // Device API

    /////////////////////////////////////////////////////////////////////////////
    // Call API

    // Create outgoing call. This requires 2 steps:
    // 1- Call_CreateOutgoing() will return new call_id, so app may store it for later use.
    // 2- Call_StartOutgoing() initiates the call, so observer may get callback with that call_id
    // This solves race condition when callback is called before app receives new call_id
    virtual CallId    Call_CreateOutgoing() = 0;

    virtual void      Call_StartOutgoing(const CallId &call_id,    // call id previously created by Call_CreateOutgoing()
                                        bool is_video,
                                        const std::vector<PeerId> &participants,
                                        const CallAppData   &app_data = "") = 0;    // All |participants| will receive app_data
                                                                                    // in OnIncomingCall() event

    // Invite new people to the call.
    virtual void      Call_InviteParticipants(const CallId  &call_id,
                                              const std::vector<PeerId> &participants,
                                              const CallAppData   &app_data = "") = 0;    // all |participants| will receive app_data
    // Drop previously invited participants.
    // Note: Currently you only can drop those, who were invited by you AND only if they hadn't answered yet.
    // This is Focus logic, though, so it can be changed.
    virtual void      Call_DropParticipants(const CallId &call_id,
                                            const std::vector<PeerId> &participants) = 0;



    virtual void      Call_AcceptIncoming(const CallId &call_id, 
                                          const CallAppData   &app_data = "") = 0;  // Caller will receive app_data in 
                                                                                    // OnOutgoingCallAccepted() event
    // Close current call session or reject incoming call.
    // |reason| must be TR_HANGUP, TR_REJECT, TR_BUSY or TR_INTERNAL_ERROR (see their description).
    // NOTE: Call_Terminate() doesn't destroy call at once,
    // it is possible that we need to send some signaling before call is terminated
    // and you may receive some signals after TerminateCall().
    // After call is terminated, observer will receive OnCallTerminated() signal.
    // According to this, app should only rely on OnCallTerminated() to destroy it's call record.
    virtual void      Call_Terminate(const CallId &call_id, TerminateReason reason) = 0;

    virtual void      Call_EnableVideoTx(const CallId &call_id, bool enable) = 0;
    virtual void      Call_EnableAudioTx(const CallId &call_id, bool enable) = 0;
    virtual void      Call_SetOnHold(const CallId &call_id, bool on_hold_mode) = 0;

    // Attach/detach video window renderer to the call.
    // Application may use built-in video render or create it's own by implementing IVoipVideoWindowRenderer interface.
    // It is possible to attach several video windows to one call.
    virtual void      Call_AttachVideoWindowRenderer(const CallId &call_id, std::shared_ptr<IVoipVideoWindowRenderer>      video_window) = 0;
    virtual void      Call_DetachVideoWindowRenderer(const CallId &call_id, std::shared_ptr<IVoipVideoWindowRenderer>      video_window) = 0;

protected:
    // disallow copy and assign
    VoipEngine() = default;
    VoipEngine & operator=(const VoipEngine&) = delete;
    VoipEngine(const VoipEngine&) = delete;
};


} // namespace voip

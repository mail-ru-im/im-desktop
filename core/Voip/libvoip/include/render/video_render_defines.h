#pragma once

#include <vector>
#include <string>

namespace rtc {
    class CriticalSection;
}

namespace webrtc {
    class VideoFrame;
}

namespace voip_legacy {


class ViEFrameCallback
{
public:
    virtual void DeliverVideoFrameI420(webrtc::VideoFrame& videoFrame) = 0;
protected:
    virtual ~ViEFrameCallback() {}
};

// Platform-dependent objects
#if __PLATFORM_ANDROID
  struct PlatformObjects
  {
    void* javaVM;
    void* env;
    void* context;
    bool  useOpenSLES;
  };
#else
  struct PlatformObjects
  {
    int  _not_required;
  };
#endif

typedef void render_window_t;
typedef void render_device_t;


enum eHighlightType {
    kHighlightType_Unhighlighted = 0,
    kHighlightType_Normal,
    kHighlightType_Over
};

//
// ChannelZOrder_AvatarForeground:
//           Draw on remote channel when NO incoming video and local video WITH camera enabled (invitation mode darkness) key:ENABLE_CHANNEL_FOREGROUND
// ChannelZOrder_AvatarSign:
// ChannelZOrder_ChannelSign
//          Must be drawn above AvatarForeground
//
// -----------------------------------------------------------------
// For INVITATION mode must be next sequence:
//
//        RendereZOrderInvitationPreview_ChannelSign      = 2,
//RendereZOrderPrimary_AvatarSign       = 1,               (3) = (1 + ChannelZOrder_InvitationOffset)
//        RendereZOrderInvitationPreview_AvatarForeground = 3,
//RendereZOrderPrimary_ChannelSign      = 2,               (4)
//RendereZOrderPrimary_AvatarForeground = 3,               (5)
//        RendereZOrderInvitationPreview_Stream           = 6,
//RendereZOrderPrimary_Stream           = 6,               (9)
//RendereZOrderPrimary_Avatar           = 7,               (10)
// -----------------------------------------------------------------
//#define ChannelZOrder_InvitationOffset 2
enum ChannelZOrder {
    //ChannelZOrder_ButtonClose      = 0,
    //ChannelZOrder_AvatarSign       = 1,
    //ChannelZOrder_ChannelSign      = 2,
    //ChannelZOrder_Avatar           = 3,
    //ChannelZOrder_AvatarForeground = 4,
    //ChannelZOrder_Stream           = 7,
    //ChannelZOrder_RoundsAnimation  = 8,
    //ChannelZOrder_StatusGlow       = 9,
    //ChannelZOrder_Border           = 10,
    //ChannelZOrder_AvatarBackground = 11,
    ChannelZOrder_MAX              = 12
};

enum ChannelStatus {
    StatusState_Allocating,
    StatusState_WaitingForAccept,
    StatusState_WaitingForAccept_Confirmed,
    StatusState_IncomingInvite,
    StatusState_ConfInviteParticipant,
    StatusState_ConfInvite,
    StatusState_Connecting,
    StatusState_Connected,
    StatusState_OnHold,

    // Not active status
    StatusState_ClosedByDecline, // must be the first inactive
    StatusState_ClosedByTimeout,
    StatusState_ClosedByError,
    StatusState_ClosedByBusy,
    StatusState_Ended
};
const std::string   ChannelStatusToString(ChannelStatus status);


enum MouseEvent {
    MOUSE_EVENT_OVER,            // mouse OVER - desktop only

                                 // MOVE sequence:
    MOUSE_EVENT_DN,              // down (catch)
    MOUSE_EVENT_MOVED,           // move object
    MOUSE_EVENT_UP,              // release object

    MOUSE_EVENT_ZOOM,            // ZOOM event from PUNCH recognizer(ios) or mouse scroll (desktop)

                                 // CLICK sequence:
    MOUSE_EVENT_CLICK_BEGIN,     // check that click single/double/long on this arear is able
                                 // click type
    MOUSE_EVENT_CLICK_SINGLE,
    MOUSE_EVENT_CLICK_DOUBLE,
    MOUSE_EVENT_CLICK_LONG,


    MOUSE_WINDOW_LOST,
    // extended
    MOUSE_EVENT_TAP,
    MOUSE_EVENT_DOUBLETAP,
    MOUSE_EVENT_LONGTAP
};


class MouseEventHandler {
public:
    virtual bool HandleMouseEvent(MouseEvent mouseEvent, signed x, signed y, signed p0, signed p1, float p2) = 0;
protected:
    virtual ~MouseEventHandler() {}
};

class WindowEventsHandler {
public:
    virtual void onWindowClosed(void* hwnd) = 0;
protected:
    virtual ~WindowEventsHandler() {}
};

#define VIDEO_ASPECT_STRETCH -1.f	// stretch to viewport
#define VIDEO_ASPECT_PAD      0.f	// pad video with fields to fit video region set by SetVideoPosition()
#define VIDEO_ASPECT_CROP     1.f	// crop video to fit video region set by SetVideoPosition()
#define VIDEO_ASPECT_OPTIMAL  2.f   // value between [pad, crop] which provides best image fit

class VideoRenderCallback {
public:
    virtual int32_t RenderFrame(webrtc::VideoFrame& videoFrame) = 0;
protected:
    virtual ~VideoRenderCallback(){}
};

struct rectF {
    rectF() : left(0), right(0), top(0), bottom(0) {}
    rectF(float l, float r, float t, float b) : left(l), right(r), top(t), bottom(b) {}

    union { float left, l; };
    union { float right, r;  };
    union { float top, t; };
    union { float bottom, b; };

    float width() const { return r - l; }
    float height() const { return b - t; }
    float centerX() const { return l + width()/2.f; }
    float centerY() const { return t + height()/2.f; }

    void moveCenter(float dX, float dY) { l += dX; r += dX; t += dY; b += dY; }

    std::string to_string() const
    {
        return "l=" + std::to_string(left) + "r=" + std::to_string(right) + "t=" + std::to_string(top) + "b=" + std::to_string(bottom);
    }
};

typedef rectF rect01;

struct pointF {
    pointF() : x(0), y(0) {}
    pointF(float x, float y) : x(x), y(y) {}

    float x;
    float y;
};

typedef pointF point01;

struct rectU {
    rectU() : left(0), right(0), top(0), bottom(0) {}
    int left, right, top, bottom;
};

struct rectUScreen : rectU {
    rectUScreen() : screenWidth(0), screenHeight(0) {}
    rect01 getRect01() const
    {
        return (screenWidth > 0 && screenHeight > 0) ? rect01(left / (float)screenWidth, right / (float)screenWidth,
            top / (float)screenHeight, bottom / (float)screenHeight) : rect01();
    }
    void fromRect01(const rect01& rect)
    {
        left   = (int)(rect.left   * screenWidth);
        right  = (int)(rect.right  * screenWidth);
        top    = (int)(rect.top    * screenHeight);
        bottom = (int)(rect.bottom * screenHeight);
    }

    int width() const { return right - left; }
    int height() const { return bottom - top; }

    int screenWidth, screenHeight;
};

struct zrect {
    rect01 rc;
    unsigned zorder;
};

struct zhrect {
    zhrect() : zorder(0) {};
    rect01 rc;
    unsigned zorder;
};

// Feedback class to be implemented by module user
class VideoRenderFeedback {

public:
    // Initial params for callback functions:
    // - size of the window/view in pixels
    // - video frame parameters in pixels
    //   with angle in grad

    struct renderUpdateIn {
        struct {
            unsigned width;   //-1 if unknown
            unsigned height;  //-1 if unknown
        } window;

        struct {
            unsigned width;
            unsigned height;
            unsigned angle;
        } frame;
    };

    struct frameParams {
        unsigned width;
        unsigned height;
        unsigned angle;
    };

    // Output data from callback

    //  - rectDraw - point within range [0.f - 1.f] which used to draw stream
    //  - rectCrop - point within range [0.f - 1.f] which used to crop bitmap/image/stream which would be draw in rectDraw
    struct renderUpdateOut {
        renderUpdateOut() : angle(0), zOrder(0), visible(true), roundRadius(0) {}
        rect01 rectDraw;
        rect01 rectCrop;
        int angle;
        unsigned zOrder;
        bool visible;
        unsigned roundRadius;
        std::string to_string() const
        {
            return "rectDraw: " + rectDraw.to_string() + " rectCrop: " + rectCrop.to_string();
        }
    };

    virtual void OnDrawingFramePositionsUpdate(int stream_id, const renderUpdateIn &inData, std::vector<renderUpdateOut> &outData) = 0;
    //
    // Inputs:
    //    - videoFrame - current video frame for the stream
    // Outputs:
    //    - extendedFrame - reserved space for probably changed videoFrame
    //    - extendedFrameUsed - if extendedFrame must be used for rendering
    //    - extendedFrameUpdated - if content of the extendedFrame was changed
    //
    // Due process if extendedFrame was changed once, and extendedFrameUsed == true, no
    // need update render texture if extendedFrameUpdated was set to false
    virtual void OnDrawingFrameDataUpdate(int stream_id,
                                          webrtc::VideoFrame &videoFrame,
                                          webrtc::VideoFrame &extendedFrame,
                                          bool &extendedFrameUpdated,
                                          bool &extendedFrameUsed)
    { extendedFrameUsed = false; extendedFrameUpdated = false; return; }
    virtual bool OnDrawingFrameDecoration(int stream_id, webrtc::VideoFrame &videoFrame) { return false; }

    virtual void UpdateRenderFrameParams(int stream_id, const frameParams& frameParams) {};

protected:
    virtual ~VideoRenderFeedback()= default;
};

enum VideoRenderWinMethod {
    kVideoRenderWinDd    = 0,
    kVideoRenderWinD3D   = 1,
    kVideoRenderWP8Proxy = 2
};

class RenderWindowHandler {
public:
    virtual bool HandleRenderUpdate(unsigned veiwWidth, unsigned veiwHeight) = 0;
protected:
    virtual ~RenderWindowHandler() {}
};

class VideoRenderModule {
public:
    static VideoRenderModule * CreateVideoRenderModule(void* window, MouseEventHandler* mouseEventHandler=NULL, PlatformObjects* systemObjects=NULL, rtc::CriticalSection* pRenderLock = NULL, RenderWindowHandler* renderWindow = NULL);
    static void DestroyVideoRenderModule(VideoRenderModule* module);

    virtual bool StartRender() = 0;
    virtual void StopRender() = 0;

    virtual VideoRenderCallback* AddIncomingRenderStream(unsigned streamId, VideoRenderFeedback *callback, const std::string& name) = 0;
    virtual void DeleteIncomingRenderStream(unsigned streamId) = 0;

    // TODO(yudin): remove all getters
    virtual int32_t GetOutputWindowResolution(unsigned& windowWidth, unsigned& windowHeight) const = 0;

    virtual void SetPostRenderEffectParams(const std::string &params_str) = 0;
protected:
    virtual bool Init() = 0;
	virtual ~VideoRenderModule() {};
};

enum VideoContent_Type {
    VideoContent_Avatar,
    VideoContent_Video,
    VideoContent_Unspecified
};

void set_pad_crop(rect01& rectDraw, rect01& rectCrop, unsigned viewW, unsigned viewH, float frame_W_div_H, float pad_to_crop);


#if !__DELIVERY_BUILD && 0
    void render_log(const char *tag, const char* file, int line, const char* func, const char* fmt, ...);
    #define RENDER_LOG(fmt, ...) render_log("VIDEO_RENDER", __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__);
#else
    #define RENDER_LOG(...)
#endif

}

#pragma once
#include "voip_video_window_renderer.h"

namespace voip {

struct BitmapDesc
{
    const int   magic = 0x424d5044;
    const void *rgba_pixels = nullptr;
    unsigned    width = 0;
    unsigned    height = 0;
};

// platform-dependent window handle.
using hwnd_t = void*;

// platform-dependent bitmap handle.
using hbmp_t = void*;

enum WindowThemeType {
    WindowTheme_One,
    WindowTheme_Two,
    WindowTheme_Three,
    WindowTheme_Four,
    WindowTheme_Five,
    WindowTheme_Six,
    WindowTheme_Seven,
    WindowTheme_Total
};

struct ChannelStatusContext {
    hbmp_t allocating = nullptr;       // outgoing call initialization
    hbmp_t calling = nullptr;          // invite sent, waiting for accept/decline/timeout
    hbmp_t ringing = nullptr;          // invite sent and received, waiting for accept/decline/timeout
    hbmp_t inviting = nullptr;         // incoming call is waiting for local accept
    hbmp_t confInviting = nullptr;     // incoming conference call is waiting for local accept
    hbmp_t connecting = nullptr;       // connecting
    hbmp_t connected = nullptr;
    hbmp_t hold = nullptr;
    hbmp_t videoPaused = nullptr;
    hbmp_t closedByDecline = nullptr;
    hbmp_t closedByTimeout = nullptr;
    hbmp_t closedByError = nullptr;
    hbmp_t closedByBusy = nullptr;
    hbmp_t ended = nullptr;
};

using ChannelStatusContexts = std::map<WindowThemeType, ChannelStatusContext>;

struct LogoStatusContext {
    hbmp_t logoBitmap = nullptr;
};

using LogoStatusContexts = std::map<WindowThemeType, LogoStatusContext>;

struct FocusContext {
    hbmp_t focusBitmap = nullptr;
};


// API for built-in video window renderer
class BuiltinVideoWindowRenderer : public IVoipVideoWindowRenderer
{
public:

    enum StaticImageId {
        Img_Background,       // Global, no |peer_id| required
        Img_Avatar,           // required |peer_id|
        Img_Username,         // required |peer_id|
        Img_UserBackground,   // required |peer_id|
        Img_UserForeground,   // required |peer_id|
    };

    enum ViewArea {
        Primary,     // (inplane)  Primary view when conference with tray activatate
        Detached,    // (detached) Detached (usually preview)
        Tray,        // (detached) Tray view
        Default,     // (inplane)  All other views
        Background,  //            All exept any view
    };

    enum MouseTap {
        Single,
        Double,
        Long,
        Over, // reported once per second
    };

    class Notifications
    {
    public:
        virtual void OnMouseTap(const PeerId &name,
                                MouseTap mouseTap,
                                ViewArea viewArea,
                                float x,
                                float y)
        {};
        virtual void OnVideoStreamChanged(const PeerId &name, bool havePicture) {};
        virtual void OnCameraZoomChanged(float scale) {};
        virtual void OnFrameSizeChanged(float aspect){};
    protected:
        virtual ~Notifications() = default;

    };

    static std::shared_ptr<BuiltinVideoWindowRenderer> Create(hwnd_t hwnd,
            const std::string &settings_json,
            const ChannelStatusContexts &statusContext,
            std::shared_ptr<Notifications> notifications,
            const FocusContext *focusContext = nullptr,
            const LogoStatusContexts *logoContexts = nullptr);

    virtual void SetRenderScaleFactor(float scale) = 0;
    virtual void SwitchAspectMode(const PeerId &name, bool animated) = 0;
    virtual void SetPrimary(const PeerId &name, bool animated) = 0;
    virtual void SetControlsStatus(bool visible, unsigned off_left, unsigned off_top, unsigned off_right, unsigned off_bottom, bool animated, bool enableOverlap) = 0;
    virtual void SetWindowTheme(WindowThemeType theme, bool animated) = 0;
    virtual void SetPointOfInterest(bool adjusting, float x, float y) = 0;
    virtual bool SetChannelStatusContext(WindowThemeType theme, const ChannelStatusContext& channelStatusContext) = 0;

    virtual bool SetStaticImage(WindowThemeType theme, StaticImageId img_id, const PeerId &peer_id, hbmp_t bitmap) = 0;

    struct AvatarSizes {
        int pictureWidth;
        int pictureHeight;
        int textWidth;
        int textHeight;
        int textFontSize;
    };
    using AvtarSizesMap = std::map<WindowThemeType, AvatarSizes>;
    static AvtarSizesMap GetStatusImageSizesFromJson(const std::string &settings_json);

};

}

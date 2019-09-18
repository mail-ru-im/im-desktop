#ifndef __WINDOW_HEADER_FORMAT_H__
#define __WINDOW_HEADER_FORMAT_H__

namespace Ui
{
    enum eVideoPanelHeaderItems {
        kVPH_ShowNone = 0x0000,

        kVPH_ShowName   = 0x0001,
        kVPH_ShowTime   = 0x0002,
        kVPH_ShowMin    = 0x0004,
        kVPH_ShowMax    = 0x0008,
        kVPH_ShowClose  = 0x0010,
        kVPH_ShowLogo   = 0x0020,
        kVPH_ShowSecure = 0x0040,
        kVPH_BackToVideo = 0x0080,

        kVPH_ShowAll = 0xffff
    };
}

#endif//__WINDOW_HEADER_FORMAT_H__
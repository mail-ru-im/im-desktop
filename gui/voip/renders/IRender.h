#pragma once
#include "../../../core/Voip/VoipManagerDefines.h"

class QWidget;
class QMenu;

namespace im
{
    class IVideoFrame;
}

namespace Ui
{
    class IRender
    {
    public:
        enum class ViewMode
        {
            SpeakerMode,
            GridMode
        };

        virtual ~IRender() = default;

        virtual QWidget* getWidget() = 0;
        virtual void setFrame(const im::IVideoFrame* frame, const std::string& peerId) = 0;
        virtual void clear() = 0;
        virtual void setRenderLayout(bool oneIsBig) = 0;
        virtual bool getRenderLayout() = 0;
        virtual void setBigPeer(const std::string& bigPeerName) = 0;
        virtual void setVCS(bool isVCS) = 0;
        virtual void setPinnedRoom(bool isPinnedRoom) = 0;
        virtual void setIncoming(bool isIncoming) = 0;
        virtual void setMiniWindow(bool isMiniWindow) = 0;
        virtual void updatePeers(const voip_manager::ContactsList& peerList) = 0;
        virtual void localMediaChanged(bool audio, bool video) = 0;
        virtual void setScreenSharing(bool _on) = 0;
        virtual void toggleRenderLayout(const QPoint& clickPos) = 0;
        virtual void setViewMode(ViewMode _mode) = 0;
        virtual ViewMode getViewMode() const = 0;
        virtual QMenu* createContextMenu(const QPoint& _p) const = 0;
    };

} // namespace Ui

#pragma once
#include "../controls/TextUnit.h"
#include "CommonUI.h"
#include "VoipProxy.h"
#include "ScreenFrame.h"
#include "media/permissions/MediaCapturePermissions.h"
#include "renders/OGLRender.h"

namespace voip_manager
{
    struct ContactEx;
}

namespace Previewer
{
    class CustomMenu;
}

class MiniWindowVideoPanel;

namespace Ui
{
    class ResizeEventFilter;
    class ShadowWindowParent;
    class PanelToolButton;
    class TransparentPanelButton;

    class MiniWindowVideoPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needShowScreenPermissionsPopup(media::permissions::DeviceType _type);
        void onMicrophoneClick();
        void onOpenCallClicked();
        void onResizeClicked();
        void onHangUpClicked();
        void onShareScreenClick(bool _on);
        void needShowVideoWindow();
        void closeChooseScreenMenu();

    private Q_SLOTS:
        void onResizeButtonClicked();
        void onOpenCallButtonClicked();
        void onAudioOnOffClicked();
        void onVideoOnOffClicked();

        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onShareScreen();
        void onShareScreenStateChanged(ScreenSharingManager::SharingState _state, int _screenIndex);
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);
        void onScreensChanged();

    public:
        MiniWindowVideoPanel(QWidget* _parent);
        ~MiniWindowVideoPanel();

        void changeResizeButtonState();

    private:
        void updateVideoDeviceButtonsState();
        void changeResizeButtonTooltip();

    protected:
        bool eventFilter(QObject* _watched, QEvent* _event) override;
        void paintEvent(QPaintEvent* _e) override;

    private:
        PanelToolButton* createButton(const QString& _icon, const QString& _text, bool _checkable);
        TransparentPanelButton* createButton(const QString& _icon, const QString& _text, Qt::Alignment _align, bool _isAnimated);
        void showShareScreenMenu(int _screenCount);

    private:
        QWidget* parent_;
        QWidget* rootWidget_;

        TransparentPanelButton* resizeButton_;
        TransparentPanelButton* openCallButton_;
        PanelToolButton* micButton_;
        PanelToolButton* stopCallButton_;
        PanelToolButton* videoButton_;
        PanelToolButton* shareScreenButton_;
        Previewer::CustomMenu* menu_;

        bool localVideoEnabled_ : 1;
        bool isScreenSharingEnabled_ : 1;
        bool isCameraEnabled_ : 1;
    };

    class MiniWindowHeader : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void mouseMoved(const QMouseEvent& _e);
    public:
        MiniWindowHeader(QWidget* _parent);
        void setTitle(const QString& _title);
    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* event) override;
    private:
        void updateTitleOffsets();
    private:
        TextRendering::TextUnitPtr text_;
    };

    class DetachedVideoWindow : public QWidget
    {
        Q_OBJECT

    protected:
        void showEvent(QShowEvent*) override;
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
        void contextMenuEvent(QContextMenuEvent*) override;

    Q_SIGNALS:
        void windowWillDeminiaturize();
        void windowDidDeminiaturize();
        void needShowScreenPermissionsPopup(media::permissions::DeviceType type);
        void onMicrophoneClick();
        void onShareScreenClick(bool _on);
        void activateMainVideoWindow();
        void requestHangup();

    private Q_SLOTS:
        void onPanelMouseEnter();
        void onPanelMouseLeave();

        void onPanelClickedClose();
        void onPanelClickedResize();

        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallNameChanged(const voip_manager::ContactsList&);

    private:
        void onMousePress(const QMouseEvent& _e);
        void onMouseMove(const QMouseEvent& _e);

        void updatePanels() const;

        enum class ResizeDirection
        {
            Minimize,
            Maximize,
        };
        static constexpr QPoint kInvalidPoint{ -1, -1 };
        void resizeAnimated(ResizeDirection _dir);

        void startTooltipTimer();
        void onTooltipTimer();

    public:
        DetachedVideoWindow(QWidget* _parent);
        ~DetachedVideoWindow();

        bool closedManualy();

        enum class WindowMode
        {
            Compact,
            Full,
            Current
        };
        void showFrame(WindowMode _mode = WindowMode::Current);
        void hideFrame();

        bool isMinimized() const;
        bool isMousePressed() const { return pressPos_ != kInvalidPoint; };

        IRender* getRender() const { return rootWidget_; };
        void setWindowTitle(const QString& _title);
        void setWindowMode(WindowMode _mode);
        WindowMode getWindowMode() const { return mode_; }

        void moveToCorner();

    private:
        ResizeEventFilter* eventFilter_ = nullptr;
        QWidget* parent_;
        QPoint pressPos_;
        bool closedManualy_ = false;
        bool microphoneEnabled_ = false;
        bool videoEnabled_ = false;
        MiniWindowVideoPanel* videoPanel_ = nullptr;
        MiniWindowHeader* header_ = nullptr;
        std::unique_ptr<ShadowWindowParent> shadow_;
        IRender* rootWidget_ = nullptr;

        QVariantAnimation* resizeAnimation_;

        QPoint tooltipPos_;
        QTimer* tooltipTimer_ = nullptr;

        WindowMode mode_ = WindowMode::Full;
    };
}

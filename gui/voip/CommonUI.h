
#pragma once

#include "../../core/Voip/VoipManagerDefines.h"
//#include <functional>
#include <memory>
//#include "../main_window/MainWindow.h"

#ifdef __APPLE__
#include "macos/VideoFrameMacos.h"
#endif

namespace voip_manager
{
    struct CipherState;
    struct FrameSize;
}

namespace Ui
{
    class BaseVideoPanel;
    class ShadowWindow;


    class ResizeEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ResizeEventFilter(std::vector<QPointer<BaseVideoPanel>>& panels, ShadowWindow* shadow, QObject* _parent);

        void addPanel(BaseVideoPanel* _panel);
        void removePanel(BaseVideoPanel* _panel);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        std::vector<QPointer<BaseVideoPanel>> panels_;
        ShadowWindow* shadow_;
    };

    class ShadowWindowParent
    {
    public:
        ShadowWindowParent(QWidget* parent);
        ~ShadowWindowParent();

        void showShadow();
        void hideShadow();
        ShadowWindow* getShadowWidget();
        void setActive(bool _value);

    protected:
        // shadow
        ShadowWindow* shadow_;
    };

    struct UIEffects
    {
        UIEffects(QWidget& _obj, bool opacity = true, bool geometry = true);
        virtual ~UIEffects();

        void fadeOut(unsigned _interval);
        void fadeIn(unsigned _interval);

        void geometryTo(const QRect& _rc, unsigned _interval);
        void geometryTo(const QRect& _rcStart, const QRect& _rcFinish, unsigned _interval);
        bool isFadedIn();
        bool isGeometryRunning();
        void forceFinish();

    private:
        bool fadedIn_;

        QGraphicsOpacityEffect* fadeEffect_;
        std::unique_ptr<QPropertyAnimation> animation_;
        std::unique_ptr<QPropertyAnimation> resizeAnimation_;

        QWidget& obj_;
    };

    class AspectRatioResizebleWnd : public QWidget
    {
        Q_OBJECT
    public:
        AspectRatioResizebleWnd();
        virtual ~AspectRatioResizebleWnd();

        bool isInFullscreen() const;
        void switchFullscreen();

        void useAspect();
        void unuseAspect();
        void saveMinSize(const QSize& size);

    protected:
        bool nativeEvent(const QByteArray& _eventType, void* _message, long* _result) override;
        virtual quintptr getContentWinId() = 0;
        virtual void escPressed() { }

        private Q_SLOTS:
        void onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs);
        //void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);

    private:
        float aspectRatio_;
        bool useAspect_;
        QSize originMinSize_;

        std::unique_ptr<UIEffects> selfResizeEffect_;
        // Commented because we always constrain size of video window.
        //bool firstTimeUseAspectRatio_;

        void applyFrameAspectRatio(float _wasAr);
        void keyReleaseEvent(QKeyEvent*) override;
#ifdef _WIN32
        bool onWMSizing(RECT& _rc, unsigned _wParam);
#endif
        void fitMinimalSizeToAspect();
    };

    // Inherit from this class to create panel in voip window.
    class BaseVideoPanel : public QWidget
    {
    public:
        BaseVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

        virtual void updatePosition(const QWidget& parent) = 0;

        virtual void fadeIn(unsigned int duration);
        virtual void fadeOut(unsigned int duration);
        virtual void forceFinishFade();

        bool isGrabMouse();
        virtual bool isFadedIn();

    protected:

        bool grabMouse;

    private:

        std::unique_ptr<UIEffects> effect_;
    };

    // Inherit from this class to create panel on top in voip window.
    class BaseTopVideoPanel : public BaseVideoPanel
    {
    public:
#if defined(__linux__)
        BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Widget);
#else
        BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#endif
        void updatePosition(const QWidget& parent) override;
    };

    // Inherit from this class to create panel on bottom in voip window.
    class BaseBottomVideoPanel : public BaseVideoPanel
    {
    public:
        BaseBottomVideoPanel(QWidget* parent,
#ifndef __linux__
          Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint
#else
          Qt::WindowFlags f = Qt::Widget
#endif
                );

        void updatePosition(const QWidget& parent) override;
    };

    // This panel fill all parent window
    class FullVideoWindowPanel : public Ui::BaseVideoPanel
    {
        Q_OBJECT

    public:

        FullVideoWindowPanel(QWidget* parent);

        virtual void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration)  override {}
        virtual void fadeOut(unsigned int duration) override {}

    signals:

        void onResize();

    protected:

        void resizeEvent(QResizeEvent *event) override;
    };

    // Use this class if you want to process
    // mouse event on transparent panels.
    class PanelBackground : public QWidget
    {
    public:
        PanelBackground(QWidget* parent);

        void updateSizeFromParent();
    };


    /**
     * This widget is used to process mouse event for another panel if it has transparent pixels.
     * Under Windows it is imposible to get mouse event for tranparent part of widgets.
     * We have outgoing panel which implements drag & drop, but it has trasporent parts.
     */
    class TransparentPanel : public BaseVideoPanel
    {
        Q_OBJECT

    public:
        TransparentPanel(QWidget* _parent, BaseVideoPanel* _eventWidget);
        ~TransparentPanel();

        void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration) override {}
        virtual void fadeOut(unsigned int duration) override {}

    protected:

        void resizeEvent(QResizeEvent * event) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent * event) override;
        void mousePressEvent(QMouseEvent * event) override;

        template <typename E> void resendMouseEventToPanel(E* event_);

    protected:

        // We will resend events to this widget.
        BaseVideoPanel* eventWidget_;

        // Background widget to able process mouse events on transparent parts of panel.
        PanelBackground* backgroundWidget_;
    };

    class MoveablePanel : public BaseTopVideoPanel
    {
        Q_OBJECT
            Q_SIGNALS :
        void onkeyEscPressed();

        private Q_SLOTS:

    public:
        MoveablePanel(QWidget* _parent);
        MoveablePanel(QWidget* _parent, Qt::WindowFlags f);
        virtual ~MoveablePanel();

    private:
        QWidget* parent_;
        struct
        {
            QPoint posDragBegin;
            bool isDraging;
        } dragState_;

        void changeEvent(QEvent*) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void keyReleaseEvent(QKeyEvent*) override;

    protected:
        virtual bool uiWidgetIsActive() const = 0;
    };

    // Show dialog to add new users to video call.
    void showAddUserToVideoConverenceDialogVideoWindow(QObject* parent, FullVideoWindowPanel* parentWindow);
    void showAddUserToVideoConverenceDialogMainWindow(QObject* parent, QWidget* parentWindow);
}

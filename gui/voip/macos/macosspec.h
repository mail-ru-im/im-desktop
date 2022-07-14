//
//  VideoFrameMacos.h
//  ICQ
//
//  Created by IceSeer on 04/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#ifndef VideoFrameMacos_h
#define VideoFrameMacos_h

namespace platform_macos
{
    void setPanelAttachedAsChild(bool attach, QWidget* parent, QWidget* child);
    void setAspectRatioForWindow(QWidget& wnd, float aspectRatio);
    void unsetAspectRatioForWindow(QWidget& wnd);

    bool windowIsOnActiveSpace(QWidget* _widget);
    bool windowIsOverlapped(QWidget* _frame);
    void setWindowPosition(QWidget& widget, const QRect& widgetRect);
    void moveAboveParentWindow(QWidget& parent, QWidget& child);

    QRect getWidgetRect(const QWidget& widget);

    // Double click interval in ms.
    int doubleClickInterval();

    // @return true, if iTunes was paused.
    bool pauseiTunes();
    void playiTunes();

    void fadeIn(QWidget* wnd);
    void fadeOut(QWidget* wnd);

    void showInWorkspace(QWidget* _w);

    QSize framebufferSize(QWidget* _w);

    // This class use Cocoa delegates and
    // notificate us about start/finish fullscreen animation.
    class FullScreenNotificaton : public QObject
    {
        Q_OBJECT
    public:
        FullScreenNotificaton (QWidget& parentWindow);
        virtual ~FullScreenNotificaton ();

    Q_SIGNALS:

        void fullscreenAnimationStart();
        void fullscreenAnimationFinish();
        void minimizeAnimationStart();
        void minimizeAnimationFinish();
        void activeSpaceDidChange();
        void changeFullscreenState(bool _isFull);
    protected:

        // Cocoa delegate
        void* _delegate;
        QWidget& _parentWindow;
    };
}

#endif /* VideoFrameMacos_h */

#pragma once
#include "ShapedWidget.h"
#include "renders/IRender.h"

namespace Ui
{
    class VideoPanel;
    class VideoControls : public ShapedWidget
    {
        Q_OBJECT

    public:
        explicit VideoControls(QWidget* _parent);
        ~VideoControls();

        VideoPanel* panel() const;

        void updateControls(const voip_manager::ContactsList& _contacts);
        void updateControls(IRender::ViewMode _mode);

    public Q_SLOTS:
        void fadeIn();
        void fadeOut();
        void onFadeTimeout();
        void delayFadeOut();

        void animationsLocked();
        void animationsUnlocked();
        void setOpacity(double _value);

        void onWindowStateChanged(Qt::WindowStates _states);

    Q_SIGNALS:
        void toggleFullscreen();
        void requestGridView();
        void requestSpeakerView();
        void requestLink();

    private:
        std::unique_ptr<class VideoControlsPrivate> d;
    };
}

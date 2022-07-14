#pragma once
#include <QtWidgets/QWidget>

namespace platform_specific
{
    void showInCurrentWorkspace(QWidget* _w, QWidget* _initiator);
    bool windowIsOverlapped(QWidget* _window);
    bool isSpacebarDown();
}

namespace ButtonStatistic
{
    Q_NAMESPACE

    enum ButtonFlag
    {
        ButtonMic = 0x1,
        ButtonSpeaker = 0x2,
        ButtonChat = 0x4,
        ButtonCamera = 0x8,
        ButtonSecurity = 0x10,
        ButtonAddToCall = 0x20
    };
    Q_DECLARE_FLAGS(ButtonFlags, ButtonFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(ButtonFlags)

    Q_FLAG_NS(ButtonFlags)

    struct Data
    {
        ButtonFlags flags_ = ButtonFlags{};
        int currentTime_ = 0;

        enum { kMinTimeForStatistic = 5 }; // for calls from 5 seconds.

        void reset();
        void send();
    };
} // end namespace ButtonStatistic
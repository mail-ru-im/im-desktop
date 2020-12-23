#pragma once

#include "ClickWidget.h"

namespace Utils
{
    class CallLinkCreator;
}

namespace Ui
{
    class ContextMenu;

    enum class CallButtonType
    {
        Standalone,
        Button,
    };

    class StartCallButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        StartCallButton(QWidget* _parent, CallButtonType _type = CallButtonType::Standalone);
        ~StartCallButton();

        void setAimId(const QString& _aimId) { aimId_ = _aimId; }

    protected:
        void paintEvent(QPaintEvent*) override;

    private Q_SLOTS:
        void startAudioCall() const;
        void startVideoCall() const;

    private:
        bool isButton() const noexcept { return type_ == CallButtonType::Button; }

        void showContextMenu();
        void createCallLink();

        enum class RotateDirection
        {
            Left,
            Right,
        };

        void rotate(const RotateDirection _dir);
    private:
        QVariantAnimation* anim_;
        double currentAngle_;
        CallButtonType type_;
        QPointer<ContextMenu> menu_;
        QString aimId_;
        Utils::CallLinkCreator* callLinkCreator_;
        bool doPreventNextMenu_;
    };
}
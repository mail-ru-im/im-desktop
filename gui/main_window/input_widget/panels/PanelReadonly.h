#pragma once

#include "types/chat.h"
#include "../InputWidgetUtils.h"

namespace Ui
{
    class CustomButton;
    class RoundButton;

    enum class ReadonlyPanelState
    {
        Invalid,

        AddToContactList,
        EnableNotifications,
        DisableNotifications,
        DeleteAndLeave,
        Unblock,
    };

    class InputPanelReadonly : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    public:
        InputPanelReadonly(QWidget* _parent);

        void setAimid(const QString& _aimId);
        void setState(const ReadonlyPanelState _state);
        ReadonlyPanelState currentState() const noexcept { return state_; }

        void updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _chatInfo);
        void setFocusOnButton();

    private:
        void onButtonClicked();
        void onShareClicked();

        void joinClicked();
        void notifClicked();
        void unblockClicked();
        void leaveClicked();

        std::string getStatsChatTypeReadonly() const;

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        QString aimId_;
        QString stamp_;
        bool isChannel_ = false;

        RoundButton* mainButton_ = nullptr;
        CustomButton* shareButton_ = nullptr;

        ReadonlyPanelState state_ = ReadonlyPanelState::Invalid;
    };
}
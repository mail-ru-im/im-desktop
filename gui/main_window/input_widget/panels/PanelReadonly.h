#pragma once

#include "controls/TextUnit.h"
#include "controls/ClickWidget.h"
#include "types/chat.h"
#include "../InputWidgetUtils.h"

namespace Ui
{
    class PanelButton : public ClickableWidget
    {
    public:
        PanelButton(QWidget* _parent);

        void setColors(const QColor& _bgNormal, const QColor& _bgHover = QColor(), const QColor& _bgActive = QColor());

        void setTextColor(const QColor& _color);
        void setText(const QString& _text);
        void setIcon(const QString& _iconPath);

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;

    private:
        QColor getBgColor() const;

    private:
        QPixmap icon_;
        TextRendering::TextUnitPtr text_;
        QPainterPath bubblePath_;

        QColor bgNormal_;
        QColor bgHover_;
        QColor bgActive_;
        QColor textColor_;
    };


    class CustomButton;
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

        PanelButton* mainButton_ = nullptr;
        CustomButton* shareButton_ = nullptr;

        ReadonlyPanelState state_ = ReadonlyPanelState::Invalid;
    };
}
#pragma once
#include "../controls/ClickWidget.h"
#include "utils/SvgUtils.h"

namespace Previewer
{
    class CustomMenu;
}

namespace Ui
{
    class PanelToolButton : public QAbstractButton
    {
        Q_OBJECT
    public:
        enum IconStyle
        {
            Circle,
            Rounded,
            Rectangle
        };
        Q_ENUM(IconStyle)

        enum ButtonRole
        {
            Regular,
            Attention
        };
        Q_ENUM(ButtonRole)

        enum PopupMode
        {
            MenuButtonPopup,
            InstantPopup
        };
        Q_ENUM(PopupMode)

        explicit PanelToolButton(QWidget* _parent = nullptr);
        explicit PanelToolButton(const QString& _text, QWidget* _parent = nullptr);
        ~PanelToolButton();

        void setKeySequence(const QKeySequence& _shortcut);
        QKeySequence keySequence() const;

        void setTooltipsEnabled(bool _on);
        bool isTooltipsEnabled() const;

        void setText(const QString& text);
        void setIcon(const QString& _iconName);
        void setBadgeCount(int _count);
        int badgeCount() const;

        void setPopupMode(PopupMode _mode);
        PopupMode popupMode() const;

        void setButtonRole(ButtonRole _role);
        ButtonRole role() const;

        void setIconStyle(IconStyle _style);
        IconStyle iconStyle() const;

        void setButtonStyle(Qt::ToolButtonStyle _style);
        Qt::ToolButtonStyle buttonStyle() const;

        void setAutoRaise(bool _on);
        bool isAutoRaise() const;

        void setMenuAlignment(Qt::Alignment _align);
        Qt::Alignment menuAlignment() const;

        void setMenu(QMenu* _menu);
        QMenu* menu() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        void showMenu(QMenu* _menu);

    Q_SIGNALS:
        void aboutToShowMenu(QPrivateSignal);

    protected:
        bool event(QEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<class PanelToolButtonPrivate> d;
    };


    class TransparentPanelButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        TransparentPanelButton(QWidget* _parent, const QString& _iconName, const QString& _tooltipText = QString(), Qt::Alignment _align = Qt::AlignCenter, bool _isAnimated = false);
        void setTooltipBoundingRect(const QRect& _r);

        void changeState();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void showToolTip() override;

    private Q_SLOTS:
        void onClicked();

    private:
        enum class RotateDirection
        {
            Left,
            Right,
        };
        enum class RotateAnimationMode
        {
            Full,
            ShowFinalState
        };

        void rotate(const RotateDirection _dir);
        QPoint getIconOffset() const;
        QRect getTooltipArea() const;

    private:
        Utils::StyledPixmap iconNormal_;
        Utils::StyledPixmap iconHovered_;
        Utils::StyledPixmap iconPressed_;
        Qt::Alignment align_;
        QVariantAnimation* anim_;
        double currentAngle_;
        RotateDirection dir_;
        RotateAnimationMode rotateMode_;
        QRect tooltipBoundingRect_;
    };


    QString microphoneIcon(bool _checked);
    QString speakerIcon(bool _checked);
    QString videoIcon(bool _checked);
    QString screensharingIcon();
    QString hangupIcon();

    QString microphoneButtonText(bool _checked);
    QString videoButtonText(bool _checked);
    QString speakerButtonText(bool _checked);
    QString screensharingButtonText(bool _checked);
    QString stopCallButtonText();
}

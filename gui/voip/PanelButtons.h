#pragma once
#include "../controls/ClickWidget.h"

namespace Ui
{
    class MoreButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        MoreButton(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _event) override;
    };

    class PanelButton : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void moreButtonClicked();
    public:
        enum class ButtonStyle
        {
            Normal,
            Transparent,
            Red
        };

        enum class ButtonSize
        {
            Big,
            Small
        };

        PanelButton(QWidget* _parent, const QString& _text, const QString& _iconName, ButtonSize _size = ButtonSize::Big, ButtonStyle _style = ButtonStyle::Normal, bool _hasMore = false);

        void updateStyle(ButtonStyle _style, const QString& _iconName = {}, const QString& _text = {});

        void setCount(int _count);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QString iconName_;
        QPixmap icon_;
        QColor circleNormal_;
        QColor circleHovered_;
        QColor circlePressed_;
        QColor textColor_;
        ButtonSize size_;

        std::unique_ptr<TextRendering::TextUnit> textUnit_;
        MoreButton* more_;
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
        int count_;
    };

    class TransparentPanelButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        TransparentPanelButton(QWidget* _parent, const QString& _iconName, const QString& _tooltipText = QString(), Qt::Alignment _align = Qt::AlignCenter, bool _isAnimated = false);
        void setTooltipText(const QString& _text) override;
        const QString& getTooltipText() const override;
        void hideTooltip();
        void setTooltipBoundingRect(const QRect& _r);

        void changeState();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void onTooltipTimer() override;
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

    protected:
        QTimer* tooltipTimer_;
        QString tooltipText_;

    private:
        QPixmap iconNormal_;
        QPixmap iconHovered_;
        QPixmap iconPressed_;
        Qt::Alignment align_;
        QVariantAnimation* anim_;
        double currentAngle_;
        RotateDirection dir_;
        RotateAnimationMode rotateMode_;
        QRect tooltipBoundingRect_;
    };

    enum class GridButtonState
    {
        ShowAll,
        ShowBig
    };

    class GridButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        GridButton(QWidget* _parent);

        void setState(const GridButtonState _state);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> text_;
        GridButtonState state_;
    };

    QString getMicrophoneButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size = PanelButton::ButtonSize::Big);
    QString getVideoButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size = PanelButton::ButtonSize::Big);
    QString getSpeakerButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size = PanelButton::ButtonSize::Big);
    QString getScreensharingButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size = PanelButton::ButtonSize::Big);
}

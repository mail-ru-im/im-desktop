#pragma once

#include "controls/ClickWidget.h"
#include "controls/SimpleListWidget.h"
#include "controls/TextUnit.h"
#include "animation/animation.h"

namespace Utils
{
    class OpacityEffect;
}

namespace Ui
{
    class InputWidget;

    class AttachFileMenuItem : public SimpleListItem
    {
        Q_OBJECT

    Q_SIGNALS:
        void selectChanged(QPrivateSignal) const;

    public:
        AttachFileMenuItem(QWidget* _parent, const QString& _icon, const QString& _caption, const QColor& _iconBgColor);

        void setSelected(bool _value) override;
        bool isSelected() const override;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        QColor iconBgColor_;
        QPixmap icon_;
        TextRendering::TextUnitPtr caption_;
        bool isSelected_ = false;
    };


    class AttachPopupBackground : public QWidget
    {
        Q_OBJECT
    public:
        AttachPopupBackground(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _event) override;
    };


    class AttachFilePopup : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void photoVideoClicked(QPrivateSignal) const;
        void fileClicked(QPrivateSignal) const;
        void cameraClicked(QPrivateSignal) const;
        void geoClicked(QPrivateSignal) const;
        void contactClicked(QPrivateSignal) const;
        void pttClicked(QPrivateSignal) const;

    public:
        static AttachFilePopup& instance();
        static bool isOpen();

        enum class ShowMode
        {
            Normal,
            Persistent
        };
        static void showPopup(const ShowMode _mode = ShowMode::Normal);
        static void hidePopup();

        void showAnimated();
        void hideAnimated();

        void selectFirstItem();
        void setPersistent(const bool _persistent);

        void updateSizeAndPos();
        bool focusNextPrevChild(bool) override { return false; }
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    protected:
        void mouseMoveEvent(QMouseEvent* _e) override;
        void leaveEvent(QEvent*) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

    private:
        explicit AttachFilePopup(QWidget* _parent, InputWidget* _input);

        void onItemClicked(const int _idx);
        void onBackgroundClicked();
        void onHideTimer();

        void hideWithDelay();

        bool isMouseInArea(const QPoint& _pos) const;
        QRect getPlusButtonRect() const;
        QPolygon getMouseAreaPoly() const;

    private:
        SimpleListWidget* listWidget_ = nullptr;
        AttachPopupBackground* widget_ = nullptr;

        InputWidget* input_ = nullptr;

        Utils::OpacityEffect* opacityEffect_ = nullptr;

        enum class AnimState
        {
            None,
            Showing,
            Hiding
        };
        AnimState animState_ = AnimState::None;
        anim::Animation opacityAnimation_;

        enum class MenuItemId
        {
            photoVideo,
            file,
            camera,
            contact,
            ptt,
            geo,
        };
        std::vector<std::pair<int, MenuItemId>> items_;

        QRect buttonRect_;
        QPolygon mouseAreaPoly_;
        bool persistent_;
        QTimer hideTimer_;
    };

}
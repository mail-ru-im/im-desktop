#pragma once

#include "controls/ClickWidget.h"
#include "controls/SimpleListWidget.h"
#include "controls/TextUnit.h"
#include "utils/SvgUtils.h"

namespace Utils
{
    class OpacityEffect;
}

namespace Ui
{
    class AttachFileMenuItem : public SimpleListItem
    {
        Q_OBJECT

    Q_SIGNALS:
        void selectChanged(QPrivateSignal) const;

    public:
        AttachFileMenuItem(QWidget* _parent, const QString& _iconPath, const QString& _caption, const Styling::ThemeColorKey& _iconBgColor);

        void setSelected(bool _value) override;
        bool isSelected() const override;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        Styling::ColorContainer iconBgColor_;
        Utils::StyledPixmap icon_;
        TextRendering::TextUnitPtr caption_;
        bool isSelected_ = false;
    };


    class AttachPopupBackground : public QWidget
    {
        Q_OBJECT
    public:
        AttachPopupBackground(QWidget* _parent);
        int heightForContent(int _height) const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
    };

    enum class AttachMediaType
    {
        photoVideo,
        file,
        camera,
        contact,
        poll,
        task,
        ptt,
        geo,
        callLink,
        webinar
    };

    class AttachFilePopup : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void itemClicked(AttachMediaType _mediaType, QPrivateSignal) const;
        void visiblityChanged(bool _isVisible, QPrivateSignal) const;

    public:
        AttachFilePopup(QWidget* _parent);

        void showAnimated(const QRect& _plusButtonRect); // in global coords
        void hideAnimated();

        void selectFirstItem();
        void setPersistent(const bool _persistent);
        bool isPersistent() const noexcept { return persistent_; }

        void updateSizeAndPos();
        bool focusNextPrevChild(bool) override { return false; }
        bool eventFilter(QObject* _obj, QEvent* _event) override;

        void setPttEnabled(bool _enabled);

    protected:
        void mouseMoveEvent(QMouseEvent* _e) override;
        void leaveEvent(QEvent*) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

    private Q_SLOTS:
        void initItems();

    private:
        void onItemClicked(const int _idx);
        void onBackgroundClicked();
        void onHideTimer();

        void hideWithDelay();

        bool isMouseInArea(const QPoint& _pos) const;
        QPolygon getMouseAreaPoly() const;

        SimpleListWidget* listWidget_ = nullptr;
        AttachPopupBackground* widget_ = nullptr;

        Utils::OpacityEffect* opacityEffect_ = nullptr;

        enum class AnimState
        {
            None,
            Showing,
            Hiding
        };
        AnimState animState_ = AnimState::None;
        QVariantAnimation* opacityAnimation_;

        std::vector<std::pair<int, AttachMediaType>> items_;

        QRect buttonRect_;
        QPolygon mouseAreaPoly_;
        bool persistent_;
        bool pttEnabled_;
        QTimer hideTimer_;
    };

}

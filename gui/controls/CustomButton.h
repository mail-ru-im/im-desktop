#pragma once

#include "animation/animation.h"

namespace Ui
{
    enum class ButtonShape
    {
        RECTANGLE,
        CIRCLE
    };

    class CustomButton : public QAbstractButton
    {
        Q_OBJECT

    Q_SIGNALS:
        void clickedWithButtons(const Qt::MouseButtons &mouseButtons);
        void changedHover(bool _hover);
        void changedPress(bool _press);

    public:
        explicit CustomButton(QWidget* _parent, const QString& _svgName = QString(), const QSize& _iconSize = QSize(), const QColor& _defaultColor = QColor());

        void setActive(const bool _isActive);
        bool isActive() const;
        bool isHovered() const;
        bool isPressed() const;

        void setIconAlignment(const Qt::Alignment _flags);
        void setIconOffsets(const int _horOffset, const int _verOffset);

        void setBackgroundColor(const QColor& _color);
        void setTextColor(const QColor& _color);
        void setHoveredTextColor(const QColor& _color);
        void setPressedTextColor(const QColor& _color);

        void setDefaultColor(const QColor& _color);
        void setHoverColor(const QColor& _color);
        void setActiveColor(const QColor& _color);
        void setDisabledColor(const QColor& _color);
        void setPressedColor(const QColor& _color);

        void addOverlayPixmap(const QPixmap& _pixmap, QPoint _pos);

        void setDefaultImage(const QString& _svgPath,  const QColor& _color = QColor(), const QSize& _size = QSize());
        void setHoverImage(const QString& _svgPath,    const QColor& _color = QColor(), const QSize& _size = QSize());
        void setActiveImage(const QString& _svgPath,   const QColor& _color = QColor(), const QSize& _size = QSize());
        void setDisabledImage(const QString& _svgPath, const QColor& _color = QColor(), const QSize& _size = QSize());
        void setPressedImage(const QString& _svgPath,  const QColor& _color = QColor(), const QSize& _size = QSize());

        void setCustomToolTip(const QString& _toopTip);
        const QString& getCustomToolTip() const;

        QSize sizeHint() const override;

        void setShape(ButtonShape _shape);

        void setTextAlignment(const Qt::Alignment _flags);
        void setTextLeftOffset(const int _offset);

        void setFocusColor(const QColor& _color);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void resizeEvent(QResizeEvent*_event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;

    private:
        QPixmap getPixmap(const QString& _path, const QColor& _color, const QSize& _size = QSize()) const;
        void setBackgroundRound();

        void onTooltipTimer();
        void showToolTip();
        void hideToolTip();

        void animateFocusIn();
        void animateFocusOut();

    private:
        QPixmap pixmapToDraw_;
        QPixmap pixmapDefault_;
        QPixmap pixmapHover_;
        QPixmap pixmapActive_;
        QPixmap pixmapDisabled_;
        QPixmap pixmapPressed_;
        QPixmap pixmapOverlay_;

        QColor bgColor_;
        QColor textColor_;
        QColor textColorHovered_;
        QColor textColorPressed_;
        Qt::Alignment textAlign_;
        int textOffsetLeft_;

        QString svgPath_;
        QSize svgSize_;

        QString toolTip_;

        int offsetHor_;
        int offsetVer_;
        Qt::Alignment align_;
        QPoint overlayCenterPos_;

        QTimer tooltipTimer_;
        bool enableTooltip_;

        bool active_;
        bool hovered_;
        bool pressed_;

        QRect bgRect_;
        ButtonShape shape_;

        QColor focusColor_;
        anim::Animation animFocus_;
    };
}


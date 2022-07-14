#pragma once

#include "TextUnit.h"
#include "ClickWidget.h"
#include "utils/SvgUtils.h"

namespace Ui
{
    enum class ButtonShape
    {
        RECTANGLE,
        CIRCLE,
        ROUNDED_RECTANGLE
    };

    class CustomButton : public QAbstractButton
    {
        Q_OBJECT

    Q_SIGNALS:
        void clickedWithButtons(const Qt::MouseButtons &mouseButtons);
        void changedHover(bool _hover);
        void changedPress(bool _press);

    public:
        explicit CustomButton(QWidget* _parent, const QString& _svgName = QString(), const QSize& _iconSize = QSize(), const Styling::ColorParameter& _defaultColor = Styling::ColorParameter());

        void setActive(const bool _isActive);
        bool isActive() const;

        void setSpinning(bool _on);
        bool isSpinning() const;

        bool isHovered() const;
        bool isPressed() const;

        void setIconAlignment(const Qt::Alignment _flags);
        void setIconOffsets(const int _horOffset, const int _verOffset);

        void setBackgroundNormal(const Styling::ColorParameter& _color);
        void setBackgroundDisabled(const Styling::ColorParameter& _color);
        void setBackgroundHovered(const Styling::ColorParameter& _color);
        void setBackgroundPressed(const Styling::ColorParameter& _color);

        void setTextColor(const Styling::ColorParameter& _color);
        void setNormalTextColor(const Styling::ColorParameter& _color);
        void setHoveredTextColor(const Styling::ColorParameter& _color);
        void setPressedTextColor(const Styling::ColorParameter& _color);

        void setDefaultColor(const Styling::ColorParameter& _color);
        void setHoverColor(const Styling::ColorParameter& _color);
        void setActiveColor(const Styling::ColorParameter& _color);
        void setDisabledColor(const Styling::ColorParameter& _color);
        void setPressedColor(const Styling::ColorParameter& _color);

        void addOverlayPixmap(const QPixmap& _pixmap, QPoint _pos);

        void setDefaultImage(const QString& _svgPath,  const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void setHoverImage(const QString& _svgPath,    const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void setActiveImage(const QString& _svgPath,   const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void setDisabledImage(const QString& _svgPath, const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void setPressedImage(const QString& _svgPath,  const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void setSpinnerImage(const QString& _svgPath,  const Styling::ColorParameter& _color = Styling::ColorParameter{}, const QSize& _size = QSize());
        void clearIcon();

        void setCustomToolTip(const QString& _toopTip);
        const QString& getCustomToolTip() const;

        QSize sizeHint() const override;

        void setShape(ButtonShape _shape);

        void setTextAlignment(const Qt::Alignment _flags);
        void setTextLeftOffset(const int _offset);

        void forceHover(bool _force);
        void setFocusColor(const Styling::ColorParameter& _color);

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
        Utils::StyledPixmap getPixmap(const QString& _path, const Styling::ColorParameter& _color, const QSize& _size = QSize()) const;
        void setBackgroundRound();

        void onTooltipTimer();
        void showToolTip();
        void hideToolTip();

        void animateFocusIn();
        void animateFocusOut();

    private:
        QPixmap pixmapToDraw_;
        Utils::StyledPixmap pixmapDefault_;
        Utils::StyledPixmap pixmapHover_;
        Utils::StyledPixmap pixmapActive_;
        Utils::StyledPixmap pixmapDisabled_;
        Utils::StyledPixmap pixmapPressed_;
        Utils::StyledPixmap pixmapSpinner_;
        QPixmap pixmapOverlay_;

        Styling::ColorContainer backgroundBrushNormal_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer backgroundBrushDisabled_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer backgroundBrushHovered_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer backgroundBrushPressed_ = Styling::ColorContainer{ Qt::transparent };

        Styling::ColorContainer textColor_;
        Styling::ColorParameter textColorNormal_;
        Styling::ColorParameter textColorHovered_;
        Styling::ColorParameter textColorPressed_;
        Qt::Alignment textAlign_;
        int textOffsetLeft_;
        int spinAngle_;

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
        bool forceHover_;

        QRect bgRect_;
        ButtonShape shape_;

        Styling::ColorContainer focusColor_;
        QVariantAnimation* animFocus_;
        QTimer* spinTimer_;
    };

    class RoundButton : public ClickableWidget
    {
    public:
        RoundButton(QWidget* _parent, int _radius = 0);

        void setColors(const Styling::ThemeColorKey& _bgNormal, const Styling::ThemeColorKey& _bgHover = {}, const Styling::ThemeColorKey& _bgActive = {});

        void setTextColor(const Styling::ThemeColorKey& _color);
        void setText(const QString& _text, int _size = 12);
        void setIcon(const QString& _iconPath, int _size = 0);
        void setIcon(QStringView _iconPath, int _size = 0)
        {
            setIcon(_iconPath.toString(), _size);
        }
        void setIcon(const Utils::StyledPixmap& _icon);

        void forceHover(bool _force);

        int textDesiredWidth() const;

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent* _event) override;
        QSize getTextSize() const;
        int getRadius() const;

    private:
        QColor getBgColor() const;

    private:
        Utils::StyledPixmap icon_;
        TextRendering::TextUnitPtr text_;
        QPainterPath bubblePath_;

        mutable Styling::ColorContainer bgNormal_;
        mutable Styling::ColorContainer bgHover_;
        mutable Styling::ColorContainer bgActive_;
        Styling::ThemeColorKey textColor_;

        int radius_;
        bool forceHover_ = false;
    };
}


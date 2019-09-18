#include "stdafx.h"
#include "LabelEx.h"

namespace Ui
{
    LabelEx::LabelEx(QWidget* _parent)
        : QLabel(_parent)
        , elideMode_(Qt::ElideNone)
        , hovered_(false)
        , pressed_(false)
    {

    }

    void LabelEx::paintEvent(QPaintEvent* _event)
    {
        if (elideMode_ == Qt::ElideNone)
            return QLabel::paintEvent(_event);

        updateCachedTexts();
        QLabel::setText(cachedElidedText_);
        QLabel::paintEvent(_event);
        QLabel::setText(cachedText_);
    }

    void LabelEx::resizeEvent(QResizeEvent* _event)
    {
        QLabel::resizeEvent(_event);
        cachedText_.clear();
    }

    void LabelEx::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            pressed_ = false;
            updateColor();
            if (rect().contains(_event->pos()) && rect().contains(pos_))
                emit clicked();
        }
        QLabel::mouseReleaseEvent(_event);
    }

    void LabelEx::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            pos_ = _event->pos();
            pressed_ = true;
            updateColor();
        }
        QLabel::mousePressEvent(_event);
    }

    void LabelEx::enterEvent(QEvent* _event)
    {
        hovered_ = true;
        updateColor();
        QLabel::enterEvent(_event);
    }

    void LabelEx::leaveEvent(QEvent* _event)
    {
        hovered_ = false;
        updateColor();
        QLabel::leaveEvent(_event);
    }

    void LabelEx::setColor(const QColor& _color)
    {
        QPalette pal;
        pal.setColor(QPalette::Foreground, _color);
        setPalette(pal);
    }

    void LabelEx::setColors(const QColor& _normalColor, const QColor& _hoverColor, const QColor& _activeColor)
    {
        normalColor_ = _normalColor;
        hoverColor_ = _hoverColor;
        activeColor_ = _activeColor;

        setColor(_normalColor);
    }

    void LabelEx::setElideMode(Qt::TextElideMode _mode)
    {
        elideMode_ = _mode;
        cachedText_.clear();
        update();
    }

    Qt::TextElideMode LabelEx::elideMode() const
    {
        return elideMode_;
    }

    void LabelEx::updateColor()
    {
        if (pressed_ && activeColor_.isValid())
            setColor(activeColor_);
        else if (hovered_ && hoverColor_.isValid())
            setColor(hoverColor_);
        else if (normalColor_.isValid())
            setColor(normalColor_);
    }

    void LabelEx::updateCachedTexts()
    {
        const auto txt = text();
        if (cachedText_ == txt)
            return;

        cachedText_ = txt;
        const QFontMetrics fm(fontMetrics());
        cachedElidedText_ = fm.elidedText(txt, elideMode_, width(), Qt::TextShowMnemonic);
    }
}

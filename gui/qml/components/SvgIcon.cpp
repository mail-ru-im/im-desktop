#include "stdafx.h"
#include "SvgIcon.h"

#include "../../utils/utils.h"

SvgIcon::SvgIcon(QQuickItem* _parent)
    : QQuickPaintedItem(_parent)
{
    connect(this, &QQuickItem::widthChanged, this, &SvgIcon::updatePixmap);
    connect(this, &QQuickItem::heightChanged, this, &SvgIcon::updatePixmap);
    connect(this, &SvgIcon::colorChanged, this, &SvgIcon::updatePixmap);
    connect(this, &SvgIcon::iconPathChanged, this, &SvgIcon::updatePixmap);
}

QString SvgIcon::iconPath() const
{
    return iconPath_;
}

void SvgIcon::paint(QPainter* _painter)
{
    if (Q_LIKELY(!pixmap_.isNull()))
        _painter->drawPixmap(0, 0, pixmap_);
}

void SvgIcon::setIconPath(const QString& _path)
{
    if (_path != iconPath_)
    {
        iconPath_ = _path;
        Q_EMIT iconPathChanged();
    }
}

QColor SvgIcon::color() const
{
    return color_;
}

void SvgIcon::setColor(const QColor& _color)
{
    if (_color != color_)
    {
        color_ = _color;
        Q_EMIT colorChanged();
    }
}

void SvgIcon::updatePixmap()
{
    pixmap_ = Utils::renderSvg(iconPath_, size().toSize(), color_);
    update();
}

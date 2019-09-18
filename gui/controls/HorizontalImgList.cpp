#include "HorizontalImgList.h"

#include <QPainter>
#include <QDebug>
#include "utils/utils.h"

namespace Ui
{

HorizontalImgList::HorizontalImgList(const Options &_options,
                                     QWidget *_parent)
    : QWidget(_parent),
      options_(_options)
{
    setMouseTracking(true);
    setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

    connect(this, &HorizontalImgList::itemHovered,
            this, &HorizontalImgList::onItemHovered);

    connect(this, &HorizontalImgList::itemClicked,
            this, &HorizontalImgList::onItemClicked);
}

void HorizontalImgList::setIdenticalItems(const ImageInfo& _info, int _count)
{
    items_.clear();
    states_.clear();

    items_.reserve(_count);
    states_.reserve(_count);

    for (auto i = 0; i < _count; ++i)
    {
        items_.push_back(ImageInfo(_info.size_, _info.defaultImage_, _info.highlightedImage_));
        states_.push_back(ImageState());
    }

    setFixedHeight(_info.size_.height());
}

void HorizontalImgList::setItemOffsets(const QMargins &_margins)
{
    itemOffsets_ = _margins;
}

int HorizontalImgList::itemsCount() const
{
    return items_.count();
}

QSize HorizontalImgList::sizeHint() const
{
    if (!items_.empty())
    {
        auto width = 0;
        auto height = 0;
        for (const auto& item: items_)
        {
            width += item.size_.width();
            width += itemOffsets_.right();

            height = std::max(height,item.size_.height());
        }

        return { width, height };
    }

    return QWidget::sizeHint();
}

void HorizontalImgList::paintEvent(QPaintEvent *_event)
{
    QWidget::paintEvent(_event);

    // draw stuff
    QPainter p(this);
    int x = 0, y = 0;

    for (int i = 0; i < items_.size(); ++i)
    {
        const auto &item = items_[i];

        states_[i].currentXPos_ = x;
        states_[i].currentWidth_ = item.size_.width();
        states_[i].currentRightOffset_ = itemOffsets_.right();

        p.drawPixmap(x, y, item.size_.width(), item.size_.height(),
                     states_[i].highlighted_ ? item.highlightedImage_ : item.defaultImage_);

        x += item.size_.width();
        x += itemOffsets_.right();
    }
}

void HorizontalImgList::mouseMoveEvent(QMouseEvent *_event)
{
     auto index = itemIndexForXPos(_event->pos().x());
     if (index != -1)
     {
         emit itemHovered(index);
     }

     QWidget::mouseMoveEvent(_event);
}

void HorizontalImgList::mouseReleaseEvent(QMouseEvent *_event)
{
    auto index = itemIndexForXPos(_event->pos().x());
    if (index != -1)
        emit itemClicked(index);

    QWidget::mouseReleaseEvent(_event);
}

void HorizontalImgList::leaveEvent(QEvent *)
{
    emit mouseLeft();

    if (options_.rememberClickedHighlight_ && options_.hlClicked_ != -1)
    {
        highlightUserSelection(options_.hlClicked_);
        return;
    }

    for (auto &state: states_)
        state.highlighted_ = false;

    update();
}

void HorizontalImgList::onItemHovered(int _index)
{
    if (!options_.highlightOnHover_ || _index == -1)
        return;

    if (options_.rememberClickedHighlight_
            && options_.hlClicked_ != -1
            && options_.hlClicked_ >= _index)
        return;

    for (int i = 0; i < states_.size(); ++i)
        states_[i].highlighted_ = (_index >= options_.hlOnHoverHigherThan_) && (i <= _index);

    update();
}

void HorizontalImgList::onItemClicked(int _index)
{
    if (!options_.rememberClickedHighlight_)
        return;

    options_.hlClicked_ = _index;

    highlightUserSelection(_index);
}

int HorizontalImgList::itemIndexForXPos(int _x)
{
    for (int i = 0; i < states_.size(); ++i)
    {
        if (states_[i].hasX(_x))
            return i;
    }

    return -1;
}

void HorizontalImgList::highlightUserSelection(int _index)
{
    for (int i = 0; i < states_.size(); ++i)
        states_[i].highlighted_ = (i <= _index);

    update();
}

// ImageState
bool HorizontalImgList::ImageState::hasX(int _x) const
{
    if (_x == -1 || currentXPos_ == -1 || currentWidth_ == -1)
        return false;

    return _x >= currentXPos_ && _x <= (currentXPos_ + currentWidth_ + currentRightOffset_);
}



}

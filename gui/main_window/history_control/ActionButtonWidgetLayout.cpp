#include "stdafx.h"

#include "../../utils/utils.h"

#include "ActionButtonWidget.h"
#include "MessageStyle.h"

#include "ActionButtonWidgetLayout.h"

UI_NS_BEGIN

ActionButtonWidgetLayout::ActionButtonWidgetLayout(ActionButtonWidget *parent)
    : QLayout(parent)
    , Item_(parent)
    , ProgressTextSize_(0, 0)
{
    assert(Item_);
}

void ActionButtonWidgetLayout::setGeometry(const QRect&)
{
    setGeometryInternal();
}

void ActionButtonWidgetLayout::addItem(QLayoutItem* /*item*/)
{
}

const QRect& ActionButtonWidgetLayout::getIconRect() const
{
    return IconRect_;
}

const QRect& ActionButtonWidgetLayout::getProgressRect() const
{
    return ProgressRect_;
}

const QRect& ActionButtonWidgetLayout::getProgressTextRect() const
{
    return ProgressTextRect_;
}

QLayoutItem* ActionButtonWidgetLayout::itemAt(int /*index*/) const
{
    return nullptr;
}

QLayoutItem* ActionButtonWidgetLayout::takeAt(int /*index*/)
{
    return nullptr;
}

int ActionButtonWidgetLayout::count() const
{
    return 0;
}

QSize ActionButtonWidgetLayout::sizeHint() const
{
    return ContentRect_.size();
}

void ActionButtonWidgetLayout::invalidate()
{
    QLayoutItem::invalidate();
}

QRect ActionButtonWidgetLayout::evaluateContentRect(const QSize &iconSize, const QSize &progressTextSize) const
{
    assert(!iconSize.isEmpty());

    auto contentRectWidth = std::max(iconSize.width(), progressTextSize.width() + 2 * MessageStyle::getProgressTextRectHMargin()) + 1;

    auto contentRectHeight = iconSize.height();

    const auto hasProgressText = !progressTextSize.isEmpty();
    if (hasProgressText)
    {
        contentRectHeight += MessageStyle::getRotatingProgressBarTextTopMargin();
        contentRectHeight += progressTextSize.height();
        contentRectHeight += MessageStyle::getProgressTextRectVMargin();
    }

    const QRect result(0, 0, contentRectWidth, contentRectHeight);

    return result;
}

QRect ActionButtonWidgetLayout::evaluateIconRect(const QRect &contentRect, const QSize &iconSize) const
{
    assert(Item_);
    assert(!iconSize.isEmpty());

    QRect result(QPoint(), iconSize);

    const auto centerX = contentRect.center().x();

    const auto centerY = result.center().y();

    result.moveCenter(QPoint(centerX, centerY));

    return result;
}

QRect ActionButtonWidgetLayout::evaluateProgressRect(const QRect &iconRect) const
{
    assert(!iconRect.isEmpty());

    auto result(iconRect);

    const auto progressBarWidth = MessageStyle::getRotatingProgressBarPenWidth();
    const auto progressBarMargin = (progressBarWidth / 2);

    const QMargins progressBarMargins(progressBarMargin, progressBarMargin, progressBarMargin, progressBarMargin);
    result = result.marginsRemoved(progressBarMargins);

    return result;
}

QRect ActionButtonWidgetLayout::evaluateProgressTextRect(const QRect &iconRect, const QSize &progressTextSize) const
{
    assert(!iconRect.isEmpty());

    auto textX = iconRect.center().x() - progressTextSize.width() / 2;

    assert(textX >= 0);

    auto textY = (iconRect.bottom() + 1);
    textY += MessageStyle::getRotatingProgressBarTextTopMargin();

    QRect result(QPoint(textX, textY), progressTextSize);

    return result;
}

void ActionButtonWidgetLayout::setGeometryInternal()
{
    assert(Item_);

    const auto &progressText = Item_->getProgressText();
    if (progressText != LastProgressText_)
    {
        ProgressTextSize_ = evaluateProgressTextSize(progressText);
        LastProgressText_ = progressText;
    }

    const auto &iconSize = Item_->getIconSize();
    assert(!iconSize.isEmpty());

    ContentRect_ = evaluateContentRect(iconSize, ProgressTextSize_);

    IconRect_ = evaluateIconRect(ContentRect_, iconSize);
    assert(!IconRect_.isEmpty());
    assert(ContentRect_.contains(IconRect_));

    ProgressRect_ = evaluateProgressRect(IconRect_);

    ProgressTextRect_ = evaluateProgressTextRect(IconRect_, ProgressTextSize_);
}

QSize ActionButtonWidgetLayout::evaluateProgressTextSize(const QString& progressText) const
{
    assert(Item_);

    if (progressText.isEmpty())
    {
        return QSize(0, 0);
    }

    const auto &font = Item_->getProgressTextFont();

    QFontMetrics m(font);

    auto textSize = m.boundingRect(progressText).size();

    // a workaround for the text width evaluation issues
    textSize.rwidth() *= 110;
    textSize.rwidth() /= 100;

    return textSize;
}

UI_NS_END

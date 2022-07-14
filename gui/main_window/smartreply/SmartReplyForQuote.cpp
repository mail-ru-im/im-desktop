#include "stdafx.h"

#include "SmartReplyForQuote.h"
#include "SmartReplyWidget.h"

#include "controls/TooltipWidget.h"
#include "types/smartreply_suggest.h"
#include "utils/utils.h"

namespace
{
    QMargins getContentMargins()
    {
        return Utils::scale_value(QMargins(8, 4,8, 4));
    }
}

namespace Ui
{
    SmartReplyForQuote::SmartReplyForQuote(QWidget* _parent)
        : QWidget(_parent)
        , widget_(new SmartReplyWidget(this, false))
    {
        tooltip_ = new TooltipWidget(_parent, widget_, Utils::scale_value(44), getContentMargins(), TooltipCompopent::All);
        tooltip_->setCursor(Qt::PointingHandCursor);

        connect(widget_, &SmartReplyWidget::needHide, this, &SmartReplyForQuote::hideAnimated);
        connect(widget_, &SmartReplyWidget::sendSmartreply, this, [this](const auto& _suggest)
        {
            Q_EMIT sendSmartreply(_suggest, QPrivateSignal());
            hideAnimated();
        });
    }

    void SmartReplyForQuote::clear()
    {
        widget_->clearItems();
    }

    void SmartReplyForQuote::setSmartReplies(const std::vector<Data::SmartreplySuggest> &_suggests)
    {
        widget_->addItems(_suggests);
        widget_->setFixedWidth(widget_->getTotalWidth());
    }

    void SmartReplyForQuote::showAnimated(const QPoint& _p, const QSize& _maxSize, const QRect& _rect)
    {
        widget_->setEnabled(true);
        tooltip_->showAnimated(_p, _maxSize, _rect);
    }

    void SmartReplyForQuote::hideAnimated()
    {
        tooltip_->hideAnimated();
    }

    void SmartReplyForQuote::hideForce()
    {
        tooltip_->hide();
    }

    bool SmartReplyForQuote::isTooltipVisible() const
    {
        return tooltip_->isVisible();
    }

    void SmartReplyForQuote::adjustTooltip(const QPoint& _p, const QSize& _maxSize, const QRect& _rect)
    {
        tooltip_->showUsual(_p, _maxSize, _rect);
    }
}
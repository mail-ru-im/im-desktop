#include "stdafx.h"

#include "SmartReplyWidget.h"
#include "SmartReplyItem.h"
#include "ScrollButton.h"

#include "main_window/contact_list/ContactListModel.h"
#include "main_window/smiles_menu/StickerPreview.h"
#include "main_window/MainWindow.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/graphicsEffects.h"
#include "utils/InterConnector.h"
#include "types/smartreply_suggest.h"
#include "core_dispatcher.h"

#include "../../../common.shared/smartreply/smartreply_types.h"

#include <boost/range/adaptor/reversed.hpp>

namespace
{
    int itemSpacing() { return Utils::scale_value(4); }
    int smartreplyHeight() { return Utils::scale_value(62); }

    int edgeGradientWidth() { return Utils::scale_value(20); }
    int edgeGradientOffset() { return Utils::scale_value(32); }

    int edgeMargin() { return Utils::scale_value(52) - itemSpacing(); }
    int arrowMargin() { return Utils::scale_value(10); }

    constexpr std::chrono::milliseconds scrollAnimDuration() { return std::chrono::milliseconds(100); }
}

namespace Ui
{
    SmartReplyWidget::SmartReplyWidget(QWidget* _parent)
        : QWidget(_parent)
        , scrollLeft_(new ScrollButton(this, ScrollButton::ButtonDirection::left))
        , scrollRight_(new ScrollButton(this, ScrollButton::ButtonDirection::right))
        , animScroll_(nullptr)
        , stickerPreview_(nullptr)
        , isFlat_(false)
        , bgTransparent_(false)
        , isUnderMouse_(false)
        , needHideOnLeave_(false)
    {
        setFixedHeight(smartreplyHeight());

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setStyleSheet(qsl("background: transparent;"));
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setFocusPolicy(Qt::NoFocus);
        scrollArea_->setFrameShape(QFrame::NoFrame);

        contentWidget_ = new QWidget(this);
        auto contentLayout = Utils::emptyHLayout(contentWidget_);
        contentLayout->setAlignment(Qt::AlignLeft);
        contentLayout->setSpacing(itemSpacing());

        {
            auto fixLeft = new QWidget(this);
            fixLeft->setFixedWidth(edgeMargin());
            contentLayout->addWidget(fixLeft);

            auto fixRight = new QWidget(this);
            fixRight->setFixedWidth(edgeMargin());
            contentLayout->addWidget(fixRight);
        }

        scrollArea_->setWidget(contentWidget_);

        Utils::grabTouchWidget(scrollArea_->viewport(), true);
        Utils::grabTouchWidget(contentWidget_);

        scrollLeft_->raise();
        scrollRight_->raise();

        connect(scrollLeft_, &ScrollButton::clicked, this, &SmartReplyWidget::scrollToLeft);
        connect(scrollRight_, &ScrollButton::clicked, this, &SmartReplyWidget::scrollToRight);

        connect(scrollArea_->horizontalScrollBar(), &QScrollBar::valueChanged, this, &SmartReplyWidget::updateEdges);
        connect(scrollArea_->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &SmartReplyWidget::updateEdges);

        afterClickTimer_.setSingleShot(true);
        afterClickTimer_.setInterval(Features::smartreplyHideTime().count());
        connect(&afterClickTimer_, &QTimer::timeout, this, &SmartReplyWidget::onAfterClickTimer);
    }

    bool SmartReplyWidget::addItems(const std::vector<Data::SmartreplySuggest>& _suggests)
    {
        if (_suggests.empty())
            return false;

        if (const auto items = getItems(); !items.empty())
        {
            const auto already = std::all_of(_suggests.begin(), _suggests.end(), [&items](const auto& s)
            {
                return std::any_of(items.begin(), items.end(), [&s](const auto item) { return item->getSuggest() == s; });
            });

            if (already)
                return false;

            for (auto item : items)
            {
                if (item->getSuggest().getType() == _suggests.front().getType() && item->getMsgId() == _suggests.front().getMsgId())
                    deleteItem(item);
            }

            for (const auto& s : _suggests)
                deleteItemBySuggest(s);
        }

        bool added = false;
        setUpdatesEnabled(false);
        for (const auto& s : _suggests)
        {
            if (const auto exItems = getItems(); added && std::any_of(exItems.begin(), exItems.end(), [&s](const auto item) { return item->getSuggest() == s; }))
                continue;

            if (auto item = makeSmartReplyItem(s))
                added |= addItem(std::move(item));
        }
        setUpdatesEnabled(true);

        return added;
    }

    bool SmartReplyWidget::addItem(std::unique_ptr<SmartReplyItem>&& _newItem)
    {
        if (!_newItem)
            return false;

        auto itemsLayout = qobject_cast<QBoxLayout*>(contentWidget_->layout());
        if (!itemsLayout)
            return false;

        const auto items = getItems();
        const auto i = std::count_if(items.crbegin(), items.crend(), [id = _newItem->getMsgId()](const auto item) { return item->getMsgId() >= id; });

        auto item = _newItem.release();
        itemsLayout->insertWidget(i + 1, item);

        item->setFlat(isFlat());
        item->setUnderlayVisible(bgTransparent_);
        connect(item, &SmartReplyItem::selected, this, &SmartReplyWidget::onItemSelected);
        connect(item, &SmartReplyItem::showPreview, this, &SmartReplyWidget::showStickerPreview);
        connect(item, &SmartReplyItem::hidePreview, this, &SmartReplyWidget::hideStickerPreview);
        connect(item, &SmartReplyItem::mouseMoved, this, &SmartReplyWidget::onItemsMouseMoved);
        connect(item, &SmartReplyItem::deleteMe, this, [this]()
        {
            deleteItem(qobject_cast<QWidget*>(sender()));
            recalcGeometry();
            updateEdges();
        });

        clearOldItems(item->getMsgId());

        updateItemMargins();
        scrollToNewest();

        updateEdges();

        return true;
    }

    void SmartReplyWidget::clearItems()
    {
        clearItemsEqLessThan(std::numeric_limits<int64_t>::max());
    }

    void SmartReplyWidget::clearDeletedItems()
    {
        const auto items = getItems();
        for (auto item : items)
        {
            if (item->isDeleted())
                deleteItem(item);
        }

        recalcGeometry();
        updateEdges();
    }

    void SmartReplyWidget::markItemsToDeletion()
    {
        const auto items = getItems();
        for (auto item : items)
            item->setIsDeleted(true);
    }

    void SmartReplyWidget::clearItemsForMessage(const qint64 _msgId)
    {
        const auto items = getItems();
        for (auto item : items)
        {
            if (item->getMsgId() == _msgId)
                deleteItem(item);
        }

        recalcGeometry();
        updateEdges();
    }

    void SmartReplyWidget::setBackgroundColor(const QColor& _color)
    {
        bgTransparent_ = !_color.isValid() || _color.alpha() == 0;
        scrollLeft_->setBackgroundVisible(bgTransparent_);
        scrollRight_->setBackgroundVisible(bgTransparent_);

        const auto items = getItems();
        for (auto item : items)
            item->setUnderlayVisible(bgTransparent_);

        updateEdges();
    }

    void SmartReplyWidget::scrollToNewest()
    {
        scrollArea_->horizontalScrollBar()->setValue(0);
    }

    void SmartReplyWidget::setFlat(const bool _flat)
    {
        if (isFlat_ != _flat)
        {
            isFlat_ = _flat;

            const auto items = getItems();
            for (auto item : items)
                item->setFlat(_flat);

            update();
        }
    }

    void SmartReplyWidget::setNeedHideOnLeave()
    {
        needHideOnLeave_ = true;
    }

    void SmartReplyWidget::showStickerPreview(const QString& _stickerId)
    {
        if (!stickerPreview_)
        {
            auto mwWidget = Utils::InterConnector::instance().getMainWindow()->getWidget();
            stickerPreview_ = new Smiles::StickerPreview(mwWidget, -1, _stickerId, Smiles::StickerPreview::Context::Popup);
            stickerPreview_->setGeometry(mwWidget->rect());
            stickerPreview_->show();
            stickerPreview_->raise();

            connect(stickerPreview_, &Smiles::StickerPreview::needClose, this, &SmartReplyWidget::hideStickerPreview, Qt::QueuedConnection); // !!! must be Qt::QueuedConnection

            const auto items = getItems();
            for (auto item : items)
                item->setIsPreviewVisible(true);
        }
        else
        {
            stickerPreview_->showSticker(-1, _stickerId);
        }
    }

    void SmartReplyWidget::hideStickerPreview()
    {
        if (!stickerPreview_)
            return;

        stickerPreview_->hide();
        stickerPreview_->deleteLater();
        stickerPreview_ = nullptr;

        const auto items = getItems();
        for (auto item : items)
            item->setIsPreviewVisible(false);
    }

    void SmartReplyWidget::onItemsMouseMoved(QPoint _pos)
    {
        const auto items = getItems();
        for (auto item : items)
        {
            if (item->getSuggest().isStickerType())
            {
                const auto globalPos = contentWidget_->mapToGlobal(item->pos());
                const auto globalRect = item->rect().translated(globalPos);
                if (globalRect.contains(_pos))
                {
                    showStickerPreview(item->getSuggest().getData());
                    break;
                }
            }
        }
    }

    void SmartReplyWidget::resizeEvent(QResizeEvent*)
    {
        recalcGeometry();
        updateEdges();
    }

    void SmartReplyWidget::enterEvent(QEvent*)
    {
        afterClickTimer_.stop();
        isUnderMouse_ = true;
    }

    void SmartReplyWidget::leaveEvent(QEvent*)
    {
        if (needHideOnLeave_)
            afterClickTimer_.start();
        isUnderMouse_ = false;
    }

    void SmartReplyWidget::wheelEvent(QWheelEvent* _e)
    {
        if (contentWidget_->width() <= scrollArea_->width())
            return;

        _e->accept();

        int numDegrees = _e->delta() / 8;
        if constexpr (!platform::is_windows())
            numDegrees /= 15;

        if (!numDegrees)
            return;

        if (numDegrees > 0)
            scrollStep(Direction::left);
        else
            scrollStep(Direction::right);
    }

    void SmartReplyWidget::hideEvent(QHideEvent*)
    {
        isUnderMouse_ = false;
        setEnabled(true);
    }

    QList<SmartReplyItem*> SmartReplyWidget::getItems() const
    {
        return contentWidget_->findChildren<SmartReplyItem*>();
    }

    void SmartReplyWidget::onItemSelected(const Data::SmartreplySuggest& _suggest)
    {
        const auto contact = Logic::getContactListModel()->selectedContact();
        if (contact.isEmpty())
            return;

        setEnabled(false);

        switch (_suggest.getType())
        {
        case core::smartreply::type::sticker:
        case core::smartreply::type::sticker_by_text:
            Ui::GetDispatcher()->sendStickerToContact(contact, _suggest.getData());
            break;

        case core::smartreply::type::text:
            Ui::GetDispatcher()->sendMessageToContact(contact, _suggest.getData());
            break;

        default:
            break;
        }

        sendStats(_suggest);

        emit Utils::InterConnector::instance().setFocusOnInput();

        afterClickTimer_.start();
    }

    void SmartReplyWidget::onAfterClickTimer()
    {
        needHideOnLeave_ = false;

        emit needHide(QPrivateSignal());
    }

    void SmartReplyWidget::scrollToRight()
    {
        if (contentWidget_->width() <= width())
            return;

        scrollStep(Direction::right);
    }

    void SmartReplyWidget::scrollToLeft()
    {
        if (contentWidget_->width() <= width())
            return;

        scrollStep(Direction::left);
    }

    void SmartReplyWidget::recalcGeometry()
    {
        scrollLeft_->move(arrowMargin(), (height() - scrollLeft_->height()) / 2);
        scrollRight_->move(width() - arrowMargin() - scrollRight_->width(), (height() - scrollRight_->height()) / 2);

        const auto items = getItems();
        const int itemsWidth = std::accumulate(items.begin(), items.end(), 0, [](int _sum, const auto& _item) { return _sum + _item->width(); });
        contentWidget_->setGeometry(0, 0, edgeMargin() * 2 + itemsWidth + (items.size() + 1) * itemSpacing(), height());

        scrollArea_->setGeometry(rect());

        updateEdgeGradients();
    }

    void SmartReplyWidget::updateEdges()
    {
        if (contentWidget_->width() > width())
        {
            auto bar = scrollArea_->horizontalScrollBar();

            const auto atLEdge = bar->value() == 0;
            const auto atREdge = bar->value() == bar->maximum();
            const auto between = !atLEdge && !atREdge;

            scrollLeft_->setVisible(atREdge || between);
            scrollRight_->setVisible(atLEdge || between);
        }
        else
        {
            scrollLeft_->hide();
            scrollRight_->hide();
        }

        updateEdgeGradients();
    }

    void SmartReplyWidget::updateEdgeGradients()
    {
        const auto lVisible = scrollLeft_->isVisibleTo(this);
        const auto rVisible = scrollRight_->isVisibleTo(this);
        if (!lVisible && !rVisible)
        {
            scrollArea_->setGraphicsEffect(nullptr);
            return;
        }

        int sides = 0;

        if (lVisible)
            sides |= Qt::AlignLeft;

        if (rVisible)
            sides |= Qt::AlignRight;

        if (sides)
        {
            if (auto curEffect = qobject_cast<Utils::GradientEdgesEffect*>(scrollArea_->graphicsEffect()))
            {
                if (curEffect->getSides() != sides)
                    curEffect->setSides((Qt::Alignment)sides);
            }
            else
            {
                scrollArea_->setGraphicsEffect(new Utils::GradientEdgesEffect(this, edgeGradientWidth(), (Qt::Alignment)sides, edgeGradientOffset()));
                update();
            }
        }
    }

    void SmartReplyWidget::updateItemMargins()
    {
        const auto items = getItems();
        for (auto i = 0; i < items.size(); ++i)
            items[i]->setHasMargins(i != items.size() - 1, i != 0);

        recalcGeometry();
    }

    void SmartReplyWidget::clearOldItems(const int64_t _newestMsgId)
    {
        int count = 1;
        int64_t lastMsgId = _newestMsgId;

        const auto items = getItems();
        for (auto item : boost::adaptors::reverse(items))
        {
            if (lastMsgId != item->getMsgId())
            {
                count++;
                lastMsgId = item->getMsgId();
            }

            if (count > Features::smartreplyMsgidCacheSize())
                deleteItem(item);
        }
    }

    void SmartReplyWidget::clearItemsEqLessThan(const int64_t _msgId)
    {
        const auto items = getItems();
        for (auto item : items)
        {
            if (item->getMsgId() <= _msgId)
                deleteItem(item);
        }
    }

    void SmartReplyWidget::deleteItem(QWidget* _item)
    {
        if (_item)
        {
            contentWidget_->layout()->removeWidget(_item);
            delete _item;
        }
    }

    void SmartReplyWidget::deleteItemBySuggest(const Data::SmartreplySuggest& _suggest)
    {
        const auto items = getItems();
        const auto it = std::find_if(items.begin(), items.end(), [&_suggest](const auto item) { return item->getSuggest().getType() == _suggest.getType() && item->getSuggest().getData() == _suggest.getData(); });
        if (it != items.end())
            deleteItem(*it);
    }

    bool SmartReplyWidget::isItemVisible(QWidget* _item) const
    {
        if (!_item)
            return false;

        const auto shift = contentWidget_->pos().x();
        const auto viewRect = scrollArea_->viewport()->rect();
        const auto itemRect = _item->rect().translated(_item->pos()).translated(shift, 0);
        return viewRect.intersects(itemRect);
    }

    void SmartReplyWidget::sendStats(const Data::SmartreplySuggest& _suggest)
    {
        using namespace core::stats;

        im_stat_event_names imStatName = im_stat_event_names::min;
        stats_event_names statName = stats_event_names::min;
        event_props_type props;

        if (_suggest.isStickerType())
        {
            props.push_back({ "stickerId", _suggest.getData().toStdString() });
            imStatName = im_stat_event_names::stickers_smartreply_sticker_sent;
            statName = stats_event_names::stickers_smartreply_sticker_sent;
        }
        else if (_suggest.isTextType())
        {
            imStatName = im_stat_event_names::smartreply_text_sent;
            statName = stats_event_names::smartreply_text_sent;
        }

        if (imStatName != im_stat_event_names::min)
        {
            GetDispatcher()->post_im_stats_to_core(imStatName, props);
            GetDispatcher()->post_stats_to_core(statName, props);
        }
    }

    void SmartReplyWidget::scrollStep(const Direction _direction)
    {
        QRect viewRect = scrollArea_->viewport()->geometry();
        auto scrollbar = scrollArea_->horizontalScrollBar();

        const int maxVal = scrollbar->maximum();
        const int minVal = scrollbar->minimum();
        const int curVal = scrollbar->value();

        const int step = viewRect.width() - edgeMargin() * 2;

        int to = 0;

        if (_direction == Direction::right)
            to = std::min(curVal + step, maxVal);
        else
            to = std::max(curVal - step, minVal);

        if (!animScroll_)
        {
            animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
            animScroll_->setEasingCurve(QEasingCurve::InQuad);
            animScroll_->setDuration(scrollAnimDuration().count());
        }

        animScroll_->stop();
        animScroll_->setStartValue(curVal);
        animScroll_->setEndValue(to);
        animScroll_->start();
    }
}
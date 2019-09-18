#include "stdafx.h"
#include "MentionCompleter.h"

#include "../contact_list/MentionItemDelegate.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../styles/ThemeParameters.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/graphicsEffects.h"
#include "../../core_dispatcher.h"
#include "../../controls/TooltipWidget.h"

namespace
{
    constexpr auto maxItemsForMaxHeight = 7;

    constexpr std::chrono::milliseconds getDurationAppear() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds getDurationDisappear() noexcept { return std::chrono::milliseconds(50); }
}

namespace Ui
{
    MentionCompleter::MentionCompleter(QWidget* _parent)
        : QWidget(_parent)
        , model_(Logic::GetMentionModel())
        , delegate_(new Logic::MentionItemDelegate(this))
        , arrowOffset_(Tooltip::getDefaultArrowOffset())
        , opacityEffect_(new Utils::OpacityEffect(this))
    {
        Testing::setAccessibleName(this, qsl("AS mention wdg"));

        view_ = CreateFocusableViewAndSetTrScrollBar(this);
        view_->setParent(this);
        view_->setSelectByMouseHover(true);
        view_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        view_->setFrameShape(QFrame::NoFrame);
        view_->setSpacing(0);
        view_->setModelColumn(0);
        view_->setUniformItemSizes(false);
        view_->setBatchSize(50);
        view_->setStyleSheet(qsl("background: transparent;"));
        view_->setMouseTracking(true);
        view_->setAcceptDrops(false);
        view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        view_->setAutoScroll(false);
        view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        view_->setResizeMode(QListView::Adjust);
        view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        Utils::grabTouchWidget(view_->viewport(), true);
        Utils::ApplyStyle(view_->verticalScrollBar(), Styling::getParameters().getScrollBarQss(4, 0));
        Testing::setAccessibleName(view_, qsl("AS mention view_"));

        view_->setModel(model_);
        view_->setItemDelegate(delegate_);

        connect(view_, &FocusableListView::clicked, this, &MentionCompleter::itemClicked);
        connect(view_, &FocusableListView::clicked, this, [this](const QModelIndex& _current) { sendSelectionStats(_current, statSelectSource::mouseClick); });
        connect(model_, &Logic::MentionModel::results, this, &MentionCompleter::onResults);

        setGraphicsEffect(opacityEffect_);
        updateStyle();
    }

    void MentionCompleter::setDialogAimId(const QString & _aimid)
    {
        model_->setDialogAimId(_aimid);
    }

    bool MentionCompleter::insertCurrent()
    {
        const auto selectedIndex = view_->selectionModel()->currentIndex();
        if (selectedIndex.isValid() && !model_->isServiceItem(selectedIndex))
            return itemClicked(selectedIndex);

        return false;
    }

    void MentionCompleter::hideEvent(QHideEvent*)
    {
        emit hidden();
    }

    void MentionCompleter::keyPressEvent(QKeyEvent * _event)
    {
        _event->ignore();

        if (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)
        {
            const auto pressedUp = _event->key() == Qt::Key_Up;
            keyUpOrDownPressed(pressedUp);
            _event->accept();
        }
        else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Tab)
        {
            const auto selectedIndex = view_->selectionModel()->currentIndex();
            if (insertCurrent())
            {
                sendSelectionStats(selectedIndex, statSelectSource::keyEnter);
                _event->accept();
            }
        }
    }

    void MentionCompleter::paintEvent(QPaintEvent*)
    {
        if (itemCount() > 0)
        {
            QPainter p(this);
            Tooltip::drawBigTooltip(p, rect(), arrowOffset_, Tooltip::ArrowDirection::Down);
        }
    }

    int MentionCompleter::calcHeight() const
    {
        const auto count = itemCount();

        auto newHeight = 0;
        QStyleOptionViewItem unused;
        for (auto i = 0; i < std::min(count, maxItemsForMaxHeight); ++i)
            newHeight += delegate_->sizeHint(unused, model_->index(i)).height();

        if (count > maxItemsForMaxHeight)
            newHeight += delegate_->itemHeight() / 2; // show user there are more items below

        return newHeight;
    }

    void MentionCompleter::sendSelectionStats(const QModelIndex& _current, const statSelectSource _source)
    {
        if (!_current.isValid() || model_->isServiceItem(_current))
            return;

        core::stats::event_props_type props = {
            { "From", _source == statSelectSource::keyEnter ? "Popup_Enter" : "Popup_MouseClick" },
            { "Section", model_->isFromChatSenders(_current) ? "From_Chat" : "Not_From_Chat" },
            { "WithSearch", model_->getSearchPattern().isEmpty() ? "No_Search" : "Search" }
        };

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_inserted, props);
    }

    void MentionCompleter::setSearchPattern(const QString& _pattern, const Logic::MentionModel::SearchCondition _cond)
    {
        model_->setSearchPattern(_pattern, _cond);
    }

    QString MentionCompleter::getSearchPattern() const
    {
        return model_->getSearchPattern();
    }

    void MentionCompleter::onResults()
    {
        recalcHeight();

        if (itemCount())
            view_->selectionModel()->setCurrentIndex(model_->index(0), QItemSelectionModel::ClearAndSelect);
        else
            view_->selectionModel()->clear();

        view_->scrollToTop();

        emit results(itemCount());
    }

    bool MentionCompleter::itemClicked(const QModelIndex& _current)
    {
        if (!model_->isServiceItem(_current))
        {
            auto item = _current.data().value<Logic::MentionSuggest>();
            emit contactSelected(item.aimId_, item.friendlyName_);
            hide();

            return true;
        }
        return false;
    }

    void MentionCompleter::keyUpOrDownPressed(const bool _isUpPressed)
    {
        auto inc = _isUpPressed ? -1 : 1;

        auto prevIndex = view_->selectionModel()->currentIndex();
        QModelIndex i = model_->index(prevIndex.row() + inc);

        while (model_->isServiceItem(i))
        {
            i = model_->index(i.row() + inc);
            if (!i.isValid())
                return;
        }

        {
            QSignalBlocker sb(view_->selectionModel());
            view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        }

        view_->update(prevIndex);
        view_->update(i);
        view_->scrollTo(i);
    }

    void MentionCompleter::recalcHeight()
    {
        const auto curBot = y() + height();
        const auto calcedHeight = std::min(calcHeight(), Tooltip::getMaxMentionTooltipHeight());
        const auto viewHeight = itemCount() < maxItemsForMaxHeight ? calcedHeight + delegate_->itemHeight() : calcedHeight - delegate_->itemHeight()/2;

        const auto addH = itemCount() < maxItemsForMaxHeight ? 2 * Tooltip::getShadowSize() : 0;
        setFixedHeight(calcedHeight + addH + Tooltip::getArrowHeight());

        view_->setFixedSize(width() - 2 * Tooltip::getShadowSize(), viewHeight);
        update();
        move((parentWidget()->width() - width())/2, curBot - height());
        view_->move(Tooltip::getShadowSize(), Tooltip::getShadowSize());

        {
            const auto radius = Tooltip::getCornerRadiusBig();
            QPainterPath path;
            path.addRoundedRect(view_->rect(), radius, radius);
            QRegion region(path.toFillPolygon().toPolygon());
            view_->setMask(region);
        }
    }

    bool MentionCompleter::completerVisible() const
    {
        return isVisible() && height() > 0;
    }

    int MentionCompleter::itemCount() const
    {
        return model_->rowCount();
    }

    bool MentionCompleter::hasSelectedItem() const
    {
        return view_->selectionModel()->hasSelection();
    }

    void MentionCompleter::updateStyle()
    {
        delegate_->dropCache();
        view_->update();
    }

    void MentionCompleter::showAnimated(const QPoint& _pos)
    {
        setArrowPosition(_pos);

        if (opacityAnimation_.isRunning())
            return;

        opacityEffect_->setOpacity(0.0);

        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            0.0,
            1.0,
            getDurationAppear().count());

        show();
    }

    void MentionCompleter::setArrowPosition(const QPoint& _pos)
    {
        const auto widthDiff = (parentWidget()->width() - width()) / 2;
        const auto globalPos = parentWidget()->mapFromGlobal(_pos);
        move(widthDiff, globalPos.y() - height() + Tooltip::getArrowHeight());
        arrowOffset_ = globalPos.x() - (widthDiff > 2 * Tooltip::getShadowSize() ? widthDiff : 0);
    }

    void MentionCompleter::hideAnimated()
    {
        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            [this]() { hide(); },
            1.0,
            0.0,
            getDurationDisappear().count());
    }
}

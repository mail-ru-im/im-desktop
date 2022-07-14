#include "stdafx.h"
#include "UnknownItemDelegate.h"

#include "RecentsTab.h"
#include "ContactListModel.h"
#include "UnknownsModel.h"
#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "../../main_window/contact_list/RecentItemDelegate.h"
#include "../containers/FriendlyContainer.h"

namespace Logic
{
    UnknownItemDelegate::ItemKey::ItemKey(
        const bool _isSelected,
        const bool _isHovered,
        const int _unreadDigitsNumber)
        : isSelected_(_isSelected)
        , isHovered_(_isHovered)
        , unreadDigitsNumber_(_unreadDigitsNumber)
    {
        im_assert(_unreadDigitsNumber >= 0);
        im_assert(_unreadDigitsNumber <= 2);
    }

    bool UnknownItemDelegate::ItemKey::operator < (const ItemKey& _key) const
    {
        if (isSelected_ != _key.isSelected_)
            return (isSelected_ < _key.isSelected_);

        if (isHovered_ != _key.isHovered_)
            return (isHovered_ < _key.isHovered_);

        if (unreadDigitsNumber_ != _key.unreadDigitsNumber_)
            return (unreadDigitsNumber_ < _key.unreadDigitsNumber_);

        return false;
    }

    UnknownItemDelegate::UnknownItemDelegate(QObject* _parent)
        : QItemDelegate(_parent)
        , stateBlocked_(false)
    {
        viewParams_.regim_ = ::Logic::MembersWidgetRegim::UNKNOWN;
    }

    void UnknownItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlg, bool _fromAlert, bool _dragOverlay) const
    {
        auto& item = items_[_dlg.AimId_];
        if (!item)
            item = Ui::RecentItemDelegate::createItem(_dlg);

        const bool isSelected_ = Logic::getContactListModel()->selectedContact() == _dlg.AimId_;
        const bool isHovered_ = (_option.state & QStyle::State_Selected) && !stateBlocked_ && !isSelected_;

        item->draw(*_painter, _option.rect, viewParams_, isSelected_, isHovered_, _dragOverlay, false);
    }

    void UnknownItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Data::DlgState dlg = _index.data(Qt::DisplayRole).value<Data::DlgState>();
        return paint(_painter, _option, dlg, false, _index == dragIndex_);
    }

    bool UnknownItemDelegate::isInRemoveContactFrame(const QPoint& _p) const
    {
        auto f = Ui::GetRecentsParams(viewParams_.regim_).removeContactFrame();
        return f.contains(_p);
    }

    QSize UnknownItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& _i) const
    {
        auto width = Ui::GetRecentsParams(viewParams_.regim_).itemWidth();
        if (Logic::getUnknownsModel()->isServiceItem(_i))
            return QSize(width, Ui::GetRecentsParams(viewParams_.regim_).serviceItemHeight());

        return QSize(width, Ui::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    QSize UnknownItemDelegate::sizeHintForAlert() const
    {
        return QSize(Ui::GetRecentsParams(viewParams_.regim_).itemWidth(), Ui::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    void UnknownItemDelegate::blockState(bool _value)
    {
        stateBlocked_ = _value;
    }

    void UnknownItemDelegate::setDragIndex(const QModelIndex& _index)
    {
        dragIndex_ = _index;
    }

    void UnknownItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool UnknownItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void UnknownItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }

    bool UnknownItemDelegate::needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor) const
    {
        const auto it = items_.find(_aimId);
        return it != items_.end() && it->second && it->second->needsTooltip(_posCursor);
    }

    QRect UnknownItemDelegate::getDraftIconRectWrapper(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor) const
    {
        const auto it = items_.find(_aimId);
        if (it != items_.end() && it->second)
            return it->second->getDraftIconRectWrapper();
        return {};
    }

    void UnknownItemDelegate::dlgStateChanged(const Data::DlgState& _dlgState)
    {
        auto iter = items_.find(_dlgState.AimId_);
        if (iter == items_.end())
            return;

        items_.erase(iter);
    }

    void UnknownItemDelegate::refreshAll()
    {
        items_.clear();
    }
}

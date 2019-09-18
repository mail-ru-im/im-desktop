#pragma once

#include "../../types/message.h"
#include "Common.h"
#include "RecentItemDelegate.h"

namespace Ui
{
    class ContactListItem;
    class RecentItemBase;
}

namespace Logic
{
    class UnknownItemDelegate : public QItemDelegate
    {
        Q_OBJECT

    public:

        UnknownItemDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlgState, bool _fromAlert = false, bool _dragOverlay = false) const;
        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        QSize sizeHintForAlert() const;
        void blockState(bool _value);

        void setDragIndex(const QModelIndex& _index);

        bool isInRemoveContactFrame(const QPoint& _p) const;
        bool isInDeleteAllFrame(const QPoint& _p) const;

        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;
        void setFixedWidth(int _newWidth);

    public Q_SLOTS:
        void dlgStateChanged(const Data::DlgState&);
        void refreshAll();

    private:
        struct ItemKey
        {
            const bool isSelected_;
            const bool isHovered_;
            const int unreadDigitsNumber_;

            ItemKey(const bool _isSelected, const bool _isHovered, const int _unreadDigitsNumber);

            bool operator < (const ItemKey& _key) const;
        };

        bool stateBlocked_;
        QModelIndex dragIndex_;
        Ui::ViewParams viewParams_;

        mutable std::unordered_map<QString, std::unique_ptr<Ui::RecentItemBase>, Utils::QStringHasher> items_;
    };
}

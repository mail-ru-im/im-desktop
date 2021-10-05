#pragma once

#include "Common.h"
#include "../../controls/TextUnit.h"

namespace Logic
{
    class TaskAssigneeListDelegate : public AbstractItemDelegateWithRegim
    {
        Q_OBJECT

    public:
        TaskAssigneeListDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        void setRegim(int _regim) override {}
        void setFixedWidth(int width) override {}
        void blockState(bool value) override {}
        void setDragIndex(const QModelIndex& index) override {}

        static int itemHeight() noexcept;

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;

    private:
        bool isFirstItem(const QModelIndex& _index) const;
        bool isLastItem(const QModelIndex& _index) const;

    private:
        Ui::TextRendering::TextUnitPtr name_;

        mutable std::unordered_map<int, bool> elidedItems_;
    };
}

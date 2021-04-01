#pragma once

#include "Common.h"
#include "../../controls/TextUnit.h"

namespace Logic
{
    class CallItemDelegate: public AbstractItemDelegateWithRegim
    {
        Q_OBJECT

    public:
        CallItemDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        void setRegim(int _regim) override;
        void setFixedWidth(int width) override;
        void blockState(bool value) override;
        void setDragIndex(const QModelIndex& index) override;

        void setPictOnlyView(bool _isPictureOnly);

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;

        QString getFriendly(const QModelIndex& _index) const;

    private:
        Ui::TextRendering::TextUnitPtr name_;
        Ui::TextRendering::TextUnitPtr serviceName_;
        Ui::TextRendering::TextUnitPtr date_;
        Ui::TextRendering::TextUnitPtr count_;

        mutable std::unordered_map<int, bool> elidedItems_;

        bool pictureOnly_;
    };
}

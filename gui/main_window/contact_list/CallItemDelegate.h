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

    private:
        Ui::TextRendering::TextUnitPtr name_;
        Ui::TextRendering::TextUnitPtr serviceName_;
        Ui::TextRendering::TextUnitPtr date_;

        bool pictureOnly_;
    };
}

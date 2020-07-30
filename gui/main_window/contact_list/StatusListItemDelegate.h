#pragma once

#include "../../statuses/Status.h"
#include "../../statuses/StatusUtils.h"
#include "StatusListModel.h"
#include "Common.h"

namespace Fonts
{
    enum class FontWeight;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class StatusItem : public QWidget
    {
        Q_OBJECT
    public:
        StatusItem(const Statuses::Status& _status = Statuses::Status());
        void paint(QPainter& _painter, bool _isHovered = false, bool _isActive = false, bool _isMoreHovered = false, int _curWidth = 0);
        ~StatusItem() = default;

        void updateParams(const Statuses::Status& _status);

    private:
        QImage status_;
        std::unique_ptr<TextRendering::TextUnit> title_;
        std::unique_ptr<TextRendering::TextUnit> time_;

        int curWidth_;
        bool isEmpty_;
    };
}

namespace Logic
{
    class CustomAbstractListModel;

    class StatusListItemDelegate : public AbstractItemDelegateWithRegim
    {
        Q_OBJECT
    public:
        StatusListItemDelegate(QObject* _parent, StatusListModel* _statusModel = nullptr);

        virtual ~StatusListItemDelegate() = default;

        void paint(QPainter *_painter, const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        void clearCache();

        void setRegim(int _regim) override {};

        void setFixedWidth(int width) override {};

        void blockState(bool value) override {};

        void setDragIndex(const QModelIndex& index) override {};

        void setHoveredModeIndex(const QModelIndex& _hoveredMore);

    private:
        StatusListModel* statusModel_;
        QModelIndex hoveredMore_;

        mutable std::map<QString, std::unique_ptr<Ui::StatusItem>> items_;
    };
}
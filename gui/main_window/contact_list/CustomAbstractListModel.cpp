#include "stdafx.h"
#include "CustomAbstractListModel.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    CustomAbstractListModel::CustomAbstractListModel(QObject *parent)
        : QAbstractListModel(parent)
        , flags_(0)
        , selectEnabled_(true)
    {
        if constexpr (platform::is_apple())
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::forceRefreshList, this, &CustomAbstractListModel::forceRefreshList);
    }

    bool CustomAbstractListModel::isServiceItem(const QModelIndex & _index) const
    {
        return false;
    }

    bool CustomAbstractListModel::isClickableItem(const QModelIndex & _index) const
    {
        return true;
    }

    bool CustomAbstractListModel::contains(const QString& _name) const
    {
        return false;
    }

    void CustomAbstractListModel::forceRefreshList(QAbstractItemModel *model, bool mouseOver)
    {
        if (model == this)
        {
            mouseOver ? setCustomFlag(CustomAbstractListModelFlags::HasMouseOver) : unsetCustomFlag(CustomAbstractListModelFlags::HasMouseOver);
            refreshList();
        }
    }

    void CustomAbstractListModel::refreshList()
    {
        emit dataChanged(index(0), index(rowCount()));
    }

    void CustomAbstractListModel::setCustomFlag(const int _flag)
    {
        flags_ |= _flag;
    }

    void CustomAbstractListModel::unsetCustomFlag(const int _flag)
    {
        flags_ &= (~_flag);
    }

    void CustomAbstractListModel::emitChanged(const int _first, const int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    void CustomAbstractListModel::setSelectEnabled(const bool _enabled)
    {
        selectEnabled_ = _enabled;
    }

    bool CustomAbstractListModel::getSelectEnabled() const
    {
        return selectEnabled_;
    }

    Qt::ItemFlags CustomAbstractListModel::flags(const QModelIndex&) const
    {
        Qt::ItemFlags result = Qt::ItemIsEnabled;
        if (selectEnabled_)
            result |= Qt::ItemIsSelectable;

        return result;
    }
}

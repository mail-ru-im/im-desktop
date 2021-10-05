#pragma once

namespace Logic
{
    enum CustomAbstractListModelFlags
    {
        HasMouseOver    = 0x00000001,
    };

    class CustomAbstractListModel: public QAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void containsPreCheckedItems(const std::vector<QString>& _names) const;

    private Q_SLOTS:
        void forceRefreshList(QAbstractItemModel *, bool);

    public:
        CustomAbstractListModel(QObject *parent = nullptr);

        void setSelectEnabled(const bool _enabled);
        bool getSelectEnabled() const;

        Qt::ItemFlags flags(const QModelIndex&) const override;

        using QAbstractListModel::sort;

        virtual bool isServiceItem(const QModelIndex& _index) const;
        virtual bool isClickableItem(const QModelIndex& _index) const;
        virtual bool isDropableItem(const QModelIndex& _index) const;
        virtual bool contains(const QString& _name) const;

        virtual void clearCheckedItems();
        virtual int getCheckedItemsCount() const;
        virtual void setCheckedItem(const QString& _name, bool _isChecked);
        virtual bool isCheckedItem(const QString& _name) const;

        void setCustomFlag(const int _flag);
        void unsetCustomFlag(const int _flag);

        inline bool isCustomFlagSet(int flag) const
        {
            return ((flags_ & flag) == flag);
        }

    protected:
        virtual void refreshList();
        void emitChanged(const int _first, const int _last);

    private:
        int flags_;
        bool selectEnabled_;
    };
}

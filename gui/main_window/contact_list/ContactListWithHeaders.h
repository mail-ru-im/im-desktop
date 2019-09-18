#pragma once

#include "CustomAbstractListModel.h"

namespace Ui
{
    class SearchDropdown;
}

namespace Logic
{
    class ContactListModel;

    class ContactListWithHeaders : public CustomAbstractListModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
        void onSelectedContactChanged();

    public:
        ContactListWithHeaders(QWidget* _parent, ContactListModel* _clModel, bool _withMenu = true);

        virtual int rowCount(const QModelIndex &_parent = QModelIndex()) const override;
        virtual QVariant data(const QModelIndex &_index, int _role = Qt::DisplayRole) const override;

        int getIndexAllContacts() const;
        int getIndexTopContacts() const;

        int getIndexAddContact() const;
        int getIndexCreateGroupchat() const;
        int getIndexNewChannel() const;

        bool isInTopContacts(const QModelIndex & _index) const;
        bool isInAllContacts(const QModelIndex & _index) const;
        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;

        enum MenuButtons
        {
            NoMenu          = 0,

            AddContact      = (1 << 0),
            CreateGroupchat = (1 << 1),
            NewChannel      = (1 << 2),
            //NewLivehannel   = (1 << 3),

            Fullmenu = AddContact | CreateGroupchat | NewChannel, //| NewLivehannel,
            FullmenuBiz = CreateGroupchat | NewChannel
        };
        void setMenu(const MenuButtons _menu);
        MenuButtons getMenu() const;

    private:
        int getMenuSize() const;
        int getMenuItemIndex(const MenuButtons _item) const;
        bool isWithHeaders() const;

    private:
        ContactListModel*   clModel_;
        MenuButtons         menu_;

        QModelIndex mapFromSource(const QModelIndex &_sourceIndex) const;
        QModelIndex mapToSource(const QModelIndex &_proxyIndex) const;
    };
}

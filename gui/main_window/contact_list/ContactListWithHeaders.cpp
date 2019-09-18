#include "stdafx.h"
#include "ContactListWithHeaders.h"
#include "ContactListModel.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto hdrTopContactsCount = 1;
    constexpr auto hdrAllContactsCount = 1;
}

namespace Logic
{
    ContactListWithHeaders::ContactListWithHeaders(QWidget* _parent, ContactListModel* _clModel, bool _withMenu)
        : CustomAbstractListModel(_parent)
        , clModel_(_clModel)
        , menu_(MenuButtons::NoMenu)
    {
        connect(clModel_, &ContactListModel::dataChanged, this, &ContactListWithHeaders::onSourceDataChanged);
        connect(clModel_, &ContactListModel::selectedContactChanged, this, &ContactListWithHeaders::onSelectedContactChanged);

        if (!_withMenu)
            return;

        connect(&Utils::InterConnector::instance(),
                &Utils::InterConnector::hideSearchDropdown,
                this,
                [this](){ setMenu(MenuButtons::NoMenu); }
        );

        connect(&Utils::InterConnector::instance(),
                &Utils::InterConnector::showSearchDropdownAddContact,
                this,
                [this](){ setMenu(MenuButtons::AddContact); }
        );

        connect(&Utils::InterConnector::instance(),
                &Utils::InterConnector::showSearchDropdownFull,
                this,
                [this](){ setMenu((build::is_biz() ||build::is_dit()) ? FullmenuBiz : Fullmenu); }
        );
    }

    int ContactListWithHeaders::rowCount(const QModelIndex& _parent) const
    {
        const auto clRowCount = clModel_->rowCount(_parent);
        const auto topCount = clModel_->getTopSortedCount();
        const auto withHeaders = topCount > 0 && topCount < clRowCount;

        auto add = 0;
        if (withHeaders)
            add += hdrTopContactsCount + hdrAllContactsCount;
        if (menu_)
            add += getMenuSize();

        return clRowCount + add;
    }

    QVariant ContactListWithHeaders::data(const QModelIndex & _index, int _role) const
    {
        if (!_index.isValid())
            return QVariant();

        const auto makeIcon = [](const QString& _icon, const Styling::StyleVariable _var)
        {
            return Utils::renderSvg(_icon, Utils::scale_value(QSize(24, 24)), Styling::getParameters().getColor(_var));
        };

        const auto dropDownBtn = [&makeIcon](const QString& _aimId, const QString& _friendly, const QString& _icon)
        {
            Data::Contact hdr;
            hdr.AimId_ = _aimId;
            hdr.Friendly_ = _friendly;

            hdr.iconPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY);
            hdr.iconHoverPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY_HOVER);
            hdr.iconPressedPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            hdr.setType(Data::ContactType::DROPDOWN_BUTTON);
            return hdr;
        };

        const auto serviceHdr = [](const QString& _aimId, const QString& _friendly)
        {
            Data::Contact hdr;
            hdr.AimId_ = _aimId;
            hdr.Friendly_ = _friendly;
            hdr.setType(Data::ContactType::SERVICE_HEADER);
            return hdr;
        };

        const auto row = _index.row();

        if (menu_)
        {
            if (row == getIndexAddContact())
            {
                if (Testing::isAccessibleRole(_role))
                    return qsl("add_contact");

                static auto hdr = dropDownBtn(qsl("~addContact~"), QT_TRANSLATE_NOOP("contact_list", "Add contact"), qsl(":/header/add_user"));
                return QVariant::fromValue(&hdr);
            }
            else if (row == getIndexCreateGroupchat())
            {
                if (Testing::isAccessibleRole(_role))
                    return qsl("new_groupchat");

                static auto hdr = dropDownBtn(qsl("~newGroupchat~"), QT_TRANSLATE_NOOP("contact_list", "New group"), qsl(":/add_groupchat"));
                return QVariant::fromValue(&hdr);
            }
            else if (row == getIndexNewChannel())
            {
                if (Testing::isAccessibleRole(_role))
                    return qsl("new_channel");

                static auto hdr = dropDownBtn(qsl("~newChannel~"), QT_TRANSLATE_NOOP("contact_list", "New channel"), qsl(":/add_channel"));
                return QVariant::fromValue(&hdr);
            }
        }

        if (isWithHeaders())
        {
            if (row == getIndexTopContacts())
            {
                if (Testing::isAccessibleRole(_role))
                    return qsl("AS contacts top_contacts");

                static auto hdr = serviceHdr(qsl("~top~"), QT_TRANSLATE_NOOP("contact_list", "TOP CONTACTS"));
                return QVariant::fromValue(&hdr);
            }
            else if (row == getIndexAllContacts())
            {
                if (Testing::isAccessibleRole(_role))
                    return qsl("AS contacts all_contacts");

                static auto hdr = serviceHdr(qsl("~all~"), QT_TRANSLATE_NOOP("contact_list", "ALL CONTACTS"));
                return QVariant::fromValue(&hdr);
            }
        }

        return clModel_->data(mapToSource(_index), _role);
    }

    int ContactListWithHeaders::getIndexAllContacts() const
    {
        const auto topCount = clModel_->getTopSortedCount();
        return topCount + 1 + getMenuSize();
    }

    int ContactListWithHeaders::getIndexTopContacts() const
    {
        return getMenuSize();
    }

    int ContactListWithHeaders::getIndexAddContact() const
    {
        return getMenuItemIndex(MenuButtons::AddContact);
    }

    int ContactListWithHeaders::getIndexCreateGroupchat() const
    {
        return getMenuItemIndex(MenuButtons::CreateGroupchat);
    }

    int ContactListWithHeaders::getIndexNewChannel() const
    {
        return getMenuItemIndex(MenuButtons::NewChannel);
    }

    bool ContactListWithHeaders::isInTopContacts(const QModelIndex & _index) const
    {
        return !isServiceItem(_index) && _index.row() < getIndexAllContacts();
    }

    bool ContactListWithHeaders::isInAllContacts(const QModelIndex & _index) const
    {
        return !isServiceItem(_index) && _index.row() > getIndexAllContacts();
    }

    bool ContactListWithHeaders::isServiceItem(const QModelIndex & _index) const
    {
        const auto i = _index.row();
        return
            (isWithHeaders() && (i == getIndexAllContacts() || i == getIndexTopContacts())) ||
            i == getIndexAddContact() ||
            i == getIndexCreateGroupchat() ||
            i == getIndexNewChannel();
    }

    bool ContactListWithHeaders::isClickableItem(const QModelIndex & _index) const
    {
        if (isWithHeaders())
        {
            const auto i = _index.row();
            return
                i != getIndexAllContacts() &&
                i != getIndexTopContacts();
        }
        return true;
    }

    void ContactListWithHeaders::setMenu(const ContactListWithHeaders::MenuButtons _menu)
    {
        if (menu_ == _menu)
            return;

        menu_ = _menu;

        emit dataChanged(index(0), index(rowCount()));
    }

    ContactListWithHeaders::MenuButtons ContactListWithHeaders::getMenu() const
    {
        return menu_;
    }

    int ContactListWithHeaders::getMenuSize() const
    {
        unsigned int num = menu_;
        auto count = 0;
        while (num)
        {
            count += num & 1;
            num >>= 1;
        }
        return count;
    }

    int ContactListWithHeaders::getMenuItemIndex(const MenuButtons _item) const
    {
        if (!(menu_ & _item))
            return -1;

        auto count = 0;
        unsigned int num = menu_ & (~_item);
        for (auto i = 0; i < _item / 2; ++i)
        {
            count += num & 1;
            num >>= 1;
        }
        return count;
    }

    bool ContactListWithHeaders::isWithHeaders() const
    {
        const auto topCount = clModel_->getTopSortedCount();
        const auto clRowCount = clModel_->rowCount();
        return topCount != 0 && topCount < clRowCount;
    }

    QModelIndex ContactListWithHeaders::mapFromSource(const QModelIndex & _sourceIndex) const
    {
        if (!_sourceIndex.isValid())
            return QModelIndex();

        const auto row = _sourceIndex.row();
        auto shift = getMenuSize();
        if (isWithHeaders())
            shift += row < clModel_->getTopSortedCount() ? hdrTopContactsCount : (hdrTopContactsCount + hdrAllContactsCount);

        return index(row + shift);
    }

    QModelIndex ContactListWithHeaders::mapToSource(const QModelIndex & _proxyIndex) const
    {
        if (!_proxyIndex.isValid())
            return QModelIndex();

        const auto row = _proxyIndex.row() - getMenuSize();
        auto shift = 0;
        if (isWithHeaders())
            shift = row <= clModel_->getTopSortedCount() ? hdrTopContactsCount : (hdrTopContactsCount + hdrAllContactsCount);

        return index(row - shift);
    }

    void ContactListWithHeaders::onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    {
        emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight));
    }

    void ContactListWithHeaders::onSelectedContactChanged()
    {
        emit dataChanged(index(0), index(rowCount()));
    }
}

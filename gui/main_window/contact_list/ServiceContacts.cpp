#include "stdafx.h"
#include "ServiceContacts.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "ContactListUtils.h"

namespace
{

struct ServiceContact
{
    QString id_;
    QString name_;
    ServiceContacts::ContactType type_;
};


const std::vector<ServiceContact>& serviceContacts()
{
    static const std::vector<ServiceContact> contacts = {
        { ServiceContacts::getThreadsName(), QT_TRANSLATE_NOOP("contact_list", "Threads"), ServiceContacts::ContactType::ThreadsFeed },
        { ServiceContacts::getTasksName(), QT_TRANSLATE_NOOP("contact_list", "Tasks"), ServiceContacts::ContactType::Tasks }
    };
    return contacts;
}

}

namespace ServiceContacts
{

bool isServiceContact(const QString& _contactId)
{
    auto it = std::find_if(serviceContacts().begin(), serviceContacts().end(), [&_contactId](const auto& contact){ return contact.id_ == _contactId; });
    return it != serviceContacts().end();
}

ContactType contactType(const QString& _contactId)
{
    auto it = std::find_if(serviceContacts().begin(), serviceContacts().end(), [&_contactId](const auto& contact){ return contact.id_ == _contactId; });
    if (it != serviceContacts().end())
        return it->type_;

    return ContactType::None;
}

const QString& contactId(ContactType _type)
{
    auto it = std::find_if(serviceContacts().begin(), serviceContacts().end(), [_type](const auto& contact){ return contact.type_ == _type; });
    if (it != serviceContacts().end())
        return it->id_;

    static QString empty;
    return empty;
}

const QString& contactName(ContactType _type)
{
    auto it = std::find_if(serviceContacts().begin(), serviceContacts().end(), [_type](const auto& contact){ return contact.type_ == _type; });
    if (it != serviceContacts().end())
        return it->name_;

    static QString empty;
    return empty;
}

const QString& contactName(const QString& _contactId)
{
    return contactName(contactType(_contactId));
}

QPixmap avatar(ContactType _type, int32_t _size)
{
    if (_type == ContactType::ThreadsFeed)
    {
        QPixmap result(QSize(_size, _size));
        result.fill(Qt::transparent);
        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing);
        const auto iconSize = QSize(_size, _size) * 0.625;
        const auto threadIcon = Utils::renderSvg(qsl(":/thread_icon_filled"), iconSize, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        auto backgroundColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        QPainterPath backgroundPath;
        backgroundPath.addEllipse(QRect(QPoint(0, 0), result.size()));
        p.fillPath(backgroundPath, backgroundColor);
        p.drawPixmap((result.width() - Utils::unscale_bitmap(threadIcon.width())) / 2, (result.height() - Utils::unscale_bitmap(threadIcon.height())) / 2, threadIcon);
        Utils::check_pixel_ratio(result);

        return result;
    }

    return QPixmap();
}

QPixmap avatar(const QString& _contactId, int32_t _size)
{
    return avatar(contactType(_contactId), _size);
}

const QString& getThreadsName() noexcept
{
    static const auto str = qsl("~threads~");
    return str;
}

const QString& getTasksName() noexcept
{
    static const auto str = qsl("~tasks~");
    return str;
}

}

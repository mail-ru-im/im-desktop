#pragma once

namespace ServiceContacts
{
    enum class ContactType
    {
        ThreadsFeed,
        Tasks,

        None
    };

    [[nodiscard]] const QString& getThreadsName() noexcept;
    [[nodiscard]] const QString& getTasksName() noexcept;
    bool isServiceContact(const QString& _contactId);
    ContactType contactType(const QString& _contactId);
    const QString& contactId(ContactType _type);
    const QString& contactName(ContactType _type);
    const QString& contactName(const QString& _contactId);
    QPixmap avatar(ContactType _type, int32_t _size);
    QPixmap avatar(const QString& _contactId, int32_t _size);
}

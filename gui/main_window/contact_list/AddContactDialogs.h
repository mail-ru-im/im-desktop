#pragma once

class QWidget;

namespace Utils
{

namespace AddContactDialogs
{
    struct Initiator
    {
        enum class From
        {
            ConlistScr,
            ScrContactTab,
            ScrChatsTab,
            HotKey,

            Unspecified
        };

        From from_ = From::Unspecified;
    };
}

using AddContactCallback = std::function<void()>;

void showAddContactsDialog(const QString& _name, const QString& _phone, AddContactCallback _callback, const AddContactDialogs::Initiator& _initiator);
void showAddContactsDialog(const AddContactDialogs::Initiator& _initiator);

}

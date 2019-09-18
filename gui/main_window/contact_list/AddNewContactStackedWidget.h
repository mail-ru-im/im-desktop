#pragma once

#include <QWidget>
#include "AddNewContactWidget.h"

class QVBoxLayout;
class QStackedWidget;

namespace Logic
{
class ContactSearcher;
}

namespace Ui
{

class ContactNotRegisteredWidget;
class DialogButton;

enum class AddContactResult
{
    Added,
    None
};

class AddNewContactStackedWidget: public QWidget
{
    Q_OBJECT

public:
    explicit AddNewContactStackedWidget(QWidget* _parent = nullptr);

    void setOkCancelButtons(const QPair<DialogButton*, DialogButton*>& _buttons);

    void setName(const QString& _name);
    void setPhone(const QString& _phone);

    AddContactResult getResult() { return result_; }

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onOkClicked();

    void onSyncAdressBook(const bool _hasError);

    void onFormDataIncomplete();
    void onFormDataSufficient();

private:
    void connectToButtons();
    void enableOkButton(bool _enable);
    void onContactNotFound(const QString& _phoneNumber, const QString& _name);
private:
    QStackedWidget* stackedWidget_;

    AddNewContactWidget* addContactWidget_;
    ContactNotRegisteredWidget* notRegisteredWidget_;

    DialogButton* okButton_;
    DialogButton* cancelButton_;

    QVBoxLayout* globalLayout_;

    AddNewContactWidget::FormData lastFormData_;

    AddContactResult result_ = AddContactResult::None;
};

}

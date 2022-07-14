#include "AddNewContactStackedWidget.h"

#include <QPushButton>

#include "ContactNotRegisteredWidget.h"

#include "ContactListModel.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "utils/utils.h"
#include "controls/DialogButton.h"

namespace
{
int AddNewContactIndex = -1;
int ContactNotRegisteredIndex = -1;
}

namespace Ui
{

AddNewContactStackedWidget::AddNewContactStackedWidget(QWidget *_parent)
    : QWidget(_parent),
      stackedWidget_(nullptr),
      okButton_(nullptr),
      cancelButton_(nullptr)
{
    globalLayout_ = Utils::emptyVLayout(this);

    setFixedHeight(Utils::scale_value(390) - 2 * Utils::scale_value(16) - Utils::scale_value(32));

    stackedWidget_ = new QStackedWidget(this);
    stackedWidget_->setContentsMargins(0, 0, 0, 0);
    stackedWidget_->layout()->setSpacing(0);

    addContactWidget_ = new AddNewContactWidget(stackedWidget_);
    notRegisteredWidget_ = new ContactNotRegisteredWidget(stackedWidget_);

    AddNewContactIndex = stackedWidget_->addWidget(addContactWidget_);
    ContactNotRegisteredIndex = stackedWidget_->addWidget(notRegisteredWidget_);

    globalLayout_->addWidget(stackedWidget_);

    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::syncronizedAddressBook, this, &AddNewContactStackedWidget::onSyncAdressBook);
    connect(addContactWidget_, &AddNewContactWidget::formDataIncomplete, this, &AddNewContactStackedWidget::onFormDataIncomplete);
    connect(addContactWidget_, &AddNewContactWidget::formDataSufficient, this, &AddNewContactStackedWidget::onFormDataSufficient);
    connect(addContactWidget_, &AddNewContactWidget::formSubmissionRequested, this, [this]()
    {
        if (!okButton_->isEnabled())
            return;

        onOkClicked();
    });

    connect(notRegisteredWidget_, &ContactNotRegisteredWidget::enterPressed, this, &AddNewContactStackedWidget::onOkClicked);
}

void AddNewContactStackedWidget::setOkCancelButtons(const QPair<DialogButton*, DialogButton*> &_buttons)
{
    im_assert(!okButton_ && !cancelButton_);

    okButton_ = _buttons.first;
    cancelButton_ = _buttons.second;

    connectToButtons();

    enableOkButton(false);
}

void AddNewContactStackedWidget::setName(const QString &_name)
{
    addContactWidget_->setName(_name);
}

void AddNewContactStackedWidget::setPhone(const QString &_phone)
{
    addContactWidget_->setPhone(_phone);
}

void AddNewContactStackedWidget::onOkClicked()
{
    // Check state
    auto index = stackedWidget_->currentIndex();
    if (index == AddNewContactIndex)
    {
        enableOkButton(false);

        lastFormData_ = addContactWidget_->getFormData();

        const auto space = lastFormData_.firstName_.isEmpty() ? QStringView() : u" ";
        const QString name = lastFormData_.firstName_ % space % lastFormData_.lastName_;
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("name", name);

        core::ifptr<core::iarray> phones(collection->create_array());
        phones->reserve(1);

        core::ifptr<core::ivalue> val(collection->create_value());
        val->set_as_string(lastFormData_.phoneNumber_.toStdString().c_str(), lastFormData_.phoneNumber_.length());
        phones->push_back(val.get());
        collection.set_value_as_array("phones", phones.get());
        Ui::GetDispatcher()->post_message_to_core("addressbook/sync", collection.get());
    }
    else if (index == ContactNotRegisteredIndex)
    {
        okButton_->setText(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "Add"));
        cancelButton_->setText(QT_TRANSLATE_NOOP("popup_window", "Cancel"));
        addContactWidget_->clearData();
        stackedWidget_->setCurrentIndex(AddNewContactIndex);
    }
}

void AddNewContactStackedWidget::onSyncAdressBook()
{
    result_ = AddContactResult::Added;
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::conlistscr_addcon_action);
    Q_EMIT finished();
}

void AddNewContactStackedWidget::onFormDataIncomplete()
{
    enableOkButton(false);
}

void AddNewContactStackedWidget::onFormDataSufficient()
{
    enableOkButton(true);
}

void AddNewContactStackedWidget::connectToButtons()
{
    // Intentionally QueuedConnection to override GeneralDialog::rightButtonClick behaviour
    connect(okButton_, &QPushButton::clicked, this, &AddNewContactStackedWidget::onOkClicked, Qt::QueuedConnection);
    connect(cancelButton_, &QPushButton::clicked, this, &AddNewContactStackedWidget::finished);
}

void AddNewContactStackedWidget::enableOkButton(bool _enable)
{
    im_assert(okButton_);
    okButton_->setEnabled(_enable);
}

void AddNewContactStackedWidget::onContactNotFound(const QString &_phoneNumber, const QString &_name)
{
    enableOkButton(true);

    okButton_->setText(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "Add another one"));
    notRegisteredWidget_->setInfo(_phoneNumber, _name);
    cancelButton_->setText(QT_TRANSLATE_NOOP("popup_window", "Close"));
    stackedWidget_->setCurrentIndex(ContactNotRegisteredIndex);
}

}

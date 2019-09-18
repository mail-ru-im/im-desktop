#include "PhoneLineEdit.h"

#include <QCompleter>
#include <QStandardItemModel>
#include "LineEditEx.h"

#include "controls/SizedView.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "fonts.h"

#include "styles/ThemeParameters.h"

namespace
{
constexpr int COUNTRY_CODE_WIDTH = 62;
constexpr int COUNTRY_CODE_LEFT_MARGIN = 32;
constexpr int PHONE_EDIT_MARGIN = 38;
constexpr int PHONE_EDIT_LEFT_MARGIN = 8;
constexpr int PHONE_EDIT_WIDTH = 242;
constexpr int ABOUT_PHONE_NUMBER_WIDTH = 296;
constexpr int COUNTRY_CODE_HINT_ROW_HEIGHT = 24;
}

namespace Ui
{

PhoneLineEdit::PhoneLineEdit(QWidget *_parent)
    : QWidget(_parent)
{
    globalLayout_ = Utils::emptyHLayout(this);

    phoneNumber_ = new LineEditEx(this);

    const auto countries = Utils::getCountryCodes();
    auto model = new QStandardItemModel(this);
    model->setColumnCount(2);
    int i = 0;
    for (auto it = countries.begin(), end = countries.end(); it != end; ++it)
    {
        QStandardItem* firstCol = new QStandardItem(it.key());
        firstCol->setFlags(firstCol->flags() & ~Qt::ItemIsEditable);
        QStandardItem* secondCol = new QStandardItem(it.value());
        secondCol->setFlags(secondCol->flags() & ~Qt::ItemIsEditable);
        secondCol->setTextAlignment(Qt::AlignRight);
        model->setItem(i, 0, firstCol);
        model->setItem(i, 1, secondCol);

        ++i;
    }

    auto sizedView = new SizedView(this);
    sizedView->setSelectionBehavior(QAbstractItemView::SelectRows);
    sizedView->header()->hide();
    sizedView->setRootIsDecorated(false);
    sizedView->setFixedWidth(Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH));
    sizedView->setItemDelegate(new CountryCodeItemDelegate(this));
    sizedView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    Utils::ApplyStyle(sizedView, Styling::getParameters().getPhoneComboboxQss());

    completer_ = new QCompleter(this);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setModel(model);
    completer_->setPopup(sizedView);
    completer_->setCompletionColumn(0);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setMaxVisibleItems(6);
    completer_->setCompletionColumn(1);

    countryCode_ = new LineEditEx(this);
    countryCode_->setText(Utils::GetTranslator()->getCurrentPhoneCode());
    countryCode_->setFont(Fonts::appFontScaled(18));
    countryCode_->setFixedWidth(Utils::scale_value(COUNTRY_CODE_WIDTH));
    Utils::ApplyStyle(countryCode_, Styling::getParameters().getLineEditCommonQss());
    countryCode_->setAttribute(Qt::WA_MacShowFocusRect, false);
    countryCode_->setAlignment(Qt::AlignCenter);
    countryCode_->setValidator(new Utils::PhoneValidator(this, true));
    completer_->setCompletionPrefix(countryCode_->text());
    countryCode_->setCompleter(completer_);
    countryCode_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

    phoneNumber_->setPlaceholderText(QT_TRANSLATE_NOOP("phone_widget", "phone number"));
    phoneNumber_->setFont(Fonts::appFontScaled(18));
    phoneNumber_->setFixedWidth(Utils::scale_value(PHONE_EDIT_WIDTH));
    Utils::ApplyStyle(phoneNumber_, Styling::getParameters().getLineEditCommonQss());
    phoneNumber_->setAttribute(Qt::WA_MacShowFocusRect, false);
    phoneNumber_->setValidator(new Utils::PhoneValidator(this, false));

    connect(phoneNumber_, &QLineEdit::textChanged, this, &PhoneLineEdit::onPhoneChanged);
    connect(phoneNumber_, &LineEditEx::emptyTextBackspace, this, &PhoneLineEdit::onPhoneEmptyTextBackspace);

    connect(countryCode_, &QLineEdit::textChanged, this, &PhoneLineEdit::onCountryCodeChanged);
    connect(countryCode_, &Ui::LineEditEx::clicked, this, &PhoneLineEdit::onCountryCodeClicked,
            /* Intentionally queued, let LineEditEx handle click first */
            Qt::QueuedConnection);

    connect(completer_, Utils::QOverload<const QString&>::of(&QCompleter::activated),
        this, [this](const QString&) {
        focusOnPhoneWidget();
    });


    globalLayout_->addWidget(countryCode_);
    globalLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(PHONE_EDIT_LEFT_MARGIN), 0, QSizePolicy::Fixed));
    globalLayout_->addWidget(phoneNumber_);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationDeactivated,
            this, &PhoneLineEdit::onAppDeactivated);
}

QCompleter *PhoneLineEdit::getCompleter()
{
    return completer_;
}

LineEditEx *PhoneLineEdit::getPhoneNumberWidget()
{
    return phoneNumber_;
}

LineEditEx *PhoneLineEdit::getCountryCodeWidget()
{
    return countryCode_;
}

void PhoneLineEdit::resetInputs()
{
    phoneNumber_->clear();
    countryCode_->setText(Utils::GetTranslator()->getCurrentPhoneCode());
}

bool PhoneLineEdit::setPhone(const QString &_phone)
{
    const auto countries = Utils::getCountryCodes();

    for (auto& code : countries)
    {
        if (_phone.startsWith(code))
        {
            countryCode_->setText(code);
            phoneNumber_->setText(_phone.mid(code.size()));
            return true;
        }
    }
    return false;
}

void PhoneLineEdit::onCountryCodeClicked()
{
    if (countryCode_->contextMenu())
        return;

    completer_->setCompletionPrefix(QString());
    completer_->complete();
}

void PhoneLineEdit::onCountryCodeChanged(const QString &_code)
{
    if (!_code.startsWith(ql1c('+')))
    {
        setCountryCode(ql1c('+') + _code);
    }

    auto cc = countryCode_->text();

    if (completer_->completionCount() == 0)
    {
        setCountryCode(cc.left(cc.length() - 1));
        phoneNumber_->setText(phoneNumber_->text() % cc.midRef(cc.length() - 1));
        focusOnPhoneWidget();
    }
    else if (completer_->completionCount() == 1 && completer_->currentCompletion() == _code)
    {
        focusOnPhoneWidget();
    }
}

void PhoneLineEdit::focusOnPhoneWidget()
{
    countryCode_->clearFocus();
    phoneNumber_->setFocus();
}

void PhoneLineEdit::setCountryCode(const QString &_code)
{
    {
        QSignalBlocker blocker(countryCode_);
        countryCode_->setText(_code);
    }

    completer_->setCompletionPrefix(_code);
}

void PhoneLineEdit::onPhoneChanged()
{
    emit fullPhoneChanged(getFullPhone());
}

void PhoneLineEdit::onPhoneEmptyTextBackspace()
{
    countryCode_->setFocus();
    completer_->complete();
}

void PhoneLineEdit::onAppDeactivated()
{
    if (completer_ && completer_->popup())
        completer_->popup()->hide();
}

QString PhoneLineEdit::getFullPhone() const
{
    return countryCode_->text() + phoneNumber_->text();
}

// CountryCodeItemDelegate
CountryCodeItemDelegate::CountryCodeItemDelegate(QObject *_parent)
    : QItemDelegate(_parent)
{
}

QSize CountryCodeItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(-1, Utils::scale_value(COUNTRY_CODE_HINT_ROW_HEIGHT));
}

}

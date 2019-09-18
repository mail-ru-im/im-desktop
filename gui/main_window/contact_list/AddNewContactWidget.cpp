#include "AddNewContactWidget.h"

#include "utils/utils.h"
#include "utils/Text.h"
#include "controls/TextBrowserEx.h"
#include "controls/LineEditEx.h"
#include "controls/PhoneLineEdit.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "gui_settings.h"

namespace
{
constexpr int LEFT_LAYOUT_MARGIN = 20;
constexpr int RIGHT_LAYOUT_MARGIN = 48;
constexpr int WIDGET_WIDTH = 380;
constexpr int FORM_FONT_SIZE = 16;
constexpr int HINT_HORIZONTAL_OFFSET = 100;
constexpr int TEXT_TO_LINE_MARGIN_BOTTOM = 7;
}

namespace Ui
{

AddNewContactWidget::AddNewContactWidget(QWidget *_parent)
    : QWidget(_parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedWidth(Utils::scale_value(WIDGET_WIDTH));

    globalLayout_ = Utils::emptyVLayout(this);
    globalLayout_->setContentsMargins(Utils::scale_value(LEFT_LAYOUT_MARGIN), 0, Utils::scale_value(RIGHT_LAYOUT_MARGIN), 0);

    auto horMargins = Utils::scale_value(LEFT_LAYOUT_MARGIN + RIGHT_LAYOUT_MARGIN);

    textUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "New contact"));
    // FONT
    textUnit_->init(Fonts::appFontScaled(23), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    textUnit_->evaluateDesiredSize();
    textUnit_->setOffsets(Utils::scale_value(16), Utils::scale_value(16));

    hintUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("add_new_contact_dialogs", ""));
    hintUnit_->init(Fonts::appFontScaled(14), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
    hintUnit_->evaluateDesiredSize();
    hintUnit_->setOffsets(Utils::scale_value(HINT_HORIZONTAL_OFFSET), Utils::scale_value(271));

    LineEditEx::Options lineEditOptions;
    lineEditOptions.noPropagateKeys_ = { Qt::Key_Up, Qt::Key_Down, Qt::Key_Tab, Qt::Key_Backtab };

    firstName_ = new LineEditEx(this, lineEditOptions);
    int left, top, right, bottom;
    firstName_->getTextMargins(&left, &top, &right, &bottom);
    QMargins lineEditMargins(left, top, right, Utils::scale_value(TEXT_TO_LINE_MARGIN_BOTTOM));
    firstName_->setTextMargins(lineEditMargins);
    firstName_->setPlaceholderText(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "First Name"));
    firstName_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    firstName_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    firstName_->setAttribute(Qt::WA_MacShowFocusRect, false);
    firstName_->setFont(Fonts::appFontScaled(FORM_FONT_SIZE));
    Utils::ApplyStyle(firstName_, Styling::getParameters().getLineEditCommonQss());
    Testing::setAccessibleName(firstName_, qsl("AS ancw firstName_"));
    firstName_->setFixedWidth(Utils::scale_value(WIDGET_WIDTH) - horMargins);
    firstName_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    firstName_->setContentsMargins(0, 0, 0, 0);

    lastName_ = new LineEditEx(this, lineEditOptions);
    lastName_->setPlaceholderText(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "Last Name"));
    lastName_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    lastName_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    lastName_->setAttribute(Qt::WA_MacShowFocusRect, false);
    lastName_->setFont(Fonts::appFontScaled(FORM_FONT_SIZE));
    Utils::ApplyStyle(lastName_, Styling::getParameters().getLineEditCommonQss());
    Testing::setAccessibleName(lastName_, qsl("AS ancw lastName_"));
    lastName_->setFixedWidth(Utils::scale_value(WIDGET_WIDTH) - horMargins);
    lastName_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    lastName_->setContentsMargins(0, 0, 0, 0);
    lastName_->setTextMargins(lineEditMargins);

    phoneInput_ = new PhoneLineEdit(this);
    phoneInput_->setContentsMargins(0, 0, 0, 0);
    phoneInput_->setFixedHeight(Utils::scale_value(50));
    phoneInput_->setFixedWidth(Utils::scale_value(WIDGET_WIDTH) - horMargins);
    phoneInput_->getCountryCodeWidget()->setFont(Fonts::appFontScaled(FORM_FONT_SIZE));
    QPalette pal(phoneInput_->getCountryCodeWidget()->palette());
    pal.setColor(QPalette::Background, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
    phoneInput_->getCountryCodeWidget()->setPalette(pal);

    phoneInput_->getPhoneNumberWidget()->setFont(Fonts::appFontScaled(FORM_FONT_SIZE));
    phoneInput_->getCountryCodeWidget()->setContentsMargins(0, 0, 0, 0);
    phoneInput_->getPhoneNumberWidget()->setContentsMargins(0, 0, 0, 0);
    phoneInput_->getCountryCodeWidget()->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    phoneInput_->getPhoneNumberWidget()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    phoneInput_->getCountryCodeWidget()->setTextMargins(lineEditMargins);
    phoneInput_->getPhoneNumberWidget()->setTextMargins(lineEditMargins);
    phoneInput_->getCountryCodeWidget()->setOptions(lineEditOptions);
    phoneInput_->getPhoneNumberWidget()->setOptions(lineEditOptions);

    rawPhoneInput_ = new LineEditEx(this, lineEditOptions);
    Utils::ApplyStyle(rawPhoneInput_, Styling::getParameters().getLineEditCommonQss());
    rawPhoneInput_->setPlaceholderText(QT_TRANSLATE_NOOP("add_new_contact_dialogs", "Phone number"));
    rawPhoneInput_->setFixedWidth(Utils::scale_value(WIDGET_WIDTH) - horMargins);
    rawPhoneInput_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    rawPhoneInput_->setAttribute(Qt::WA_MacShowFocusRect, false);
    rawPhoneInput_->setFont(Fonts::appFontScaled(FORM_FONT_SIZE));
    rawPhoneInput_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    rawPhoneInput_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    rawPhoneInput_->setContentsMargins(0, 0, 0, 0);
    rawPhoneInput_->setTextMargins(lineEditMargins);
    rawPhoneInput_->setVisible(false);

    auto textChangedCallback = [this](const QString& _text)
    {
        Q_UNUSED(_text);
        onFormChanged();
    };

    connect(firstName_, &LineEditEx::textChanged, this, textChangedCallback);
    connect(lastName_, &LineEditEx::textChanged, this, textChangedCallback);

    connect(phoneInput_, &PhoneLineEdit::fullPhoneChanged, this, &AddNewContactWidget::onFullPhoneChanged);
    connect(rawPhoneInput_, &LineEditEx::textChanged, this, &AddNewContactWidget::onFullPhoneChanged);

    globalLayout_->addSpacing(Utils::scale_value(80));
    globalLayout_->addWidget(firstName_);
    globalLayout_->addSpacing(Utils::scale_value(20));
    globalLayout_->addWidget(lastName_);
    globalLayout_->addSpacing(Utils::scale_value(19));
    globalLayout_->addWidget(phoneInput_);
    globalLayout_->addWidget(rawPhoneInput_);
    globalLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(40), QSizePolicy::Fixed, QSizePolicy::Expanding));

    QObject::connect(GetDispatcher(), &Ui::core_dispatcher::phoneInfoResult, this, &AddNewContactWidget::onPhoneInfoResult);

    firstName_->setFocus();

    connectToLineEdit(firstName_, phoneInput_->getPhoneNumberWidget(), lastName_);
    connectToLineEdit(lastName_, firstName_, phoneInput_->getCountryCodeWidget());
    connectToLineEdit(phoneInput_->getCountryCodeWidget(), lastName_, phoneInput_->getPhoneNumberWidget());
    connectToLineEdit(phoneInput_->getPhoneNumberWidget(), phoneInput_->getCountryCodeWidget(), firstName_);
}

AddNewContactWidget::FormData AddNewContactWidget::getFormData() const
{
    FormData formData;

    formData.firstName_ = firstName_->text();
    formData.lastName_ = lastName_->text();
    formData.phoneNumber_ = getPhoneNumber();

    return formData;
}

void AddNewContactWidget::clearData()
{
    firstName_->clear();
    lastName_->clear();
    phoneInput_->resetInputs();
}

void AddNewContactWidget::setName(const QString &_name)
{
    firstName_->setText(_name);
}

void AddNewContactWidget::setPhone(const QString &_phone)
{
    if (!phoneInput_->setPhone(_phone))
    {
        rawPhoneInput_->setText(_phone);
        phoneInput_->setVisible(false);
        rawPhoneInput_->setVisible(true);
    }
}

void AddNewContactWidget::onFormChanged()
{
    if (isFormComplete())
        emit formDataSufficient();
    else
        emit formDataIncomplete();
}

void AddNewContactWidget::onFullPhoneChanged(const QString& _fullPhone)
{
    // Disable proceeding while we check the phone correctness
    setIsPhoneCorrect(false);

    if (_fullPhone.isEmpty() || _fullPhone.size() < 3)
    {
        updatePhoneNumberLine(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        updateHint(QString(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        return;
    }

    gui_coll_helper collection(GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("phone", _fullPhone);
    collection.set_value_as_qstring("gui_locale", get_gui_settings()->get_value(settings_language, QString()));

    auto reqId = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
    coreReqs_.insert(reqId);
}

void AddNewContactWidget::onPhoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data)
{
    const auto it = coreReqs_.find(_seq);
    if (it == coreReqs_.end())
        return;
    else
        coreReqs_.erase(it);

    if (_data.remaining_lengths_.empty())
        return cleanHint();

    if (_data.is_phone_valid_)
    {
        setIsPhoneCorrect(true);
        cleanHint();

        return;
    }

    auto remaining = _data.remaining_lengths_[0];

    if (_data.is_phone_valid_ || !remaining)
    {
        setIsPhoneCorrect(true);
        cleanHint();

        return;
    }

    emit formDataIncomplete();

    QString text = Utils::GetTranslator()->getNumberString(
        std::abs(remaining),
        (remaining > 0) ? QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 digit left", "1")
        : QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 extra digit", "1"),
        (remaining > 0) ? QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 digits left", "2")
        : QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 extra digits", "2"),
        (remaining > 0) ? QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 digits left", "5")
        : QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 extra digits", "5"),
        (remaining > 0) ? QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 digits left", "21")
        : QT_TRANSLATE_NOOP3("add_new_contact_dialogs", "%1 extra digits", "21")).arg(std::abs(remaining));

    auto color = (remaining > 0) ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::SECONDARY_ATTENTION;

    updatePhoneNumberLine( Styling::getParameters().getColor(color));
    updateHint(text, Styling::getParameters().getColor(color));
}

void AddNewContactWidget::paintEvent(QPaintEvent *_event)
{
    QWidget::paintEvent(_event);

    QPainter p(this);
    textUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
    hintUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
}

void AddNewContactWidget::keyPressEvent(QKeyEvent *_event)
{
    QWidget::keyPressEvent(_event);

    bool isEnter = _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter;
    if (isEnter && isFormComplete())
    {
        emit formSubmissionRequested();
    }
}

void AddNewContactWidget::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);
    firstName_->setFocus();
}

bool AddNewContactWidget::isFormComplete() const
{
    return !firstName_->text().isEmpty()
        && isPhoneCorrect();
}

bool AddNewContactWidget::isPhoneComplete() const
{
    return !phoneInput_->getCountryCodeWidget()->text().isEmpty() &&
           !phoneInput_->getPhoneNumberWidget()->text().isEmpty();
}

bool AddNewContactWidget::isPhoneCorrect() const
{
    return isPhoneCorrect_;
}

QString AddNewContactWidget::getPhoneNumber() const
{
    return phoneInput_->isVisible() ? phoneInput_->getFullPhone() : rawPhoneInput_->text();
}

void AddNewContactWidget::cleanHint()
{
    updateHint(QString(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
    Utils::ApplyStyle(phoneInput_->getPhoneNumberWidget(), Styling::getParameters().getLineEditCommonQss());

    phoneInput_->getPhoneNumberWidget()->changeTextColor(Styling::getParameters().getColor(
        phoneInput_->getPhoneNumberWidget()->text().length() > 0
        ? Styling::StyleVariable::TEXT_SOLID
        : Styling::StyleVariable::BASE_PRIMARY));
    if (isFormComplete())
        emit formDataSufficient();

    update();
}

void AddNewContactWidget::updateHint(const QString& _hint, const QColor& _color)
{
    hintUnit_->setText(_hint, _color);
    hintUnit_->getHeight(Utils::scale_value(WIDGET_WIDTH - HINT_HORIZONTAL_OFFSET - 46 /* Right offset */));
    update();
}

void AddNewContactWidget::updatePhoneNumberLine(const QColor &_lineColor)
{
    Utils::ApplyStyle(phoneInput_->getPhoneNumberWidget(), Styling::getParameters().getLineEditCustomQss(_lineColor, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
    phoneInput_->getPhoneNumberWidget()->changeTextColor(Styling::getParameters().getColor(
        phoneInput_->getPhoneNumberWidget()->text().length() > 0
        ? Styling::StyleVariable::TEXT_SOLID
        : Styling::StyleVariable::BASE_PRIMARY));
}

void AddNewContactWidget::setIsPhoneCorrect(bool _correct)
{
    isPhoneCorrect_ = _correct;

    if (!_correct)
        emit formDataIncomplete();
}

void AddNewContactWidget::connectToLineEdit(LineEditEx *_lineEdit, LineEditEx *_onUp, LineEditEx *_onDown) const
{
    if (_onUp)
    {
        connect(_lineEdit, &LineEditEx::upArrow, _onUp, Utils::QOverload<>::of(&LineEditEx::setFocus));
        connect(_lineEdit, &LineEditEx::backtab, _onUp, [_onUp](){ _onUp->setFocus(Qt::BacktabFocusReason); });
    }

    if (_onDown)
    {
        connect(_lineEdit, &LineEditEx::downArrow, _onDown, Utils::QOverload<>::of(&LineEditEx::setFocus));
        connect(_lineEdit, &LineEditEx::tab, _onDown, [_onDown](){ _onDown->setFocus(Qt::TabFocusReason); });
    }
}

}

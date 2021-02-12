#include "stdafx.h"
#include "StatusDurationWidget.h"
#include "StatusCommonUi.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "core_dispatcher.h"
#include "controls/DialogButton.h"
#include "controls/TooltipWidget.h"
#include "controls/LineEditEx.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace
{

constexpr auto lineEditHeight = 32;
constexpr auto validationInterval() noexcept { return std::chrono::seconds(1); }

int maxWidth()
{
    return Utils::scale_value(342);
}

int sideMargin()
{
    return Utils::scale_value(32);
}

QFont headerLabelFont()
{
    if constexpr (platform::is_apple())
        return Fonts::appFontScaled(22, Fonts::FontWeight::Medium);

    return Fonts::appFontScaled(22);
}

QFont hintFont()
{
    return Fonts::appFontScaled(15);
}

int hintTopMargin()
{
    return Utils::scale_value(8);
}

QString dateFormat()
{
    return qsl("d.M.yyyy");
}

QString timeFormat()
{
    return qsl("h:m");
}

int labelBottomMargin()
{
    if constexpr (platform::is_apple())
        return Utils::scale_value(14);

    return Utils::scale_value(18);
}

int labelTopMargin()
{
    if constexpr (platform::is_apple())
        return Utils::scale_value(18);

    return Utils::scale_value(14);
}

//////////////////////////////////////////////////////////////////////////
// DateValidator
//////////////////////////////////////////////////////////////////////////

class DateValidator : public QValidator
{
public:
    DateValidator(QObject* _parent) : QValidator(_parent)
    {
        re.setPattern(qsl("\\A(?:(([0-2]{0,1}\\d{1})|([3][0,1]{1}))\\.(([0]{0,1}\\d{1})|([1]{1}[0-2]{1}))\\.([2]{1}[0]{1}[2-9]{1}\\d{1}))\\z"));
    }
    QValidator::State validate(QString& input, int&) const override
    {
        const auto match = re.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
        if (match.hasPartialMatch() || input.isEmpty())
            return QValidator::Intermediate;

        if (!match.hasMatch())
            return QValidator::Invalid;

        const auto date = QDate::fromString(input, dateFormat());
        if (!date.isValid() || QDate::currentDate() > date)
            return QValidator::Invalid;

        return QValidator::Acceptable;
    }
private:
    QRegularExpression re;
};

}


namespace Statuses
{

using namespace Ui;

//////////////////////////////////////////////////////////////////////////
// StatusDurationWidget_p
//////////////////////////////////////////////////////////////////////////

class StatusDurationWidget_p
{
public:

    QDate date() const
    {
        return QDate::fromString(date_->text(), dateFormat());
    }

    QTime time() const
    {
        return QTime::fromString(time_->text(), timeFormat());
    }

    LineEditEx* date_ = nullptr;
    LineEditEx* time_ = nullptr;
    DialogButton* nextButton_ = nullptr;
    QString status_;
    QString description_;
};

//////////////////////////////////////////////////////////////////////////
// StatusDurationWidget
//////////////////////////////////////////////////////////////////////////

StatusDurationWidget::StatusDurationWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<StatusDurationWidget_p>())
{
    setFixedSize(maxWidth(), Utils::scale_value(194));

    auto layout = Utils::emptyVLayout(this);

    auto label = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "Status duration"));
    label->init(headerLabelFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    label->setMaxWidthAndResize(maxWidth());

    layout->addSpacing(labelTopMargin());
    layout->addWidget(label);
    layout->addSpacing(labelBottomMargin());

    auto inputLayout = Utils::emptyHLayout();
    inputLayout->addSpacing(sideMargin());

    auto dateLayout = Utils::emptyVLayout();
    const LineEditEx::Options options{ {Qt::Key_Enter, Qt::Key_Return} };
    d->date_ = new LineEditEx(this, options);
    d->date_->setFont(Fonts::appFontScaled(15));
    d->date_->setCustomPlaceholder(QT_TRANSLATE_NOOP("status_popup", "Date"));
    d->date_->setValidator(new DateValidator(this));
    d->date_->setFixedWidth(Utils::scale_value(120));
    d->date_->setCustomPlaceholderColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    auto dateHint = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "dd.mm.yyyy"));
    dateHint->init(hintFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    Testing::setAccessibleName(d->date_, qsl("AS StatusDurationWidget dateEdit"));

    auto timeLayout = Utils::emptyVLayout();
    d->time_ = new LineEditEx(this, options);
    d->time_->setFont(Fonts::appFontScaled(15));
    d->time_->setCustomPlaceholder(QT_TRANSLATE_NOOP("status_popup", "Time"));
    d->time_->setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("([0-1][0-9]|[2][0-3]):([0-5][0-9])")), this));
    d->time_->setFixedWidth(Utils::scale_value(60));
    d->time_->setCustomPlaceholderColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    auto timeHint = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "hh:mm"));
    timeHint->init(hintFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    Testing::setAccessibleName(d->time_, qsl("AS StatusDurationWidget timeEdit"));

    Utils::ApplyStyle(d->date_, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));
    Utils::ApplyStyle(d->time_, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));

    connect(d->date_, &LineEditEx::textChanged, this, &StatusDurationWidget::validateInput);
    connect(d->time_, &LineEditEx::textChanged, this, &StatusDurationWidget::validateInput);
    connect(d->date_, &LineEditEx::enter, this, &StatusDurationWidget::applyNewDuration);
    connect(d->time_, &LineEditEx::enter, this, &StatusDurationWidget::applyNewDuration);

    auto validationTimer = new QTimer(this);
    validationTimer->setInterval(validationInterval());
    connect(validationTimer, &QTimer::timeout, this, &StatusDurationWidget::validateInput);
    validationTimer->start();

    dateLayout->addWidget(d->date_);
    dateLayout->addSpacing(hintTopMargin());
    dateLayout->addWidget(dateHint);
    timeLayout->addWidget(d->time_);
    timeLayout->addSpacing(hintTopMargin());
    timeLayout->addWidget(timeHint);

    inputLayout->addLayout(dateLayout);
    inputLayout->addSpacing(Utils::scale_value(40));
    inputLayout->addLayout(timeLayout);
    inputLayout->addStretch();

    layout->addLayout(inputLayout);

    auto backButton = createButton(this, QT_TRANSLATE_NOOP("status_popup", "Back"), DialogButtonRole::CANCEL);
    Testing::setAccessibleName(backButton, qsl("AS StatusDurationWidget backButton"));
    d->nextButton_ = createButton(this, QT_TRANSLATE_NOOP("status_popup", "Done"), DialogButtonRole::CONFIRM, false);
    Testing::setAccessibleName(d->nextButton_, qsl("AS StatusDurationWidget nextButton"));

    auto buttonsLayout = Utils::emptyHLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(backButton);
    buttonsLayout->addSpacing(Utils::scale_value(20));
    buttonsLayout->addWidget(d->nextButton_);
    buttonsLayout->addStretch();

    layout->addStretch();
    layout->addLayout(buttonsLayout);
    layout->addSpacing(Utils::scale_value(16));

    connect(backButton, &DialogButton::clicked, this, [this](){ Q_EMIT backClicked(QPrivateSignal()); });
    connect(d->nextButton_, &DialogButton::clicked, this, &StatusDurationWidget::applyNewDuration);
}

StatusDurationWidget::~StatusDurationWidget() = default;

void StatusDurationWidget::setStatus(const QString& _status, const QString& _description)
{
    d->status_ = _status;
    d->description_ = _description;
}

void StatusDurationWidget::showEvent(QShowEvent* _event)
{
    d->date_->setFocus();
}

bool StatusDurationWidget::validateInput()
{
    const auto date = d->date();
    const auto time = d->time();

    bool valid = false;
    if (date.isValid() && time.isValid())
        valid = QDateTime(date, time) > QDateTime::currentDateTime();
    else if (date.isValid() && d->time_->text().isEmpty())
        valid = date >= QDate::currentDate();
    else if (time.isValid() && d->date_->text().isEmpty())
        valid = true;

    d->nextButton_->setEnabled(valid);

    return valid;
}

void StatusDurationWidget::applyNewDuration()
{
    if (!validateInput())
        return;

    const auto date = d->date();
    const auto time = d->time();

    auto dateTime = QDateTime(date.isValid() ? date : QDate::currentDate(), time.isValid() ? time : QTime());

    if (!time.isValid() || !date.isValid() && time < QTime::currentTime())
        dateTime = dateTime.addDays(1);

    const auto seconds = QDateTime::currentDateTime().secsTo(dateTime);
    GetDispatcher()->setStatus(d->status_, seconds, d->description_);
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    Statuses::showToastWithDuration(std::chrono::seconds(seconds));
}


}

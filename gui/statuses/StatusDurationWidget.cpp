#include "stdafx.h"
#include "StatusDurationWidget.h"
#include "StatusCommonUi.h"
#include "utils/utils.h"
#include "../app_config.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "core_dispatcher.h"
#include "controls/TextWidget.h"
#include "controls/DialogButton.h"
#include "controls/DateTimeWidget.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace
{

int maxWidth() noexcept
{
    return Utils::scale_value(342);
}

int sideMargin() noexcept
{
    return Utils::scale_value(50);
}

auto rightMargin() noexcept
{
    return Utils::scale_value(50);
}

QFont headerLabelFont()
{
    if constexpr (platform::is_apple())
        return Fonts::appFontScaled(22, Fonts::FontWeight::Medium);

    return Fonts::appFontScaled(22);
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
        return dateTime_->date();
    }

    QTime time() const
    {
        return dateTime_->time();
    }
    DateTimePicker* dateTime_ = nullptr;
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
    TextRendering::TextUnit::InitializeParameters params{ headerLabelFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
    params.align_ = TextRendering::HorAligment::CENTER;
    label->init(params);
    label->setMaxWidthAndResize(maxWidth());

    layout->addSpacing(labelTopMargin());
    layout->addWidget(label);
    layout->addSpacing(labelBottomMargin());

    d->dateTime_ = new DateTimePicker(this, Ui::GetAppConfig().showSecondsInTimePicker(), true);
    d->dateTime_->setAccessibilityPrefix(qsl("StatusDurationWidget"));
    connect(d->dateTime_, &DateTimePicker::dateTimeChanged, this, &StatusDurationWidget::validateDate);
    connect(d->dateTime_, &DateTimePicker::dateTimeValidityChanged, this, &StatusDurationWidget::validateDate);
    connect(d->dateTime_, &DateTimePicker::enter, this, &StatusDurationWidget::applyNewDuration);

    auto dateTimeLayout = Utils::emptyHLayout();
    dateTimeLayout->addSpacing(sideMargin());
    dateTimeLayout->addWidget(d->dateTime_);
    dateTimeLayout->addSpacing(rightMargin());
    layout->addLayout(dateTimeLayout);

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

void StatusDurationWidget::setDateTime(const QDateTime& _dateTime)
{
    d->dateTime_->setDateTime(_dateTime);
}

void StatusDurationWidget::showEvent(QShowEvent* _event)
{
    d->dateTime_->setEnabledTime(true);
    d->dateTime_->setFocus();
}

bool StatusDurationWidget::validateDate()
{
    const auto isValid = d->dateTime_->isDateTimeValid();
    d->nextButton_->setEnabled(isValid);
    return isValid;
}

void StatusDurationWidget::applyNewDuration()
{
    if (!validateDate())
        return;

    const auto date = d->date();
    const auto time = d->time();

    auto dateTime = QDateTime(date.isValid() ? date : QDate::currentDate(), time.isValid() ? time : QTime());

    if (!time.isValid() || !date.isValid() && time < QTime::currentTime())
        dateTime = dateTime.addDays(1);

    // msecs are used (instead of .secsTo()) to garantee status target time might not jump backward, only forward
    const auto seconds = std::lrint(std::ceil(QDateTime::currentDateTime().msecsTo(dateTime) / 1000.0));

    GetDispatcher()->setStatus(d->status_, seconds, d->description_);
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    Statuses::showToastWithDuration(std::chrono::seconds(seconds));
}

}

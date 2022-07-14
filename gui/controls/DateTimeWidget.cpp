#include "stdafx.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"
#include "LineEditEx.h"
#include "controls/TooltipWidget.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

#include "DateTimeWidget.h"
#include "DateTimeWidget_p.h"

namespace
{
    constexpr auto lineEditHeightNonScalable() noexcept
    {
        return 32; // must be non-scaled
    }

    constexpr auto lineEditWidthTimeNonScalable() noexcept
    {
        return 32; // must be non-scaled
    }

    auto lineEditHeightScaled() noexcept
    {
        return Utils::scale_value(lineEditHeightNonScalable());
    }

    auto lineEditFont()
    {
        return Fonts::appFontScaled(15);
    }

    //! 1 <= _month <= 12
    const QString& getShortMonthName(size_t _month) noexcept
    {
        static auto months = std::array{
            QT_TRANSLATE_NOOP("calendar", "Jan"),
            QT_TRANSLATE_NOOP("calendar", "Feb"),
            QT_TRANSLATE_NOOP("calendar", "Mar"),
            QT_TRANSLATE_NOOP("calendar", "Apr"),
            QT_TRANSLATE_NOOP("calendar", "MayS"),
            QT_TRANSLATE_NOOP("calendar", "Jun"),
            QT_TRANSLATE_NOOP("calendar", "Jul"),
            QT_TRANSLATE_NOOP("calendar", "Aug"),
            QT_TRANSLATE_NOOP("calendar", "Sep"),
            QT_TRANSLATE_NOOP("calendar", "Oct"),
            QT_TRANSLATE_NOOP("calendar", "Nov"),
            QT_TRANSLATE_NOOP("calendar", "Dec"),
        };
        static auto fallbackString = qsl("????");

        return (_month >= 1 && _month <= months.size()) ? months[_month - 1] : fallbackString;
    }

    constexpr double alphaChannelDisableFields() noexcept
    {
        return 0.4;
    }

    const auto lineEditBottomPadding() noexcept
    {
        return Utils::scale_value(8);
    }

    void applyStyleLineEdit(Ui::LineEditEx* _edit, bool _isActive = false, bool _isError = false)
    {
        if (_edit->isEnabled())
        {
            if (_isActive)
            {
                Utils::ApplyStyle(_edit, Styling::getParameters().getLineEditCommonQss(
                        _isError,
                        lineEditHeightNonScalable(),
                        Styling::StyleVariable::PRIMARY));
                _edit->changeTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            }
            else
            {
                Utils::ApplyStyle(_edit, Styling::getParameters().getLineEditCommonQss(
                        _isError,
                        lineEditHeightNonScalable(),
                        Styling::StyleVariable::BASE_SECONDARY));
                _edit->changeTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            }
        }
        else
        {
            Utils::ApplyStyle(_edit, Styling::getParameters().getLineEditDisabledQss(
                    lineEditHeightNonScalable(),
                    Styling::StyleVariable::BASE_SECONDARY,
                    alphaChannelDisableFields()));
            _edit->changeTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY, alphaChannelDisableFields() });
        }
        _edit->update();
    }

    const auto lineEditWidthTime() noexcept
    {
        return Utils::scale_value(lineEditWidthTimeNonScalable());
    }

    const auto lineEditWidthDate(bool _showSeconds, bool _statusDate) noexcept
    {
        const auto maxWidth = 320;
        double reducingFactor = 3;

        if (_showSeconds && _statusDate)
            reducingFactor = 6;

        else if (_statusDate)
            reducingFactor = 4.5;

        else if (_showSeconds)
            reducingFactor = 4;

        return Utils::scale_value(maxWidth - static_cast<int>(reducingFactor * lineEditWidthTimeNonScalable()));
    }

    void setTextLineEditBottomMargins(Ui::LineEditEx* _edit)
    {
        auto textLineEditMargins = _edit->textMargins();
        textLineEditMargins.setBottom(lineEditBottomPadding());
        _edit->setTextMargins(textLineEditMargins);
    }
}

namespace Ui
{
    DateTimePicker::DateTimePicker(QWidget* _parent, bool _showSeconds, bool _statusDate)
        : QWidget(_parent)
    {
        static const LineEditEx::Options options{ {Qt::Key_Enter, Qt::Key_Return} };

        auto addTimeSeparator = [this](QHBoxLayout* _layout)
        {
            auto text = new LabelEx(this);
            text->setText(qsl(":"));
            text->setFont(lineEditFont());
            text->setAlignment(Qt::AlignCenter);
            text->setFixedHeight(lineEditHeightScaled());
            text->setContentsMargins(Utils::scale_value(QMargins(0, 0, 0, lineEditBottomPadding() + 2)));
            _layout->addSpacing(Utils::scale_value(3));
            _layout->addWidget(text, 0, Qt::AlignRight);
            _layout->addSpacing(Utils::scale_value(3));

            return text;
        };

        auto createTimeDigitsWidget = [this](auto _scrollCallback)
        {
            auto result = new LineEditEx(this, options);
            result->setFont(lineEditFont());
            result->setFixedWidth(lineEditWidthTime());
            result->setAlignment(Qt::AlignHCenter);
            setTextLineEditBottomMargins(result);

            connect(result, &LineEditEx::enter, this, &DateTimePicker::enter);
            connect(result, &LineEditEx::textChanged, this, &DateTimePicker::onTimeChangedByUser);
            connect(result, &LineEditEx::scrolled, this, std::forward<decltype(_scrollCallback)>(_scrollCallback));
            connect(result, &LineEditEx::focusIn, this, [result]()
            {
                QTimer::singleShot(0, result, [result]()
                {
                    result->selectAll();
                });
            });

            return result;
        };

        auto createDateWidget = [this, _showSeconds, _statusDate]()
        {
            auto dateEdit = new LineEditEx(this, options);
            dateEdit->setFont(lineEditFont());
            dateEdit->setFixedWidth(lineEditWidthDate(_showSeconds, _statusDate));
            dateEdit->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            dateEdit->setReadOnly(true);
            dateEdit->setCursor(Qt::PointingHandCursor);
            dateEdit->setFocusPolicy(Qt::NoFocus);
            setTextLineEditBottomMargins(dateEdit);

            connect(dateEdit, &LineEditEx::clicked, this, [this](Qt::MouseButton _button)
            {
                if (Qt::MouseButton::LeftButton == _button && !calendar_)
                    showCalendar();
            });

            return dateEdit;
        };

        dateEdit_ = createDateWidget();
        hoursEdit_ = createTimeDigitsWidget(&DateTimePicker::onHoursScroll);
        hoursEdit_->setValidator(new QRegExpValidator(QRegExp(qsl("^[0-9]|0[0-9]|1[0-9]|2[0-3]$"))));
        connect(hoursEdit_, &LineEditEx::focusOut, this, &DateTimePicker::validateHours);

        minutesEdit_ = createTimeDigitsWidget(&DateTimePicker::onMinutesScroll);
        minutesEdit_->setValidator(new QRegExpValidator(QRegExp(qsl("^[0-5][0-9]$"))));
        connect(minutesEdit_, &LineEditEx::focusOut, this, &DateTimePicker::validateMinutes);

        if (_showSeconds)
        {
            secondsEdit_ = createTimeDigitsWidget(&DateTimePicker::onSecondsScroll);
            secondsEdit_->setValidator(new QRegExpValidator(QRegExp(qsl("^[0-5][0-9]$"))));
        }

        auto timeLayout = Utils::emptyHLayout();
        timeLayout->addWidget(hoursEdit_, 0, Qt::AlignRight);
        separatorTime_ = addTimeSeparator(timeLayout);
        timeLayout->addWidget(minutesEdit_, 0, Qt::AlignRight);
        if (_showSeconds)
        {
            addTimeSeparator(timeLayout);
            timeLayout->addWidget(secondsEdit_);
        }

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(dateEdit_);
        rootLayout->addStretch();
        rootLayout->addLayout(timeLayout);

        setDateTime(QDateTime::currentDateTime());

        // To validate entered time as seconds in "now" moves on
        auto nowTimer = new QTimer(this);
        nowTimer->setInterval(std::chrono::seconds(1));
        connect(nowTimer, &QTimer::timeout, this, &DateTimePicker::validate);
        nowTimer->start();
    }

    void DateTimePicker::setDateTime(const QDateTime& _dateTime)
    {
        setDate(_dateTime.date());
        setTime(_dateTime.time());
    }

    void DateTimePicker::setDate(QDate _date)
    {
        date_ = _date;

        const auto dateText = (date_ == QDate::currentDate())
            ? QT_TRANSLATE_NOOP("calendar", "Today")
            : ((date_ == QDate::currentDate().addDays(1))
                ? QT_TRANSLATE_NOOP("calendar", "Tomorrow")
                : date_.toString(QT_TRANSLATE_NOOP("calendar", "%1. d, yyyy")).arg(getShortMonthName(date_.month())));
        dateEdit_->setText(dateText);
        Q_EMIT dateTimeChanged();
        validate();
    }

    void DateTimePicker::setTime(QTime _time)
    {
        hoursEdit_->setText(_time.toString(u"hh"));
        minutesEdit_->setText(_time.toString(u"mm"));
        if (secondsEdit_)
            secondsEdit_->setText(_time.toString(u"ss"));
        Q_EMIT dateTimeChanged();
        validate();
    }

    void DateTimePicker::setAccessibilityPrefix(const QString& _prefix)
    {
        Testing::setAccessibleName(dateEdit_, u"AS " % _prefix % u" dateEdit");
        Testing::setAccessibleName(hoursEdit_, u"AS " % _prefix % u" timeHoursEdit");
        Testing::setAccessibleName(minutesEdit_, u"AS " % _prefix % u" timeMinutesEdit");
        if (secondsEdit_)
            Testing::setAccessibleName(secondsEdit_, u"AS " % _prefix % u" timeSecondsEdit");
    }

    void DateTimePicker::showCalendar()
    {
        calendar_ = new CalendarWidget(this);
        calendar_->selectDay(date_);

        isCalendarVisible_ = true;

        connect(calendar_, &CalendarWidget::dateSelected, this, &DateTimePicker::setDate);
        connect(calendar_, &CalendarWidget::hidden, calendar_, &CalendarWidget::deleteLater, Qt::QueuedConnection);
        connect(calendar_, &CalendarWidget::hidden, this, &DateTimePicker::onCalendarHide);

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
        const auto boundaryGeometry = QGuiApplication::screens()[screen]->geometry();

        auto offset = QPoint((width() - calendar_->width()) / 2,
        dateEdit_->geometry().bottom() + dateEdit_->height() / 2);
        offset = mapToGlobal(offset);
        offset.ry() -= qMax(0, offset.y() + calendar_->height() - boundaryGeometry.bottom());
        calendar_->move(offset);

        applyStyleLineEdit(dateEdit_, true);

        calendar_->show();
    }

    void DateTimePicker::onCalendarHide()
    {
        isCalendarVisible_ = false;
        applyStyleLineEdit(dateEdit_, false);
    }

    void DateTimePicker::validate()
    {
        const auto wasDateTimeValid = timeValidity_.all();
        const auto t = time();
        const auto now = QTime::currentTime();
        const auto isToday = date() == QDate::currentDate();
        timeValidity_.date_ = date() >= QDate::currentDate();
        timeValidity_.hours_ = !isToday || t.hour() >= now.hour();
        timeValidity_.minutes_ = !isToday || t.hour() != now.hour() || t.minute() > now.minute() || (secondsEdit_ && t.minute() == now.minute());
        if (secondsEdit_)
            timeValidity_.seconds_ = !isToday || t.hour() != now.hour() || t.minute() != now.minute() || t.second() >= now.second();
        applyStyleLineEdit(dateEdit_,  isCalendarVisible_, !timeValidity_.date_);
        applyStyleLineEdit(hoursEdit_, false, !timeValidity_.hours_);
        applyStyleLineEdit(minutesEdit_, false, !timeValidity_.minutes_);
        if (secondsEdit_)
            applyStyleLineEdit(secondsEdit_, false, !timeValidity_.seconds_);

        if (const auto isValid = timeValidity_.all(); isValid != wasDateTimeValid)
            Q_EMIT dateTimeValidityChanged();
    }

    void DateTimePicker::validateHours()
    {
        if (hoursEdit_ && hoursEdit_->text().size() == 1)
            hoursEdit_->setText(ql1c('0') % hoursEdit_->text());
    }

    void DateTimePicker::validateMinutes()
    {
        if (minutesEdit_ && minutesEdit_->text().size() == 1)
            minutesEdit_->setText(ql1c('0') % minutesEdit_->text());
    }

    void DateTimePicker::maybeMoveFocusAfterUserInput()
    {
        if (isTimeEditScrolled_)
            return;

        LineEditEx* edit = hoursEdit_->hasFocus() && timeValidity_.hours_
            ? hoursEdit_
            : (minutesEdit_->hasFocus() && timeValidity_.minutes_ ? minutesEdit_ : nullptr);

        if (edit && edit->text().size() == 2)
        {
            if (auto next = qobject_cast<LineEditEx*>(edit->nextInFocusChain()))
            {
                next->selectAll();
                next->setFocus();
            }
        }
    }

    void DateTimePicker::onHoursScroll(int _step)
    {
        if (hoursEdit_)
        {
            isTimeEditScrolled_ = true;
            const auto hour = std::clamp(hoursEdit_->text().toInt() + _step, 0, 23);
            hoursEdit_->setText(QString::number(hour));
            isTimeEditScrolled_ = false;
        }
    }

    void DateTimePicker::onMinutesScroll(int _step)
    {
        updateTimeOnScroll(minutesEdit_, _step, [this](int _s) { onHoursScroll(_s); });
    }

    void DateTimePicker::onSecondsScroll(int _step)
    {
        updateTimeOnScroll(secondsEdit_, _step, [this](int _s) { onMinutesScroll(_s); });
    }

    void DateTimePicker::updateTimeOnScroll(LineEditEx* _edit, int _step, std::function<void(int)> _nextUpdate)
    {
        if (_edit)
        {
            isTimeEditScrolled_ = true;
            auto time = _edit->text().toInt() + _step;
            if (time > 59)
            {
                _nextUpdate(1);
                time = 0;
            }
            else if (time < 0)
            {
                _nextUpdate(-1);
                time = 59;
            }
            _edit->setText(QString::number(time));
            isTimeEditScrolled_ = false;
        }
    }

    void DateTimePicker::onTimeChangedByUser()
    {
        validate();
        maybeMoveFocusAfterUserInput();
    }

    QTime DateTimePicker::time() const
    {
        return {
            hoursEdit_->text().toInt(),
            minutesEdit_->text().toInt(),
            secondsEdit_ ? secondsEdit_->text().toInt() : 0
        };
    }

    void DateTimePicker::setEnabledDate(bool _isEnabled)
    {
        dateEdit_->setEnabled(_isEnabled);
        applyStyleLineEdit(dateEdit_, isCalendarVisible_);
    }

    void DateTimePicker::setEnabledTime(bool _isEnabled)
    {
        hoursEdit_->setEnabled(_isEnabled);
        minutesEdit_->setEnabled(_isEnabled);
        applyStyleLineEdit(hoursEdit_);
        applyStyleLineEdit(minutesEdit_);
        separatorTime_->setColor(_isEnabled ?
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } :
            Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY, alphaChannelDisableFields() });

        if (secondsEdit_)
        {
            secondsEdit_->setEnabled(_isEnabled);
            applyStyleLineEdit(secondsEdit_);
        }
    }
}

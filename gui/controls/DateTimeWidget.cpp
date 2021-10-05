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
    auto lineEditHeight() noexcept
    {
        return Utils::scale_value(32);
    }

    QString dateFormat()
    {
        return qsl("d.M.yyyy");
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

    void applyStyle(Ui::LineEditEx* _edit, bool _isError)
    {
        Utils::ApplyStyle(_edit, Styling::getParameters().getLineEditCommonQss(_isError, lineEditHeight()));
        _edit->update();
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


namespace Ui
{
    DateTimePicker::DateTimePicker(QWidget* _parent, bool _showSeconds)
        : QWidget(_parent)
    {
        static const LineEditEx::Options options{ {Qt::Key_Enter, Qt::Key_Return} };

        auto addTimeSeparator = [this](QHBoxLayout* _layout)
        {
            auto text = new LabelEx(this);
            text->setText(qsl(":"));
            text->setFont(lineEditFont());
            text->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
            text->setAlignment(Qt::AlignCenter);
            text->setFixedHeight(lineEditHeight());
            text->setContentsMargins(Utils::scale_value(QMargins(0, 0, 0, 2)));
            _layout->addSpacing(Utils::scale_value(3));
            _layout->addWidget(text, 0, Qt::AlignRight);
            _layout->addSpacing(Utils::scale_value(3));
        };

        auto createTimeDigitsWidget = [this](auto _scrollCallback)
        {
            auto result = new LineEditEx(this, options);
            result->setFont(lineEditFont());
            result->setFixedWidth(Utils::scale_value(32));
            result->setAlignment(Qt::AlignHCenter);
            applyStyle(result, false);
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

        auto createDateWidget = [this]()
        {
            auto dateEdit = new LineEditEx(this, options);
            dateEdit->setFont(lineEditFont());
            dateEdit->setFixedWidth(Utils::scale_value(120));
            dateEdit->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            Utils::ApplyStyle(dateEdit, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight()));
            dateEdit->setReadOnly(true);
            dateEdit->setCursor(Qt::PointingHandCursor);

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
        minutesEdit_ = createTimeDigitsWidget(&DateTimePicker::onMunutesScroll);
        minutesEdit_->setValidator(new QRegExpValidator(QRegExp(qsl("^[0-5][0-9]$"))));
        if (_showSeconds)
        {
            secondsEdit_ = createTimeDigitsWidget(&DateTimePicker::onSecondsScroll);
            secondsEdit_->setValidator(new QRegExpValidator(QRegExp(qsl("^[0-5][0-9]$"))));
        }

        auto timeLayout = Utils::emptyHLayout();
        timeLayout->addWidget(hoursEdit_, 0, Qt::AlignRight);
        addTimeSeparator(timeLayout);
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

    bool DateTimePicker::isTimeEmpty() const
    {
        return hoursEdit_->text().isEmpty() && minutesEdit_->text().isEmpty() && (!secondsEdit_ || secondsEdit_->text().isEmpty());
    }

    void DateTimePicker::showCalendar()
    {
        calendar_ = new CalendarWidget(this);
        calendar_->selectDay(date_);

        connect(calendar_, &CalendarWidget::dateSelected, this, &DateTimePicker::setDate);
        connect(calendar_, &CalendarWidget::hidden, calendar_, &CalendarWidget::deleteLater, Qt::QueuedConnection);

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
        const auto boundaryGeometry = QGuiApplication::screens()[screen]->geometry();

        auto offset = QPoint((width() - calendar_->width()) / 2,
            dateEdit_->geometry().bottom() + dateEdit_->height() / 2);
        offset = mapToGlobal(offset);
        offset.ry() -= qMax(0, offset.y() + calendar_->height() - boundaryGeometry.bottom());
        calendar_->move(offset);

        calendar_->show();
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
        applyStyle(dateEdit_, !timeValidity_.date_);
        applyStyle(hoursEdit_, !timeValidity_.hours_);
        applyStyle(minutesEdit_, !timeValidity_.minutes_);
        if (secondsEdit_)
            applyStyle(secondsEdit_, !timeValidity_.seconds_);

        if (const auto isValid = timeValidity_.all(); isValid != wasDateTimeValid)
            Q_EMIT dateTimeValidityChanged();
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

    void DateTimePicker::onMunutesScroll(int _step)
    {
        updateTimeOnScroll(minutesEdit_, _step, [this](int _s) { onHoursScroll(_s); });
    }

    void DateTimePicker::onSecondsScroll(int _step)
    {
        updateTimeOnScroll(secondsEdit_, _step, [this](int _s) { onMunutesScroll(_s); });
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

}

#pragma once

namespace Ui
{
    class LineEditEx;
    class CalendarWidget;

    class DateTimePicker : public QWidget
    {
        Q_OBJECT
    public:
        DateTimePicker(QWidget* _parent, bool _showSeconds = true);
        bool isTimeEmpty() const;
        void setDateTime(const QDateTime& _dateTime);
        QTime time() const;
        void setAccessibilityPrefix(const QString& _prefix);
        QDate date() const { return date_; }
        bool isDateTimeValid() const { return timeValidity_.all(); }

    Q_SIGNALS:
        void dateTimeChanged();
        void dateTimeValidityChanged();
        void enter();

    private:
        void setDate(QDate _date);
        void setTime(QTime _time);
        void showCalendar();
        void validate();
        void onTimeChangedByUser();
        void maybeMoveFocusAfterUserInput();

        void onHoursScroll(int _step);
        void onMunutesScroll(int _step);
        void onSecondsScroll(int _step);
        void updateTimeOnScroll(LineEditEx* _edit, int _step, std::function<void(int)> _nextUpdate);

    private:
        QDate date_;
        LineEditEx* dateEdit_ = nullptr;
        LineEditEx* hoursEdit_ = nullptr;
        LineEditEx* minutesEdit_ = nullptr;
        LineEditEx* secondsEdit_ = nullptr;
        QPointer<CalendarWidget> calendar_;
        bool isTimeEditScrolled_ = false;

        struct {
            bool date_ = true;
            bool hours_ = true;
            bool minutes_ = true;
            bool seconds_ = true;
            bool all() const { return date_ && hours_ && minutes_ && seconds_; }
        } timeValidity_;
    };
}

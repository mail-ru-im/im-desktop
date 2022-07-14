#pragma once
#include "../utils/animations/SlideToNewState.h"


namespace Ui
{
    class LabelEx;
    class CustomButton;

    constexpr int numRowsInCalendar() noexcept { return 6; }
    constexpr int numDaysInWeek() noexcept { return 7; }

    class Month : public QDate
    {
    public:
        Month() : QDate() {}
        Month(QDate _monthYear) : QDate(_monthYear) {}
        Month(int _month, int _year) : QDate(_year, _month, 1) {}

    private:
        friend bool operator<(const Month& _m1, const Month& _m2)
        {
            return _m2.year() > _m1.year() || (_m2.year() == _m1.year() && _m2.month() > _m1.month());
        }
    };


    class MonthYearHeader : public QWidget
    {
    public:
        MonthYearHeader(QWidget* _parent);

        void setMonth(Month _month);
        Month month() const { return month_; }

    protected:
        LabelEx* monthWidget_ = nullptr;
        LabelEx* yearWidget_ = nullptr;
        Month month_;
    };

    class MonthYearHeaderAnimated : public Utils::SlideToNewStateAnimated<MonthYearHeader>
    {
    public:
        MonthYearHeaderAnimated(QWidget* _parent);

        //! In case of animation external call to SlideToNewStateAnimated::animation()->start is needed
        void setMonth(Month _month, bool _animate = false);
    };


    class DayWidget : public QPushButton
    {
    public:
        DayWidget(QWidget* _parent);

        bool isToday() const;

        void setDate(QDate _date, int _currentMonth);

        QDate date() const { return date_; }
        void updateAppearence();
        bool isHovered() const;
        void setHovered(bool _isHovered);

        //! Mark as a selected calendar day
        void select();

        void paintEvent(QPaintEvent*) override;
        void leaveEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;

    private:
        QDate date_;
        QColor textColor_;
        QColor backgroundColor_;
        bool isHovered_ = false;

        int currentMonth_;
    };


    class DaysGrid : public QWidget
    {
        Q_OBJECT
    public:
        DaysGrid(QWidget* _parent);

        DayWidget* getDayWidget(int _row, int _column);

        DayWidget* getDayWidget(QDate _date);
        DayWidget* getFirstGridDay();

        void setSelectedDay(QDate _date);
        void setMonth(Month _month);

        Month getCurrentMonth();

    Q_SIGNALS:
        void dateSelected(QDate);

    protected:
        void createGridDays(QGridLayout* _layout);

    protected:
        std::array<DayWidget*, numRowsInCalendar() * numDaysInWeek()> days_;
        QDate selectedDay_;
    };


    class DaysGridAnimated : public Utils::SlideToNewStateAnimated<DaysGrid>
    {
    public:
        DaysGridAnimated(QWidget* _parent);

        //! In case of animation external call to SlideToNewStateAnimated::animation()->start is needed
        void setMonth(Month _month, bool _animate = false);
    };


    class CalendarWidget : public QWidget
    {
        Q_OBJECT
    public:
        CalendarWidget(QWidget* _parent);
        ~CalendarWidget() override;
        void setMonth(Month _month, bool _animate = false);
        void selectDay(QDate _day);

    Q_SIGNALS:
        void dateSelected(QDate);
        void hidden();

    protected:
        Month getCurrentMonth();
        void paintEvent(QPaintEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        const bool allowPastChoice_ = false;

        CustomButton* monthBackButton_ = nullptr;
        CustomButton* monthForwardButton_ = nullptr;
        MonthYearHeaderAnimated* monthYearHeader_ = nullptr;
        DaysGridAnimated* days_ = nullptr;
        QParallelAnimationGroup* groupAnimation_ = nullptr;
    };
}

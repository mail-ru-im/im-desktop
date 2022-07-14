#include "stdafx.h"
#include "DateTimeWidget_p.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "LabelEx.h"
#include "CustomButton.h"
#include "../styles/ThemeParameters.h"
#include "../fonts.h"


namespace
{
using namespace std::chrono_literals;

QSize calendarSize() noexcept { return Utils::scale_value(QSize(324, 368)); }

QSize daysGridSize() noexcept { return Utils::scale_value(QSize(304, 260)); }

QSize monthYearHeaderSize() noexcept { return Utils::scale_value(QSize{224, 52}); }

QSize monthNavigationButtonSize() noexcept { return Utils::scale_value(QSize{20, 20}); }

QSize dayWidgetSize() noexcept { return Utils::scale_value(QSize{40, 40}); }

QMargins calendarPadding() noexcept { return Utils::scale_value(QMargins(10, 0, 10, 16)); }

int dayWidgetRectangleRoundRadius() noexcept { return Utils::scale_value(6); }

int daysGridSpacing() noexcept { return Utils::scale_value(4); }

QFont calendarFont() { return Fonts::appFontScaled(16); }

QFont selectedDayFont() { return Fonts::appFontScaled(16, Fonts::FontWeight::Medium); }

constexpr std::chrono::milliseconds animationDuration() noexcept { return 200ms; }

//! 0 <= _day < 7
const QString& getDayOfWeekShortString(size_t _day) noexcept
{
    static auto daysOfWeek = std::array{
        QT_TRANSLATE_NOOP("calendar", "Mon"),
        QT_TRANSLATE_NOOP("calendar", "Tue"),
        QT_TRANSLATE_NOOP("calendar", "Wed"),
        QT_TRANSLATE_NOOP("calendar", "Thu"),
        QT_TRANSLATE_NOOP("calendar", "Fri"),
        QT_TRANSLATE_NOOP("calendar", "Sat"),
        QT_TRANSLATE_NOOP("calendar", "Sun"),
    };
    static auto fallbackString = qsl("??");

    return (_day >= 0 && _day < daysOfWeek.size()) ? daysOfWeek[_day] : fallbackString;
}

//! 1 <= _month <= 12
const QString& getMonthName(size_t _month) noexcept
{
    static auto months = std::array{
        QT_TRANSLATE_NOOP("calendar", "January"),
        QT_TRANSLATE_NOOP("calendar", "Febuary"),
        QT_TRANSLATE_NOOP("calendar", "March"),
        QT_TRANSLATE_NOOP("calendar", "April"),
        QT_TRANSLATE_NOOP("calendar", "May"),
        QT_TRANSLATE_NOOP("calendar", "June"),
        QT_TRANSLATE_NOOP("calendar", "July"),
        QT_TRANSLATE_NOOP("calendar", "August"),
        QT_TRANSLATE_NOOP("calendar", "September"),
        QT_TRANSLATE_NOOP("calendar", "October"),
        QT_TRANSLATE_NOOP("calendar", "November"),
        QT_TRANSLATE_NOOP("calendar", "December"),
    };
    static auto fallbackString = qsl("????");

    return (_month >= 1 && _month <= months.size()) ? months[_month - 1] : fallbackString;
}

Styling::ThemeColorKey normalTextColorKey()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
}

QColor normalTextColor()
{
    return Styling::getColor(normalTextColorKey());
}

QColor pastTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
}

QColor nextMonthDayTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
}

QColor todayTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
}

auto weekDayNameColorKey()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
}

auto monthNavigationButtonNormalColor()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
}

auto monthNavigationButtonDisabledColorKey()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY };
}

QColor selectedDayTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
}

QColor defaultBackgroundColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
}

QColor hoveredDayBackgroundColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
}

QColor selectedDayBackgroundColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
}

QColor focusedDayBorderColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
}

enum class NavigationArrowDirection
{
    Forward,
    Backward,
};

QDate calcFirstDateOfMonthGrid(int _month, int _year)
{
    auto start = QDate(_year, _month, 1);
    auto startSupplement = start.dayOfWeek() - 1;
    auto endSupplement = Ui::numDaysInWeek() - (start.addDays(start.daysInMonth() - 1)).dayOfWeek();

    const auto daysUsed = start.daysInMonth() + startSupplement + endSupplement;
    im_assert(daysUsed % Ui::numDaysInWeek() == 0);
    im_assert(daysUsed / Ui::numDaysInWeek() == 5 || daysUsed / Ui::numDaysInWeek() == 6);
    if (daysUsed / Ui::numDaysInWeek() == 5 && startSupplement < endSupplement)
        startSupplement += Ui::numDaysInWeek();

    return start.addDays(-startSupplement);
}

class CustomButtomCustomHover : public Ui::CustomButton
{
public:
explicit CustomButtomCustomHover(QWidget* _parent, const QString& _svgName = QString(), const QSize& _iconSize = QSize(), const Styling::ColorParameter& _defaultColor = Styling::ColorParameter())
    : CustomButton(_parent, _svgName, _iconSize, _defaultColor) {}

void leaveEvent(QEvent* _e) override
{
    if constexpr (platform::is_apple())
        QGuiApplication::changeOverrideCursor(Qt::ArrowCursor);
    CustomButton::leaveEvent(_e);
}

void enterEvent(QEvent* _e) override
{
    if constexpr (platform::is_apple())
    {
        if (isEnabled())
            QGuiApplication::changeOverrideCursor(Qt::PointingHandCursor);
    }
    CustomButton::enterEvent(_e);
}
};

}


namespace Ui
{

DayWidget::DayWidget(QWidget* _parent)
    : QPushButton(_parent)
    , textColor_(Qt::black)
{
    setFixedSize(dayWidgetSize());
    setCheckable(true);
    setFocusPolicy(Qt::FocusPolicy::TabFocus);
    setAutoDefault(true);
    setFont(calendarFont());
    updateAppearence();
}

bool DayWidget::isToday() const
{
    return date_ == QDate::currentDate();
}

void DayWidget::setDate(QDate _date, int _currentMonth)
{
    date_ = _date;
    currentMonth_ = _currentMonth;
    setText(date_.isValid() ? QString::number(date_.day()) : QString());
    setChecked(false);
    updateAppearence();
}

void DayWidget::paintEvent(QPaintEvent* _event)
{
    static const auto cornerRadius = dayWidgetRectangleRoundRadius();

    QPainter p(this);
    p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath border;
    border.addRoundedRect(rect(), cornerRadius, cornerRadius);
    p.setClipPath(border);

    if (backgroundColor_.isValid())
        p.fillRect(rect(), backgroundColor_);

    if (hasFocus())
    {
        p.setBrush(Qt::NoBrush);
        p.setPen({focusedDayBorderColor(), Utils::fscale_value(2.0)});
        p.drawPath(border);
    }

    if (const auto t = text(); !t.isEmpty())
    {
        im_assert(textColor_.isValid());
        p.setPen(textColor_);
        p.setFont(font());
        p.drawText(rect(), Qt::AlignCenter, t);
    }
}

void DayWidget::updateAppearence()
{
    setEnabled(date_ >= QDate::currentDate());
    setCursor(isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if (isChecked())
        backgroundColor_ = selectedDayBackgroundColor();
    else if (isHovered())
        backgroundColor_ = hoveredDayBackgroundColor();
    else
        backgroundColor_ = defaultBackgroundColor();

    if (isChecked())
        textColor_ = selectedDayTextColor();
    else if (!isEnabled())
        textColor_ = pastTextColor();
    else if (isToday())
        textColor_ = todayTextColor();
    else if (date_.month() == currentMonth_)
        textColor_ = normalTextColor();
    else
        textColor_ = nextMonthDayTextColor();

    setFont(isChecked() ? selectedDayFont() : calendarFont());

    update();
}

bool DayWidget::isHovered() const
{
    return isHovered_;
}

void DayWidget::select()
{
    setChecked(true);
    updateAppearence();
}

void DayWidget::setHovered(bool _isHovered)
{
    if (_isHovered == isHovered_ || !isEnabled())
        return;

    isHovered_ = _isHovered;
    updateAppearence();
}

void DayWidget::leaveEvent(QEvent* _e)
{
    setHovered(false);
    if constexpr (platform::is_apple())
        QGuiApplication::changeOverrideCursor(Qt::ArrowCursor);
    QPushButton::leaveEvent(_e);
}

void DayWidget::enterEvent(QEvent* _e)
{
    setHovered(true);
    if constexpr (platform::is_apple())
        QGuiApplication::changeOverrideCursor(cursor());
    QPushButton::enterEvent(_e);
}


CalendarWidget::CalendarWidget(QWidget* _parent)
    : QWidget(_parent, Qt::Popup | Qt::NoDropShadowWindowHint)
    , groupAnimation_(new QParallelAnimationGroup(this))
{
    setFixedSize(calendarSize());
    setStyleSheet(qsl("background-color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    if constexpr (platform::is_apple())
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, this, &CalendarWidget::close);

    auto createMonthNavigationWidget = [this](const QString& _name, NavigationArrowDirection _direction)
    {
        auto button = new CustomButtomCustomHover(this, _name, monthNavigationButtonSize(), monthNavigationButtonNormalColor());
        button->setDisabledColor(monthNavigationButtonDisabledColorKey());
        button->setFixedSize(dayWidgetSize());
        Testing::setAccessibleName(button, _direction == NavigationArrowDirection::Forward
            ? qsl("AS calendar nextMonth")
            : qsl("AS calendar previousMonth"));

        connect(button, &CustomButton::clicked, [this, _direction]()
        {
            const auto increment = (_direction == NavigationArrowDirection::Forward) ? 1 : -1;
            const auto date = getCurrentMonth().addMonths(increment);
            setMonth(date, true);
            setFocus();
        });
        return button;
    };

    auto createDayOfWeekWidget = [this](int _dayOfWeek)
    {
        auto w = new LabelEx(this);
        w->setText(getDayOfWeekShortString(_dayOfWeek));
        w->setFixedSize(dayWidgetSize());
        w->setColor(weekDayNameColorKey());
        w->setFont(calendarFont());
        w->setAlignment(Qt::AlignCenter);
        return w;
    };

    auto headerLayout = Utils::emptyHLayout();
    headerLayout->addWidget(monthBackButton_ = createMonthNavigationWidget(qsl(":/calendar/month_back_button"), NavigationArrowDirection::Backward));
    headerLayout->addWidget(monthYearHeader_ = new MonthYearHeaderAnimated(this));
    headerLayout->addWidget(monthForwardButton_ = createMonthNavigationWidget(qsl(":/calendar/month_forward_button"), NavigationArrowDirection::Forward));
    headerLayout->setAlignment(monthBackButton_, Qt::AlignRight);

    auto weekDaysHeaderLayout = Utils::emptyHLayout();
    weekDaysHeaderLayout->addWidget(createDayOfWeekWidget(0));
    for (auto dayNum = 1; dayNum < numDaysInWeek(); ++dayNum)
    {
        weekDaysHeaderLayout->addSpacing(daysGridSpacing());
        weekDaysHeaderLayout->addWidget(createDayOfWeekWidget(dayNum));
    }

    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setContentsMargins(calendarPadding());
    rootLayout->addLayout(headerLayout);
    rootLayout->addLayout(weekDaysHeaderLayout);
    rootLayout->addWidget(days_ = new DaysGridAnimated(this));

    setMonth(QDate::currentDate());

    groupAnimation_->addAnimation(monthYearHeader_->animation());
    groupAnimation_->addAnimation(days_->animation());

    connect(days_->content(), &DaysGrid::dateSelected, this, [this](QDate _date)
    {
        Q_EMIT dateSelected(_date);
        close();
    });

    if constexpr (platform::is_apple())
        QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
}

CalendarWidget::~CalendarWidget()
{
    if constexpr (platform::is_apple())
        QGuiApplication::restoreOverrideCursor();
}

void CalendarWidget::setMonth(Month _month, bool _animate)
{
    monthYearHeader_->setMonth(_month, _animate);
    days_->setMonth(_month, _animate);
    if (_animate)
    {
        if (groupAnimation_->state() == QAbstractAnimation::Running)
            groupAnimation_->stop();
        groupAnimation_->start();
    }

    if (!allowPastChoice_)
    {
        const auto today = QDate::currentDate();
        const auto current = getCurrentMonth();
        monthBackButton_->setDisabled(today.month() == current.month() && today.year() == current.year());
    }
}

void CalendarWidget::selectDay(QDate _day)
{
    if (_day.isValid())
    {
        setMonth(_day);
        days_->content()->setSelectedDay(_day);
    }
}

void DaysGrid::setSelectedDay(QDate _date)
{
    selectedDay_ = _date;

    if (const auto day = getDayWidget(_date))
    {
        day->select();
        day->setFocus();
    }
}

DaysGrid::DaysGrid(QWidget* _parent)
    : QWidget(_parent)
{
    setFixedSize(daysGridSize());

    auto daysLayout = Utils::emptyGridLayout(this);
    createGridDays(daysLayout);
    daysLayout->setSpacing(daysGridSpacing());
}

void DaysGrid::createGridDays(QGridLayout* _layout)
{
    auto dayIt = std::begin(days_);
    for (auto row = 0; row < numRowsInCalendar(); ++row)
    {
        for (auto dayNum = 0; dayNum < numDaysInWeek(); ++dayNum)
        {
            im_assert(dayIt < std::end(days_));
            *dayIt = new Ui::DayWidget(this);
            Testing::setAccessibleName(*dayIt, qsl("AS calendar day-row%1-column%2").arg(row, dayNum));
            if constexpr (platform::is_apple())  // https://stackoverflow.com/a/41466352
                (*dayIt)->setAttribute(Qt::WA_LayoutUsesWidgetRect);
            _layout->addWidget(*dayIt, row, dayNum);

            connect(*dayIt, &QAbstractButton::clicked, this, [this, day = *dayIt]()
            {
                Q_EMIT dateSelected(day->date());
            });

            ++dayIt;
        }
    }
}

void DaysGrid::setMonth(Month _month)
{
    auto date = calcFirstDateOfMonthGrid(_month.month(), _month.year());
    for (auto row = 0; row < numRowsInCalendar(); ++row)
    {
        for (auto dayNum = 0; dayNum < numDaysInWeek(); ++dayNum)
        {
            getDayWidget(row, dayNum)->setDate(date, _month.month());
            date = date.addDays(1);
        }
    }

    setSelectedDay(selectedDay_);
}

DaysGridAnimated::DaysGridAnimated(QWidget* _parent)
    : Utils::SlideToNewStateAnimated<DaysGrid>(_parent)
{
    setDuration(animationDuration().count());
}

MonthYearHeaderAnimated::MonthYearHeaderAnimated(QWidget* _parent)
    : Utils::SlideToNewStateAnimated<MonthYearHeader>(_parent)
{
    setDuration(animationDuration().count());
    setAttribute(Qt::WA_NoSystemBackground);
}

void MonthYearHeaderAnimated::setMonth(Month _month, bool _animate)
{
    if (_animate)
    {
        prepareNextState(
            [_month](MonthYearHeader* _header) { _header->setMonth(_month); },
            content()->month() < _month ? AnimationDirection::Right : AnimationDirection::Left);
    }
    else
    {
        content()->setMonth(_month);
    }
}

void DaysGridAnimated::setMonth(Month _month, bool _animate)
{
    if (_animate)
    {
        prepareNextState(
            [_month](DaysGrid* _grid) { _grid->setMonth(_month); },
            content()->getCurrentMonth() < _month ? AnimationDirection::Right : AnimationDirection::Left);
    }
    else
    {
        content()->setMonth(_month);
    }
}

MonthYearHeader::MonthYearHeader(QWidget* _parent)
    : QWidget(_parent)
{
    setFixedSize(monthYearHeaderSize());
    setAttribute(Qt::WA_NoSystemBackground);

    auto createMonthOrYearWidget = [this]()
    {
        auto w = new LabelEx(this);
        w->setAlignment(Qt::AlignCenter);
        w->setFont(calendarFont());
        w->setColor(normalTextColorKey());
        return w;
    };

    auto rootLayout = Utils::emptyHLayout(this);
    rootLayout->addStretch();
    rootLayout->addWidget(monthWidget_ = createMonthOrYearWidget());
    rootLayout->setAlignment(monthWidget_, Qt::AlignCenter);
    rootLayout->addSpacing(Utils::scale_value(8));
    rootLayout->addWidget(yearWidget_ = createMonthOrYearWidget());
    rootLayout->setAlignment(yearWidget_, Qt::AlignCenter);
    rootLayout->addStretch();
}

void MonthYearHeader::setMonth(Month _month)
{
    month_ = _month;
    monthWidget_->setText(getMonthName(_month.month()));
    yearWidget_->setText(QString::number(_month.year()));
}

DayWidget* DaysGrid::getDayWidget(int _row, int _column)
{
    if (size_t i = _row * numDaysInWeek() + _column; i >= 0 && i < days_.size())
        return days_[i];

    return nullptr;
}

DayWidget* DaysGrid::getDayWidget(QDate _date)
{
    if (_date.isValid())
    {
        const auto numInGrid = getFirstGridDay()->date().daysTo(_date);
        return getDayWidget(numInGrid / numDaysInWeek(), numInGrid % numDaysInWeek());
    }

    return nullptr;
}

DayWidget* DaysGrid::getFirstGridDay()
{
    return getDayWidget(0, 0);
}

Month DaysGrid::getCurrentMonth()
{
    return getDayWidget(2, 3)->date();
}

Month CalendarWidget::getCurrentMonth()
{
    return days_->content()->getCurrentMonth();
}

void CalendarWidget::paintEvent(QPaintEvent* _event)
{
    static const auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE);

    QWidget::paintEvent(_event);

    // Draw border as a substitue of shadow around window
    QPainter painter(this);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(borderColor, Utils::fscale_value(2.0)));
    painter.drawRect(rect());
}

void CalendarWidget::hideEvent(QHideEvent* _event)
{
    QWidget::hideEvent(_event);
    Q_EMIT hidden();
}

} // namespace


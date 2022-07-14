#include "TabBar.h"

#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/InterConnector.h"
#include "../utils/features.h"

#include "../controls/TooltipWidget.h"
#include "MainWindow.h"
#include "AppsPage.h"

#include "../fonts.h"

#include "styles/ThemeParameters.h"
#include "mini_apps/MiniAppsUtils.h"
#include "Common.h"

namespace
{
    int tabIconSize() noexcept
    {
        return Utils::scale_value(28);
    }

    int itemPadding() noexcept
    {
        return Utils::scale_value(Features::isStatusInAppsNavigationBar() ? 8 : 4);
    }

    int badgeTopOffset() noexcept
    {
        return Utils::scale_value(Features::isStatusInAppsNavigationBar() ? 3 : 2);
    }

    int badgeHeight() noexcept
    {
        return Utils::scale_value(18);
    }

    int badgeOffset() noexcept
    {
        return Utils::scale_value(16);
    }

    int badgeRightOffset() noexcept
    {
        return Utils::scale_value(4);
    }

    QFont getNameTextFont()
    {
        const auto static f = Fonts::appFontScaled(12, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        return f;
    }

    QFont getBadgeTextForNumbersFont()
    {
        const auto isNavigationBar = Features::isStatusInAppsNavigationBar();
        static const auto fontWeight = isNavigationBar ? (platform::is_apple() ? Fonts::FontWeight::Bold : Fonts::FontWeight::SemiBold)
                                       : (platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        static const auto f = Fonts::appFontScaled(isNavigationBar ? 13 : 11, fontWeight);
        return f;
    }

    QFont getCalendarFont()
    {
        const auto static f = Fonts::appFontScaled(14, Fonts::FontWeight::Medium);
        return f;
    }

    Styling::ThemeColorKey hoveredColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER };
    }

    Styling::ThemeColorKey normalColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY };
    }

    Styling::ThemeColorKey activeColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
    }

    Styling::ThemeColorKey badgeTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
    }

    QColor badgeBalloonColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    auto tabBarDefaultHeight() noexcept
    {
        return Utils::scale_value(Features::isStatusInAppsNavigationBar() ? 48 : 52);
    }

    auto appBarContentMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto appBarItemSize() noexcept
    {
        return Utils::scale_value(Features::isStatusInAppsNavigationBar() ? 56 : 60);
    }

    auto appBarItemCornerRadius() noexcept
    {
        return Utils::scale_value(4);
    }

    auto appBarItemNormalColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    auto appBarItemSelectedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_SELECTED);
    }

    auto appBarItemHoveredColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    auto appBarIconNormalColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    auto appBarIconActiveColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    constexpr auto tooltipDelay() noexcept
    {
        return std::chrono::milliseconds(500);
    }

    auto getBadgeColor(const QString& _type) noexcept
    {
        static std::unordered_set<QString> grayCounterTypes = { Utils::MiniApps::getTasksId(), Utils::MiniApps::getNewsId(), Utils::MiniApps::getPollsId()};
        if (grayCounterTypes.find(_type) != grayCounterTypes.end())
            return Utils::Badge::Color::Gray;
        return Utils::Badge::Color::Red;
    }
}

namespace Ui
{
    BaseBarItem::BaseBarItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : SimpleListItem(_parent)
        , activeIconPixmap_(Utils::StyledPixmap(_iconActivePath, { tabIconSize(), tabIconSize() }, activeColor()))
        , hoveredIconPixmap_(Utils::StyledPixmap(_iconPath, { tabIconSize(), tabIconSize() }, hoveredColor()))
        , normalIconPixmap_(Utils::StyledPixmap(_iconPath, { tabIconSize(), tabIconSize() }, normalColor()))
        , isSelected_(false)
    {

    }

    BaseBarItem::~BaseBarItem() = default;

    void BaseBarItem::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;
            update();
        }
    }

    bool BaseBarItem::isSelected() const
    {
        return isSelected_;
    }

    void BaseBarItem::setBadgeText(const QString& _text)
    {
        if (badgeText_ == _text)
            return;

        badgeText_ = _text;
        if (badgeTextUnit_)
        {
            badgeTextUnit_->setText(badgeText_);
        }
        else
        {
            badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText_);
            setBadgeFont(getBadgeTextForNumbersFont());
        }
        update();
    }

    void BaseBarItem::setBadgeIcon(const QString& _icon)
    {
        if (badgeIcon_ != _icon)
        {
            badgeIcon_ = _icon;
            using st = Styling::StyleVariable;
            auto getColor = [](st _color)
            {
                return Styling::ThemeColorKey { _color };
            };
            badgePixmap_ = Utils::LayeredPixmap(
                badgeIcon_,
                {
                    { qsl("border"), getColor(st::BASE_GLOBALWHITE) },
                    { qsl("bg"), getColor(st::SECONDARY_ATTENTION) },
                    { qsl("star"), getColor(st::BASE_GLOBALWHITE) },
                },
                { badgeHeight(), badgeHeight() });
            update();
        }
    }

    void BaseBarItem::updateItemInfo()
    {
        const auto counters = Logic::getUnreadCounters();
        setBadgeText(Utils::getUnreadsBadgeStr(counters.unreadMessages_));
        setBadgeIcon(counters.attention_ ? qsl(":/tab/attention") : QString());
    }

    void BaseBarItem::resetItemInfo()
    {
        setBadgeText(Utils::getUnreadsBadgeStr(0));
        setBadgeIcon(QString());
    }

    void BaseBarItem::setName(const QString& _name)
    {
        name_ = _name;
    }

    void BaseBarItem::setNormalIconPixmap(const Utils::StyledPixmap& _px)
    {
        if (normalIconPixmap_ == _px)
            return;
        normalIconPixmap_ = _px;
        update();
    }

    void BaseBarItem::setHoveredIconPixmap(const Utils::StyledPixmap& _px)
    {
        if (hoveredIconPixmap_ == _px)
            return;
        hoveredIconPixmap_ = _px;
        update();
    }

    void BaseBarItem::setActiveIconPixmap(const Utils::StyledPixmap& _px)
    {
        if (activeIconPixmap_ == _px)
            return;
        activeIconPixmap_ = _px;
        update();
    }

    QPixmap BaseBarItem::activeIconPixmap()
    {
        return activeIconPixmap_.actualPixmap();
    }

    QPixmap BaseBarItem::hoveredIconPixmap()
    {
        return hoveredIconPixmap_.actualPixmap();
    }

    QPixmap BaseBarItem::normalIconPixmap()
    {
        return normalIconPixmap_.actualPixmap();
    }

    void BaseBarItem::setBadgeFont(const QFont & _font)
    {
        if (badgeTextUnit_ && badgeTextUnit_->getFont() != _font)
        {
            TextRendering::TextUnit::InitializeParameters params{ _font, badgeTextColor() };
            params.align_ = TextRendering::HorAligment::CENTER;
            badgeTextUnit_->init(params);
        }
    }

    void BaseBarItem::drawBadge(QPainter& _p, Utils::Badge::Color _color, const QColor& _borderColor)
    {
        const auto inNavigationBar = Features::isStatusInAppsNavigationBar();
        const auto balloonY = badgeTopOffset() + (inNavigationBar ? appBarContentMargin() : 0);
        if (!badgeText_.isEmpty())
        {
            auto balloonX = 0;
            if (inNavigationBar)
                balloonX = width() - appBarContentMargin() - badgeRightOffset();
            else
                balloonX = (width() + tabIconSize() + Utils::Badge::getSize(1).width()) / 2;

            Utils::Badge::drawBadge(badgeTextUnit_, _p, balloonX, balloonY, _color, _borderColor, inNavigationBar);
        }
        else if (const auto pixmap = badgePixmap_.actualPixmap(); !pixmap.isNull())
        {
            const auto balloonX = ((width() - tabIconSize()) / 2.0) + badgeOffset();
            _p.drawPixmap(balloonX, balloonY, badgeHeight(), badgeHeight(), pixmap);
        }
    }


    AppBarItem::AppBarItem(const QString& _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : BaseBarItem(_iconPath, _iconActivePath, _parent)
        , tooltipTimer_(new QTimer(this))
        , type_(_type)
        , tooltipVisible_(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        setCursor(Qt::PointingHandCursor);

        tooltipTimer_->setSingleShot(true);
        tooltipTimer_->setInterval(tooltipDelay());

        connect(tooltipTimer_, &QTimer::timeout, this, &AppBarItem::onTooltipTimer);

        connect(this, &SimpleListItem::hoverChanged, this, &AppBarItem::onHoverChanged);
    }

    void AppBarItem::setName(const QString& _name)
    {
        if (name_ != _name)
        {
            name_ = _name;
            if (isHovered() && tooltipVisible_)
            {
                hideTooltip();
                tooltipTimer_->start();
            }
        }
    }

    void AppBarItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto m = appBarContentMargin();
        const auto iconSize = tabIconSize();
        const auto w = width();
        const auto padding = itemPadding();
        const auto cornerRadius = appBarItemCornerRadius();
        const auto r = rect().marginsRemoved({ m, m, m, m });
        p.setPen(Qt::NoPen);
        if (isSelected())
        {
            p.setBrush(appBarItemSelectedColor());
            p.drawRoundedRect(r, cornerRadius, cornerRadius);
        }
        else if (isHovered())
        {
            p.setBrush(appBarItemHoveredColor());
            p.drawRoundedRect(r, cornerRadius, cornerRadius);
        }

        p.drawPixmap((w - iconSize) / 2.0, (height() - iconSize) / 2.0, isSelected() ? activeIconPixmap() : (isHovered() ? hoveredIconPixmap() : normalIconPixmap()));

        drawBadge(p, getBadgeColor(type_), (isSelected() || isHovered()) ? p.brush().color() : appBarItemNormalColor());
    }

    QSize AppBarItem::sizeHint() const
    {
        const auto size = appBarItemSize();
        return QSize(size, size);
    }

    void AppBarItem::onTooltipTimer()
    {
        if (isHovered() && !name_.isEmpty())
            showTooltip();
    }

    void AppBarItem::onHoverChanged(bool _hovered)
    {
        if (_hovered)
        {
            tooltipTimer_->start();
        }
        else
        {
            if (tooltipVisible_)
                hideTooltip();
        }

    }

    void AppBarItem::showTooltip()
    {
        tooltipVisible_ = true;
        const auto r = rect();
        QRect tooltipRect(mapToGlobal(r.topLeft()), r.size());
        bool tooltipInverted = false;

        // Move tooltip to bottom for first item in fullscreen mode
        if (auto w = Utils::InterConnector::instance().getMainWindow())
            tooltipInverted = (geometry().y() < appBarItemSize()) && w->isFullScreen();

        const auto direction = tooltipInverted ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down;
        const auto position = tooltipInverted ? Tooltip::ArrowPointPos::Bottom : Tooltip::ArrowPointPos::Top;
        Tooltip::show(name_, QRect(mapToGlobal(r.topLeft()), r.size()), { 0, 0 }, direction, position);
    }

    void AppBarItem::hideTooltip()
    {
        tooltipVisible_ = false;
        Tooltip::hide();
    }

    CalendarItem::CalendarItem(const QString& _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : CalendarItem(_iconPath, _iconActivePath, _parent)
    {

    }

    CalendarItem::CalendarItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : AppBarItem(Utils::MiniApps::getCalendarId(), _iconPath, _iconActivePath, _parent)
    {

    }

    void CalendarItem::paintEvent(QPaintEvent* _event)
    {
        AppBarItem::paintEvent(_event);
        QPainter p(this);
        p.setFont(getCalendarFont());
        using st = Styling::StyleVariable;
        p.setPen(Styling::getParameters().getColor(isSelected() ? st::BASE_GLOBALWHITE : st::BASE_PRIMARY));
        p.setRenderHint(QPainter::TextAntialiasing);
        if constexpr (platform::is_apple())
            p.translate(0, Utils::scale_value(1));

        p.drawText(rect(), QDate::currentDate().toString(qsl("d")), QTextOption(Qt::AlignCenter));
    }

    TabItem::TabItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : BaseBarItem(_iconPath, _iconActivePath, _parent)
    {
        setFixedHeight(tabBarDefaultHeight());
        QObject::connect(this, &SimpleListItem::hoverChanged, this, &TabItem::updateTextColor);
    }

    void TabItem::setSelected(bool _value)
    {
        if (BaseBarItem::isSelected() != _value)
        {
            BaseBarItem::setSelected(_value);
            updateTextColor();
            update();
        }
    }

    void TabItem::setName(const QString& _name)
    {
        if (name_ != _name)
        {
            name_ = _name;
            if (nameTextUnit_)
            {
                nameTextUnit_->setText(name_);
            }
            else
            {
                nameTextUnit_ = TextRendering::MakeTextUnit(name_);
                TextRendering::TextUnit::InitializeParameters params{ getNameTextFont(), normalColor() };
                params.align_ = TextRendering::HorAligment::CENTER;
                nameTextUnit_->init(params);
                updateTextColor();
            }
            update();
        }
    }

    void TabItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto iconSize = tabIconSize();
        const auto w = width();
        const auto padding = itemPadding();

        p.drawPixmap((w - iconSize) / 2.0, padding, isSelected() ? activeIconPixmap() : (isHovered() ? hoveredIconPixmap() : normalIconPixmap()));

        drawBadge(p, Utils::Badge::Color::Red);

        if (nameTextUnit_)
        {
            nameTextUnit_->setOffsets(0, padding + iconSize + Utils::scale_value(1));
            nameTextUnit_->getHeight(w);
            nameTextUnit_->draw(p);
        }
    }

    void TabItem::updateTextColor()
    {
        if (nameTextUnit_)
        {
            using sv = Styling::StyleVariable;
            const auto var = isSelected() ? sv::TEXT_PRIMARY : (isHovered() ? sv::BASE_SECONDARY_HOVER : sv::BASE_PRIMARY);
            nameTextUnit_->setColor(Styling::ThemeColorKey{ var });
        }
    }

    BaseTabBar::BaseTabBar(Qt::Orientation _orientation, QWidget* _parent)
        : SimpleListWidget(_orientation, _parent)
    {
    }

    void BaseTabBar::setBadgeText(int _index, const QString& _text)
    {
        if (isValidIndex(_index))
        {
            if (auto item = qobject_cast<BaseBarItem*>(itemAt(_index)))
                item->setBadgeText(_text);
            else
                qWarning("TabBar: can not cast to TabItem");
            update();
        }
        else
        {
            qWarning("TabBar: invalid index");
        }
    }

    void BaseTabBar::setBadgeIcon(int _index, const QString& _icon)
    {
        if (isValidIndex(_index))
        {
            if (auto item = qobject_cast<BaseBarItem*>(itemAt(_index)))
                item->setBadgeIcon(_icon);
            else
                qWarning("TabBar: can not cast to TabItem");
        }
        else
        {
            qWarning("TabBar: invalid index");
        }
    }

    AppsBar::AppsBar(QWidget* _parent)
        : BaseTabBar(Qt::Vertical, _parent)
    {
        setFixedWidth(AppsNavigationBar::defaultWidth());
        const auto margin = appBarContentMargin();
        setContentsMargins(margin, margin, margin, margin);
    }

    TabBar::TabBar(QWidget* _parent)
        : BaseTabBar(Qt::Horizontal, _parent)
    {
        setContentsMargins(0, 0, 0, 0);

        setFixedHeight(tabBarDefaultHeight());
    }

    TabBar::~TabBar() = default;

    void TabBar::paintEvent(QPaintEvent* _e)
    {
        {
            QPainter p(this);

            const auto border = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
            Utils::drawBackgroundWithBorders(p, rect(), QColor(), border, Qt::AlignTop);
        }

        SimpleListWidget::paintEvent(_e);
    }

}

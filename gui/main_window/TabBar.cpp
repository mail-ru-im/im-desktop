#include "TabBar.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"

#include "../controls/TooltipWidget.h"
#include "MainWindow.h"
#include "AppsPage.h"

#include "../fonts.h"

#include "styles/ThemeParameters.h"

namespace
{
    bool areOnlyDigits(const QString& s)
    {
        return std::all_of(s.begin(), s.end(), [](auto c) { return c.isDigit(); });
    }

    int tabIconSize() noexcept
    {
        return Utils::scale_value(28);
    }

    int itemPadding() noexcept
    {
        return Utils::scale_value(4);
    }

    int badgeTopOffset() noexcept
    {
        return Utils::scale_value(2);
    }

    int badgeHeight() noexcept
    {
        return Utils::scale_value(18);
    }

    int badgeOffset() noexcept
    {
        return Utils::scale_value(16);
    }

    QFont getNameTextFont()
    {
        const auto static f = Fonts::appFontScaled(12, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        return f;
    }

    QFont getBadgeTextFont()
    {
        const auto static f = Fonts::appFontScaled(9);
        return f;
    }

    QFont getBadgeTextForNumbersFont()
    {
        const auto static f = Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        return f;
    }

    QColor hoveredColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
    }

    QColor normalColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    QColor activeColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    QColor badgeTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    QColor badgeBalloonColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    auto tabBarDefaultHeight() noexcept
    {
        return Utils::scale_value(52);
    }

    auto appBarContentMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto appBarItemSize() noexcept
    {
        return Utils::scale_value(60);
    }

    auto appBarItemCornerRadius() noexcept
    {
        return Utils::scale_value(4);
    }

    auto appBarItemSelectedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.1);
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
}

namespace Ui
{
    BaseBarItem::BaseBarItem(const QString& _iconPath, const QString& _iconActivePath, const QColor& _activeColor, const QColor& _hoveredColor, const QColor& _normalColor, QWidget* _parent)
        : SimpleListItem(_parent)
        , iconPath_(_iconPath)
        , iconActivePath_(_iconActivePath)
        , activeIconPixmap_(Utils::renderSvg(iconActivePath_, { tabIconSize(), tabIconSize() }, activeColor()))
        , hoveredIconPixmap_(Utils::renderSvg(iconPath_, { tabIconSize(), tabIconSize() }, hoveredColor()))
        , normalIconPixmap_(Utils::renderSvg(iconPath_, { tabIconSize(), tabIconSize() }, normalColor()))
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
        if (badgeText_ != _text)
        {
            badgeText_ = _text;
            if (badgeTextUnit_)
                badgeTextUnit_->setText(badgeText_);
            else
                badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText_);
            setBadgeFont(areOnlyDigits(badgeText_) ? getBadgeTextForNumbersFont() : getBadgeTextFont());
            update();
        }
    }

    void BaseBarItem::setBadgeIcon(const QString& _icon)
    {
        if (badgeIcon_ != _icon)
        {
            badgeIcon_ = _icon;
            badgePixmap_ = !badgeIcon_.isEmpty() ? Utils::renderSvgLayered(badgeIcon_,
                {
                    {qsl("border"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)},
                    {qsl("bg"), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION)},
                    {qsl("star"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)},
                },
                {badgeHeight(), badgeHeight()}) : QPixmap();
            update();
        }
    }

    void BaseBarItem::setName(const QString& _name)
    {
        name_ = _name;
    }

    const QPixmap& BaseBarItem::activeIconPixmap() const
    {
        return activeIconPixmap_;
    }

    const QPixmap& BaseBarItem::hoveredIconPixmap() const
    {
        return hoveredIconPixmap_;
    }

    const QPixmap& BaseBarItem::normalIconPixmap() const
    {
        return normalIconPixmap_;
    }

    void BaseBarItem::setBadgeFont(const QFont & _font)
    {
        if (badgeTextUnit_ && badgeTextUnit_->getFont() != _font)
            badgeTextUnit_->init(_font, badgeTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    }

    AppBarItem::AppBarItem(AppPageType _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : BaseBarItem(_iconPath, _iconActivePath, appBarIconActiveColor(), appBarIconNormalColor(), appBarIconNormalColor(), _parent)
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

        p.drawPixmap((w - iconSize) / 2, (height() - iconSize) / 2, isSelected() ? activeIconPixmap() : (isHovered() ? hoveredIconPixmap() : normalIconPixmap()));

        if (!badgeText_.isEmpty())
        {
            const auto balloonX = (w - iconSize) / 2 + badgeOffset();
            Utils::Badge::drawBadge(badgeTextUnit_, p, balloonX, badgeTopOffset() + m, Utils::Badge::Color::Red);
        }
        else if (!badgePixmap_.isNull())
        {
            const auto h = badgeHeight();
            const auto balloonX = (w - iconSize) / 2.0 + badgeOffset();
            p.drawPixmap(balloonX, badgeTopOffset() + m, h, h, badgePixmap_);
        }
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

    CalendarItem::CalendarItem(AppPageType _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : CalendarItem(_iconPath, _iconActivePath, _parent)
    {

    }

    CalendarItem::CalendarItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : AppBarItem(AppPageType::calendar, _iconPath, _iconActivePath, _parent)
    {

    }

    void CalendarItem::paintEvent(QPaintEvent* _event)
    {
        AppBarItem::paintEvent(_event);
        QPainter p(this);
        p.setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        p.setPen(Styling::getParameters().getColor(isSelected() ? Styling::StyleVariable::BASE_GLOBALWHITE : Styling::StyleVariable::BASE_PRIMARY));
        p.setRenderHint(QPainter::TextAntialiasing);
        if constexpr (platform::is_apple())
            p.translate(0, Utils::scale_value(1));

        p.drawText(rect(), QDate::currentDate().toString(qsl("dd")), QTextOption(Qt::AlignCenter));
    }

    TabItem::TabItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : BaseBarItem(_iconPath, _iconActivePath, activeColor(), hoveredColor(), normalColor(), _parent)
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
                nameTextUnit_->init(getNameTextFont(), normalColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
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

        if (!badgeText_.isEmpty())
        {
            const auto balloonX = (w - iconSize) / 2.0 + badgeOffset();
            Utils::Badge::drawBadge(badgeTextUnit_, p, balloonX, badgeTopOffset(), Utils::Badge::Color::Red);
        }
        else if (!badgePixmap_.isNull())
        {
            const auto h = badgeHeight();
            const auto balloonX = (w - iconSize) / 2.0 + badgeOffset();
            p.drawPixmap(balloonX, badgeTopOffset(), h, h, badgePixmap_);
        }

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
            nameTextUnit_->setColor(Styling::getParameters().getColor(isSelected() ? Styling::StyleVariable::TEXT_PRIMARY : (isHovered() ? Styling::StyleVariable::BASE_SECONDARY_HOVER : Styling::StyleVariable::BASE_PRIMARY)));
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

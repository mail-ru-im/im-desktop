#include "TabBar.h"

#include "../utils/utils.h"

#include "../controls/TextUnit.h"

#include "../fonts.h"

#include "styles/ThemeParameters.h"

namespace
{
    bool areOnlyDigits(const QString& s)
    {
        return std::all_of(s.begin(), s.end(), [](auto c) { return c.isDigit(); });
    }
}

namespace
{
    int tabIconSize()
    {
        return Utils::scale_value(28);
    }

    int itemPadding()
    {
        return Utils::scale_value(4);
    }

    int badgeTopOffset()
    {
        return Utils::scale_value(2);
    }

    int badgeTextPadding()
    {
        return Utils::scale_value(4);
    }

    int badgeBorderWidth()
    {
        return Utils::scale_value(1);
    }

    int badgeHeight()
    {
        return Utils::scale_value(18);
    }

    int badgeOffset()
    {
        return Utils::scale_value(16);
    }

    QColor itemBackground()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
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
        const auto static f = Fonts::appFontScaled(platform::is_apple() ? 10 : 11,
                                                   platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
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
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor badgeBalloonColor()
    {
        static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        return c;
    }
}

namespace Ui
{
    TabItem::TabItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent)
        : SimpleListItem(_parent)
        , isSelected_(false)
        , iconPath_(_iconPath)
        , iconActivePath_(_iconActivePath)
        , activeIconPixmap_(Utils::renderSvg(iconActivePath_, { tabIconSize(), tabIconSize() }, activeColor()))
        , hoveredIconPixmap_(Utils::renderSvg(iconPath_, { tabIconSize(), tabIconSize() }, hoveredColor()))
        , normalIconPixmap_(Utils::renderSvg(iconPath_, { tabIconSize(), tabIconSize() }, normalColor()))
    {
        setFixedHeight(TabBar::getDefaultHeight());
        QObject::connect(this, &SimpleListItem::hoverChanged, this, &TabItem::updateTextColor);
    }

    TabItem::~TabItem()
    {
    }

    void TabItem::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;
            updateTextColor();
            update();
        }
    }

    bool TabItem::isSelected() const
    {
        return isSelected_;
    }

    void TabItem::setBadgeText(const QString& _text)
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

    void TabItem::setBadgeIcon(const QString& _icon)
    {
        if (badgeIcon_ != _icon)
        {
            badgeIcon_ = _icon;
            badgePixmap_ = !badgeIcon_.isEmpty() ? Utils::renderSvgLayered(badgeIcon_,
                {
                    {qsl("border"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)},
                    {qsl("bg"), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION)},
                    {qsl("star"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT)},
                },
                {badgeHeight(), badgeHeight()}) : QPixmap();
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

        p.drawPixmap((w - iconSize) / 2.0, padding, isSelected() ? activeIconPixmap_ : (isHovered() ? hoveredIconPixmap_ : normalIconPixmap_));

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
            nameTextUnit_->setOffsets(0, padding + iconSize);
            nameTextUnit_->getHeight(w);
            nameTextUnit_->draw(p);
        }
    }

    void TabItem::updateTextColor()
    {
        if (nameTextUnit_)
            nameTextUnit_->setColor(Styling::getParameters().getColor(isSelected() ? Styling::StyleVariable::TEXT_PRIMARY : (isHovered() ? Styling::StyleVariable::BASE_SECONDARY_HOVER : Styling::StyleVariable::BASE_PRIMARY)));
    }

    void TabItem::setBadgeFont(const QFont & _font)
    {
        if (badgeTextUnit_ && badgeTextUnit_->getFont() != _font)
            badgeTextUnit_->init(_font, badgeTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    }


    TabBar::TabBar(QWidget* _parent)
        : SimpleListWidget(Qt::Horizontal, _parent)
    {
        setContentsMargins(0, 0, 0, 0);

        setFixedHeight(TabBar::getDefaultHeight());
    }

    TabBar::~TabBar()
    {
    }

    void TabBar::setBadgeText(int _index, const QString& _text)
    {
        if (isValidIndex(_index))
        {
            if (auto item = qobject_cast<TabItem*>(itemAt(_index)))
                item->setBadgeText(_text);
            else
                qWarning("TabBar: can not cast to TabItem");
        }
        else
        {
            qWarning("TabBar: invalid index");
        }
    }

    void TabBar::paintEvent(QPaintEvent* _e)
    {
        {
            QPainter p(this);

            const auto border = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
            Utils::drawBackgroundWithBorders(p, rect(), QColor(), border, Qt::AlignTop);
        }

        SimpleListWidget::paintEvent(_e);
    }

    void TabBar::setBadgeIcon(int _index, const QString& _icon)
    {
        if (isValidIndex(_index))
        {
            if (auto item = qobject_cast<TabItem*>(itemAt(_index)))
                item->setBadgeIcon(_icon);
            else
                qWarning("TabBar: can not cast to TabItem");
        }
        else
        {
            qWarning("TabBar: invalid index");
        }
    }

    int TabBar::getDefaultHeight()
    {
        return Utils::scale_value(52);
    }
}

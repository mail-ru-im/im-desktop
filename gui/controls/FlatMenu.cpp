#include "stdafx.h"


#include "../fonts.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../main_window/MainWindow.h"
#include "styles/ThemeParameters.h"

#include "FlatMenu.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace Ui
{
    int FlatMenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        int s = QProxyStyle::pixelMetric(_metric, _option, _widget);
        if (_metric == QStyle::PM_SmallIconSize)
        {
            s = Utils::scale_value(24);
        }
        return s;
    }

    int FlatMenu::shown_ = 0;

    FlatMenu::FlatMenu(QWidget* _parent/* = nullptr*/, BorderStyle _shape/* = BorderStyle::SOLID*/)
        : QMenu(_parent),
        iconSticked_(false),
        needAdjustSize_(false)
    {
        setWindowFlags(windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);

        Utils::SetProxyStyle(this, new FlatMenuStyle());
        Utils::ApplyStyle(this, Styling::getParameters().getFlatMenuQss(_shape == BorderStyle::SOLID ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE));

        setFont(Fonts::appFontScaled(15));
        if constexpr (platform::is_apple())
        {
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, this, &FlatMenu::close);
        }
    }

    FlatMenu::~FlatMenu()
    {
        //
    }

    void FlatMenu::hideEvent(QHideEvent* _event)
    {
        QMenu::hideEvent(_event);
        --shown_;
    }

    void FlatMenu::adjustSize()
    {
        if (const auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            if (const auto currentActions = actions(); !currentActions.isEmpty())
            {
                const auto static magrin = Utils::scale_value(20);
                const auto mwGeometry = mw->geometry();

                const auto geom = QRect(pos(), size());
                const auto diff = geom.right() - mwGeometry.right();
                const auto newWidth = geom.width() - diff - magrin;
                for (auto action : currentActions)
                {
                    const auto elidedText = fontMetrics().elidedText(action->property(sourceTextPropName()).toString(), Qt::ElideRight, newWidth);
                    action->setText(elidedText);
                }
            }
        }
    }

    void FlatMenu::showEvent(QShowEvent* _event)
    {
//        if (parentWidget() && !parentWidget()->isActiveWindow())
//        {
//            close();
//            return;
//        }
        if (!parentWidget())
        {
            return QMenu::showEvent(_event);
        }
        auto posx = pos().x();
        auto posy = pos().y();
        if (iconSticked_)
        {
            if (auto button = qobject_cast<QPushButton *>(parentWidget()))
            {
                posx -= ((button->size().width() - button->iconSize().width()) / 2);
                posy -= ((button->size().height() - button->iconSize().height()) / 2);
                move(posx, posy);
            }
        }
        if ((expandDirection_ & Qt::AlignLeft) == Qt::AlignLeft)
        {
            move(pos().x() + parentWidget()->geometry().width() - geometry().width(), pos().y());
        }
        if ((expandDirection_ & Qt::AlignTop) == Qt::AlignTop)
        {
            move(pos().x(), pos().y() - parentWidget()->geometry().height() - geometry().height());
        }
        if ((expandDirection_ & Qt::AlignBottom) == Qt::AlignBottom)
        {
            move(pos().x() + Utils::scale_value(2), pos().y() + Utils::scale_value(11));
        }
        if (needAdjustSize_)
            adjustSize();
        ++shown_;
    }

    void FlatMenu::setExpandDirection(Qt::Alignment _direction)
    {
        expandDirection_ = _direction;
    }

    void FlatMenu::stickToIcon()
    {
        iconSticked_ = true;
    }

    void FlatMenu::setNeedAdjustSize(bool _value)
    {
        needAdjustSize_ = _value;
    }

}

#include "stdafx.h"


#include "ContextMenu.h"

#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

#include "styles/ThemeParameters.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const QSize iconSize(20, 20);
}

namespace Ui
{
    int MenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        if (_metric == QStyle::PM_SmallIconSize)
            return Utils::scale_value(20);
        return QProxyStyle::pixelMetric(_metric, _option, _widget);
    }

    ContextMenu::ContextMenu(QWidget* _parent)
        : QMenu(_parent)
        , InvertRight_(false)
        , Indent_(0)
    {
        applyStyle(this, true, Utils::scale_value(15), Utils::scale_value(36));
    }

    void ContextMenu::applyStyle(QMenu* _menu, bool _withPadding, int _fontSize, int _height)
    {
        const auto itemHeight = _height;
        const auto iconPadding = Utils::scale_value(_withPadding ? 20 : 12) + Utils::scale_value(iconSize.width());
        auto itemPaddingLeft = Utils::scale_value(_withPadding ? (20 + 16 + iconSize.width()) : 20);
        const auto itemPaddingRight = Utils::scale_value(_withPadding ? 28 : 12);

        if (platform::is_linux() && !_withPadding) // fix for linux native menu with icons
            itemPaddingLeft += iconPadding;

        QString styleString = Styling::getParameters().getContextMenuQss(itemHeight, itemPaddingLeft, itemPaddingRight, iconPadding);
        _menu->setWindowFlags(_menu->windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        _menu->setStyle(new MenuStyle());

        Utils::ApplyStyle(_menu, styleString);

        auto font = Fonts::appFont(_fontSize);

        _menu->setFont(font);

        if constexpr (platform::is_apple())
        {
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowHide, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowClose, _menu, &QMenu::close);
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data)
    {
        QAction* action = addAction(_icon, _name);
        action->setData(_data);
        return action;
    }

    QAction* ContextMenu::addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        return addActionWithIcon(makeIcon(_iconPath), _name, _data);
    }

    bool ContextMenu::hasAction(const QString& _command)
    {
        assert(!_command.isEmpty());
        const auto actions = this->actions();
        for (const auto& action : actions)
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find(qsl("command"));
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                return true;
            }
        }

        return false;
    }

    void ContextMenu::removeAction(const QString& _command)
    {
        assert(!_command.isEmpty());
        const auto actions = this->actions();
        for (const auto& action : actions)
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find(qsl("command"));
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                QWidget::removeAction(action);

                return;
            }
        }
    }

    void ContextMenu::invertRight(bool _invert)
    {
        InvertRight_ = _invert;
    }

    void ContextMenu::setIndent(int _indent)
    {
        Indent_ = _indent;
    }

    void ContextMenu::popup(const QPoint& _pos, QAction* _at)
    {
        Pos_ = _pos;
        QMenu::popup(_pos, _at);
    }

    void ContextMenu::clear()
    {
        QMenu::clear();
    }

    void ContextMenu::hideEvent(QHideEvent* _e)
    {
        QMenu::hideEvent(_e);
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
        if (InvertRight_ || Indent_ != 0)
        {
            QPoint p;
            if (pos().x() != Pos_.x())
                p = Pos_;
            else
                p = pos();

            if (InvertRight_)
                p.setX(p.x() - width() - Indent_);
            else
                p.setX(p.x() + Indent_);
            move(p);
        }
    }

    void ContextMenu::focusOutEvent(QFocusEvent *_e)
    {
        if (parentWidget() && !parentWidget()->isActiveWindow())
        {
            close();
            return;
        }
        QMenu::focusOutEvent(_e);
    }

    QIcon ContextMenu::makeIcon(const QString& _iconPath) const
    {
        static std::map<QString, QIcon> iconCache;

        auto it = iconCache.find(_iconPath);
        if (it == iconCache.end())
            it = iconCache.insert({ _iconPath, QIcon(Utils::renderSvgScaled(_iconPath, iconSize, QColor(0xB7, 0xBC, 0xC9))) }).first;

        return it->second;
    }
}

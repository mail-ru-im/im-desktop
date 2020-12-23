#include "stdafx.h"


#include "ContextMenu.h"

#include "ClickWidget.h"

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
    auto itemHeight() noexcept { return Utils::scale_value(36); }
    auto fontSize() noexcept { return Utils::scale_value(15);  }
}

namespace Ui
{
    MenuStyle::MenuStyle(const int _iconSize)
        : QProxyStyle()
        , iconSize_(_iconSize)
    {
    }

    int MenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        if (_metric == QStyle::PM_SmallIconSize)
            return Utils::scale_value(iconSize_);
        return QProxyStyle::pixelMetric(_metric, _option, _widget);
    }

    TextAction::TextAction(const QString& text, QObject* parent)
        : QWidgetAction(parent)
    {
        textWidget_ = new ClickableTextWidget(nullptr, Fonts::appFont(fontSize()), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), TextRendering::HorAligment::LEFT);
        textWidget_->setFixedHeight(itemHeight());
        textWidget_->setCursor(Qt::ArrowCursor);
        textWidget_->setText(text);
        textWidget_->setLeftPadding(Utils::scale_value(20));
        textWidget_->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        QObject::connect(textWidget_, &ClickableTextWidget::clicked, this, &QAction::trigger);
        QObject::connect(textWidget_, &ClickableTextWidget::hoverChanged, this, [this](bool _v)
        {
            if (_v)
                if (auto m = qobject_cast<QMenu*>(parentWidget()))
                    m->setActiveAction(this);
        });

        setDefaultWidget(textWidget_);
    }

    void TextAction::setHovered(bool _v)
    {
        textWidget_->setHovered(_v);
    }

    ContextMenu::ContextMenu(QWidget* _parent, Color _color, int _iconSize)
        : QMenu(_parent)
        , color_(_color)
        , iconSize_(_iconSize, _iconSize)
    {
        applyStyle(this, true, fontSize(), itemHeight(), _color, iconSize_);
        QObject::connect(this, &QMenu::hovered, this, [this](QAction* action)
        {
            const auto actionList = actions();
            for (auto a : actionList)
            {
                if (a == action)
                {
                    if (auto textAction = qobject_cast<TextAction*>(action))
                    {
                        QSignalBlocker blocker(textAction);
                        textAction->setHovered(true);
                    }
                }
                else if (auto textAction = qobject_cast<TextAction*>(a))
                {
                    textAction->setHovered(false);
                }
            }
        });
    }

    ContextMenu::ContextMenu(QWidget* _parent)
        : ContextMenu(_parent, Color::Default)
    {
    }

    ContextMenu::ContextMenu(QWidget* _parent, int _iconSize)
        : ContextMenu(_parent, Color::Default, _iconSize)
    {
    }

    void ContextMenu::applyStyle(QMenu* menu, bool withPadding, int fonSize, int height, const QSize& _iconSize)
    {
        applyStyle(menu, withPadding, fonSize, height, Color::Default, _iconSize);
    }

    void ContextMenu::applyStyle(QMenu* _menu, bool _withPadding, int _fontSize, int _height, Color color, const QSize& _iconSize)
    {
        const auto itemHeight = _height;
        const auto iconPadding = Utils::scale_value((_withPadding ? 20 : 12) + 20);

        auto itemPaddingLeft = Utils::scale_value(_withPadding ? 36 : (platform::is_linux() ? 0 : 20));
        const auto itemPaddingRight = Utils::scale_value(_withPadding ? 28 : 12);

        if (platform::is_linux() && !_withPadding) // fix for linux native menu with icons
            itemPaddingLeft += iconPadding;

        QString styleString = color == Color::Default
            ? Styling::getParameters().getContextMenuQss(itemHeight, itemPaddingLeft, itemPaddingRight, iconPadding)
            : Styling::getParameters().getContextMenuDarkQss(itemHeight, itemPaddingLeft, itemPaddingRight, iconPadding);
        _menu->setWindowFlags(_menu->windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);

        Utils::SetProxyStyle(_menu, new MenuStyle(_iconSize.width()));
        Utils::ApplyStyle(_menu, styleString);

        _menu->setFont(Fonts::appFont(_fontSize));

        if constexpr (platform::is_apple())
        {
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowHide, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowClose, _menu, &QMenu::close);
        }
        else
        {
            QObject::connect(new QShortcut(QKeySequence::Copy, _menu), &QShortcut::activated, Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::copy);
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data)
    {
        QAction* action = addAction(_icon, _name);
        action->setData(_data);
        return action;
    }

    QAction* ContextMenu::findAction(QStringView _command) const
    {
        assert(!_command.isEmpty());
        const auto actionList = actions();
        for (const auto& action : actionList)
        {
            const auto actionParams = action->data().toMap();
            if (const auto commandIter = actionParams.find(qsl("command")); commandIter != actionParams.end())
                if (commandIter->toString() == _command)
                    return action;
        }
        return nullptr;
    }

    QAction* ContextMenu::addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        return addActionWithIcon(makeIcon(_iconPath), _name, _data);
    }

    bool ContextMenu::modifyAction(QStringView _command, const QString& _iconPath, const QString& _name, const QVariant& _data, bool _enable)
    {
        if (auto action = findAction(_command); action)
        {
            action->setIcon(makeIcon(_iconPath));
            action->setText(_name);
            action->setData(_data);
            action->setEnabled(_enable);
            return true;
        }

        return false;
    }

    bool ContextMenu::hasAction(QStringView _command) const
    {
        return findAction(_command) != nullptr;
    }

    void ContextMenu::removeAction(QStringView _command)
    {
        if (auto action = findAction(_command); action)
            QWidget::removeAction(action);
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

    void ContextMenu::setWheelCallback(std::function<void(QWheelEvent*)> _callback)
    {
        onWheel_ = std::move(_callback);
    }

    void ContextMenu::setShowAsync(const bool _async)
    {
        showAsync_ = _async;
    }

    bool ContextMenu::isShowAsync() const
    {
        return showAsync_;
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
        if (InvertRight_ || Indent_ != 0)
        {
            auto g = geometry();
            QPoint p;
            if (pos().x() != Pos_.x())
                p = Pos_;
            else
                p = pos();

            if (InvertRight_)
                p.setX(p.x() - width() - Indent_);
            else
                p.setX(p.x() + Indent_);
            g.moveTopLeft(p);
            setGeometry(g);
            resize(g.size());
        }
    }

    void ContextMenu::hideEvent(QHideEvent* _event)
    {
        QMenu::hideEvent(_event);
        Q_EMIT hidden(QPrivateSignal());
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

    void ContextMenu::wheelEvent(QWheelEvent* _event)
    {
        if (onWheel_)
            onWheel_(_event);
        QMenu::wheelEvent(_event);
    }

    void ContextMenu::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (!rect().contains(_event->pos()) && isShowAsync())
            close();
        else
            QMenu::mouseReleaseEvent(_event);
    }

    QIcon ContextMenu::makeIcon(const QString& _iconPath) const
    {
        static std::map<QString, QIcon> iconCache;

        auto& icon = iconCache[_iconPath];
        if (icon.isNull())
            icon = QIcon(Utils::renderSvgScaled(_iconPath, iconSize_, Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY)));
        return icon;
    }
}

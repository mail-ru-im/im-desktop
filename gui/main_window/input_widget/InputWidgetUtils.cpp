#include "stdafx.h"

#include "InputWidget.h"
#include "InputWidgetUtils.h"
#include "HistoryTextEdit.h"
#include "AttachFilePopup.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "controls/CustomButton.h"
#include "controls/textrendering/TextRendering.h"
#include "fonts.h"

#include "main_window/contact_list/ContactListModel.h"

#include "core_dispatcher.h"
#include "gui_settings.h"

namespace Ui
{
    std::string getStatsChatType(const QString& _contact)
    {
        return Utils::isChat(_contact) ? (Logic::getContactListModel()->isChannel(_contact) ? "channel" : "group") : "chat";
    }

    int getInputTextLeftMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    int getDefaultInputHeight() noexcept
    {
        return Utils::scale_value(52);
    }

    int getHorMargin() noexcept
    {
        return Utils::scale_value(52);
    }

    int getVerMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    int getHorSpacer() noexcept
    {
        return Utils::scale_value(12);
    }

    int getEditHorMarginLeft() noexcept
    {
        return Utils::scale_value(20);
    }

    int getQuoteRowHeight() noexcept
    {
        return Utils::scale_value(16);
    }

    int getBubbleCornerRadius() noexcept
    {
        return Utils::scale_value(18);
    }

    QSize getCancelBtnIconSize() noexcept
    {
        return QSize(16, 16);
    }

    QSize getCancelButtonSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    void drawInputBubble(QPainter& _p, const QRect& _widgetRect, const QColor& _color, const int _topMargin, const int _botMargin)
    {
        if (_widgetRect.isEmpty() || !_color.isValid())
            return;

        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);

        const auto bubbleRect = _widgetRect.adjusted(0, _topMargin, 0, -_botMargin);
        const auto radius = getBubbleCornerRadius();

        QPainterPath path;
        path.addRoundedRect(bubbleRect, radius, radius);

        Utils::drawBubbleShadow(_p, path, radius);

        _p.setPen(Qt::NoPen);
        _p.setCompositionMode(QPainter::CompositionMode_Source);
        _p.fillPath(path, _color);
    }

    bool isEmoji(QStringView _text, qsizetype& _pos)
    {
        const auto emojiMaxSize = Emoji::EmojiCode::maxSize();
        for (size_t i = 0; i < emojiMaxSize; ++i)
        {
            qsizetype position = _pos - i;
            if (Emoji::getEmoji(_text, position) != Emoji::EmojiCode())
            {
                _pos = position;
                return true;
            }
        }
        return false;
    }

    void updateButtonColors(CustomButton* _button, const InputStyleMode _mode)
    {
        if (_mode == InputStyleMode::Default)
            Styling::InputButtons::Default::setColors(_button);
        else
            Styling::InputButtons::Alternate::setColors(_button);
    }

    QColor getPopupHoveredColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.05);
    }

    QColor getPopupPressedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.08);
    }

    Styling::ThemeColorKey focusColorPrimaryKey()
    {
        return Styling::getParameters().getPrimaryTabFocusColorKey();
    }

    QColor focusColorPrimary()
    {
        return Styling::getParameters().getPrimaryTabFocusColor();
    }

    Styling::ThemeColorKey focusColorAttentionKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION, 0.22 };
    }

    void sendStat(const QString& _contact, core::stats::stats_event_names _event, const std::string_view _from)
    {
        Ui::GetDispatcher()->post_stats_to_core(_event, { { "from", std::string(_from) }, {"chat_type", Ui::getStatsChatType(_contact) } });
    }

    void sendShareStat(const QString& _contact, bool _sent)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharecontactscr_contact_action,
        {
            {"chat_type", Ui::getStatsChatType(_contact) },
            { "type", std::string("internal") },
            { "status", std::string( _sent ? "sent" : "not_sent") }
        });
    }

    void sendAttachStat(const QString& _contact, AttachMediaType _mediaType)
    {
        std::optional<core::stats::stats_event_names> statEvent;

        switch (_mediaType)
        {
        case AttachMediaType::photoVideo:
            statEvent = core::stats::stats_event_names::chatscr_openmedgallery_action;
            break;

        case AttachMediaType::file:
            statEvent = core::stats::stats_event_names::chatscr_openfile_action;
            break;

        case AttachMediaType::camera:
            statEvent = core::stats::stats_event_names::chatscr_opencamera_action;
            break;

        case AttachMediaType::contact:
            statEvent = core::stats::stats_event_names::chatscr_opencontact_action;
            break;

        case AttachMediaType::poll:
            break;

        case AttachMediaType::task:
            statEvent = core::stats::stats_event_names::chatscr_createtask_action;
            break;

        case AttachMediaType::ptt:
            statEvent = core::stats::stats_event_names::chatscr_openptt_action;
            break;

        case AttachMediaType::geo:
            statEvent = core::stats::stats_event_names::chatscr_opengeo_action;
            break;

        case AttachMediaType::callLink:
            statEvent = core::stats::stats_event_names::chatscr_callbylink_action;
            break;

        case AttachMediaType::webinar:
            statEvent = core::stats::stats_event_names::chatscr_webinar_action;
            break;

        default:
            return;
        }

        if (statEvent)
            sendStat(_contact, *statEvent, "plus");
    }

    BubbleWidget::BubbleWidget(QWidget* _parent, const int _topMargin, const int _botMargin, const Styling::StyleVariable _bgColor)
        : QWidget(_parent)
        , topMargin_(_topMargin)
        , botMargin_(_botMargin)
        , bgColor_(_bgColor)
    {}

    void BubbleWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        Ui::drawInputBubble(p, rect(), Styling::getParameters().getColor(bgColor_), topMargin_, botMargin_);
    }

    bool isApplePageUp(int _key, Qt::KeyboardModifiers _modifiers) noexcept
    {
        return platform::is_apple() && _modifiers.testFlag(Qt::KeyboardModifier::ControlModifier) && _key == Qt::Key_Up;
    }

    bool isApplePageDown(int _key, Qt::KeyboardModifiers _modifiers) noexcept
    {
        return platform::is_apple() && _modifiers.testFlag(Qt::KeyboardModifier::ControlModifier) && _key == Qt::Key_Down;
    }

    bool isApplePageEnd(int _key, Qt::KeyboardModifiers _modifiers) noexcept
    {
        return platform::is_apple() && ((_modifiers.testFlag(Qt::KeyboardModifier::MetaModifier) && _key == Qt::Key_Right) || _key == Qt::Key_End);
    }

    bool isCtrlEnd(int _key, Qt::KeyboardModifiers _modifiers) noexcept
    {
        return (_modifiers == Qt::CTRL && _key == Qt::Key_End) || isApplePageEnd(_key, _modifiers);
    }

    bool isApplePageUp(QKeyEvent* _e) noexcept
    {
        return _e && isApplePageUp(_e->key(), _e->modifiers());
    }

    bool isApplePageDown(QKeyEvent* _e) noexcept
    {
        return _e && isApplePageDown(_e->key(), _e->modifiers());
    }

    bool isApplePageEnd(QKeyEvent* _e) noexcept
    {
        return _e && isApplePageEnd(_e->key(), _e->modifiers());
    }

    bool isCtrlEnd(QKeyEvent* _e) noexcept
    {
        return _e && isCtrlEnd(_e->key(), _e->modifiers());
    }


    std::vector<InputWidget*>& getInputWidgetsImpl()
    {
        static std::vector<InputWidget*> inputs;
        return inputs;
    }

    const std::vector<InputWidget*>& getInputWidgetInstances()
    {
        return getInputWidgetsImpl();
    }

    void registerInputWidgetInstance(InputWidget* _input)
    {
        if (!_input)
            return;

        auto& inputs = getInputWidgetsImpl();
        if (std::none_of(inputs.begin(), inputs.end(), [_input](auto i) { return i == _input; }))
            inputs.push_back(_input);
    }

    void deregisterInputWidgetInstance(InputWidget* _input)
    {
        auto& inputs = getInputWidgetsImpl();
        inputs.erase(std::remove(inputs.begin(), inputs.end(), _input), inputs.end());
    }
}

namespace Styling::InputButtons::Default
{
    void setColors(Ui::CustomButton* _button)
    {
        im_assert(_button);
        if (_button)
        {
            _button->setDefaultColor(defaultColorKey());
            _button->setHoverColor(hoverColorKey());
            _button->setPressedColor(pressedColorKey());
            _button->setActiveColor(activeColorKey());
        }
    }

    Styling::ThemeColorKey defaultColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    Styling::ThemeColorKey hoverColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER };
    }

    QColor defaultColor()
    {
        return Styling::getColor(defaultColorKey());
    }

    QColor hoverColor()
    {
        return Styling::getColor(hoverColorKey());
    }

    Styling::ThemeColorKey pressedColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_ACTIVE };
    }

    QColor pressedColor()
    {
        return Styling::getColor(pressedColorKey());
    }

    Styling::ThemeColorKey activeColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
    }
}

namespace Styling::InputButtons::Alternate
{
    void setColors(Ui::CustomButton* _button)
    {
        im_assert(_button);
        if (_button)
        {
            _button->setDefaultColor(defaultColorKey());
            _button->setHoverColor(hoverColorKey());
            _button->setPressedColor(pressedColorKey());
            _button->setActiveColor(activeColorKey());
        }
    }

    Styling::ThemeColorKey defaultColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY_INVERSE };
    }

    QColor defaultColor()
    {
        return Styling::getColor(defaultColorKey());
    }

    Styling::ThemeColorKey hoverColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY_INVERSE_HOVER };
    }

    QColor hoverColor()
    {
        return Styling::getColor(hoverColorKey());
    }

    Styling::ThemeColorKey pressedColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE };
    }

    QColor pressedColor()
    {
        return Styling::getColor(pressedColorKey());
    }

    Styling::ThemeColorKey activeColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE };
    }
}

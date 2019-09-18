#include "stdafx.h"

#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

#include "Common.h"
#include "ContactList.h"
#include "../../gui_settings.h"

#include "styles/ThemeParameters.h"

namespace Ui
{
    VisualDataBase::VisualDataBase()
        : IsHovered_(false)
        , IsSelected_(false)
        , HasLastSeen_(false)
        , isCheckedBox_(false)
        , isChatMember_(false)
        , isOfficial_(false)
        , drawLastRead_(false)
        , unreadsCounter_(0)
        , membersCount_(0)
        , isMuted_(false)
        , notInCL_(false)
        , hasAttention_(false)
        , isCreator_(false)
    {
    }

    VisualDataBase::VisualDataBase(
        const QString& _aimId,
        const QPixmap& _avatar,
        const QString& _state,
        const QString& _status,
        const bool _isHovered,
        const bool _isSelected,
        const QString& _contactName,
        const QString& _nick,
        const std::vector<QString>& _highlights,
        const bool _hasLastSeen,
        const QDateTime& _lastSeen,
        const bool _isWithCheckBox,
        const bool _isChatMember,
        const bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const QString& _role,
        const int _unreadsCounter,
        const bool _muted,
        const bool _notInCL,
        const bool _hasAttention,
        const bool _isCreator)
        : AimId_(_aimId)
        , Avatar_(_avatar)
        , State_(_state)
        , ContactName_(_contactName)
        , nick_(_nick)
        , highlights_(_highlights)
        , IsHovered_(_isHovered)
        , IsSelected_(_isSelected)
        , HasLastSeen_(_hasLastSeen)
        , LastSeen_(_lastSeen)
        , isCheckedBox_(_isWithCheckBox)
        , isChatMember_(_isChatMember)
        , isOfficial_(_isOfficial)
        , drawLastRead_(_drawLastRead)
        , lastReadAvatar_(_lastReadAvatar)
        , role_(_role)
        , unreadsCounter_(_unreadsCounter)
        , membersCount_(0)
        , isMuted_(_muted)
        , notInCL_(_notInCL)
        , hasAttention_(_hasAttention)
        , isCreator_(_isCreator)
        , Status_(_status)
    {
        assert(!AimId_.isEmpty());
        assert(!ContactName_.isEmpty());
    }

    const QString& VisualDataBase::GetStatus() const
    {
        return Status_;
    }

    bool VisualDataBase::HasStatus() const
    {
        return !Status_.isEmpty();
    }

    void VisualDataBase::SetStatus(const QString& _status)
    {
        Status_ = _status;
    }

    QString FormatTime(const QDateTime& _time)
    {
        if (!_time.isValid())
        {
            return QString();
        }

        const auto current = QDateTime::currentDateTime();

        const auto days = _time.daysTo(current);

        if (days == 0)
        {
            return _time.time().toString(Qt::SystemLocaleShortDate);
        }

        if (days == 1)
        {
            return QT_TRANSLATE_NOOP("contact_list", "yesterday");
        }

        const auto date = _time.date();
        return Utils::GetTranslator()->formatDate(date, date.year() == current.date().year());
    }

    int ItemWidth(bool _fromAlert, bool _isPictureOnlyView)
    {
        if (_isPictureOnlyView)
        {
            auto params = ::Ui::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
            params.setIsCL(false);
            return params.itemHorPadding() + params.getAvatarSize() + params.itemHorPadding();
        }

        if (_fromAlert)
            return Ui::GetRecentsParams(Logic::MembersWidgetRegim::FROM_ALERT).itemWidth();

        //return std::min(Utils::scale_value(400), ItemLength(true, 1. / 3, 0));
        return ItemLength(true, 1. / 3, 0);
    }

    int ItemWidth(const ViewParams& _viewParams)
    {
        return ItemWidth(_viewParams.regim_ == ::Logic::MembersWidgetRegim::FROM_ALERT, _viewParams.pictOnly_);
    }

    int CorrectItemWidth(int _itemWidth, int _fixedWidth)
    {
        return _fixedWidth == -1 ? _itemWidth : _fixedWidth;
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth)
    {
        return ItemLength(_isWidth, _koeff, _addWidth, Utils::InterConnector::instance().getMainWindow());
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent)
    {
        assert(!!parent && "Common.cpp (ItemLength)");
        auto mainRect = Utils::GetWindowRect(parent);
        if (mainRect.width() && mainRect.height())
        {
            auto mainLength = _isWidth ? mainRect.width() : mainRect.height();
            return _addWidth + mainLength * _koeff;
        }
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return 0;
    }

    QColor ContactListParams::getNameFontColor(bool _isSelected, bool _isMemberChecked) const
    {
        return _isSelected ?
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT) : _isMemberChecked ?
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY) :
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QString ContactListParams::getRecentsMessageFontColor(const bool _isUnread) const
    {
        const auto color = _isUnread ?
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY) :
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY);

        return color;
    };

    QString ContactListParams::getMessageStylesheet(const int _fontSize, const bool _isUnread, const bool _isSelected) const
    {
        const auto fontWeight = Fonts::FontWeight::Normal;
        const auto fontQss = Fonts::appFontFullQss(_fontSize, Fonts::defaultAppFontFamily(), fontWeight);
        const auto fontColor = _isSelected ? Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT) : getRecentsMessageFontColor(_isUnread);
        return qsl("%1; color: %2; background-color: transparent").arg(fontQss, fontColor);
    };

    QColor ContactListParams::timeFontColor(bool _isSelected) const
    {
        return Styling::getParameters().getColor(_isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_SECONDARY);
    }

    QColor ContactListParams::groupColor() const
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    ContactListParams& GetContactListParams()
    {
        static ContactListParams params(true);
        return params;
    }

    ContactListParams& GetRecentsParams()
    {
        static ContactListParams params(false);

        params.setIsCL(false);

        return params;
    }

    ContactListParams& GetRecentsParams(int _regim)
    {
        if (_regim == Logic::MembersWidgetRegim::FROM_ALERT || _regim == Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            static ContactListParams params(false);
            return params;
        }
        else if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            return GetContactListParams();
        }
        else
        {
            const auto show_last_message = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
            static ContactListParams params(!show_last_message);
            params.setIsCL(!show_last_message);
            return params;
        }
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const ViewParams& _viewParams, const int _height)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        QRect rect(0, 0, width, _height);

        RenderMouseState(_painter, _isHovered, _isSelected, rect);
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const QRect& _rect)
    {
        QColor color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        if (_isSelected)
            color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        else if (_isHovered)
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);

        _painter.fillRect(_rect, color);
    }

    int GetXOfRemoveImg(int _width)
    {
        auto _contactList = GetContactListParams();
        return
            CorrectItemWidth(ItemWidth(false, false), _width)
            - _contactList.itemHorPadding()
            - _contactList.removeSize().width()
            + Utils::scale_value(8);
    }

    bool IsSelectMembers(int regim)
    {
        return (regim == Logic::MembersWidgetRegim::SELECT_MEMBERS) || (regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE);
    }

    QString getStateString(const QString& _state)
    {
        if (_state == ql1s("online"))
        {
            return QT_TRANSLATE_NOOP("state", "Online");
        }
        else if (_state == ql1s("offline"))
        {
            return QT_TRANSLATE_NOOP("state", "Offline");
        }
        else if (_state == ql1s("away"))
        {
            return QT_TRANSLATE_NOOP("state", "Away");
        }
        else if (_state == ql1s("dnd"))
        {
            return QT_TRANSLATE_NOOP("state", "Do not disturb");
        }
        else if (_state == ql1s("invisible"))
        {
            return QT_TRANSLATE_NOOP("state", "Invisible");
        }

        return _state;
    }

    QString getStateString(const core::profile_state _state)
    {
        switch (_state)
        {
        case core::profile_state::online:
            return QT_TRANSLATE_NOOP("state", "Online");
        case core::profile_state::dnd:
            return QT_TRANSLATE_NOOP("state", "Do not disturb");
        case core::profile_state::invisible:
            return QT_TRANSLATE_NOOP("state", "Invisible");
        case core::profile_state::away:
            return QT_TRANSLATE_NOOP("state", "Away");
        case core::profile_state::offline:
            return QT_TRANSLATE_NOOP("state", "Offline");
        default:
            assert(!"unknown core::profile_state value");
            return QString();
        }
    }
}

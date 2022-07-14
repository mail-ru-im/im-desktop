#include "stdafx.h"
#include "IconsManager.h"
#include "../../utils/utils.h"

namespace Logic
{
    Utils::LayeredPixmap IconsManager::getIconLayered(const bool _selected, const QString& _iconPath, const QString& _tag, const QSize _size)
    {
        using st = Styling::StyleVariable;
        const auto bg = _selected ? st::TEXT_SOLID_PERMANENT : st::PRIMARY;
        const auto dog = _selected ? st::PRIMARY : st::BASE_GLOBALWHITE;
        if (_size.isValid())
        {
            return Utils::LayeredPixmap(
                _iconPath,
                {
                    { qsl("bg"), Styling::ThemeColorKey { bg } },
                    { _tag, Styling::ThemeColorKey { dog } },
                },
                _size);
        }

        return Utils::LayeredPixmap(
            _iconPath,
            {
                { qsl("bg"), Styling::ThemeColorKey { bg } },
                { _tag, Styling::ThemeColorKey { dog } },
            });
    }

    Utils::LayeredPixmap IconsManager::getMentionIcon(const bool _selected, const QSize _size)
    {
        return getIconLayered(_selected, qsl(":/recent_mention_icon"), qsl("dog"), _size);
    }

    QSize IconsManager::getDraftSize() noexcept
    {
        return Utils::scale_value(QSize(16, 16));
    }

    Utils::StyledPixmap IconsManager::getIcon(const QString& _path, Styling::StyleVariable _color, const QSize& _size)
    {
        return Utils::StyledPixmap(_path, _size, Styling::ThemeColorKey{ _color });
    }

    Utils::StyledPixmap& IconsManager::getDraftIcon(const bool _isSelected)
    {
        using st = Styling::StyleVariable;
        static Utils::StyledPixmap draftIcon = IconsManager::getIcon(qsl(":/pencil_icon"), st::BASE_TERTIARY, getDraftSize());
        static Utils::StyledPixmap selectedDraftIcon = IconsManager::getIcon(qsl(":/pencil_icon"), st::TEXT_SOLID_PERMANENT, getDraftSize());
        return _isSelected ? selectedDraftIcon : draftIcon;
    }

    QSize IconsManager::getMediaIconSize() noexcept
    {
        return QSize(16, 16);
    }

    QPixmap IconsManager::getMediaTypePix(const Ui::MediaType _mediaType, const bool _isSelected)
    {
        auto getPixmap = [_isSelected](const QString& _path)
        {
            return Utils::renderSvgScaled(_path, getMediaIconSize(), _isSelected ? Qt::white : Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
        };
        using mt = Ui::MediaType;
        switch (_mediaType)
        {
            case mt::mediaTypeSticker:
            {
                return getPixmap(qsl(":/message_type_sticker_icon"));
            }
            case mt::mediaTypeVideo:
            case mt::mediaTypeFsVideo:
            {
                return getPixmap(qsl(":/message_type_video_icon"));
            }
            case mt::mediaTypePhoto:
            case mt::mediaTypeFsPhoto:
            {
                return getPixmap(qsl(":/message_type_photo_icon"));
            }
            case mt::mediaTypeGif:
            case mt::mediaTypeFsGif:
            {
                return getPixmap(qsl(":/message_type_video_icon"));
            }
            case mt::mediaTypePtt:
            {
                return getPixmap(qsl(":/message_type_audio_icon"));
            }
            case mt::mediaTypeVoip:
            {
                return getPixmap(qsl(":/message_type_phone_icon"));
            }
            case mt::mediaTypeFileSharing:
            {
                return getPixmap(qsl(":/message_type_file_icon"));
            }
            case mt::mediaTypeContact:
            {
                return getPixmap(qsl(":/message_type_contact_icon"));
            }

            case mt::mediaTypeGeo:
            {
                return getPixmap(qsl(":/message_type_geo_icon"));
            }

            case mt::mediaTypePoll:
            {
                return getPixmap(qsl(":/message_type_poll_icon"));
            }

            case mt::mediaTypeTask:
            {
                return getPixmap(qsl(":/message_type_task_icon"));
            }

            default:
            {
                im_assert(false);
                return {};
            }
        }
    }

    const QSize IconsManager::pinnedServiceItemIconSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    Utils::StyledPixmap IconsManager::pinnedServiceItemPixmap(Data::DlgState::PinnedServiceItemType _type, const bool _isSelected)
    {
        const auto iconSize = pinnedServiceItemIconSize();
        std::function getPixmap = [iconSize, _isSelected](const QString& path) -> Utils::StyledPixmap
        {
            using st = Styling::StyleVariable;
            return getIcon(path, _isSelected ? st::PRIMARY : st::BASE_SECONDARY, iconSize);
        };

        switch (_type)
        {
            case Data::DlgState::PinnedServiceItemType::Threads:
                return getPixmap(qsl(":/thread_icon_filled"));
            case Data::DlgState::PinnedServiceItemType::ScheduledMessages:
                return getPixmap(qsl(":/contact_list/scheduled"));
            case Data::DlgState::PinnedServiceItemType::Reminders:
                return getPixmap(qsl(":/contact_list/reminders"));
            case Data::DlgState::PinnedServiceItemType::Favorites:
                return getPixmap(qsl(":/favorites_icon"));
            default:
            {
                im_assert(!"Cant get pinned service item icon");
                return Utils::StyledPixmap{};
            }
        }
    }

    QSize IconsManager::getAttentionSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    Utils::LayeredPixmap IconsManager::getAttentionIcon(const bool _selected)
    {
        return getIconLayered(_selected, qsl(":/unread_mark_icon"), qsl("star"));
    }

    int IconsManager::getUnreadBubbleHeight() noexcept
    {
        return Utils::scale_value(20);
    }

    int IconsManager::getUnreadsSize() noexcept
    {
        return Utils::scale_value(20);
    }

    int IconsManager::getUnreadLeftPadding() noexcept
    {
        return Utils::scale_value(4);
    }

    QSize IconsManager::getMentionSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int IconsManager::getUnreadBubbleRightMargin(bool _compactMode) noexcept
    {
        return getRigthLineCenter(_compactMode) - (getUnreadBubbleWidthMin() / 2);
    }

    int IconsManager::getRigthLineCenter(const bool _compactMode) noexcept
    {
        return Utils::scale_value(22);
    }

    int IconsManager::getUnreadBubbleWidthMin() noexcept
    {
        return Utils::scale_value(20);
    }

    int IconsManager::getUnreadLeftPaddingPictOnly() noexcept
    {
        return Utils::scale_value(2);
    }

    int IconsManager::getUnreadBubbleRightMarginPictOnly() noexcept
    {
        return Utils::scale_value(4);
    }

    int IconsManager::getUnreadBalloonWidth(int _unreadCount)
    {
        return Utils::getUnreadsBadgeSize(_unreadCount, Logic::IconsManager::getUnreadBubbleHeight()).width();
    }

    int IconsManager::getUnknownRightOffset() noexcept
    {
        return Utils::scale_value(40);
    }

    int IconsManager::buttonWidth() noexcept
    {
        return Utils::scale_value(32);
    }
}


#pragma once

#include "../../styles/ThemeParameters.h"
#include "../mediatype.h"
#include "../../types/message.h"
#include "utils/SvgUtils.h"

namespace Logic
{
    class IconsManager
    {
        public:
            static QSize getDraftSize() noexcept;
            static Utils::StyledPixmap& getDraftIcon(const bool _isSelected);

            static QSize getMentionSize() noexcept;
            static Utils::LayeredPixmap getMentionIcon(const bool _selected, const QSize _size = QSize());

            static QSize getAttentionSize() noexcept;
            static Utils::LayeredPixmap getAttentionIcon(const bool _selected);

            static Utils::LayeredPixmap getIconLayered(const bool _selected, const QString& _iconPath, const QString& _tag, const QSize _size = {});
            static Utils::StyledPixmap getIcon(const QString& _path, Styling::StyleVariable _color, const QSize& _size);

            static QSize getMediaIconSize() noexcept;
            static QPixmap getMediaTypePix(const Ui::MediaType _mediaType, const bool _isSelected);

            static const QSize pinnedServiceItemIconSize() noexcept;
            static Utils::StyledPixmap pinnedServiceItemPixmap(Data::DlgState::PinnedServiceItemType _type, const bool _isSelected);

            static int getUnreadBubbleHeight() noexcept;
            static int getUnreadBubbleRightMargin(bool _compactMode = true) noexcept;
            static int getUnreadsSize() noexcept;
            static int getUnreadLeftPadding() noexcept;
            static int getRigthLineCenter(const bool _compactMode) noexcept;
            static int getUnreadBubbleWidthMin() noexcept;
            static int getUnreadLeftPaddingPictOnly() noexcept;
            static int getUnreadBubbleRightMarginPictOnly() noexcept;
            static int getUnreadBalloonWidth(int _unreadCount);
            static int getUnknownRightOffset() noexcept;
            static int buttonWidth() noexcept;
    };
}

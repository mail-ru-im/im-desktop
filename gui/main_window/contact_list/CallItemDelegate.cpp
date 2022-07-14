#include "stdafx.h"

#include "CallItemDelegate.h"
#include "CallsModel.h"
#include "CallsSearchModel.h"
#include "SearchHighlight.h"
#include "../../styles/StyleVariable.h"
#include "../../styles/ThemeParameters.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../history_control/VoipEventInfo.h"

namespace
{
    constexpr auto AVATAR_SIZE = 32;
    constexpr auto HOR_OFFSET = 12;
    constexpr auto ICON_HOR_OFFSET = 16;
    constexpr auto SMALL_HOR_OFFSET = 4;
    constexpr auto ITEM_HEIGHT = 44;
    constexpr auto SERVICE_ITEM_HEIGHT = 40;
    constexpr auto SPACE_ITEM_HEIGHT = 8;
    constexpr auto TYPE_ICON_SIZE = 16;
    constexpr auto BUTTON_ICON_SIZE = 24;
    constexpr auto ICONS_VER_OFFSET = 2;

    constexpr QSize iconSize() noexcept
    {
        return { TYPE_ICON_SIZE, TYPE_ICON_SIZE };
    }

    QPixmap getCallIcon(const bool _isAudio, const bool _isMissed)
    {
        static QPixmap pixPhone, pixPhoneMissed, pixVideo, pixVideoMissed;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || pixPhone.isNull())
        {
            pixPhone = Utils::renderSvgScaled(qsl(":/message_type_phone_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            pixPhoneMissed = Utils::renderSvgScaled(qsl(":/message_type_phone_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            pixVideo = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            pixVideoMissed = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
        }

        if (_isAudio)
            return _isMissed ? pixPhoneMissed : pixPhone;

        return _isMissed ? pixVideoMissed : pixVideo;
    }

    QPixmap getCallTypeIcon(const bool _isOutgoing, const bool _isMissed)
    {
        static QPixmap pixIncoming, pixIncomingMissed, pixOutgoing, pixOutgoingMissed;

        auto rotate = [](const auto& pix)
        {
            QMatrix m;
            m.rotate(180);
            QPixmap result = pix.transformed(m);
            Utils::check_pixel_ratio(result);

            return result;
        };

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || pixIncoming.isNull())
        {
            pixIncoming = Utils::renderSvgScaled(qsl(":/call_type"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            pixIncomingMissed = Utils::renderSvgScaled(qsl(":/call_type"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            pixOutgoing = rotate(pixIncoming);
            pixOutgoingMissed = rotate(pixIncomingMissed);
        }

        if (_isMissed)
            return _isOutgoing ? pixOutgoingMissed : pixIncomingMissed;

        return _isOutgoing ? pixOutgoing : pixIncoming;
    }

    QPixmap getGroupCallIcon()
    {
        static auto pixIncoming = Utils::StyledPixmap::scaled(qsl(":/groupcall"), iconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
        return pixIncoming.actualPixmap();
    }

    QPixmap getButtonIcon(QStringView _aimid)
    {
        static QPixmap pixCall, pixCallByLink, pixWebinar;
        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || pixCall.isNull())
        {
            pixCall = Utils::renderSvgScaled(qsl(":/header/call"), QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            pixCallByLink = Utils::renderSvgScaled(qsl(":/link_icon"), QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            pixWebinar = Utils::renderSvgScaled(qsl(":/webinar"), QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        }

        if (_aimid == u"~group_call~")
            return pixCall;

        if (_aimid == u"~call_by_link~")
            return pixCallByLink;

        if (_aimid == u"~webinar~")
            return pixWebinar;

        return QPixmap();
    }

    QFont getNameFont(bool _isButton)
    {
        return Fonts::appFontScaled(16, (platform::is_apple() && !_isButton) ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
    }

    int countLabelMargin() noexcept
    {
        return Utils::scale_value(8);
    }
}

using namespace Ui::TextRendering;

namespace Logic
{
    CallItemDelegate::CallItemDelegate(QObject* _parent)
        : AbstractItemDelegateWithRegim(_parent)
        , pictureOnly_(false)
    {
        name_ = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        name_->init({ getNameFont(false), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID} });

        serviceName_ = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        serviceName_->init({ getNameFont(true), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });

        date_ = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        date_->init({ Fonts::appFontScaled(14), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });

        count_ = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        count_->init({ getNameFont(false), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID} });
    }

    void CallItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::CallInfoPtr>();
        if (!item || item->isSpaceItem())
            return;

        Utils::PainterSaver saver(*_painter);
        Ui::RenderMouseState(*_painter, (_option.state & QStyle::State_Selected), false, _option.rect);
        _painter->translate(_option.rect.topLeft());

        const auto ratio = Utils::scale_bitmap_ratio();

        if (item->isService())
        {
            const auto& icon = getButtonIcon(item->getServiceAimid());
            const auto x = pictureOnly_ ? (_option.rect.width() / 2 - icon.width() / ratio / 2) : Utils::scale_value(ICON_HOR_OFFSET);
            _painter->drawPixmap(x, (_option.rect.height() - icon.height() / ratio) / 2, icon);

            if (!pictureOnly_)
            {
                serviceName_->setText(item->getButtonsText(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
                serviceName_->setOffsets(Utils::scale_value(HOR_OFFSET * 2) + Utils::scale_value(AVATAR_SIZE), _option.rect.height() / 2);
                serviceName_->draw(*_painter, VerPosition::MIDDLE);
            }
            return;
        }

        bool isDefault = false;
        if (!item->isSingleItem())
            Logic::GetAvatarStorage()->UpdateDefaultAvatarIfNeed(Utils::getDefaultCallAvatarId());

        const auto avatar = Logic::GetAvatarStorage()->GetRounded(item->isSingleItem() ? item->getAimId() : Utils::getDefaultCallAvatarId(), item->getFriendly(), Utils::scale_bitmap(Utils::scale_value(AVATAR_SIZE)), isDefault, false, false);

        const auto x = pictureOnly_ ? (_option.rect.width() / 2 - avatar.width() / ratio / 2) : Utils::scale_value(HOR_OFFSET);
        const auto y = (_option.rect.height() - avatar.height() / ratio) / 2;
        const auto y2 = y + avatar.height() / ratio / 2;

        const auto isMissed = item->isMissed();

        _painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        Utils::drawAvatarWithBadge(*_painter, QPoint(x, y), avatar, item->isSingleItem() ? item->getAimId() : Utils::getDefaultCallAvatarId());

        if (pictureOnly_)
            return;

        const auto isSearch = (qobject_cast<const Logic::CallsSearchModel*>(_index.model()) != nullptr);
        QFontMetrics m(getNameFont(false));

        const auto groupCallIcon = getGroupCallIcon();
        auto elidedWidth = _option.rect.width() - Utils::scale_value(HOR_OFFSET * 3) - Utils::scale_value(AVATAR_SIZE) - (item->isSingleItem() ? 0 : Utils::scale_value(TYPE_ICON_SIZE + SMALL_HOR_OFFSET));

        const auto callsCount = item->count();
        const auto needCount = callsCount > 1;

        if (needCount)
        {
            count_->setText(u'(' % QString::number(callsCount) % u')');
            count_->evaluateDesiredSize();
            elidedWidth -= count_->desiredWidth() + countLabelMargin();
        }

        const auto nameHorizontalOffset = Utils::scale_value(HOR_OFFSET * 2) + Utils::scale_value(AVATAR_SIZE);
        auto countHorizontalOffset = nameHorizontalOffset;

        if (isSearch)
        {
            auto name = Ui::createHightlightedText(
                    item->getFriendly(),
                    getNameFont(false),
                    Styling::ThemeColorKey{ isMissed ? Styling::StyleVariable::SECONDARY_ATTENTION : Styling::StyleVariable::TEXT_SOLID },
                    Ui::getTextHighlightedColor(),
                    1,
                    item->getHightlights());

            name->elide(elidedWidth);
            name->evaluateDesiredSize();
            name->setOffsets(nameHorizontalOffset, 0);
            name->draw(*_painter);

            countHorizontalOffset += name->desiredWidth();

            if (!item->isSingleItem())
                _painter->drawPixmap(name->isElided() ? name->horOffset() + elidedWidth :
                                     name->horOffset() + name->cachedSize().width() + Utils::scale_value(SMALL_HOR_OFFSET),
                                     name->cachedSize().height() / 2 + (platform::is_apple() ? 0 : m.descent() / 2) - groupCallIcon.height() / 2 / ratio, groupCallIcon);
            elidedItems_[_index.row()] = name->isElided();
        }
        else
        {
            name_->setText(item->getFriendly(), Styling::ThemeColorKey{ isMissed ? Styling::StyleVariable::SECONDARY_ATTENTION : Styling::StyleVariable::TEXT_SOLID });
            name_->elide(elidedWidth);
            name_->evaluateDesiredSize();
            name_->setOffsets(nameHorizontalOffset, 0);
            name_->draw(*_painter);

            if (!item->isSingleItem())
                _painter->drawPixmap(name_->isElided() ? name_->horOffset() + elidedWidth :
                                     name_->horOffset() + name_->cachedSize().width() + Utils::scale_value(SMALL_HOR_OFFSET),
                                     name_->cachedSize().height() / 2 + (platform::is_apple() ? 0 : m.descent() / 2)  - groupCallIcon.height() / 2 / ratio, groupCallIcon);
            elidedItems_[_index.row()] = name_->isElided();

            countHorizontalOffset += name_->desiredWidth();
        }

        if (needCount)
        {
            countHorizontalOffset += countLabelMargin() + (item->isSingleItem() ? 0 : groupCallIcon.width() / ratio);
            count_->setOffsets(countHorizontalOffset, 2 * name_->verOffset());
            count_->draw(*_painter);
        }

        auto status = Data::LastSeen(item->time()).getStatusString(true);
        if (!status.isEmpty())
            status[0] = status.at(0).toUpper();

        date_->setText(status, Styling::ThemeColorKey{ isMissed ? Styling::StyleVariable::SECONDARY_ATTENTION : Styling::StyleVariable::BASE_PRIMARY });
        date_->setOffsets(Utils::scale_value(HOR_OFFSET * 2 + AVATAR_SIZE + TYPE_ICON_SIZE + SMALL_HOR_OFFSET), y2);
        date_->draw(*_painter);

        const auto iconsAddOffset = Utils::scale_value(ICONS_VER_OFFSET);
        _painter->drawPixmap(Utils::scale_value(HOR_OFFSET * 2) + Utils::scale_value(AVATAR_SIZE), y2 + iconsAddOffset, getCallIcon(!item->isVideo(), isMissed));
        _painter->drawPixmap(date_->horOffset() + date_->cachedSize().width() + Utils::scale_value(SMALL_HOR_OFFSET), y2 + iconsAddOffset, getCallTypeIcon(item->isOutgoing(), isMissed));
    }

    void CallItemDelegate::setRegim(int _regim)
    {

    }

    void CallItemDelegate::setFixedWidth(int width)
    {

    }

    void CallItemDelegate::blockState(bool value)
    {

    }

    void CallItemDelegate::setDragIndex(const QModelIndex& index)
    {

    }

    void CallItemDelegate::setPictOnlyView(bool _isPictureOnly)
    {
        pictureOnly_ = _isPictureOnly;
        Logic::GetCallsModel()->setTopSpaceVisible(_isPictureOnly);
    }

    QSize CallItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::CallInfoPtr>();
        return QSize(_option.rect.width(), Utils::scale_value(item && item->isService() ? (item->isSpaceItem() ? SPACE_ITEM_HEIGHT : SERVICE_ITEM_HEIGHT) : ITEM_HEIGHT));
    }

    bool CallItemDelegate::needsTooltip(const QString&, const QModelIndex& _index, QPoint) const
    {
        const auto it = elidedItems_.find(_index.row());
        return it != elidedItems_.end() && it->second;
    }

    QString CallItemDelegate::getFriendly(const QModelIndex& _index) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::CallInfoPtr>();
        if (!item || item->isSpaceItem())
            return {};

        return item->getFriendly();
    }
}

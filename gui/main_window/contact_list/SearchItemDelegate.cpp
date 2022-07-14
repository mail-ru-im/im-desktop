#include "stdafx.h"

#include "Common.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "SearchModel.h"
#include "SearchItemDelegate.h"
#include "SearchHighlight.h"

#include "../containers/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../proxy/FriendlyContaInerProxy.h"
#include "../proxy/AvatarStorageProxy.h"
#include "../../app_config.h"
#include "../../my_info.h"
#include "FavoritesUtils.h"

#include "styles/ThemeParameters.h"
#include "controls/TextUnit.h"
#include "IconsManager.h"

using namespace Ui::TextRendering;

namespace
{
    const auto addIconSize = QSize(20, 20);
    constexpr std::chrono::milliseconds dropCacheTimeout = std::chrono::minutes(3);

    constexpr auto maxMsgLinesCount = 3;
    constexpr auto largeMessageThreshold = 100;
    constexpr auto leftCropCount = 30;

    int getContactNameVerOffset()
    {
        return Utils::scale_value(4);
    }

    int getMsgNameVerOffset()
    {
        return Utils::scale_value(8);
    }

    int getHorMargin()
    {
        return Utils::scale_value(12);
    }

    QSize getRemoveSuggestIconSize()
    {
        return Utils::scale_value(QSize(16, 16));
    }

    int getNickOffset()
    {
        return Utils::scale_value(4);
    }

    QPixmap getRemoveSuggestIcon(const bool _isHovered)
    {
        const auto getIcon = [](const QString& _iconPath, Styling::StyleVariable _color)
        {
            return Utils::StyledPixmap(_iconPath, getRemoveSuggestIconSize(), Styling::ThemeColorKey{ _color });
        };
        // COLOR
        using st = Styling::StyleVariable;
        static auto del = getIcon(qsl(":/controls/close_icon"), st::BASE_SECONDARY);
        static auto delHovered = getIcon(qsl(":/controls/close_icon"), st::BASE_SECONDARY_HOVER);

        return (_isHovered ? delHovered : del).actualPixmap();
    }

    Styling::ThemeColorKey getTextColorSelectedState()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
    }

    Styling::ThemeColorKey getNameTextColor(const bool _isSelected, const bool _isFavorites = false)
    {
        if (_isSelected)
            return getTextColorSelectedState();
        else if (_isFavorites)
            return Favorites::nameColor();

        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    Styling::ThemeColorKey getNickTextColor(const bool _isSelected)
    {
        return _isSelected ? getTextColorSelectedState() : Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    Styling::ThemeColorKey getNameTextHighlightedColor()
    {
        return Ui::getTextHighlightedColor();
    }

    Styling::ThemeColorKey getMessageTextColor(const bool _isSelected)
    {
        return _isSelected ? getTextColorSelectedState() : Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    Styling::ThemeColorKey getMessageTextHighlightedColor(const bool _isSelected)
    {
        return getNameTextHighlightedColor();
    }

    Ui::TextRendering::TextUnitPtr createName(const QString& _text, const Styling::ThemeColorKey& _nameColor, const int _maxWidth, const Ui::highlightsV& _highlights = {})
    {
        if (_text.isEmpty())
            return Ui::TextRendering::TextUnitPtr();

        static const auto font = Ui::GetRecentsParams().getContactNameFont(Fonts::FontWeight::Normal);

        Ui::TextRendering::TextUnitPtr unit = Ui::createHightlightedText(
            _text,
            font,
            _nameColor,
            getNameTextHighlightedColor(),
            1,
            _highlights);

        unit->getHeight(_maxWidth);

        return unit;
    }

    Ui::TextRendering::TextUnitPtr createNick(const QString& _text, const bool _isSelected, const int _maxWidth, const Ui::highlightsV& _highlights = {})
    {
        if (_text.isEmpty())
            return Ui::TextRendering::TextUnitPtr();

        static const auto font = Ui::GetRecentsParams().getContactNameFont(Fonts::FontWeight::Normal);

        Ui::TextRendering::TextUnitPtr unit = Ui::createHightlightedText(
            Utils::makeNick(_text),
            font,
            getNickTextColor(_isSelected),
            getNameTextHighlightedColor(),
            1,
            _highlights);

        unit->getHeight(_maxWidth);

        return unit;
    }

    QString getMessageText(const Data::MessageBuddySptr& _msg, const Ui::highlightsV& _highlights, const Ui::HighlightPosition _hlPos)
    {
        std::vector<QString> parts;
        parts.reserve(_msg->Quotes_.size() + (_msg->snippets_.size() * 3) + 1);

        for (const auto& q : std::as_const(_msg->Quotes_))
        {
            if (!q.isSticker() && !q.text_.isEmpty())
                parts.push_back(q.text_.string());
        }

        for (const auto& s : _msg->snippets_)
        {
            if (!s.title_.isEmpty())
                parts.push_back(s.title_);
            if (!s.description_.isEmpty())
                parts.push_back(s.description_);
            if (!s.url_.isEmpty())
                parts.push_back(s.url_);
        }

        if (!_msg->IsSticker() && !_msg->GetSourceText().isEmpty())
            parts.push_back(_msg->GetSourceText().string());

        for (const auto& part: parts)
        {
            if (const auto hl = findNextHighlight(part, _highlights, 0, _hlPos); hl.indexStart_ != -1)
            {
                const auto cropCount = hl.indexStart_ >= largeMessageThreshold ? leftCropCount : largeMessageThreshold;
                const auto startPos = hl.indexStart_ - cropCount;
                const auto len = hl.length_ + cropCount + largeMessageThreshold;

                const auto res = part.midRef(std::max(startPos, 0), len);

                if (hl.indexStart_ >= largeMessageThreshold)
                    return Ui::getEllipsis() % res;

                return res.toString();
            }
        }

        return _msg->GetSourceText().string();
    }
}

namespace Logic
{
    SearchItemDelegate::SearchItemDelegate(QObject* _parent)
        : AbstractItemDelegateWithRegim(_parent)
        , selectedMessage_(-1)
        , lastDrawTime_(QTime::currentTime())
    {
        cacheTimer_.setInterval(dropCacheTimeout);
        cacheTimer_.setTimerType(Qt::VeryCoarseTimer);
        connect(&cacheTimer_, &QTimer::timeout, this, [this]()
        {
            if (lastDrawTime_.msecsTo(QTime::currentTime()) >= cacheTimer_.interval())
                dropCache();
        });
        cacheTimer_.start();

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, this, &SearchItemDelegate::dlgStateChanged);
    }

    void SearchItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Utils::PainterSaver saver(*_painter);

        auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
            return;

        const auto [isSelected, isHovered] = getMouseState(_option, _index);

        Ui::RenderMouseState(*_painter, isHovered, isSelected, _option.rect);

        drawOffset_ =_option.rect.topLeft();
        _painter->translate(_option.rect.topLeft());

        switch (item->getType())
        {
        case Data::SearchResultType::service:
            drawServiceItem(*_painter, _option, std::static_pointer_cast<Data::SearchResultService>(item));
            break;

        case Data::SearchResultType::contact:
        case Data::SearchResultType::chat:
            drawContactOrChatItem(*_painter, _option, item, isSelected, _index == dragIndex_);
            break;

        case Data::SearchResultType::message:
            drawMessageItem(*_painter, _option, std::static_pointer_cast<Data::SearchResultMessage>(item), isSelected);
            break;

        case Data::SearchResultType::suggest:
            drawSuggest(*_painter, _option, std::static_pointer_cast<Data::SearchResultSuggest>(item), _index, isHovered);
            break;

        default:
            im_assert(false);
            break;
        }

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            if (!item->isService())
            {
                _painter->setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
                _painter->setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

                const auto x = _option.rect.width();
                const auto y = _option.rect.height();
                const QStringList debugInfo =
                {
                    Logic::getContactListModel()->isChannel(item->getAimId()) ? qsl("Chan") : QString(),
                    Logic::getContactListModel()->contains(item->getAimId())
                        ? (item->isContact() || item->isChat()) ? u"oc:" % QString::number(Logic::getContactListModel()->getOutgoingCount(item->getAimId())) : QString()
                        : qsl("!CL"),
                    item->isLocalResult_ ? qsl("local") : qsl("server"),
                    item->getAimId(),
                    item->isMessage() ? QString::number(item->getMessageId()) : QString()
                };
                Utils::drawText(*_painter, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, debugInfo.join(ql1c(' ')));
            }
        }
    }

    void SearchItemDelegate::drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const
    {
        if (!needDrawDraft(_viewParams, _state) || !_painter)
            return;

        using im = Logic::IconsManager;

        const QPixmap badge = im::getDraftIcon(_isSelected).actualPixmap();
        _rightAreaRight -= Utils::unscale_bitmap(badge.width());

        if (needDrawUnreads(_state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getUnreadBalloonWidth(_state.UnreadCount_);
        }

        if (needDrawAttention(_state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getAttentionSize().width();
        }

        if (needDrawMentions(_viewParams, _state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getMentionSize().width();
        }

        const auto draft_y = (_rect.height() - im::getDraftSize().height())/2;
        _painter->drawPixmap(_rightAreaRight, draft_y, badge);

        draftIconRect_.setRect(_rightAreaRight + drawOffset_.x(), draft_y + drawOffset_.y(), badge.width(), badge.height());
    }

    bool SearchItemDelegate::needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        Q_UNUSED(_viewParams);
        return _state.hasDraft();
    }

    bool SearchItemDelegate::needDrawUnreads(const Data::DlgState& _state) const
    {
        return _state.UnreadCount_ > 0;
    }

    void SearchItemDelegate::drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams,
                                         int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY); Q_UNUSED(_viewParams);

        if (!needDrawUnreads(_state) || !_painter)
            return;

        const auto unreadBalloonWidth = _state.UnreadCount_ > 0 ? Utils::getUnreadsBadgeSize(_state.UnreadCount_, IconsManager::getUnreadBubbleHeight()).width() : 0;
        _unreadsX -= IconsManager::getUnreadBubbleRightMargin();
        _unreadsX -= unreadBalloonWidth;

        using st = Styling::StyleVariable;
        std::function getColor = [_isSelected](st _selectedColor, st _color){ return Styling::getParameters().getColor(_isSelected ? _selectedColor : _color); };
        const auto muted_ = Logic::getContactListModel()->isMuted(_state.AimId_);
        const auto bgColor = getColor(st::TEXT_SOLID_PERMANENT, muted_ ? st::BASE_TERTIARY : st::PRIMARY);
        const auto textColor = getColor(st::PRIMARY_SELECTED, st::BASE_GLOBALWHITE);

        auto x = _rect.left() + _unreadsX;
        auto y = (_rect.height() - IconsManager::getUnreadsSize()) / 2;

        auto contactListParams = Ui::GetContactListParams();
        Utils::drawUnreads(*(_painter), contactListParams.unreadsFont(), bgColor, textColor, _state.UnreadCount_, IconsManager::getUnreadBubbleHeight(), x, y);
    }

    bool SearchItemDelegate::needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        Q_UNUSED(_viewParams);
        return _state.unreadMentionsCount_ > 0;
    }

    bool SearchItemDelegate::needDrawAttention(const Data::DlgState& _state) const
    {
        return !needDrawUnreads(_state) && _state.Attention_;
    }

    void SearchItemDelegate::drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY);

        if (!needDrawMentions(_viewParams, _state) || !_painter)
            return;

        _unreadsX -= needDrawUnreads(_state) ? IconsManager::getUnreadLeftPadding() : IconsManager::getUnreadBubbleRightMargin();
        _unreadsX -= IconsManager::getMentionSize().width();

        _painter->drawPixmap(_rect.left() + _unreadsX, (_rect.height() - IconsManager::getMentionSize().height()) / 2, IconsManager::getMentionIcon(_isSelected).cachedPixmap());
    }

    void SearchItemDelegate::drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY); Q_UNUSED(_viewParams);
        if (!needDrawAttention(_state) || !_painter)
            return;

        _unreadsX -= IconsManager::getUnreadBubbleRightMargin();
        _unreadsX -= IconsManager::getAttentionSize().width();

        auto x = _rect.left() + _unreadsX;
        auto y = (_rect.height() - IconsManager::getAttentionSize().height()) / 2;

        _painter->drawPixmap(x, y, IconsManager::getAttentionIcon(_isSelected).cachedPixmap());
    }

    void SearchItemDelegate::onContactSelected(const QString& _aimId, qint64 _msgId, qint64 _threadMsgId)
    {
        selectedMessage_ = _threadMsgId == -1 ? _msgId : _threadMsgId;
    }

    void SearchItemDelegate::clearSelection()
    {
        selectedMessage_ = -1;
    }

    void SearchItemDelegate::dropCache()
    {
        contactCache_.clear();
        messageCache_.clear();
    }

    void SearchItemDelegate::drawContactOrChatItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected, bool _isDrag) const
    {
        const auto& params = Ui::GetContactListParams();

        drawAvatar(_painter, params, _item->getAimId(), _option.rect.height(), _isSelected, _item->isMessage());
        drawCachedItem(_painter, _option, _item, _isSelected);

        const auto &state = Logic::getRecentsModel()->getDlgState(_item->getAimId());
        auto unreadsX = _option.rect.width();
        auto unreadsY = 0;
        auto right_area_right = _option.rect.width() - Logic::IconsManager::getUnreadBubbleRightMargin();
        Ui::ViewParams viewParams;

        drawUnreads(&_painter, state, _isSelected, _option.rect, viewParams, unreadsX, unreadsY);
        drawMentions(&_painter, state, _isSelected, _option.rect, viewParams, unreadsX, unreadsY);
        drawAttention(&_painter, state, _isSelected, _option.rect, viewParams, unreadsX, unreadsY);
        drawDraft(&_painter, state, _isSelected, _option.rect, viewParams, right_area_right);
        if (_isDrag)
            renderDragOverlay(_painter, QRect { QPoint { 0, 0 }, _option.rect.size() }, viewParams);
    }

    void SearchItemDelegate::drawMessageItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::MessageSearchResultSptr& _item, const bool _isSelected) const
    {
        drawAvatar(_painter, Ui::GetRecentsParams(), _item->getSenderAimId(), _option.rect.height(), _isSelected, _item->isMessage());
        drawCachedItem(_painter, _option, _item, _isSelected);
    }

    void SearchItemDelegate::drawServiceItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::ServiceSearchResultSptr& _item) const
    {
        Utils::PainterSaver saver(_painter);

        static auto pen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        _painter.setPen(pen);

        // FONT
        _painter.setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));

        const auto x = Ui::GetContactListParams().itemHorPadding();
        const auto y = _option.rect.height() / 2;
        Utils::drawText(_painter, QPointF(x, y), Qt::AlignVCenter, _item->getFriendlyName());
    }

    void SearchItemDelegate::drawSuggest(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::SuggestSearchResultSptr& _item, const QModelIndex& _index, const bool _isHovered) const
    {
        const auto ratio = Utils::scale_bitmap_ratio();
        {
            static auto icon = Utils::StyledPixmap::scaled(qsl(":/search_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
            auto pixmap = icon.actualPixmap();
            const auto iconX = getHorMargin();
            const auto iconY = (_option.rect.height() - pixmap.height() / ratio) / 2;
            _painter.drawPixmap(iconX, iconY, pixmap.width() / ratio, pixmap.height() / ratio, pixmap);
        }

        const auto textLeftOffset = Utils::scale_value(44);
        int maxTextWidth = _option.rect.width() - textLeftOffset - getHorMargin();

        if (_isHovered)
        {
            const bool delHovered = hoveredRemoveSuggestBtn_.isValid() && _index == hoveredRemoveSuggestBtn_;
            const auto icon = getRemoveSuggestIcon(delHovered);

            const auto iconX = _option.rect.width() - icon.width() / ratio - getHorMargin();
            const auto iconY = (_option.rect.height() - icon.height() / ratio) / 2;
            _painter.drawPixmap(iconX, iconY, icon.width() / ratio, icon.height() / ratio, icon);

            maxTextWidth = iconX - textLeftOffset - getHorMargin();
        }
        fixMaxWidthIfIcons(maxTextWidth, _item);

        static Ui::TextRendering::TextUnitPtr text;
        if (!text)
        {
            // FONT
            text = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
            text->init({ Fonts::appFontScaled(16), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });

            text->setOffsets(textLeftOffset, _option.rect.height() / 2 - Utils::scale_value(2));
        }

        text->setText(_item->getFriendlyName());
        text->elide(maxTextWidth);
        text->evaluateDesiredSize();
        text->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);
    }

    bool SearchItemDelegate::updateItem(std::unique_ptr<CachedItem>& _ci, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected, const int _maxTextWidth) const
    {
        if (_ci && !_ci->needUpdate(_option, _item, _isSelected))
            return false;

        _ci = std::make_unique<CachedItem>();
        _ci->update(_option, _item, _isSelected);
        if (_item->isMessage())
            fillMessageItem(*_ci, _option, _maxTextWidth);
        else if (_item->isChat() || _item->isContact())
            fillContactItem(*_ci, _option, _maxTextWidth);
        return true;
    }

    SearchItemDelegate::CachedItem* SearchItemDelegate::getCachedItem(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const
    {
        if (_item->getFriendlyName().isEmpty())
            return nullptr;

        const auto& params = _item->isMessage() ? Ui::GetRecentsParams() : Ui::GetContactListParams();
        auto maxTextWidth = _option.rect.width() - params.getContactNameX() - params.itemHorPadding();
        fixMaxWidthIfIcons(maxTextWidth, _item);

        if (_item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(_item);
            auto& ci = messageCache_[msg->getMessageId()];
            bool needUpdate = updateItem(ci, _option, _item, _isSelected, maxTextWidth);
            if (ci && !needUpdate && ci->needUpdateWidth(_option))
                reelideMessage(*ci, _option, maxTextWidth);
            return ci.get();
        }
        else if (_item->isContact() || _item->isChat())
        {
            auto& ci = contactCache_[_item->getAimId()];
            updateItem(ci, _option, _item, _isSelected, maxTextWidth);
            return ci.get();
        }
        return nullptr;
    }

    SearchItemDelegate::CachedItem* SearchItemDelegate::drawCachedItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const
    {
        if (const auto ci = getCachedItem(_option, _item, _isSelected))
        {
            const auto& params = _item->isMessage() ? Ui::GetRecentsParams() : Ui::GetContactListParams();
            auto maxTextWidth = _option.rect.width() - params.getContactNameX() - params.itemHorPadding();
            fixMaxWidthIfIcons(maxTextWidth, _item);

            reelideMessage(*ci, _option, maxTextWidth);
            ci->draw(_painter);
            lastDrawTime_ = QTime::currentTime();
            return ci;
        }

        return nullptr;
    }

    void SearchItemDelegate::fillContactItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto leftOffset = Ui::GetContactListParams().getContactNameX();
        auto nameVerOffset = _option.rect.height() / 2;
        ci.addName(replaceFavorites_, _maxTextWidth);
        if (ci.addText(_maxTextWidth, leftOffset))
            nameVerOffset = getContactNameVerOffset();
        const auto textWidth = ci.addNick(replaceFavorites_, _maxTextWidth, getNickOffset(), nameVerOffset, leftOffset);
        ci.setOffsets(getNickOffset(), nameVerOffset, leftOffset, textWidth);
    }

    void SearchItemDelegate::fixMaxWidthIfIcons(int& maxWidth, const Data::AbstractSearchResultSptr& _item) const
    {
        if (!_item->isContact() && !_item->isChat())
            return;

        Data::DlgState state = Logic::getRecentsModel()->getDlgState(_item->getAimId());
        using im = Logic::IconsManager;
        Ui::ViewParams viewParams;

        if (needDrawUnreads(state))
        {
            maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            maxWidth -= im::getUnreadBalloonWidth(state.UnreadCount_);
        }

        if (needDrawAttention(state))
        {
            maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            maxWidth -= im::getAttentionSize().width();
        }

        if (needDrawMentions(viewParams, state))
        {
            maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            maxWidth -= im::getMentionSize().width();
        }

        if (needDrawDraft(viewParams, state))
        {
            maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            maxWidth -= (im::getDraftSize().width() + Ui::GetContactListParams().official_hor_padding());
        }
    }

    void SearchItemDelegate::fillMessageItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(ci.searchResult_);
        const auto& params = Ui::GetRecentsParams();
        const auto leftOffset = params.getContactNameX();

        ci.addTime(_option);
        ci.addMessageName(replaceFavorites_, _maxTextWidth);
        ci.addMessageText(getSenderName(msg->message_, msg->dialogSearchResult_), _maxTextWidth);
    }

    void SearchItemDelegate::reelideMessage(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto& params = Ui::GetRecentsParams();
        if (ci.name_)
        {
            const auto leftOffset = params.getContactNameX();
            const auto maxWidth = ci.time_ ? ci.time_->horOffset() - leftOffset : _maxTextWidth;
            ci.fixNameWidth(getNickOffset(), getMsgNameVerOffset(), leftOffset, maxWidth);
        }

        if (ci.text_)
            ci.text_->getHeight(_maxTextWidth);

        if (ci.time_)
            ci.time_->setOffsets(_option.rect.width() - ci.time_->desiredWidth() - params.itemHorPadding(), params.timeY());
    }

    std::pair<bool, bool> SearchItemDelegate::getMouseState(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
            return { false , false };

        auto isSelected = false;
        if (selectedMessage_ != -1)
            isSelected = selectedMessage_ == item->getMessageId();
        else
            isSelected = !item->isMessage() && item->getAimId() == Logic::getContactListModel()->selectedContact();

        const auto isHovered = (_option.state & QStyle::State_Selected) && !isSelected && !item->isService();

        return { isSelected, isHovered };
    }

    void SearchItemDelegate::drawAvatar(QPainter& _painter, const Ui::ContactListParams& _params, const QString& _aimid, const int _itemHeight, const bool _isSelected, const bool _isMessage) const
    {
        const auto avatarSize = _params.getAvatarSize();

        bool isDefault = false;
        const auto avatar = Logic::getAvatarStorageProxy(avatarProxyFlags()).GetRounded(
            _aimid,
            QString(),
            Utils::scale_bitmap(avatarSize),
            isDefault,
            false,
            false
        );

        if (!avatar.isNull())
        {
            const auto ratio = Utils::scale_bitmap_ratio();
            const auto x = _params.getAvatarX();
            const auto y = (_itemHeight - avatar.height() / ratio) / 2;

            _painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

            if (Favorites::isFavorites(_aimid))
            {
                Utils::drawAvatarWithoutBadge(_painter, QPoint(x, y), avatar);
            }
            else
            {
                Utils::StatusBadgeFlags statusFlags { Utils::StatusBadgeFlag::Small };
                statusFlags.setFlag(Utils::StatusBadgeFlag::Selected, _isSelected);
                statusFlags.setFlag(Utils::StatusBadgeFlag::SmallOnline, !_isMessage);
                Utils::drawAvatarWithBadge(_painter, QPoint(x, y), avatar, _aimid, Utils::StatusBadgeState::CanBeOff, statusFlags);
            }
        }
    }

    QString SearchItemDelegate::getSenderName(const Data::MessageBuddySptr& _msg, const bool _fromDialogSearch) const
    {
        if (!_fromDialogSearch && _msg->Chat_)
        {
            if (const QString sender = _msg->GetChatSender(); !sender.isEmpty() && sender != _msg->AimId_)
            {
                if (sender != Ui::MyInfo()->aimId())
                    return Logic::getFriendlyContainerProxy(friendlyProxyFlags()).getFriendly(sender) % u": ";
            }
        }

        return QString();
    }

    QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        const auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
        {
            im_assert(false);
            return QSize();
        }

        int height = 0;
        if (item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(item);
            const auto [isSelected, _] = getMouseState(_option, _index);

            const auto ci = getCachedItem(_option, msg, isSelected);
            if (ci && ci->text_ && ci->text_->getLinesCount() == 3)
                height = Utils::scale_value(88);
            else
                height = Ui::GetRecentsParams().itemHeight();
        }
        else if (item->isService())
        {
            if (auto serviceItem = std::static_pointer_cast<Data::SearchResultService>(item))
                height = Ui::GetContactListParams().serviceItemHeight();
        }
        else if (item->isContact() || item->isChat())
        {
            height = Ui::GetContactListParams().itemHeight();
        }
        else if (item->isSuggest())
        {
            height = Utils::scale_value(48);
        }

        im_assert(height > 0);
        return QSize(_option.rect.width(), height);
    }

    bool SearchItemDelegate::setHoveredSuggetRemove(const QModelIndex& _itemIndex)
    {
        if (hoveredRemoveSuggestBtn_ == _itemIndex)
            return false;

        hoveredRemoveSuggestBtn_ = _itemIndex;
        return true;
    }

    bool SearchItemDelegate::needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
            return false;
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(_aimId);
        if (state.hasDraft() && isInDraftIconRect(_posCursor) &&
            (item->getType() == Data::SearchResultType::contact || item->getType() == Data::SearchResultType::chat))
            return true;
        if (item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(item);
            const auto it = messageCache_.find(msg->getMessageId());
            return it != messageCache_.end() && it->second && it->second->name_->isElided();
        }
        const auto it = contactCache_.find(_aimId);
        return it != contactCache_.end() && it->second && it->second->name_->isElided();
    }

    void SearchItemDelegate::dlgStateChanged(const Data::DlgState& _dlgState)
    {
        if (model_)
            model_->onDataChanged(_dlgState.AimId_);
    }

    void SearchItemDelegate::setRegim(int)
    {
    }

    void SearchItemDelegate::setFixedWidth(int)
    {
    }

    void SearchItemDelegate::blockState(bool)
    {
    }

    void SearchItemDelegate::setDragIndex(const QModelIndex& _index)
    {
        dragIndex_ = _index;
    }

    bool SearchItemDelegate::CachedItem::needUpdate(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, bool _isSelected)
    {
        if (!_item->isMessage() && !_item->isContact() && !_item->isChat())
            return false;

        const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(_item);
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(_item->getAimId());
        const auto cachedMsg = std::static_pointer_cast<Data::SearchResultMessage>(searchResult_);

        bool needUpdate = isSelected_ != _isSelected || searchResult_->highlights_ != _item->highlights_;
        if (_item->isMessage())
        {
            needUpdate |= searchResult_->isLocalResult_ != msg->isLocalResult_ ||
                          cachedMsg->message_->GetUpdatePatchVersion() < msg->message_->GetUpdatePatchVersion() ||
                          searchResult_->getFriendlyName() != msg->getFriendlyName() ||
                          searchResult_->highlights_ != msg->highlights_;
        }
        else
        {
            needUpdate |= cachedWidth_ != _option.rect.width() ||
                          searchResult_->getFriendlyName() != _item->getFriendlyName() ||
                          hasDraft_ != state.hasDraft() ||
                          hasUnread_ != (state.UnreadCount_ > 0) ||
                          hasAttention_ != ((state.UnreadCount_ == 0) && state.Attention_) ||
                          hasMentions_ != (state.unreadMentionsCount_ > 0);
        }
        return needUpdate;
    }

    bool SearchItemDelegate::CachedItem::needUpdateWidth(const QStyleOptionViewItem& _option)
    {
        return cachedWidth_ != _option.rect.width();
    }

    void SearchItemDelegate::CachedItem::update(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, bool _isSelected)
    {
        if (!_item->isMessage() && !_item->isContact() && !_item->isChat())
            return;

        Data::DlgState state = Logic::getRecentsModel()->getDlgState(_item->getAimId());
        cachedWidth_ = _option.rect.width();
        isSelected_ = _isSelected;

        if (_item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(_item);
            searchResult_ = msg;
            isOfficial_ = Logic::getContactListModel()->isOfficial(msg->getSenderAimId());
        }
        else if (_item->isContact() || _item->isChat())
        {
            searchResult_ = _item;
            isOfficial_ = Logic::getContactListModel()->isOfficial(_item->getAimId());
            hasDraft_ = state.hasDraft();
            hasUnread_ = (state.UnreadCount_ > 0);
            hasAttention_ = ((state.UnreadCount_ == 0) && state.Attention_);
            hasMentions_ = (state.unreadMentionsCount_ > 0);
        }
    }

    void SearchItemDelegate::CachedItem::addTime(const QStyleOptionViewItem& _option)
    {
        const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(searchResult_);
        const auto timeText = Ui::FormatTime(QDateTime::fromSecsSinceEpoch(msg->message_->GetTime()));
        if (timeText.isEmpty())
            return;
        time_ = MakeTextUnit(timeText, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        const auto& params = Ui::GetRecentsParams();
        time_->init({ params.timeFont(), params.timeFontColor(isSelected_) });
        time_->evaluateDesiredSize();
        time_->setOffsets(_option.rect.width() - time_->desiredWidth() - params.itemHorPadding(), params.timeY());
    }

    void SearchItemDelegate::CachedItem::addMessageText(const QString& _senderName, const int _maxTextWidth)
    {
        const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(searchResult_);
        const auto& params = Ui::GetRecentsParams();
        const auto msgFont = Fonts::appFont(Utils::scale_value(params.messageFontSize()), Fonts::FontWeight::Normal, Fonts::FontAdjust::NoAdjust);
        const auto hlPos = msg->isLocalResult_ ? Ui::HighlightPosition::anywhere : Ui::HighlightPosition::wordStart;

        auto messageText = Ui::createHightlightedText(
                getMessageText(msg->message_, msg->highlights_, hlPos),
                msgFont,
                getMessageTextColor(isSelected_),
                getMessageTextHighlightedColor(isSelected_),
                maxMsgLinesCount,
                msg->highlights_,
                msg->message_->Mentions_,
                hlPos);

        if (!messageText)
            return;

        if (!_senderName.isEmpty())
        {
            auto senderUnit = MakeTextUnit(_senderName, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
            Ui::TextRendering::TextUnit::InitializeParameters params{ msgFont, getNameTextColor(isSelected_) };
            params.highlightColor_ = Ui::getHighlightColor();
            params.maxLinesCount_ = maxMsgLinesCount;
            senderUnit->init(params);
            senderUnit->append(std::move(messageText));
            messageText = std::move(senderUnit);
        }

        text_ = std::move(messageText);
        text_->getHeight(_maxTextWidth);
        const auto leftOffset = params.getContactNameX();
        text_->setOffsets(leftOffset, params.messageY());
    }

    void SearchItemDelegate::CachedItem::addMessageName(bool _replaceFavorites, const int _maxTextWidth)
    {
        const auto leftOffset = Ui::GetRecentsParams().getContactNameX();
        const auto maxWidth = time_ ? time_->horOffset() - leftOffset : _maxTextWidth;
        const auto isFavorites = Favorites::isFavorites(searchResult_->getAimId()) && _replaceFavorites;
        const auto nameText = isFavorites ? Favorites::name() : searchResult_->getFriendlyName();
        name_ = createName(nameText, getNameTextColor(isSelected_, isFavorites), maxWidth);
        name_->setOffsets(leftOffset, getMsgNameVerOffset());
    }

    void SearchItemDelegate::CachedItem::addName(bool _replaceFavorites, const int _maxTextWidth)
    {
        const auto isFavorites = Favorites::isFavorites(searchResult_->getAimId()) && _replaceFavorites;
        const auto nameText = isFavorites ? Favorites::name() : searchResult_->getFriendlyName();
        name_ = createName(nameText, getNameTextColor(isSelected_, isFavorites), _maxTextWidth, searchResult_->highlights_);
    }

    bool SearchItemDelegate::CachedItem::addText(const int _maxTextWidth, const int _leftOffset)
    {
        if (!searchResult_->isChat() || searchResult_->isLocalResult_)
            return false;

        const auto chatItem = std::static_pointer_cast<Data::SearchResultChat>(searchResult_);

        const auto& chatInfo = chatItem->chatInfo_;
        if (chatInfo.MembersCount_ <= 0)
            return false;

        const QString text = Utils::getMembersString(chatInfo.MembersCount_, chatInfo.isChannel());
        text_ = MakeTextUnit(text, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
        Ui::TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(13, Fonts::FontWeight::Normal), getMessageTextColor(isSelected_) };
        params.maxLinesCount_ = 1;
        text_->init(params);
        text_->evaluateDesiredSize();
        text_->elide(_maxTextWidth);
        text_->setOffsets(_leftOffset, Utils::scale_value(24));

        return true;
    }

    void SearchItemDelegate::CachedItem::setOffsets(const int _nickOffset, const int _nameVerOffset, const int _leftOffset, const int _textWidth)
    {
        if (nick_)
            nick_->setOffsets(_leftOffset + _textWidth + _nickOffset, _nameVerOffset);
        if (name_)
            name_->setOffsets(_leftOffset, _nameVerOffset);
    }

    int SearchItemDelegate::CachedItem::fixNameWidth(const int _nickOffset, const int _nameVerOffset, const int _leftOffset, const int _maxTextWidth)
    {
        const auto getWidth = [](Ui::TextRendering::TextUnitPtr& _unit, int _offset = 0){
            if (!_unit)
                return 0;
            if (_unit->isElided())
                _unit->evaluateDesiredSize();
            return _unit->getLastLineWidth() + _offset;
        };

        int textWidth = getWidth(name_);
        const int nickWidth = getWidth(nick_, _nickOffset);

        if (textWidth + nickWidth > _maxTextWidth)
        {
            name_->getHeight(_maxTextWidth - std::min(nickWidth, _maxTextWidth / 2));
            textWidth = name_->getLastLineWidth();

            if (nickWidth > _maxTextWidth / 2)
                nick_->getHeight(_maxTextWidth - textWidth - _nickOffset);
        }
        return textWidth;
    }

    int SearchItemDelegate::CachedItem::addNick(bool _replaceFavorites, const int _maxTextWidth, int _nickOffset, int _nameVerOffset, int _leftOffset)
    {
        if (searchResult_->isChat() || !searchResult_->isLocalResult_)
            return 0;
        const auto nick = searchResult_->getNick();
        const auto skipNick = _replaceFavorites && Favorites::isFavorites(searchResult_->getAimId());

        if (skipNick || nick.isEmpty() || Ui::findNextHighlight(nick, searchResult_->highlights_).indexStart_ == -1)
            return 0;

        nick_ = createNick(nick, isSelected_, _maxTextWidth, searchResult_->highlights_);

        return fixNameWidth(_nickOffset, _nameVerOffset, _leftOffset, _maxTextWidth);
    }

    void SearchItemDelegate::CachedItem::draw(QPainter& _painter)
    {
        if (text_ && name_)
        {
            name_->draw(_painter, VerPosition::TOP);
            text_->draw(_painter, VerPosition::TOP);
        }
        else if (name_)
        {
            name_->draw(_painter, VerPosition::MIDDLE);

            if (nick_)
                nick_->draw(_painter, VerPosition::MIDDLE);
        }

        if (time_)
            time_->draw(_painter, VerPosition::MIDDLE);
    }
}

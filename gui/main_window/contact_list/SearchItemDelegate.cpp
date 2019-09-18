#include "stdafx.h"

#include "Common.h"
#include "ContactListModel.h"
#include "SearchModel.h"
#include "SearchItemDelegate.h"
#include "SearchHighlight.h"

#include "../friendly/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../app_config.h"
#include "../../my_info.h"

#include "styles/ThemeParameters.h"

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
        // COLOR
        static const auto del = Utils::renderSvg(qsl(":/controls/close_icon"), getRemoveSuggestIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        static const auto delHovered = Utils::renderSvg(qsl(":/controls/close_icon"), getRemoveSuggestIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));

        return _isHovered ? delHovered : del;
    }

    QPixmap getAddContactIcon(const bool _isSelected)
    {
        // COLOR
        static const QPixmap add(Utils::renderSvgScaled(qsl(":/controls/add_icon"), addIconSize, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        static const QPixmap addSelected(Utils::renderSvgScaled(qsl(":/controls/add_icon"), addIconSize, Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)));

        return _isSelected ? addSelected : add;
    }

    QColor getTextColorSelectedState()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor getNameTextColor(const bool _isSelected)
    {
        return _isSelected ? getTextColorSelectedState() : Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor getNickTextColor(const bool _isSelected)
    {
        return _isSelected ? getTextColorSelectedState() : Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor getNameTextHighlightedColor()
    {
        return Ui::getTextHighlightedColor();
    }

    QColor getMessageTextColor(const bool _isSelected)
    {
        return _isSelected ? getTextColorSelectedState() : Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor getMessageTextHighlightedColor(const bool _isSelected)
    {
        return getNameTextHighlightedColor();
    }

    void drawAvatar(QPainter& _painter, const Ui::ContactListParams& _params, const QString& _aimid, const int _itemHeight, const bool _isSelected)
    {
        const auto avatarSize = _params.getAvatarSize();

        bool isDefault = false;
        const auto avatar = *Logic::GetAvatarStorage()->GetRounded(
            _aimid,
            QString(),
            Utils::scale_bitmap(avatarSize),
            QString(),
            isDefault,
            false,
            false
        );

        if (!avatar.isNull())
        {
            const auto ratio = Utils::scale_bitmap_ratio();
            const auto x = _params.getAvatarX();
            const auto y = (_itemHeight - avatar.height() / ratio) / 2;

            Utils::drawAvatarWithBadge(_painter, QPoint(x, y), avatar, _aimid, false, _isSelected);
        }
    }

    Ui::TextRendering::TextUnitPtr createName(const QString& _text, const bool _isSelected, const int _maxWidth, const Ui::highlightsV& _highlights = {})
    {
        if (_text.isEmpty())
            return Ui::TextRendering::TextUnitPtr();

        static const auto font = Ui::GetRecentsParams().getContactNameFont(Fonts::FontWeight::Normal);

        Ui::TextRendering::TextUnitPtr unit = Ui::createHightlightedText(
            _text,
            font,
            getNameTextColor(_isSelected),
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
                parts.push_back(q.text_);
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

        if (!_msg->IsSticker() && !_msg->GetText().isEmpty())
            parts.push_back(_msg->GetText());

        for (const auto& part: parts)
        {
            if (const auto hl = findNextHighlight(part, _highlights, 0, _hlPos); hl.indexStart_ != -1)
            {
                const auto cropCount = hl.indexStart_ >= largeMessageThreshold ? leftCropCount : largeMessageThreshold;
                const auto startPos = hl.indexStart_ - cropCount;
                const auto len = hl.length_ + cropCount + largeMessageThreshold;

                const auto res = part.midRef(std::max(startPos, 0), len);

                if (hl.indexStart_ >= largeMessageThreshold)
                    return ql1s("...") % res;

                return res.toString();
            }
        }

        return _msg->GetText();
    }

    QString getSenderName(const Data::MessageBuddySptr& _msg, const bool _fromDialogSearch)
    {
        if (!_fromDialogSearch && _msg->Chat_)
        {
            if (const QString sender = _msg->GetChatSender(); !sender.isEmpty() && sender != _msg->AimId_)
            {
                if (sender != Ui::MyInfo()->aimId())
                    return Logic::GetFriendlyContainer()->getFriendly(sender) % qsl(": ");
            }
        }

        return QString();
    }
}

namespace Logic
{
    SearchItemDelegate::SearchItemDelegate(QObject* _parent)
        : AbstractItemDelegateWithRegim(_parent)
        , selectedMessage_(-1)
        , lastDrawTime_(QTime::currentTime())
    {
        cacheTimer_.setInterval(dropCacheTimeout.count());
        cacheTimer_.setTimerType(Qt::VeryCoarseTimer);
        connect(&cacheTimer_, &QTimer::timeout, this, [this]()
        {
            if (lastDrawTime_.msecsTo(QTime::currentTime()) >= cacheTimer_.interval())
                dropCache();
        });
        cacheTimer_.start();
    }

    void SearchItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Utils::PainterSaver saver(*_painter);

        auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
            return;

        const auto [isSelected, isHovered] = getMouseState(_option, _index);

        Ui::RenderMouseState(*_painter, isHovered, isSelected, _option.rect);

        _painter->translate(_option.rect.topLeft());

        switch (item->getType())
        {
        case Data::SearchResultType::service:
            drawServiceItem(*_painter, _option, std::static_pointer_cast<Data::SearchResultService>(item));
            break;

        case Data::SearchResultType::contact:
        case Data::SearchResultType::chat:
            drawContactOrChatItem(*_painter, _option, item, isSelected);
            break;

        case Data::SearchResultType::message:
            drawMessageItem(*_painter, _option, std::static_pointer_cast<Data::SearchResultMessage>(item), isSelected);
            break;

        case Data::SearchResultType::suggest:
            drawSuggest(*_painter, _option, std::static_pointer_cast<Data::SearchResultSuggest>(item), _index, isHovered);
            break;

        default:
            assert(false);
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
                        ? (item->isContact() || item->isChat()) ? qsl("oc:") % QString::number(Logic::getContactListModel()->getOutgoingCount(item->getAimId())) : QString()
                        : qsl("!CL"),
                    item->isLocalResult_ ? qsl("local") : qsl("server"),
                    item->getAimId(),
                    item->isMessage() ? QString::number(item->getMessageId()) : QString()
                };
                Utils::drawText(*_painter, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, debugInfo.join(ql1c(' ')));
            }
        }
    }

    void SearchItemDelegate::onContactSelected(const QString& _aimId, qint64 _msgId, qint64)
    {
        selectedMessage_ = _msgId;
    }

    void SearchItemDelegate::clearSelection()
    {
        selectedMessage_ = -1;
    }

    void SearchItemDelegate::dropCache()
    {
        contactCache.clear();
        messageCache.clear();
    }

    void SearchItemDelegate::drawContactOrChatItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const
    {
        const auto& params = Ui::GetContactListParams();

        drawAvatar(_painter, params, _item->getAimId(), _option.rect.height(), _isSelected);
        drawCachedItem(_painter, _option, _item, _isSelected);
    }

    void SearchItemDelegate::drawMessageItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::MessageSearchResultSptr& _item, const bool _isSelected) const
    {
        drawAvatar(_painter, Ui::GetRecentsParams(), _item->getSenderAimId(), _option.rect.height(), _isSelected);
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
            static const auto icon = Utils::renderSvgScaled(qsl(":/search_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            const auto iconX = getHorMargin();
            const auto iconY = (_option.rect.height() - icon.height() / ratio) / 2;
            _painter.drawPixmap(iconX, iconY, icon.width() / ratio, icon.height() / ratio, icon);
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

        static Ui::TextRendering::TextUnitPtr text;
        if (!text)
        {
            // FONT
            text = MakeTextUnit(QString(), {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
            text->init(Fonts::appFontScaled(16),
                        Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

            text->setOffsets(textLeftOffset, _option.rect.height() / 2 - Utils::scale_value(2));
        }

        text->setText(_item->getFriendlyName());
        text->elide(maxTextWidth);
        text->evaluateDesiredSize();
        text->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);
    }

    SearchItemDelegate::CachedItem* SearchItemDelegate::getCachedItem(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const
    {
        if (_item->getFriendlyName().isEmpty())
            return nullptr;

        const auto& params = _item->isMessage() ? Ui::GetRecentsParams() : Ui::GetContactListParams();
        auto maxTextWidth = _option.rect.width() - params.getContactNameX() - params.itemHorPadding();

        if (_item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(_item);

            auto& ci = messageCache[msg->getMessageId()];
            bool needUpdateMessage = false;
            bool needUpdateWidth = false;

            if (ci)
            {
                const auto cachedMsg = std::static_pointer_cast<Data::SearchResultMessage>(ci->searchResult_);

                needUpdateMessage =
                    ci->isSelected_ != _isSelected ||
                    ci->searchResult_->isLocalResult_ != msg->isLocalResult_ ||
                    cachedMsg->message_->GetUpdatePatchVersion() < msg->message_->GetUpdatePatchVersion() ||
                    ci->searchResult_->getFriendlyName() != msg->getFriendlyName() ||
                    ci->searchResult_->highlights_ != msg->highlights_;

                needUpdateWidth = ci->cachedWidth_ != _option.rect.width();
            }

            if (!ci || needUpdateMessage)
            {
                ci = std::make_unique<CachedItem>();
                ci->searchResult_ = msg;
                ci->cachedWidth_ = _option.rect.width();
                ci->isSelected_ = _isSelected;
                ci->isOfficial_ = Logic::getContactListModel()->isOfficial(msg->getSenderAimId());
                fillMessageItem(*ci, _option, maxTextWidth);
            }
            else if (ci && needUpdateWidth)
            {
                reelideMessage(*ci, _option, maxTextWidth);
            }

            return ci.get();
        }
        else if (_item->isContact() || _item->isChat())
        {
            auto& ci = contactCache[_item->getAimId()];
            bool needUpdate = false;

            if (ci)
            {
                needUpdate =
                    ci->isSelected_ != _isSelected ||
                    ci->cachedWidth_ != _option.rect.width() ||
                    ci->searchResult_->getFriendlyName() != _item->getFriendlyName() ||
                    ci->searchResult_->highlights_ != _item->highlights_;
            }

            if (!ci || needUpdate)
            {
                ci = std::make_unique<CachedItem>();
                ci->searchResult_ = _item;
                ci->cachedWidth_ = _option.rect.width();
                ci->isSelected_ = _isSelected;
                ci->isOfficial_ = Logic::getContactListModel()->isOfficial(_item->getAimId());
                fillContactItem(*ci, _option, maxTextWidth);
            }

            return ci.get();
        }

        return nullptr;
    }

    SearchItemDelegate::CachedItem* SearchItemDelegate::drawCachedItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const
    {
        if (const auto ci = getCachedItem(_option, _item, _isSelected))
        {
            ci->draw(_painter);
            lastDrawTime_ = QTime::currentTime();
            return ci;
        }

        return nullptr;
    }

    void SearchItemDelegate::fillContactItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto& params = Ui::GetContactListParams();

        ci.name_ = createName(ci.searchResult_->getFriendlyName(), ci.isSelected_, _maxTextWidth, ci.searchResult_->highlights_);

        const auto leftOffset = params.getContactNameX();

        auto nameVerOffset = _option.rect.height() / 2;

        if (ci.searchResult_->isChat())
        {
            if (!ci.searchResult_->isLocalResult_)
            {
                const auto& chatItem = std::static_pointer_cast<Data::SearchResultChat>(ci.searchResult_);
                if (chatItem->chatInfo_.MembersCount_ > 0)
                {
                    const QString text = QString::number(chatItem->chatInfo_.MembersCount_) % ql1c(' ') %
                        Utils::GetTranslator()->getNumberString(
                            chatItem->chatInfo_.MembersCount_,
                            QT_TRANSLATE_NOOP3("groupchats", "member", "1"),
                            QT_TRANSLATE_NOOP3("groupchats", "members", "2"),
                            QT_TRANSLATE_NOOP3("groupchats", "members", "5"),
                            QT_TRANSLATE_NOOP3("groupchats", "members", "21")
                        );

                    ci.text_ = MakeTextUnit(text, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
                    ci.text_->init(Fonts::appFontScaled(13, Fonts::FontWeight::Normal), getMessageTextColor(ci.isSelected_), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::LEFT, 1);
                    ci.text_->evaluateDesiredSize();
                    ci.text_->elide(_maxTextWidth);
                    ci.text_->setOffsets(leftOffset, Utils::scale_value(24));

                    nameVerOffset = getContactNameVerOffset();
                }
            }
        }
        else if (const auto nick = ci.searchResult_->getNick(); !nick.isEmpty() && Ui::findNextHighlight(nick, ci.searchResult_->highlights_).indexStart_ != -1)
        {
            ci.nick_ = createNick(nick, ci.isSelected_, _maxTextWidth, ci.searchResult_->highlights_);

            if (ci.name_->isElided())
                ci.name_->evaluateDesiredSize();
            int textWidth = ci.name_->getLastLineWidth();

            if (ci.nick_->isElided())
                ci.nick_->evaluateDesiredSize();
            int nickWidth = ci.nick_->getLastLineWidth() + getNickOffset();

            if (textWidth + nickWidth > _maxTextWidth)
            {
                if (nickWidth <= _maxTextWidth / 2)
                {
                    ci.name_->getHeight(_maxTextWidth - nickWidth);
                    textWidth = ci.name_->getLastLineWidth();
                }
                else
                {
                    ci.name_->getHeight(_maxTextWidth / 2);
                    textWidth = ci.name_->getLastLineWidth();

                    ci.nick_->getHeight(_maxTextWidth - textWidth - getNickOffset());
                }
            }
            ci.nick_->setOffsets(leftOffset + textWidth + getNickOffset(), nameVerOffset);
        }

        if (ci.name_)
            ci.name_->setOffsets(leftOffset, nameVerOffset);
    }

    void SearchItemDelegate::fillMessageItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(ci.searchResult_);
        const auto& params = Ui::GetRecentsParams();
        const auto leftOffset = params.getContactNameX();

        auto timeText = Ui::FormatTime(QDateTime::fromTime_t(msg->message_->GetTime()));
        if (!timeText.isEmpty())
        {
            ci.time_ = MakeTextUnit(timeText, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
            ci.time_->init(params.timeFont(), params.timeFontColor(ci.isSelected_));
            ci.time_->evaluateDesiredSize();
            ci.time_->setOffsets(_option.rect.width() - ci.time_->desiredWidth() - params.itemHorPadding(), params.timeY());
        }

        const auto maxWidth = ci.time_ ? ci.time_->horOffset() - leftOffset : _maxTextWidth;
        ci.name_ = createName(ci.searchResult_->getFriendlyName(), ci.isSelected_, maxWidth);
        ci.name_->setOffsets(leftOffset, getMsgNameVerOffset());

        const auto msgFont = Fonts::appFont(params.messageFontSize(), Fonts::FontWeight::Normal);

        const auto hlPos = msg->isLocalResult_ ? Ui::HighlightPosition::anywhere : Ui::HighlightPosition::wordStart;
        auto messageText = Ui::createHightlightedText(
            getMessageText(msg->message_, msg->highlights_, hlPos),
            msgFont,
            getMessageTextColor(ci.isSelected_),
            getMessageTextHighlightedColor(ci.isSelected_),
            maxMsgLinesCount,
            msg->highlights_,
            msg->message_->Mentions_,
            hlPos);

        if (messageText)
        {
            const auto senderText = getSenderName(msg->message_, msg->dialogSearchResult_);
            if (!senderText.isEmpty())
            {
                auto senderUnit = MakeTextUnit(senderText, {}, LinksVisible::DONT_SHOW_LINKS, ProcessLineFeeds::REMOVE_LINE_FEEDS);
                senderUnit->init(msgFont, getNameTextColor(ci.isSelected_), QColor(), QColor(), Ui::getHighlightColor(), HorAligment::LEFT, maxMsgLinesCount);
                senderUnit->append(std::move(messageText));

                messageText = std::move(senderUnit);
            }

            ci.text_ = std::move(messageText);
            ci.text_->getHeight(_maxTextWidth);
            ci.text_->setOffsets(leftOffset, params.messageY());
        }
    }

    void SearchItemDelegate::reelideMessage(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const
    {
        const auto& params = Ui::GetRecentsParams();
        if (ci.name_)
        {
            const auto leftOffset = params.getContactNameX();
            const auto maxWidth = ci.time_ ? ci.time_->horOffset() - leftOffset : _maxTextWidth;
            ci.name_->elide(maxWidth);
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
            isSelected = !item->isLocalResult_ && !item->isMessage() && item->getAimId() == Logic::getContactListModel()->selectedContact();

        const auto isHovered = (_option.state & QStyle::State_Selected) && !isSelected && !item->isService();

        return { isSelected, isHovered };
    }

    QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        const auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        if (!item)
        {
            assert(false);
            return QSize();
        }

        int height = 0;
        if (item->isMessage())
        {
            const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(item);
            const auto [isSelected, _] = getMouseState(_option, _index);

            const auto ci = getCachedItem(_option, msg, isSelected);
            if (ci && ci->text_ && ci->text_->getLineCount() == 3)
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

        assert(height > 0);
        return QSize(_option.rect.width(), height);
    }

    bool SearchItemDelegate::setHoveredSuggetRemove(const QModelIndex& _itemIndex)
    {
        if (hoveredRemoveSuggestBtn_ == _itemIndex)
            return false;

        hoveredRemoveSuggestBtn_ = _itemIndex;
        return true;
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

    void SearchItemDelegate::setDragIndex(const QModelIndex&)
    {
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

#include "stdafx.h"

#include "../../../app_config.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../controls/ContextMenu.h"
#include "../../../controls/TooltipWidget.h"
#include "../../../controls/CustomButton.h"
#include "../../../gui_settings.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/features.h"
#include "../../../utils/log/log.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/stat_utils.h"
#include "../../../my_info.h"
#include "../StickerInfo.h"
#include "../SnippetCache.h"
#include "../FileStatus.h"

#include "../../contact_list/RecentsTab.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/RecentsModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"
#include "../../contact_list/FavoritesUtils.h"
#include "../../containers/FriendlyContainer.h"
#include "../../containers/StatusContainer.h"
#include "../../input_widget/InputWidgetUtils.h"

#include "../../ReportWidget.h"

#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"
#include "../MessagesScrollArea.h"

#include "ComplexMessageItemLayout.h"
#include "IItemBlockLayout.h"
#include "IItemBlock.h"
#include "QuoteBlock.h"

#include "TextBlock.h"
#include "DebugTextBlock.h"
#include "SnippetBlock.h"

#include "ComplexMessageItem.h"
#include "../../mediatype.h"

#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"
#include "spellcheck/Spellchecker.h"

#include "../../../common.shared/config/config.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    QMap<QString, QVariant> makeData(const QString& command, const QString& arg = QString())
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;

        if (!arg.isEmpty())
            result[qsl("arg")] = arg;

        return result;
    }

    QColor getBaseButtonTextColor(const QString& _contact, bool _isOutgoing)
    {
        QColor result;
        const auto& contactWpId = Styling::getThemesContainer().getContactWallpaperId(_contact);
        const auto& themeDefWpId = Styling::getThemesContainer().getCurrentTheme()->getDefaultWallpaperId();

        if (_isOutgoing)
        {
            result = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE);
        }
        else
        {
            if (contactWpId == themeDefWpId)
            {
                result = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
            }
            else
            {
                result = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE);
            }
        }

        return result;
    }

    std::pair<QColor, QPixmap> getButtonColorAndIcon(const Data::ButtonData::style _style, const QString& _contact, bool _isOutgoing)
    {
        static auto out = Utils::renderSvg(qsl(":/button_out"), QSize(Utils::scale_value(8), Utils::scale_value(8)), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        static auto out_attention = Utils::renderSvg(qsl(":/button_out"), Utils::scale_value(QSize(8, 8)), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
        static auto out_primary = Utils::renderSvg(qsl(":/button_out"), Utils::scale_value(QSize(8, 8)), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));

        QColor color;
        QPixmap icon;

        switch (_style)
        {
        case Data::ButtonData::style::ATTENTION:
            color = Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            icon = out_attention;
            break;

        case Data::ButtonData::style::PRIMARY:
            color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
            icon = out_primary;
            break;

        case Data::ButtonData::style::BASE:
        default:
            color = getBaseButtonTextColor(_contact, _isOutgoing);
            icon = out;
            break;
        }

        return std::make_pair(std::move(color), std::move(icon));
    }

    void drawAnimation(QPainter& _p, double _current, const QRect& _rect)
    {
        const auto QT_ANGLE_MULT = 16;
        const auto baseAngle = _current * QT_ANGLE_MULT;
        const auto progress = 0.75;
        const auto progressAngle = (int)std::ceil(progress * 360 * QT_ANGLE_MULT);

        Utils::PainterSaver ps(_p);
        _p.setPen(MessageStyle::getRotatingProgressBarPen());
        _p.drawArc(_rect, -baseAngle, -progressAngle);
    }

    QColor getButtonBackgroundColor(const QString& _contact, bool _isOutgoing, bool _hover, bool _active)
    {
        QColor result;
        const auto& contactWpId = Styling::getThemesContainer().getContactWallpaperId(_contact);
        const auto& themeDefWpId = Styling::getThemesContainer().getCurrentTheme()->getDefaultWallpaperId();

        if (_isOutgoing)
        {
            result = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_SECONDARY);
            result.setAlpha(contactWpId == themeDefWpId ? 0.7 * 255 : 0.9 * 255);
        }
        else
        {
            auto var = Styling::StyleVariable::CHAT_PRIMARY;
            if (_active)
                var = Styling::StyleVariable::CHAT_PRIMARY_ACTIVE;
            else if (_hover)
                var = Styling::StyleVariable::CHAT_PRIMARY_HOVER;

            result = Styling::getParameters().getColor(var);
            result.setAlpha(contactWpId == themeDefWpId ? 0.7 * 255 : 0.9 * 255);
        }

        return result;
    }

    bool isInternalUrl(const QString& _url)
    {
        if (_url.isEmpty())
            return true;

        if (Utils::isInternalScheme(QUrl(_url).scheme()))
            return true;

        Utils::UrlParser parser;
        parser.process(_url);
        return parser.hasUrl() && parser.getUrl().type_ == common::tools::url::type::profile;
    }

    void handleBotAction(const QString& _url, const QString& _text = {}, bool _showAlert = false)
    {
        if (!_url.isEmpty())
        {
            Utils::UrlParser parser;
            parser.process(_url);
            if (parser.hasUrl())
            {
                if (!isInternalUrl(_url) && !Utils::isUrlVCS(_url))
                {
                    if (!_text.isEmpty())
                    {
                        if (Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("bots", "Cancel"), QT_TRANSLATE_NOOP("bots", "Open link"),
                            _text,
                            QString(), nullptr))
                        {
                            Utils::openUrl(_url);
                        }
                    }
                    else
                    {
                        Utils::openUrl(_url, Utils::OpenUrlConfirm::Yes);
                    }
                }
                else
                {
                    if (_showAlert && !_text.isEmpty())
                        Utils::ShowBotAlert(_text);
                    Utils::openUrl(_url);
                }
            }
        }
        else if (!_text.isEmpty())
        {
            if (_showAlert)
                Utils::ShowBotAlert(_text);
            else
                Utils::showTextToastOverContactDialog(_text);
        }
    }

    Data::FString borrowStylesFromWord(Data::FStringView _source, const QString& _target)
    {
        auto result = Data::FString(_target);
        if (auto firstLetter = _source.left(1); firstLetter.hasFormatting())
        {
            core::data::format::builder formats;
            for (const auto& style : firstLetter.getStyles())
                formats %= core::data::range_format(style.type_, {0, result.size()});

            result.setFormatting(formats.finalize());
        }
        return result;
    };

    constexpr auto BUTTON_ANIMATION_START_TIMEOUT = std::chrono::milliseconds(200);

    constexpr auto MAX_BUTTONS_IN_LINE = 8;
    constexpr auto MAX_LINES_OF_BUTTONS = 14;
}

void MessageButton::clearProgress()
{
    seq_ = -1;
    reqId_.clear();
    pressTime_ = QDateTime();
    animating_ = false;
}

void MessageButton::initButton()
{
    originalText_ = data_.text_;
    if (originalText_.count(QChar::LineFeed) > 1)
        originalText_ = originalText_.left(originalText_.indexOf(QChar::LineFeed, originalText_.indexOf(QChar::LineFeed) + 1));

    isTwoLines_ = originalText_.contains(QChar::LineFeed);
    isExternalUrl_ = !isInternalUrl(data_.url_);
}

ComplexMessageItem::ComplexMessageItem(QWidget* _parent, const Data::MessageBuddy& _msg)
    : HistoryControlPageItem(_parent)
    , ChatAimid_(_msg.AimId_)
    , Date_(_msg.GetDate())
    , hoveredBlock_(nullptr)
    , hoveredSharingBlock_(nullptr)
    , Id_(_msg.Id_)
    , PrevId_(_msg.Prev_)
    , internalId_(_msg.InternalId_)
    , IsOutgoing_(_msg.IsOutgoing())
    , deliveredToServer_(_msg.IsDeliveredToServer())
    , Layout_(nullptr)
    , MenuBlock_(nullptr)
    , MouseLeftPressedOverAvatar_(false)
    , MouseLeftPressedOverSender_(false)
    , MouseRightPressedOverItem_(false)
    , desiredSenderWidth_(0)
    , SenderAimid_(_msg.getSender())
    , shareButton_(nullptr)
    , shareButtonAnimation_(nullptr)
    , shareButtonOpacityEffect_(nullptr)
    , SourceText_(_msg.GetSourceText())
    , TimeWidget_(nullptr)
    , timeAnimation_(nullptr)
    , Time_(-1)
    , bQuoteAnimation_(false)
    , bObserveToSize_(false)
    , bubbleHovered_(false)
    , hasTrailingLink_(false)
    , hasLinkInText_(false)
    , hideEdit_(false)
    , mentions_(_msg.Mentions_)
    , buttonsUpdateTimer_(nullptr)
    , buttonsAnimation_(nullptr)
{
    im_assert(Id_ >= -1);
    im_assert(!SenderAimid_.isEmpty());
    im_assert(!ChatAimid_.isEmpty());

    setContact(ChatAimid_); // order is important, must be called as soon as possible
    setBuddy(_msg);

    SenderAimidForDisplay_ = Logic::getContactListModel()->isChannel(ChatAimid_) ? ChatAimid_ : _msg.getSender();
    SenderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(_msg.Chat_ ? SenderAimid_ : (_msg.IsOutgoing() ? Ui::MyInfo()->aimId() : _msg.AimId_));

    buttonLabel_ = TextRendering::MakeTextUnit(QString());
    initButtonsLabel();

    if (!isHeadless())
        im_assert(Date_.isValid());

    Layout_ = new ComplexMessageItemLayout(this);
    setLayout(Layout_);

    connectSignals();
}

ComplexMessageItem::~ComplexMessageItem()
{
    if (shareButtonAnimation_)
        shareButtonAnimation_->stop();

    if (timeAnimation_)
        timeAnimation_->stop();

    if (!isHeadless())
        Logic::GetStatusContainer()->setAvatarVisible(SenderAimid_, false);
}

void ComplexMessageItem::clearBlockSelection()
{
    im_assert(!Blocks_.empty());

    for (auto block : Blocks_)
        block->clearSelection();

    Layout_->onBlockSizeChanged();
    update();
}

void ComplexMessageItem::clearSelection(bool _keepSingleSelection)
{
    if (!_keepSingleSelection)
        clearBlockSelection();

    HistoryControlPageItem::clearSelection(_keepSingleSelection);
}

QString ComplexMessageItem::formatRecentsText() const
{
    im_assert(!Blocks_.empty());

    if (Blocks_.empty())
        return qsl("warning: invalid message block");

    QString textOnly;
    textOnly.reserve(1024);

    QString tmp;

    unsigned textBlocks = 0, fileSharingBlocks = 0, linkBlocks = 0, quoteBlocks = 0, pollBlocks = 0, taskBlocks = 0, otherBlocks = 0;

    for (const auto b : Blocks_)
    {
        switch (b->getContentType())
        {
        case IItemBlock::ContentType::Text:
            if (!b->getTrimmedText().isEmpty())
            {
                const auto t = b->formatRecentsText();
                if (!textOnly.isEmpty() && !t.isEmpty())
                    textOnly += QChar::Space;
                textOnly += t;
                ++textBlocks;
            }
            break;

        case IItemBlock::ContentType::Link:
            tmp = b->formatRecentsText();
            if (getBlockCount() - textBlocks - quoteBlocks != 1 || !textOnly.contains(tmp))
            {
                if (linkBlocks)
                    textOnly += QChar::Space;
                textOnly += tmp;
            }
            ++linkBlocks;
            break;

        case IItemBlock::ContentType::FileSharing:
            if (fileSharingBlocks)
                textOnly += QChar::Space;
            textOnly += b->formatRecentsText();
            ++fileSharingBlocks;
            break;

        case IItemBlock::ContentType::Quote:
            ++quoteBlocks;
            break;

        case IItemBlock::ContentType::DebugText:
            continue;
            break;

        case IItemBlock::ContentType::Poll:
            ++pollBlocks;
            break;

        case IItemBlock::ContentType::Task:
            ++taskBlocks;
            textOnly += b->formatRecentsText();
            break;

        case IItemBlock::ContentType::Other:
        default:
            if (otherBlocks)
                textOnly += QChar::Space;
            textOnly += b->formatRecentsText();
            ++otherBlocks;
            break;
        }
    }

    QString res;
    if (quoteBlocks && !(fileSharingBlocks || linkBlocks || otherBlocks || textBlocks))
    {
        for (const auto& _block : Blocks_)
        {
            if (_block->getContentType() == IItemBlock::ContentType::Quote)
            {
                res = _block->formatRecentsText();
                break;
            }
        }
    }
    else if (pollBlocks)
    {
        res = QT_TRANSLATE_NOOP("poll_block", "Poll: ") % textOnly;
    }
    else if (!textOnly.isEmpty() && ((linkBlocks || quoteBlocks) || (fileSharingBlocks && (linkBlocks || otherBlocks || textBlocks || quoteBlocks))) || taskBlocks)
    {
        res = textOnly;
    }
    else
    {
        for (const auto& _block : Blocks_)
        {
            if ((fileSharingBlocks || otherBlocks) && _block != Blocks_.front())
                res += QChar::Space;
            res += _block->formatRecentsText();
        }
    }

    if (!mentions_.empty())
        return Utils::convertMentions(res, mentions_);

    return res;
}

MediaType ComplexMessageItem::getMediaType(MediaRequestMode _mode) const
{
    if (!Blocks_.empty())
    {
        // contains poll?
        if (findBlock(Blocks_.begin(), Blocks_.end(), uint32_t(IItemBlock::ContentType::Poll)) != nullptr)
            return Ui::MediaType::mediaTypePoll;

        // contains task?
        if (findBlock(Blocks_.begin(), Blocks_.end(), uint32_t(IItemBlock::ContentType::Task)) != nullptr)
            return Ui::MediaType::mediaTypeTask;

        const auto mediaBlocksNumber = std::count_if(Blocks_.begin(), Blocks_.end(), [](const auto& _block) { return _block->isPreviewable(); });

        for (const auto _block : Blocks_)
        {
            const auto contentType = _block->getContentType();

            if (contentType == IItemBlock::ContentType::Quote || contentType == IItemBlock::ContentType::DebugText)
                continue;

            const auto mediaType = _block->getMediaType();

            if (contentType == IItemBlock::ContentType::FileSharing && mediaBlocksNumber == 1)
            {
                if (mediaType == Ui::MediaType::mediaTypePhoto)
                    return Ui::MediaType::mediaTypeFsPhoto;
                else if (mediaType == Ui::MediaType::mediaTypeVideo)
                    return Ui::MediaType::mediaTypeFsVideo;
                else if (mediaType == Ui::MediaType::mediaTypeGif)
                    return Ui::MediaType::mediaTypeFsGif;
                else
                    return mediaType;
            }
            else
            {
                if (mediaBlocksNumber != 1 && _mode == MediaRequestMode::Chat)
                    return Ui::MediaType::noMedia;
                else
                    return mediaType;
            }
        }
    }

    return MediaType::noMedia;
}

void ComplexMessageItem::updateFriendly(const QString& _aimId, const QString& _friendly)
{
    if (!_aimId.isEmpty() && !_friendly.isEmpty())
    {
        bool needUpdate = false;
        if (isChat() && SenderAimidForDisplay_ == _aimId && SenderFriendly_ != _friendly)
        {
            SenderFriendly_ = _friendly;
            setMchatSender(_friendly);
            needUpdate = true;
        }

        if (auto it = mentions_.find(_aimId); it != mentions_.end() && it->second != _friendly)
            it->second = _friendly;

        for (const auto block : Blocks_)
            needUpdate |= block->updateFriendly(_aimId, _friendly);

        if (needUpdate)
            updateSize();
    }
}

PinPlaceholderType ComplexMessageItem::getPinPlaceholder() const
{
    std::vector<PinPlaceholderType> types;
    types.reserve(getBlockCount());
    for (const auto b : Blocks_)
    {
        if (b->getContentType() == IItemBlock::ContentType::Quote)
            return PinPlaceholderType::None;

        if (const auto ph = b->getPinPlaceholder(); ph != PinPlaceholderType::None)
            types.push_back(ph);
    }

    if (types.size() == 1)
        return types.front();

    return PinPlaceholderType::None;
}

void ComplexMessageItem::requestPinPreview(PinPlaceholderType _type)
{
    if (_type == PinPlaceholderType::None || _type == PinPlaceholderType::Filesharing)
        return;

    const auto it = std::find_if(
        Blocks_.cbegin(),
        Blocks_.cend(),
        [_type](auto _block) { return _block->getPinPlaceholder() == _type; }
    );

    if (it != Blocks_.cend())
        (*it)->requestPinPreview();
}

QString ComplexMessageItem::getFirstLink() const
{
    if (auto blk = findBlock(Blocks_.begin(), Blocks_.end(), uint32_t(IItemBlock::ContentType::Sticker) |
                                                             uint32_t(IItemBlock::ContentType::FileSharing)))
        return blk->getSourceText().string();

    return QString();
}

Data::StickerId ComplexMessageItem::getFirstStickerId() const
{
    if (auto blk = findBlock(Blocks_.begin(), Blocks_.end(), uint32_t(IItemBlock::ContentType::Sticker)))
        return blk->getStickerId();
    return {};
}

qint64 ComplexMessageItem::getId() const
{
    return Id_;
}

qint64 ComplexMessageItem::getPrevId() const
{
    return PrevId_;
}

const QString& ComplexMessageItem::getInternalId() const
{
    return internalId_;
}

QString ComplexMessageItem::getQuoteHeader() const
{
    return getSenderFriendly() % u" (" % QDateTime::fromSecsSinceEpoch(Time_).toString(u"dd.MM.yyyy hh:mm") % u"):\n";
}

Data::FString ComplexMessageItem::getSelectedText(const bool _isQuote, TextFormat _format) const
{
    Data::FString text;
    if (!isSelected())
        return text;

    text.reserve(1024);

    if (_isQuote)
        text += getQuoteHeader();

    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
    {
        if (!Blocks_.empty() && Blocks_.front()->getContentType() == IItemBlock::ContentType::DebugText)
        {
            const auto dest = _format == TextFormat::Raw ? IItemBlock::TextDestination::quote : IItemBlock::TextDestination::selection;
            text += Blocks_.front()->getSelectedText(true, dest);
            text += QChar::LineFeed;
        }

        if (getBlockCount() < 2 && !Description_.isEmpty())
        {
            text += Url_ % QChar::LineFeed;
            text += Description_;
        }
        else
        {
            text += getBlocksText(Blocks_, false, _format);
        }

        return text;
    }

    const auto selectedText = getBlocksText(Blocks_, true, _format);
    if (selectedText.isEmpty())
        return text;

    text += selectedText;

    return text;
}

const QString& ComplexMessageItem::getChatAimid() const
{
    return ChatAimid_;
}

QDate ComplexMessageItem::getDate() const
{
    im_assert(Date_.isValid());
    return Date_;
}

const QString& ComplexMessageItem::getSenderAimid() const
{
    im_assert(!SenderAimid_.isEmpty());
    return SenderAimid_;
}

bool ComplexMessageItem::isOutgoing() const
{
    return IsOutgoing_;
}

bool ComplexMessageItem::isEditable() const
{
    if (HistoryControlPageItem::isEditable())
    {
        im_assert(!Blocks_.empty());

        auto textBlocks = 0;
        auto fileBlocks = 0;
        auto mediaBlocks = 0;
        auto linkBlocks = 0;
        auto quoteBlocks = 0;
        auto pollBlocks = 0;
        auto taskBlocks = 0;
        auto profileBlocks = 0;
        auto otherBlocks = 0;
        auto forward = false;

        for (const auto block : Blocks_)
        {
            switch (block->getContentType())
            {
            case IItemBlock::ContentType::DebugText:
                continue;

            case IItemBlock::ContentType::Text:
                ++textBlocks;
                break;
            case IItemBlock::ContentType::FileSharing:
                if (block->isPreviewable())
                    ++mediaBlocks;
                else
                    ++fileBlocks;
                break;
            case IItemBlock::ContentType::Link:
                ++linkBlocks;
                break;
            case IItemBlock::ContentType::Quote:
                ++quoteBlocks;
                if (!forward && block->getQuote().isForward_)
                    forward = true;
                break;
            case IItemBlock::ContentType::Poll:
                ++pollBlocks;
                break;
            case IItemBlock::ContentType::Task:
                ++taskBlocks;
                break;
            case IItemBlock::ContentType::Profile:
                ++profileBlocks;
                break;
            default:
                ++otherBlocks;
                break;
            }
        }

        const auto textOnly = textBlocks && !(fileBlocks || linkBlocks || pollBlocks || taskBlocks || otherBlocks);
        const auto textAndMedia = textBlocks && (linkBlocks || fileBlocks || mediaBlocks);
        const auto mediaOnly = mediaBlocks && !(textBlocks || linkBlocks);
        const auto linkOnly = linkBlocks;
        const auto quoteAndText = !forward && quoteBlocks;
        return (textOnly || textAndMedia || linkOnly || quoteAndText || mediaOnly || profileBlocks) && !pollBlocks && !taskBlocks;
    }

    return false;
}

bool ComplexMessageItem::isSingleMedia() const
{
    static constexpr auto mediaTypes =
    {
        MediaType::mediaTypeFsPhoto,
        MediaType::mediaTypeFsVideo,
        MediaType::mediaTypeFsGif,
    };

    return isSingleFilesharing()
        && std::any_of(mediaTypes.begin(), mediaTypes.end(), [type = getMediaType()](auto t){ return t == type; });
}

bool ComplexMessageItem::isSimple() const
{
    return getBlockCount() == 1 && Blocks_.front()->isSimple();
}

bool ComplexMessageItem::isSingleSticker() const
{
    return isSingleBlockOfType(IItemBlock::ContentType::Sticker);
}

bool ComplexMessageItem::isSingleFilesharing() const
{
    return isSingleBlockOfType(IItemBlock::ContentType::FileSharing);
}

void ComplexMessageItem::onHoveredBlockChanged(IItemBlock *newHoveredBlock)
{
    if (hoveredBlock_ == newHoveredBlock || Utils::InterConnector::instance().isMultiselect(ChatAimid_))
    {
        if (!newHoveredBlock)
            hoveredBlock_ = nullptr;
        return;
    }

    hoveredBlock_ = newHoveredBlock;
    Q_EMIT hoveredBlockChanged();

    updateTimeAnimated();
}

void ComplexMessageItem::onSharingBlockHoverChanged(IItemBlock* newHoveredBlock)
{
    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
    {
        if (!newHoveredBlock)
            hoveredSharingBlock_ = nullptr;
        return;
    }

    if (newHoveredBlock && !newHoveredBlock->isSharingEnabled())
    {
        if (newHoveredBlock->containsSharingBlock())
            newHoveredBlock = hoveredSharingBlock_;
        else
            newHoveredBlock = nullptr;
    }

    const auto sharingBlockChanged = (newHoveredBlock != hoveredSharingBlock_);
    if (!sharingBlockChanged)
        return;

    hoveredSharingBlock_ = newHoveredBlock;

    if (hoveredSharingBlock_)
        initializeShareButton();

    updateShareButtonGeometry();
}

void ComplexMessageItem::updateStyle()
{
    updateSenderControlColor();

    if (TimeWidget_)
        TimeWidget_->updateStyle();

    for (auto block : Blocks_)
        block->updateStyle();

    update();
}

void ComplexMessageItem::updateFonts()
{
    initSender();
    initButtonsLabel();

    for (auto block : Blocks_)
        block->updateFonts();

    if (TimeWidget_)
        TimeWidget_->updateText();

    updateSize();
}

void ComplexMessageItem::startSpellChecking()
{
    for (auto block : Blocks_)
        block->startSpellChecking();
}

void ComplexMessageItem::onActivityChanged(const bool isActive)
{
    for (auto block : Blocks_)
        block->onActivityChanged(isActive);

    HistoryControlPageItem::onActivityChanged(isActive);
}

void ComplexMessageItem::onVisibleRectChanged(const QRect& _visibleRect)
{
    if (!isOutgoingPosition() && !isHeadless() && (hasAvatar() || needsAvatar()))
        Logic::GetStatusContainer()->setAvatarVisible(SenderAimidForDisplay_, !_visibleRect.isEmpty());

    const auto visContent = Layout_->getBlocksContentRect().intersected(_visibleRect);
    for (auto block : Blocks_)
        block->onVisibleRectChanged(visContent.intersected(block->getBlockGeometry()));

    HistoryControlPageItem::onVisibleRectChanged(_visibleRect);
}

void ComplexMessageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    for (auto block : Blocks_)
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
}

void ComplexMessageItem::replaceBlockWithSourceText(IItemBlock* _block, ReplaceReason _reason)
{
    const auto scopedExit = Utils::ScopeExitT([this]() { Q_EMIT layoutChanged(QPrivateSignal()); });
    cleanupBlock(_block);

    for (auto b : Blocks_)
    {
        if (b->replaceBlockWithSourceText(_block))
        {
            Layout_->onBlockSizeChanged();
            return;
        }
    }

    if (!mergeToPreviousOrReplaceByText(Blocks_, _block, this, _reason))
    {
        im_assert(!"block is missing");
        return;
    }

    updateTimeWidgetUnderlay();
    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::removeBlock(IItemBlock* _block)
{
    const auto scopedExit = Utils::ScopeExitT([this]() { Q_EMIT layoutChanged(QPrivateSignal()); });
    cleanupBlock(_block);

    for (auto b : Blocks_)
    {
        if (b->removeBlock(_block))
        {
            Layout_->onBlockSizeChanged();
            return;
        }
    }

    const auto iter = std::find(Blocks_.begin(), Blocks_.end(), _block);

    if (iter == Blocks_.end())
    {
        im_assert(!"block is missing");
        return;
    }

    Blocks_.erase(iter);

    _block->deleteLater();
    updateTimeWidgetUnderlay();

    for (auto b : Blocks_)
        b->markDirty();

    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::cleanupBlock(IItemBlock* _block)
{
    im_assert(_block);
    im_assert(!Blocks_.empty());

    const auto isMenuBlockReplaced = (MenuBlock_ == _block);
    if (isMenuBlockReplaced)
        cleanupMenu();

    const auto isHoveredBlockReplaced = (hoveredBlock_ == _block || hoveredSharingBlock_ == _block);
    if (isHoveredBlockReplaced)
        resetHover();
}

void ComplexMessageItem::selectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo)
{
    im_assert(!Blocks_.empty());

    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        clearBlockSelection();

    if (handleSelectByPos(from, to, areaFrom, areaTo))
    {
        return;
    }

    const auto isSelectionTopToBottom = (from.y() <= to.y());

    const auto &topPoint = (isSelectionTopToBottom ? from : to);
    const auto &bottomPoint = (isSelectionTopToBottom ? to : from);
    im_assert(topPoint.y() <= bottomPoint.y());

    for (auto block : Blocks_)
    {
        const auto blockGeometry = block->getBlockGeometry();

        const QRect globalBlockGeometry(
            mapToGlobal(blockGeometry.topLeft()),
            mapToGlobal(blockGeometry.bottomRight()));

        const auto isBlockAboveSelection = (globalBlockGeometry.bottom() < topPoint.y());
        const auto isBlockBelowSelection = (globalBlockGeometry.top() > bottomPoint.y());
        if (isBlockAboveSelection || isBlockBelowSelection)
        {
            block->clearSelection();
            continue;
        }

        block->selectByPos(topPoint, bottomPoint, isSelectionTopToBottom);
        if (block->isSelected())
        {
            if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
                clearBlockSelection();

            handleSelectByPos(from, to, areaFrom, areaTo);
            Q_EMIT Utils::InterConnector::instance().selectionStateChanged(ChatAimid_, getId(), getInternalId(), true);
            Q_EMIT Utils::InterConnector::instance().updateSelectedCount(ChatAimid_);
        }
    }
}

void ComplexMessageItem::setHasAvatar(const bool _hasAvatar)
{
    HistoryControlPageItem::setHasAvatar(_hasAvatar);

    if (!isOutgoingPosition() && (_hasAvatar || needsAvatar()))
        loadAvatar();
    else
        Avatar_ = QPixmap();

    updateSize();
}

void ComplexMessageItem::setHasSenderName(const bool _hasSender)
{
    const auto senderVisiblePrev = isSenderVisible();

    HistoryControlPageItem::setHasSenderName(_hasSender);

    if (senderVisiblePrev != isSenderVisible())
    {
        for (auto block : Blocks_)
            block->resetClipping();
    }
}

void ComplexMessageItem::setItems(IItemBlocksVec blocks)
{
    im_assert(Blocks_.empty());
    im_assert(!blocks.empty());
    im_assert(!shareButton_);
    im_assert(
        std::all_of(
            blocks.cbegin(),
            blocks.cend(),
            [](IItemBlock *block) { return block; }));

    Blocks_ = std::move(blocks);
    fillFilesPlaceholderMap();

    updateTimeWidgetUnderlay();
}

void ComplexMessageItem::setMchatSender(const QString& _senderName)
{
    if (isOutgoingPosition() || _senderName.isEmpty())
        return;

    if (!Sender_)
        Sender_ = TextRendering::MakeTextUnit(_senderName, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    else
        Sender_->setText(_senderName);

    initSender();
}

void ComplexMessageItem::setLastStatus(LastStatus _lastStatus)
{
    if (getLastStatus() != _lastStatus)
    {
        HistoryControlPageItem::setLastStatus(_lastStatus);
        updateSize();
    }
}

void ComplexMessageItem::setTime(const int32_t time)
{
    im_assert(time >= -1);
    im_assert(Time_ == -1);

    Time_ = time;

    if (!TimeWidget_)
    {
        TimeWidget_ = new MessageTimeWidget(this);
        TimeWidget_->setContact(getContact());
        TimeWidget_->setOutgoing(isOutgoingPosition());
        TimeWidget_->setEdited(isEdited());
        TimeWidget_->setHideEdit(hideEdit_);
        Testing::setAccessibleName(TimeWidget_, u"AS HistoryPage messageTime " % QString::number(Id_));
    }

    TimeWidget_->setTime(time);

    updateTimeWidgetUnderlay();
}

int32_t ComplexMessageItem::getTime() const
{
    return Time_;
}

QSize ComplexMessageItem::sizeHint() const
{
    if (Layout_)
        return Layout_->sizeHint();

    return QSize(-1, MessageStyle::getMinBubbleHeight());
}

bool ComplexMessageItem::hasPictureContent() const
{
    switch (getPinPlaceholder())
    {
    case PinPlaceholderType::Link:
    case PinPlaceholderType::Image:
    case PinPlaceholderType::FilesharingImage:
    case PinPlaceholderType::Video:
    case PinPlaceholderType::FilesharingVideo:
    case PinPlaceholderType::Sticker:
        return true;
    default:
        return false;
    }
}

template<class _BlockIt, class _OutIt>
void findMismatchedBlocks(_BlockIt currFirst, _BlockIt currLast, _BlockIt otherFirst, _BlockIt otherLast, _OutIt result)
{
    size_t currSize = std::distance(currFirst, currLast);
    size_t otherSize = std::distance(otherFirst, otherLast);
    size_t n = std::min(currSize, otherSize);

    for (size_t i = 0; i < n; ++i, ++currFirst, ++otherFirst)
    {
        auto currBlock = *currFirst;
        auto otherBlock = *otherFirst;
        if (currBlock->getContentType() != otherBlock->getContentType() ||
            currBlock->getSourceText() != otherBlock->getSourceText())
        {
            *result = otherBlock;
            ++result;
        }
    }

    if (currSize < otherSize)
        std::copy(otherFirst, otherLast, result);
    else
        std::copy(currFirst, currLast, result);
}

void ComplexMessageItem::removeBlocks(IItemBlocksVec::const_iterator _first, IItemBlocksVec::const_iterator _last)
{
    if (_first == _last)
        return;

    // create a temporary array since std::vector always invalidate iterators
    std::vector<IItemBlock*> blks(_first, _last);
    while (!blks.empty())
    {
        if (auto blk = blks.back())
        {
            blk->hideBlock();
            removeBlock(blk);
        }
        blks.pop_back();
    }
}

void ComplexMessageItem::insertBlocks(IItemBlocksVec::const_iterator _first, IItemBlocksVec::const_iterator _last, ComplexMessageItem* _other)
{
    im_assert(_other != nullptr);
    if (_first == _last)
        return;

    std::copy(_first, _last, std::back_inserter(Blocks_));
    std::for_each(_first, _last, [this, _other](auto b)
    {
        QWidget* w = dynamic_cast<QWidget*>(b);
        im_assert(w != nullptr);
        _other->Layout_->removeWidget(w);
        b->setParentMessage(this);
        b->onActivityChanged(true);
        w->show();
    });
    _other->Blocks_.erase(_first, _last);

    Layout_->onBlockSizeChanged();
}

bool ComplexMessageItem::canBeUpdatedWith(const ComplexMessageItem& _other) const
{
    if (getBlockCount() != _other.getBlockCount())
    {
        std::vector<IItemBlock*> mismatched;
        findMismatchedBlocks(Blocks_.begin(), Blocks_.end(), _other.Blocks_.begin(), _other.Blocks_.end(), std::back_inserter(mismatched));

        return (mismatched.size() == 1 && mismatched.back()->getContentType() == IItemBlock::ContentType::Text);
    }

    auto iter = Blocks_.begin();
    auto iterOther = _other.Blocks_.begin();
    while (iter != Blocks_.end())
    {
        auto block = *iter;
        auto otherBlock = *iterOther;

        const auto bothText = block->getContentType() == IItemBlock::ContentType::Text && otherBlock->getContentType() == IItemBlock::ContentType::Text;
        if (!bothText && block->getSourceText() != otherBlock->getSourceText())
            return false;

        ++iter;
        ++iterOther;
    }

    return true;
}

void ComplexMessageItem::updateWith(ComplexMessageItem &_update)
{
    if (_update.Id_ != -1 || (_update.Id_ == -1 && Id_ == -1))
    {
        im_assert((Id_ == -1) || (Id_ == _update.Id_));
        Id_ = _update.Id_;
        PrevId_ = _update.PrevId_;
        internalId_ = _update.internalId_;
        Url_ = _update.Url_;
        mentions_ = _update.mentions_;
        SourceText_ = _update.SourceText_;
        SenderFriendly_ = _update.SenderFriendly_;
        SenderAimid_ = _update.SenderAimid_;

        const auto editedChanged = _update.isEdited() != isEdited();
        const auto reactionsChanged = hasReactions() != _update.hasReactions();
        const auto buttonsMightChanged = (buttons_.size() + _update.buttons_.size()) > 0;
        const auto threadIdChanged = getThreadId() != _update.getThreadId();

        const auto needUpdateSize = editedChanged || reactionsChanged || buttonsMightChanged || threadIdChanged;

        setBuddy(_update.buddy());

        if (editedChanged)
            setEdited(_update.isEdited());

        setHideEdit(_update.hideEdit_);
        updateButtonsFromOther(_update.buttons_);

        auto iter = Blocks_.begin();
        auto iterOther = _update.Blocks_.begin();
        size_t n = std::min(getBlockCount(), _update.getBlockCount());
        for (size_t i = 0; i < n; ++i, ++iter, ++iterOther)
        {
            auto block = *iter;
            auto otherBlock = *iterOther;

            block->updateWith(otherBlock);
        }

        if (getBlockCount() < _update.getBlockCount())
            insertBlocks(iterOther, _update.Blocks_.end(), &_update);
        else
            removeBlocks(iter, Blocks_.end());

        Description_ = _update.Description_;

        fillFilesPlaceholderMap();
        updateTimeWidgetUnderlay();

        setChainedToPrev(_update.isChainedToPrevMessage());
        setChainedToNext(_update.isChainedToNextMessage());

        if (needUpdateSize)
            updateSize();

        if (TimeWidget_)
            Testing::setAccessibleName(TimeWidget_, u"AS HistoryPage messageTime " % QString::number(Id_));
    }
}

void ComplexMessageItem::getUpdatableInfoFrom(ComplexMessageItem& _update)
{
    auto iter = Blocks_.begin();
    auto iterOther = _update.Blocks_.begin();
    while (getBlockCount() == _update.getBlockCount() && iter != Blocks_.end() && iterOther != _update.Blocks_.end())
    {
        auto block = *iter;
        auto otherBlock = *iterOther;

        if (block->getContentType() == otherBlock->getContentType() && block->getContentType() == IItemBlock::ContentType::FileSharing)
            block->updateWith(otherBlock);

        ++iter;
        ++iterOther;
    }

    copyPlates(&_update);
}

void ComplexMessageItem::leaveEvent(QEvent *event)
{
    event->ignore();

    MouseRightPressedOverItem_ = false;
    MouseLeftPressedOverAvatar_ = false;
    MouseLeftPressedOverSender_ = false;

    bubbleHovered_ = false;

    resetHover();
    Q_EMIT leave();

    if (hasButtons())
    {
        for (auto& line : buttons_)
            for (auto& btn : line)
                btn.hovered_ = false;

        update();
    }

    setCursor(Qt::ArrowCursor);
    HistoryControlPageItem::leaveEvent(event);
}

void ComplexMessageItem::mouseMoveEvent(QMouseEvent *_event)
{
    _event->ignore();

    if (!Utils::InterConnector::instance().isMultiselect(ChatAimid_))
    {
        const auto mousePos = _event->pos();

        auto sharingBlockUnderCursor = findBlockUnder(mousePos, FindForSharing::Yes);
        onSharingBlockHoverChanged(sharingBlockUnderCursor);

        auto blockUnderCursor = findBlockUnder(mousePos);
        onHoveredBlockChanged(blockUnderCursor);

        bool buttonHovered = false;
        const auto role = Logic::getContactListModel()->getYourRole(ChatAimid_);
        const auto isDisabled = role == u"notamember" || role == u"readonly";

        if (hasButtons() && !isDisabled)
        {
            for (auto& line : buttons_)
                for (auto& btn : line)
                {
                    const auto contains = btn.rect_.contains(mousePos) && !btn.pressTime_.isValid();
                    btn.hovered_ = contains;
                    buttonHovered |= contains;
                }

            update();
        }

        if (isOverAvatar(mousePos) || isOverSender(mousePos) || buttonHovered)
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);
    }

    HistoryControlPageItem::mouseMoveEvent(_event);
}

void ComplexMessageItem::mousePressEvent(QMouseEvent *event)
{
    const auto isLeftButtonPressed = (event->button() == Qt::LeftButton);
    if (isLeftButtonPressed && Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        return HistoryControlPageItem::mousePressEvent(event);

    MouseLeftPressedOverAvatar_ = false;
    MouseLeftPressedOverSender_ = false;
    MouseRightPressedOverItem_ = false;

    if (isLeftButtonPressed)
    {
        if (isOverAvatar(event->pos()))
        {
            MouseLeftPressedOverAvatar_ = true;
        }
        else if (isOverSender(event->pos()))
        {
            MouseLeftPressedOverSender_ = true;
        }
        else if (hasButtons())
        {
            for (auto& line : buttons_)
            {
                for (auto& btn : line)
                {
                    if (btn.rect_.contains(event->pos()) && !btn.pressTime_.isValid())
                    {
                        btn.pressed_ = true;
                    }
                }
            }
            update();
        }
    }

    const auto isRightButtonPressed = (event->button() == Qt::RightButton);
    if (isRightButtonPressed)
        MouseRightPressedOverItem_ = true;

    if (!bubbleHovered_ && !MouseLeftPressedOverAvatar_ && !MouseLeftPressedOverSender_)
        event->ignore();

    PressPoint_ = event->pos();

    if (isLeftButtonPressed)
    {
        auto pressed = false;
        for (auto b : Blocks_)
            pressed |= b->pressed(event->pos());

        if (pressed)
        {
            Q_EMIT Utils::InterConnector::instance().selectionStateChanged(ChatAimid_, getId(), getInternalId(), true);
            Q_EMIT Utils::InterConnector::instance().updateSelectedCount(ChatAimid_);
            event->accept();
            return;
        }
    }

    if (Bubble_.contains(event->pos()))
    {
        if (!isSelected() && isLeftButtonPressed)
        {
            if (auto scrollArea = qobject_cast<MessagesScrollArea*>(parent()))
                scrollArea->clearSelection();
        }
    }

    return HistoryControlPageItem::mousePressEvent(event);
}

void ComplexMessageItem::mouseReleaseEvent(QMouseEvent *event)
{
    for (auto b : Blocks_)
        b->anyMouseButtonReleased();

    const auto isLeftButtonReleased = (event->button() == Qt::LeftButton);
    if (isLeftButtonReleased && Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        return HistoryControlPageItem::mouseReleaseEvent(event);

    event->ignore();

    const auto role = Logic::getContactListModel()->getYourRole(ChatAimid_);
    const auto isDisabled = role == u"notamember" || role == u"readonly";

    for (auto& line : buttons_)
        for (auto& btn : line)
            btn.pressed_ = false;

    if (isLeftButtonReleased)
    {
        if ((isOverAvatar(event->pos()) && MouseLeftPressedOverAvatar_) || (isOverSender(event->pos()) && MouseLeftPressedOverSender_))
        {
            Utils::openDialogOrProfile(SenderAimidForDisplay_);
            event->accept();
        }
        else if (hasButtons() && Utils::clicked(PressPoint_, event->pos()))
        {
            for (auto& line : buttons_)
            {
                for (auto& btn : line)
                {
                    if (btn.rect_.contains(event->pos()) && !btn.pressTime_.isValid())
                    {
                        if (isDisabled)
                        {
                            QTimer::singleShot(0, []()
                            {
                                Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("bots", "You are not a member of this chat or you was banned to write"));
                            });
                            continue;
                        }

                        if (!btn.data_.url_.isEmpty())
                        {
                            QTimer::singleShot(0, [url = btn.data_.url_]() { handleBotAction(url); });
                        }
                        else
                        {
                            btn.seq_ = Ui::GetDispatcher()->getBotCallbackAnswer(ChatAimid_, btn.data_.callbackData_, getId());
                            btn.pressTime_ = QDateTime::currentDateTime();

                            startButtonsTimer(std::chrono::milliseconds(100));
                            update();
                        }
                    }
                }
            }
            event->accept();
        }
    }

    auto globalPos = event->globalPos();
    if (isContextMenuEnabled())
    {
        const auto isRightButtonPressed = (event->button() == Qt::RightButton);
        const auto rightButtonClickOnWidget = (isRightButtonPressed && MouseRightPressedOverItem_);
        if (rightButtonClickOnWidget)
        {
            if (isOverAvatar(event->pos()) && !isContextMenuReplyOnly() && !Utils::InterConnector::instance().isMultiselect(ChatAimid_))
            {
                Q_EMIT avatarMenuRequest(SenderAimidForDisplay_);
                return;
            }

            if (!Tooltip::isVisible())
                trackMenu(globalPos);
        }
    }
    else
    {
        MenuBlock_ = findBlockUnder(mapFromGlobal(globalPos));
        if (MenuBlock_ && MenuBlock_->getContentType() == IItemBlock::ContentType::Text)
        {
            auto url = MenuBlock_->linkAtPos(globalPos).url_;
            if (!url.isEmpty())
            {
                Utils::UrlParser parser;
                parser.process(url);
                if (parser.hasUrl())
                {
                    auto menu = new ContextMenu(this);
                    Testing::setAccessibleName(menu, qsl("AS ComplexMessageItem contextMenu"));

                    menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy link"),
                        makeData(qsl("copy_link"), url));

                    connect(menu, &ContextMenu::triggered, this, &ComplexMessageItem::onMenuItemTriggered, Qt::QueuedConnection);
                    connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
                    connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

                    menu->popup(globalPos);
                }
            }
        }
    }

    if (isLeftButtonReleased)
    {
        for (auto b : Blocks_)
            b->releaseSelection();

        if (Utils::clicked(PressPoint_, event->pos()))
        {
            for (auto b : Blocks_)
                b->clicked(event->pos());
        }
    }

    PressPoint_ = QPoint();

    MouseRightPressedOverItem_ = false;
    MouseLeftPressedOverAvatar_ = false;
    MouseLeftPressedOverSender_ = false;

    return HistoryControlPageItem::mouseReleaseEvent(event);
}

void ComplexMessageItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        return;

    const auto isLeftButtonReleased = (event->button() == Qt::LeftButton);

    if (isLeftButtonReleased)
    {
        auto emitQuote = true;
        if (hasButtons())
        {
            for (auto& line : buttons_)
            {
                for (auto& btn : line)
                {
                    if (btn.rect_.contains(event->pos()))
                    {
                        emitQuote = false;
                        break;
                    }
                }
                if (!emitQuote)
                    break;
            }
        }

        const auto &bubbleRect = Layout_->getBubbleRect();
        if (bubbleRect.contains(event->pos()))
        {
            for (auto block : Blocks_)
            {
                const auto blockRect = block->getBlockGeometry();
                const auto bockContain = blockRect.contains(event->pos());
                if (bockContain)
                {
                    block->doubleClicked(event->pos(), [&emitQuote](bool result) { if (result) emitQuote = false; });
                    if (block->isSelected())
                    {
                        Q_EMIT Utils::InterConnector::instance().selectionStateChanged(ChatAimid_, getId(), getInternalId(), true);
                        Q_EMIT Utils::InterConnector::instance().updateSelectedCount(ChatAimid_);
                    }
                }
            }
        }

        if (emitQuote && getId() != -1)
        {
            Q_EMIT quote(getQuotes(false));
            return;
        }


        if (auto scrollArea = qobject_cast<MessagesScrollArea*>(parent()))
            scrollArea->continueSelection(scrollArea->mapFromGlobal(mapToGlobal(event->pos())));

        Q_EMIT selectionChanged();
    }
}

void ComplexMessageItem::renderDebugInfo()
{
    auto appConfig = GetAppConfig();
    if (!appConfig.IsShowMsgIdsEnabled() && !appConfig.ShowMsgOptionHasChanged())
        return;

    bool needInitialize = false;
    if (appConfig.IsShowMsgIdsEnabled())
    {
        // insert or update
        auto countDbgMsgs = 0;

        auto updateVisibility = [](DebugTextBlock *debugTextBlock) {
            debugTextBlock->onActivityChanged(true);
            debugTextBlock->setHidden(false);
            debugTextBlock->setBubbleRequired(true);
        };

        for (auto& block : Blocks_)
        {

            if (block->getContentType() != IItemBlock::ContentType::DebugText)
                continue;

            auto dbgBlock = dynamic_cast<DebugTextBlock *>(block);
            im_assert(dbgBlock);
            if (!dbgBlock || dbgBlock->getSubtype() != DebugTextBlock::Subtype::MessageId)
                continue;

            ++countDbgMsgs;

            if (dbgBlock->getMessageId() == getId())
                continue;

            block->deleteLater();
            auto debugTextBlock = new DebugTextBlock(this, getId(), DebugTextBlock::Subtype::MessageId);
            block = debugTextBlock;
            updateVisibility(debugTextBlock);
            needInitialize = true;
        }

        if (!countDbgMsgs)
        {
            auto child = new DebugTextBlock(this, getId(), DebugTextBlock::Subtype::MessageId);
            Blocks_.insert(Blocks_.begin(), child);
            updateVisibility(child);
            needInitialize = true;
        }
    }
    else
    {
        auto isNotDebugMessageId = [](auto block) {
            if (block->getContentType() != IItemBlock::ContentType::DebugText)
                return true;

            auto dbgBlock = dynamic_cast<DebugTextBlock *>(block);
            im_assert(dbgBlock);
            if (!dbgBlock)
                return true;

            return dbgBlock->getSubtype() != DebugTextBlock::Subtype::MessageId;
        };

        const auto it = std::stable_partition(Blocks_.begin(), Blocks_.end(), isNotDebugMessageId);
        needInitialize = it != Blocks_.end();

        std::for_each(it, Blocks_.end(), [](auto x) { x->deleteLater(); });
        Blocks_.erase(it, Blocks_.end());
    }

    if (needInitialize)
        initialize();
}

void ComplexMessageItem::paintEvent(QPaintEvent *event)
{
    HistoryControlPageItem::paintEvent(event);

    if (!Layout_)
        return;

    // NOTE: Doesn't do anything unless debug settings have been changed in runtime
    renderDebugInfo();

    QPainter p(this);
    p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        drawSelection(p, Layout_->getBubbleRect());

    drawButtons(p, QuoteAnimation_->quoteColor());

    drawBubble(p, QuoteAnimation_->quoteColor());

    drawAvatar(p);

    if (Sender_ && isSenderVisible())
        Sender_->draw(p, TextRendering::VerPosition::BASELINE);

    if (MessageStyle::isBlocksGridEnabled())
        drawGrid(p);

    if (const auto lastStatus = getLastStatus(); lastStatus != LastStatus::None)
        drawLastStatusIcon(p, lastStatus, SenderAimidForDisplay_, getSenderFriendly(), 0);

    drawHeads(p);
}

void ComplexMessageItem::onAvatarChanged(const QString& aimId)
{
    im_assert(!SenderAimidForDisplay_.isEmpty());

    if (SenderAimidForDisplay_ != aimId || (!hasAvatar() && !needsAvatar()))
        return;

    loadAvatar();
    update();
}

void ComplexMessageItem::onMenuItemTriggered(QAction *action)
{
    const auto params = action->data().toMap();
    const auto command = params[qsl("command")].toString();
    const auto commandView = QStringView(command);

    if (constexpr QStringView devPrefix = u"dev:"; commandView.startsWith(devPrefix))
    {
        const auto subCommand = commandView.mid(devPrefix.size());

        if (subCommand.isEmpty())
        {
            im_assert(!"unknown subcommand");
            return;
        }

        if (onDeveloperMenuItemTriggered(subCommand))
            return;
    }

    core::stats::event_props_type stat_props;
    stat_props.reserve(3);
    stat_props.emplace_back("chat_type", Utils::chatTypeByAimId(ChatAimid_));
    stat_props.emplace_back("message_affiliation", isOutgoing() ? "own" : "foreign");

    if (MenuBlock_ && MenuBlock_->onMenuItemTriggered(params))
    {
        if (commandView == u"copy_link" || commandView == u"copy_email" || commandView == u"copy_file")
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_copy_action, stat_props);
        return;
    }

    const auto isPendingMessage = (Id_ <= -1);
    const auto containsPoll = std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& _block) { return _block->getContentType() == IItemBlock::ContentType::Poll; });

    if (commandView == u"copy")
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Copy);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_copy_action, stat_props);
    }
    else if (commandView == u"quote")
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Quote);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_reply_action, stat_props);
    }
    else if (commandView == u"forward")
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Forward);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_forward_action, stat_props);
    }
    else if (commandView == u"report")
    {
        auto confirm = false;

        const auto guard = QPointer(this);
        if (MenuBlock_ && MenuBlock_->getContentType() != IItemBlock::ContentType::Sticker)
            confirm = ReportMessage(getSenderAimid(), ChatAimid_, Id_, SourceText_.string());
        else
            confirm = ReportSticker(getSenderAimid(), isChat_ ? ChatAimid_ : QString(), MenuBlock_ ? MenuBlock_->getStickerId() : Data::StickerId{});

        if (!guard)
            return;
        // delete this message
        if (confirm)
            Ui::GetDispatcher()->deleteMessages(ChatAimid_, { DeleteMessageInfo(Id_, internalId_, false) });

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_report_action, stat_props);
    }
    else if (commandView == u"delete_all" || commandView == u"delete")
    {
        const auto is_shared = command == u"delete_all";

        im_assert(!getSenderAimid().isEmpty());

        const auto guard = QPointer(this);

        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?"),
            QT_TRANSLATE_NOOP("popup_window", "Delete message"),
            nullptr
        );

        if (!guard)
            return;

        if (confirm)
            GetDispatcher()->deleteMessages(ChatAimid_, { DeleteMessageInfo(Id_, internalId_, is_shared) });

        stat_props.emplace_back("delete_target", is_shared ? "all" : "self");
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_delete_action, stat_props);

        if (is_shared && containsPoll)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::polls_action, { {"chat_type", Ui::getStatsChatType(ChatAimid_) }, { "action", "del_for_all" } });
    }
    else if (commandView == u"edit")
    {
        callEditing();
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_edit_action, stat_props);
    }
    else if (commandView == u"select")
    {
        clearBlockSelection();
        Utils::InterConnector::instance().clearPartialSelection(ChatAimid_);
        Utils::InterConnector::instance().setMultiselect(true, ChatAimid_);
        setSelected(true);
        Q_EMIT Utils::InterConnector::instance().messageSelected(getId(), getInternalId());
    }
    else if (commandView == u"add_to_favorites")
    {
        Q_EMIT addToFavorites(getQuotes(isSelected() && !isAllSelected(), true));
    }
    else if (command == u"make_readonly")
    {
        const auto userId = params[qsl("arg")].toString();
        Q_EMIT readOnlyUser(userId, true, QPrivateSignal());
    }
    else if (command == u"revoke_readonly")
    {
        const auto userId = params[qsl("arg")].toString();
        Q_EMIT readOnlyUser(userId, false, QPrivateSignal());
    }
    else if (command == u"block")
    {
        const auto userId = params[qsl("arg")].toString();
        Q_EMIT blockUser(userId, true, QPrivateSignal());
    }
    else if (!isPendingMessage)
    {
        if (commandView == u"pin" || commandView == u"unpin")
        {
            const bool isUnpin = commandView == u"unpin";
            Q_EMIT pin(ChatAimid_, Id_, isUnpin);

            if (isUnpin)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_unpin_action, stat_props);
            else
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_pin_action, stat_props);

            if (!isUnpin && containsPoll)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::polls_action, { {"chat_type", Ui::getStatsChatType(ChatAimid_) }, { "action", "pin" } });
        }
    }
}

void ComplexMessageItem::onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta)
{
    if (auto it = snippetsWaitingForMeta_.find(_seq); it != snippetsWaitingForMeta_.end())
    {
        const auto scopedExit = Utils::ScopeExitT([this]() { Q_EMIT layoutChanged(QPrivateSignal()); });
        auto& snippet = it->second;

        if (_success || snippet.linkInText_)
        {
            auto blockIt = std::find(Blocks_.begin(), Blocks_.end(), snippet.textBlock_);
            if (blockIt != Blocks_.end() || !snippet.textBlock_)
            {
                auto it = snippet.textBlock_ ? Blocks_.erase(blockIt) : Blocks_.begin() + std::min(snippet.insertAt_, (size_t)getBlockCount());

                if (snippet.textBlock_)
                    snippet.textBlock_->deleteLater();

                if (_success)
                {
                    auto snippetBlock = new SnippetBlock(this, snippet.link_, snippet.linkInText_, snippet.estimatedType_);
                    Blocks_.insert(it, snippetBlock);
                    snippetBlock->onActivityChanged(true);
                    snippetBlock->onVisibleRectChanged(rect());

                    snippetBlock->show();
                }

                Layout_->onBlockSizeChanged();
            }
        }

        snippetsWaitingForMeta_.erase(it);
    }
}

void ComplexMessageItem::onBotCallbackAnswer(const int64_t _seq, const QString& _reqId, const QString& text, const QString& _url, bool _showAlert)
{
    if (Logic::getContactListModel()->selectedContact() != ChatAimid_)
        return;

    for (auto& line : buttons_)
    {
        for (auto& btn : line)
        {
            if (btn.seq_ == _seq || (!_reqId.isEmpty() && btn.reqId_ == _reqId))
            {
                btn.seq_ = -1;
                if (text.isEmpty() && _url.isEmpty() && btn.reqId_.isEmpty())
                {
                    btn.reqId_ = _reqId;
                    continue;
                }

                QTimer::singleShot(0, [_url, text, _showAlert]() { handleBotAction(_url, text, _showAlert); });

                btn.clearProgress();
            }
        }

        update();
    }
}

void ComplexMessageItem::updateButtons()
{
    bool hasAnimatingButtons = false, hasPressedButtons = false;
    for (auto& line : buttons_)
    {
        for (auto& btn : line)
        {
            if (btn.pressTime_.isValid())
            {
                hasPressedButtons = true;

                const auto& pressTime = btn.pressTime_;
                const auto current = QDateTime::currentDateTime();

                if (pressTime.msecsTo(current) > Features::getAsyncResponseTimeout() * 1000)
                {
                    btn.clearProgress();
                    continue;
                }

                btn.animating_ = (pressTime.msecsTo(current) > BUTTON_ANIMATION_START_TIMEOUT.count());
                hasAnimatingButtons |= btn.animating_;
            }
        }
    }

    if (hasPressedButtons)
    {
        if (hasAnimatingButtons)
        {
            ensureButtonsAnimationInitialized();
            if (buttonsAnimation_->state() != QAbstractAnimation::Running)
                buttonsAnimation_->start();
        }
        startButtonsTimer(hasAnimatingButtons ? std::chrono::seconds(1) : std::chrono::milliseconds(100));
    }
    else
    {
        if (buttonsAnimation_)
            buttonsAnimation_->stop();
    }

    update();
}

void ComplexMessageItem::callEditingImpl()
{
    auto msg = std::make_shared<Data::MessageBuddy>(buddy());
    msg->SetText(getEditableText(InstantEdit::No));
    msg->Files_ = getFilesPlaceholders();
    Q_EMIT edit(msg, getMediaType());
}

void ComplexMessageItem::cleanupMenu()
{
    MenuBlock_ = nullptr;
}

void ComplexMessageItem::connectSignals()
{
    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ComplexMessageItem::onAvatarChanged);

    if constexpr (platform::is_apple())
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationDeactivated, this, &ComplexMessageItem::resetHover);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentMessageChanged, this, qOverload<>(&ComplexMessageItem::updateSize));
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectAnimationUpdate, this, qOverload<>(&ComplexMessageItem::updateSize));
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &ComplexMessageItem::updateShareButtonGeometry);

    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::botCallbackAnswer, this, &ComplexMessageItem::onBotCallbackAnswer);
    connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimId)
    {
        if (hasAvatar() && SenderAimidForDisplay_ == _aimId)
            update();
    });
}

bool ComplexMessageItem::containsShareableBlocks() const
{
    im_assert(!Blocks_.empty());

     return std::any_of(
         Blocks_.cbegin(),
         Blocks_.cend(),
         []
         (const IItemBlock *block)
         {
             im_assert(block);
             return block->isSharingEnabled() || block->containsSharingBlock();
         });
}

void ComplexMessageItem::drawAvatar(QPainter& _p)
{
    if (Avatar_.isNull() || (!hasAvatar() && !needsAvatar()))
        return;

    if (const auto avatarRect = Layout_->getAvatarRect(); avatarRect.isValid())
        Utils::drawAvatarWithBadge(_p, avatarRect.topLeft(), Avatar_, SenderAimidForDisplay_, true);
}

void ComplexMessageItem::drawBubble(QPainter &p, const QColor& quote_color)
{
    const auto &bubbleRect = Layout_->getBubbleRect();
    if (bubbleRect.isNull())
        return;

    if (MessageStyle::isBlocksGridEnabled())
    {
        Utils::PainterSaver ps(p);

        p.setPen(Qt::gray);
        p.drawRect(bubbleRect);
    }

    const auto bubbleRequired = isBubbleRequired();
    const auto headerRequired = isHeaderRequired();

    if (!bubbleRequired && !headerRequired)
        return;

    if (bubbleRect != BubbleGeometry_)
    {
        int flags = Utils::RenderBubbleFlags::AllRounded;
        if (isChainedToPrevMessage())
            flags |= (isOutgoingPosition() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

        if (isChainedToNextMessage())
            flags |= (isOutgoingPosition() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);

        auto r = bubbleRect;
        if (!bubbleRequired && headerRequired)
        {
            // draw bubble only for header
            const auto newHeight = r.height() - Blocks_.front()->getBlockGeometry().height();
            if (newHeight > 0)
                r.setHeight(newHeight);
            flags &= ~(Utils::RenderBubbleFlags::RightBottomSmall);
            flags &= ~(Utils::RenderBubbleFlags::LeftBottomSmall);
            flags &= ~(Utils::RenderBubbleFlags::BottomSideRounded);
        }

        Bubble_ = Utils::renderMessageBubble(r, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadiusSmall(), (Utils::RenderBubbleFlags)flags);
        BubbleGeometry_ = bubbleRect;
    }

    const auto needBorder = !isOutgoingPosition() && Styling::getParameters(getContact()).isBorderNeeded();
    if (!needBorder && !isHeaderRequired())
        Utils::drawBubbleShadow(p, Bubble_);

    p.fillPath(Bubble_, MessageStyle::getBodyBrush(isOutgoingPosition(), getContact()));

    if (quote_color.isValid())
        p.fillPath(Bubble_, QBrush(quote_color));

    if (needBorder)
        Utils::drawBubbleRect(p, Bubble_.boundingRect(), MessageStyle::getBorderColor(), MessageStyle::getBorderWidth(), MessageStyle::getBorderRadius());
}

void ComplexMessageItem::drawButtons(QPainter &p, const QColor& quote_color)
{
    if (!hasButtons())
        return;

    auto bubbleRect = Layout_->getBubbleRect();
    if (bubbleRect.isNull())
        return;

    const auto tranparent = hasButtons() && isSingleSticker();

    if (tranparent && !isSenderVisible())
        bubbleRect.moveBottom(bubbleRect.bottom() - MessageStyle::getBubbleVerPadding());

    const auto multiply = tranparent ? -1 : 1;
    auto r = QRect(bubbleRect.x(), bubbleRect.bottomLeft().y() - (MessageStyle::getBorderRadius() * multiply), bubbleRect.width(), buttonsHeight() + (tranparent ? 0 : MessageStyle::getBorderRadius()));

    auto buttonsBubble = Utils::renderMessageBubble(r, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadiusSmall(), tranparent ? Utils::RenderBubbleFlags::AllRounded : Utils::RenderBubbleFlags::BottomSideRounded);

    p.fillPath(buttonsBubble, getButtonBackgroundColor(ChatAimid_, isOutgoingPosition(), false, false));

    if (quote_color.isValid())
        p.fillPath(buttonsBubble, QBrush(quote_color));

    r.setY(r.y() - (tranparent ? 0 : MessageStyle::getBorderRadius()));
    Utils::drawBubbleRect(p, r, Styling::getParameters().getColor(Styling::StyleVariable::GHOST_QUATERNARY), MessageStyle::getBorderWidth(), MessageStyle::getBorderRadius());

    QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_QUATERNARY), MessageStyle::getBorderWidth());
    p.setPen(pen);

    auto drawButton = [this, &p, buttonsBubble](MessageButton& btn, const QRect& _rect)
    {
        btn.rect_ = _rect;

        const auto [color, icon] = getButtonColorAndIcon(btn.data_.getStyle(), ChatAimid_, isOutgoingPosition());

        if (btn.hovered_ || btn.pressed_)
        {
            QPainterPath path;
            path.addRect(_rect);
            auto r = path.intersected(buttonsBubble);
            p.fillPath(r, getButtonBackgroundColor(ChatAimid_, isOutgoingPosition(), btn.hovered_, btn.pressed_));
        }

        buttonLabel_->setText(btn.data_.text_, color);
        buttonLabel_->setLinkColor(color);
        const auto x = _rect.center().x() - buttonLabel_->desiredWidth() / 2;
        const auto y = _rect.center().y() - (btn.isTwoLines_ ? buttonLabel_->cachedSize().height() / 2 : 0);

        buttonLabel_->setOffsets(x, y);

        {
            Utils::PainterSaver saver(p);
            if (btn.pressTime_.isValid())
                p.setOpacity(0.5);

            buttonLabel_->draw(p, btn.isTwoLines_ ? TextRendering::VerPosition::TOP : TextRendering::VerPosition::MIDDLE);
        }

        if (btn.isExternalUrl_)
            p.drawPixmap(btn.rect_.topRight().x() - icon.width() / Utils::scale_bitmap(1) - MessageStyle::getUrlInButtonOffset(), btn.rect_.topRight().y() + MessageStyle::getUrlInButtonOffset(), icon);

        if (btn.animating_)
        {
            const auto animationSize = MessageStyle::getButtonsAnimationSize();
            const auto animationRect = QRect(QPoint(_rect.center().x() - animationSize.width() / 2,
                _rect.center().y() - animationSize.height() / 2),
                animationSize);
            drawAnimation(p, buttonsAnimation_ ? buttonsAnimation_->currentValue().toDouble() : 0.0, animationRect);
        }
    };

    size_t lineNumber = 0;
    auto curHeigh = 0;
    for (auto& line : buttons_)
    {
        auto count = line.size();
        auto lineHeight = buttonLineHeights_[lineNumber];
        if (count > 1)
        {
            auto buttonWidth = (qreal)bubbleRect.width() / count;
            for (size_t i = 1; i < count; ++i)
            {
                p.drawLine(QPointF(bubbleRect.x() + i * buttonWidth, bubbleRect.bottomLeft().y() + curHeigh + MessageStyle::getBorderWidth() + (tranparent ? MessageStyle::getBorderRadius() : 0)),
                    QPointF(bubbleRect.x() + i * buttonWidth, bubbleRect.bottomLeft().y() + curHeigh + lineHeight - MessageStyle::getBorderWidth() + (tranparent ? MessageStyle::getBorderRadius() : 0)));
            }

            int j = 0;
            for (auto& btn : line)
            {
                drawButton(btn, QRect(bubbleRect.x() + j * buttonWidth + MessageStyle::getBorderWidth(),
                    bubbleRect.bottomLeft().y() + curHeigh + MessageStyle::getBorderWidth() + (tranparent ? MessageStyle::getBorderRadius() : 0),
                    buttonWidth - MessageStyle::getBorderWidth() * 2, lineHeight - MessageStyle::getBorderWidth() * 2));

                ++j;
            }
        }
        else if (count == 1)
        {
            drawButton(line[0], QRect(bubbleRect.x() + MessageStyle::getBorderWidth(),
                bubbleRect.bottomLeft().y() + curHeigh + MessageStyle::getBorderWidth() + (tranparent ? MessageStyle::getBorderRadius() : 0),
                bubbleRect.width() - MessageStyle::getBorderWidth() * 2, lineHeight - MessageStyle::getBorderWidth() * 2));
        }

        ++lineNumber;
        if (lineNumber != buttons_.size())
        {
            p.drawLine(QPointF(bubbleRect.x(), bubbleRect.bottomLeft().y() + curHeigh + lineHeight + (tranparent ? MessageStyle::getBorderRadius() : 0)),
                QPointF(bubbleRect.x() + bubbleRect.width(), bubbleRect.bottomLeft().y() + curHeigh + lineHeight + (tranparent ? MessageStyle::getBorderRadius() : 0)));
        }

        curHeigh += lineHeight;
    }
}

Data::FString ComplexMessageItem::getBlocksText(const IItemBlocksVec& _items, const bool _isSelected, TextFormat _format) const
{
    Data::FString result;

    // to reduce the number of reallocations
    result.reserve(1024);

    const auto selectedItemCount = _isSelected
        ? std::count_if(_items.cbegin(), _items.cend(), [](auto item) { return item->isSelected(); })
        : 0;

    for (const auto item : _items)
    {
        if (!_isSelected && item->getContentType() == IItemBlock::ContentType::DebugText)
            continue;

        const auto dest = _format == TextFormat::Raw ? IItemBlock::TextDestination::quote : IItemBlock::TextDestination::selection;
        const auto itemText = _isSelected
            ? item->getSelectedText(selectedItemCount > 1, dest)
            : (_format == TextFormat::Raw && item->hasSourceText()) ? item->getSourceText()
                                                                    : item->getTextForCopy();

        if (itemText.isEmpty() || (item->getContentType() == IItemBlock::ContentType::Link && result.string().contains(itemText.string())))
            continue;

        result += itemText;
        if (!result.endsWith(QChar::LineFeed))
            result += QChar::LineFeed;
    }

    if (result.endsWith(QChar::LineFeed))
        result.chop(1);

    return result;
}

void ComplexMessageItem::drawGrid(QPainter &p)
{
    p.setPen(Qt::blue);
    p.drawRect(Layout_->getBlocksContentRect());
}

IItemBlock* ComplexMessageItem::findBlockUnder(const QPoint& _pos, FindForSharing _findFor) const
{
    for (auto block : Blocks_)
    {
        if (!block)
            continue;

        auto b = block->findBlockUnder(_pos);
        if (b)
            return b;

        auto rc = block->getBlockGeometry();
        if (_findFor == FindForSharing::Yes && block->isSharingEnabled())
        {
            const auto extendedGeometry = BubbleGeometry_.isEmpty() ? rc : BubbleGeometry_;

            if (isOutgoingPosition())
                rc.setLeft(extendedGeometry.left() - getSharingAdditionalWidth());
            else
                rc.setRight(extendedGeometry.right() + getSharingAdditionalWidth());
        }

        if (rc.contains(_pos))
            return block;
    }

    return nullptr;
}

QString ComplexMessageItem::getSenderFriendly() const
{
    if (Logic::getContactListModel()->isChannel(ChatAimid_))
        return Logic::GetFriendlyContainer()->getFriendly(ChatAimid_);

    im_assert(!SenderFriendly_.isEmpty());
    return SenderFriendly_;
}

Data::QuotesVec ComplexMessageItem::getQuotes(const bool _selectedTextOnly, const bool _isForward) const
{
    Data::QuotesVec result;

    const auto quotesOnly = getBlockCount(uint32_t(IItemBlock::ContentType::Quote)) == static_cast<int>(Blocks_.size());
    const auto dropQuotes = !_isForward && !quotesOnly;

    for (const auto block : Blocks_)
    {
        const auto blockType = block->getContentType();
        if (blockType == IItemBlock::ContentType::DebugText)
            continue;

        if (_selectedTextOnly && block->getSelectedText().isEmpty())
            continue;

        if (!_selectedTextOnly && dropQuotes && blockType ==IItemBlock::ContentType::Quote)
            continue;

        if (blockType == IItemBlock::ContentType::Link && hasLinkInText_ && !block->isLinkMediaPreview())
            continue;

        auto quote = getQuoteFromBlock(block, _selectedTextOnly);
        quote.hasReply_ = !quotesOnly;
        if (!quote.isEmpty())
        {
            if (!result.isEmpty())
            {
                auto& prevQuote = result.back();
                const auto canMergeQuotes = prevQuote.msgId_ == quote.msgId_;

                if (canMergeQuotes)
                {
                    prevQuote.text_.reserve(prevQuote.text_.size() + quote.text_.size() + 1);

                    if (!prevQuote.text_.endsWith(QChar::LineFeed) && !prevQuote.text_.endsWith(u'\n') && !quote.text_.startsWith(ql1c('\n')))
                        prevQuote.text_ += QChar::LineFeed;

                    prevQuote.text_ += quote.text_;
                }
                else
                {
                    result.push_back(std::move(quote));
                }
            }
            else
            {
                result.push_back(std::move(quote));
            }
        }
    }

    return result;
}

Data::QuotesVec ComplexMessageItem::getQuotesForEdit() const
{
    Data::QuotesVec quotes;
    quotes.reserve(getBlockCount(uint32_t(IItemBlock::ContentType::ExcludeDebug_Mask)));

    for (const auto b : Blocks_)
        if (auto q = b->getQuote(); !q.isEmpty())
            quotes.push_back(std::move(q));

    return quotes;
}

void ComplexMessageItem::setSourceText(const Data::FString& _text)
{
    SourceText_ = _text;
}

void ComplexMessageItem::initialize()
{
    im_assert(Layout_);
    im_assert(!Blocks_.empty());

    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    loadSnippetsMetaInfo();

    updateSize();

    HistoryControlPageItem::initialize();
}

void ComplexMessageItem::initializeShareButton()
{
    if (shareButton_ || Blocks_.empty())
        return;

    if (!containsShareableBlocks() || !shareButtonEnabled())
        return;

    shareButton_ = new CustomButton(this, qsl(":/share_button"), Utils::unscale_value(MessageStyle::getSharingButtonSize()));
    shareButton_->setHoverImage(qsl(":/share_button_hover"));
    shareButton_->setPressedImage(qsl(":/share_button_active"));
    shareButton_->setFixedSize(MessageStyle::getSharingButtonSize());

    shareButtonOpacityEffect_ = new QGraphicsOpacityEffect(shareButton_);
    shareButton_->setGraphicsEffect(shareButtonOpacityEffect_);

    shareButton_->hide();

    connect(shareButton_, &CustomButton::clicked, this, &ComplexMessageItem::onShareButtonClicked);
}

void ComplexMessageItem::initSender()
{
    if (Sender_)
    {
        Sender_->init(MessageStyle::getSenderFont(), QColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        Sender_->evaluateDesiredSize();
        desiredSenderWidth_ = Sender_->desiredWidth();
    }

    updateSenderControlColor();
}

void ComplexMessageItem::initButtonsLabel()
{
    if (buttonLabel_)
        buttonLabel_->init(MessageStyle::getButtonsFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), {}, {}, {}, TextRendering::HorAligment::CENTER);
}

bool ComplexMessageItem::isBubbleRequired() const
{
    return getBlockCount() > 1 || std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isBubbleRequired(); })
            || (hasButtons() && !isSingleSticker());
}

bool ComplexMessageItem::isMarginRequired() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isMarginRequired(); });
}

bool ComplexMessageItem::isHeaderRequired() const
{
    return isSenderVisible() && getBlockCount() == 1 && !Blocks_.front()->isBubbleRequired() && !isSingleSticker();
}

bool ComplexMessageItem::isSmallPreview() const
{
    return getBlockCount() == 1 && Blocks_.front()->isSmallPreview();
}

bool ComplexMessageItem::canStretchWithSender() const
{
    return !(getBlockCount() == 1 && !Blocks_.front()->canStretchWithSender());
}

int ComplexMessageItem::getMaxWidth() const
{
    int maxWidth = -1;

    if (isSingleSticker())
        return maxWidth;

    for (const auto block : Blocks_)
    {
        int blockMaxWidth = block->getMaxWidth();

        if (blockMaxWidth > 0)
        {
            if (maxWidth == -1 || maxWidth > blockMaxWidth)
                maxWidth = blockMaxWidth;
        }
    }

    return maxWidth;
}

void ComplexMessageItem::setEdited(const bool _isEdited)
{
    HistoryControlPageItem::setEdited(_isEdited);

    if (TimeWidget_)
        TimeWidget_->setEdited(isEdited());
}

void ComplexMessageItem::setUpdatePatchVersion(const common::tools::patch_version& _version)
{
    version_ = _version;
}

const Data::MentionMap& ComplexMessageItem::getMentions() const
{
    return mentions_;
}

void ComplexMessageItem::fillFilesPlaceholderMap()
{
    files_.clear();

    for (const auto& b : Blocks_)
    {
        const auto& ct = b->getContentType();
        if (ct == IItemBlock::ContentType::FileSharing || ct == IItemBlock::ContentType::Sticker || ct == IItemBlock::ContentType::Quote)
        {
            if (const auto files = b->getFilePlaceholders(); !files.empty())
                files_.insert(files.begin(), files.end());
        }
    }
}

void ComplexMessageItem::initButtonsTimerIfNeeded()
{
    if (!buttonsUpdateTimer_)
    {
        buttonsUpdateTimer_ = new QTimer(this);
        buttonsUpdateTimer_->setSingleShot(true);
        connect(buttonsUpdateTimer_, &QTimer::timeout, this, &ComplexMessageItem::updateButtons);
    }
}

void ComplexMessageItem::startButtonsTimer(std::chrono::milliseconds _timeout)
{
    initButtonsTimerIfNeeded();
    buttonsUpdateTimer_->start(_timeout);
}

void ComplexMessageItem::ensureButtonsAnimationInitialized()
{
    if (buttonsAnimation_)
        return;

    buttonsAnimation_ = new QVariantAnimation(this);
    buttonsAnimation_->setStartValue(0.0);
    buttonsAnimation_->setEndValue(360.0);
    buttonsAnimation_->setDuration(700);
    buttonsAnimation_->setLoopCount(-1);
    connect(buttonsAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&ComplexMessageItem::update));
}

const Data::FilesPlaceholderMap& ComplexMessageItem::getFilesPlaceholders() const
{
    return files_;
}

Data::FString ComplexMessageItem::getEditableText(InstantEdit _mode) const
{
    using editPair = std::pair<IItemBlock::ContentType, Data::FString>;
    std::vector<editPair> pairs;
    pairs.reserve(getBlockCount(uint32_t(IItemBlock::ContentType::ExcludeDebug_Mask)));

    for (const auto b : Blocks_)
    {
        const auto ct = b->getContentType();
        switch (ct)
        {
        case IItemBlock::ContentType::Text:
        case IItemBlock::ContentType::FileSharing:
        case IItemBlock::ContentType::Sticker:
        case IItemBlock::ContentType::Profile:
            pairs.emplace_back(ct, _mode == InstantEdit::Yes ? b->getTextInstantEdit() : b->getSourceText());
            break;
        case IItemBlock::ContentType::Link:
            if (!hasLinkInText_ || b->isLinkMediaPreview())
                pairs.emplace_back(ct, b->getSourceText());
            break;
        default:
            break;
        }
    }

    Data::FString::Builder result;
    bool lastLineFeed = false;
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        auto& [type, text] = pairs[i];

        if (i > 0 && !text.startsWith(QChar::LineFeed))
        {
            const auto curText = type == IItemBlock::ContentType::Text;
            const auto prevText = pairs[i - 1].first == IItemBlock::ContentType::Text;
            if ((curText || !prevText) || (curText != prevText && !lastLineFeed))
                result %= qsl("\n");
        }

        lastLineFeed = text.endsWith(QChar::LineFeed);

        result %= text;
    }

    return result.finalize();
}

int ComplexMessageItem::desiredWidth() const
{
    int result = 0;
    for (auto b : Blocks_)
        result = std::max(result, b->desiredWidth(width()));

    result = std::max(result, desiredSenderWidth_);

    result = std::max(result, buttonsDesiredWidth());

    if (isBubbleRequired() || isSingleSticker())
        result += Layout_->getBubbleLeftPadding() + Layout_->getBubbleRightPadding();

    return result;
}

int ComplexMessageItem::maxEffectiveBlockWidth() const
{
    auto result = 0;
    for (const auto b : Blocks_)
    {
        result = std::max(result, b->effectiveBlockWidth());
    }

    return result;
}

int ComplexMessageItem::buttonsDesiredWidth() const
{
    int result = 0;
    if (hasButtons())
    {
        for (const auto& line : buttons_)
        {
            auto width = 0;
            for (const auto& btn : line)
            {
                buttonLabel_->setText(btn.originalText_);
                width = std::max(width, (buttonLabel_->desiredWidth() + MessageStyle::getButtonHorOffset() * 2));
            }
            result = std::max(result, width * (int)line.size());
        }
    }

    return result;
}

int ComplexMessageItem::calcButtonsWidth(int availableWidth)
{
    int result = 0;
    bool elided = false;

    for (auto& line : buttons_)
    {
        auto width = 0;
        const auto available = availableWidth / line.size();
        for (auto& btn : line)
        {
            buttonLabel_->setText(btn.originalText_);
            buttonLabel_->elide(available - MessageStyle::getButtonHorOffset() * 2);
            if (buttonLabel_->isElided())
                btn.data_.text_ = buttonLabel_->getText();
            else
                btn.data_.text_ = btn.originalText_;

            elided |= buttonLabel_->isElided();
            width = std::max(width, (buttonLabel_->cachedSize().width() + MessageStyle::getButtonHorOffset() * 2));
        }

        result = std::max(result, width * (int)line.size());
    }

    return elided ? availableWidth : result;
}

int ComplexMessageItem::buttonsHeight() const
{
    return std::accumulate(buttonLineHeights_.begin(), buttonLineHeights_.end(), 0);
}

void ComplexMessageItem::initSize()
{
    Layout_->updateSize(width());
    HistoryControlPageItem::initSize();
}

void ComplexMessageItem::updateSize()
{
    Layout_->onBlockSizeChanged();
    HistoryControlPageItem::updateSize();
}

void ComplexMessageItem::callEditing()
{
    callEditingImpl();
}

void ComplexMessageItem::setHasTrailingLink(const bool _hasLink)
{
    hasTrailingLink_ = _hasLink;
}

void ComplexMessageItem::setHasLinkInText(const bool _hasLinkInText)
{
    hasLinkInText_ = _hasLinkInText;
}

void ComplexMessageItem::setButtons(const std::vector<std::vector<Data::ButtonData>>& _buttons)
{
    buttons_.clear();
    buttonLineHeights_.clear();

    auto linesCount = 0;

    for (const auto& line : _buttons)
    {
        auto buttonsCount = 0;
        std::vector<MessageButton> buttons;
        buttons.reserve(line.size());
        bool hasTwoLineButton = false;
        for (const auto& btn : line)
        {
            MessageButton b;
            b.data_ = btn;
            b.initButton();
            hasTwoLineButton |= b.isTwoLines_;
            buttons.push_back(std::move(b));

            if (++buttonsCount >= MAX_BUTTONS_IN_LINE)
                break;
        }

        buttons_.push_back(std::move(buttons));
        buttonLineHeights_.push_back(hasTwoLineButton ? MessageStyle::getTwoLineButtonHeight() : MessageStyle::getButtonHeight());

        if (++linesCount >= MAX_LINES_OF_BUTTONS)
            break;
    }
}

void ComplexMessageItem::updateButtonsFromOther(const std::vector<std::vector<MessageButton>>& _buttons)
{
    buttons_.clear();
    buttonLineHeights_.clear();

    for (const auto& line : _buttons)
    {
        std::vector<MessageButton> buttons;
        buttons.reserve(line.size());

        bool hasTwoLineButton = false;
        for (const auto& btn : line)
        {
            MessageButton b;
            b.data_ = btn.data_;
            b.initButton();
            hasTwoLineButton |= b.isTwoLines_;
            buttons.push_back(std::move(b));
        }
        buttons_.push_back(std::move(buttons));
        buttonLineHeights_.push_back(hasTwoLineButton ? MessageStyle::getTwoLineButtonHeight() : MessageStyle::getButtonHeight());
    }
}

void ComplexMessageItem::setHideEdit(bool _hideEdit)
{
    hideEdit_ = _hideEdit;
    if (TimeWidget_)
        TimeWidget_->setHideEdit(hideEdit_);
}

int ComplexMessageItem::getBlockCount(uint32_t _typeMask) const
{
    return countBlocks(Blocks_.begin(), Blocks_.end(), _typeMask);
}

IItemBlock* ComplexMessageItem::getHoveredBlock() const
{
    return hoveredBlock_;
}

void ComplexMessageItem::resetHover()
{
    onHoveredBlockChanged(nullptr);
    onSharingBlockHoverChanged(nullptr);
}

int ComplexMessageItem::getSharingAdditionalWidth() const
{
    return shareButton_
        ? MessageStyle::getSharingButtonMargin() + MessageStyle::getSharingButtonSize().width()
        : 0;
}

bool ComplexMessageItem::isFirstBlockDebug() const
{
    return !Blocks_.empty() && Blocks_.front()->getContentType() == IItemBlock::ContentType::DebugText;
}

bool ComplexMessageItem::hasButtons() const
{
    return !buttons_.empty();
}

const IItemBlocksVec &ComplexMessageItem::getBlocks() const
{
    return Blocks_;
}

std::vector<Ui::ComplexMessage::GenericBlock*> ComplexMessageItem::getBlockWidgets() const
{
    std::vector<GenericBlock*> result;
    for (auto b : Blocks_)
        result.push_back((GenericBlock*)(b));

    return result;
}

bool ComplexMessageItem::isLastBlock(const IItemBlock* _block) const
{
    return !Blocks_.empty() && Blocks_.back() == _block;
}

bool ComplexMessageItem::isOverAvatar(const QPoint &pos) const
{
    im_assert(Layout_);

    if (hasAvatar() || needsAvatar())
        return Layout_->isOverAvatar(pos);

    return false;
}

bool ComplexMessageItem::isOverSender(const QPoint & pos) const
{
    if (!Sender_)
        return false;

    const auto senderNameSize = QSize(std::min(Sender_->cachedSize().width(), Sender_->desiredWidth()), Sender_->cachedSize().height());
    const auto senderRect = QRect(Sender_->offsets(), senderNameSize).translated(0, -MessageStyle::getSenderBaseline());
    return isSenderVisible() && senderRect.contains(pos);
}

bool ComplexMessageItem::isSelected() const
{
    return HistoryControlPageItem::isSelected() || std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isSelected(); });
}

bool ComplexMessageItem::isAllSelected() const
{
    return HistoryControlPageItem::isSelected() || std::all_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isAllSelected(); });
}

bool ComplexMessageItem::isSenderVisible() const
{
    return Sender_&& hasSenderName();
}

bool ComplexMessageItem::hasSharedContact() const
{
    return buddy().sharedContact_.has_value();
}

GenericBlock* ComplexMessageItem::addSnippetBlock(Data::FStringView _link, const bool _linkInText, SnippetBlock::EstimatedType _estimatedType, size_t _insertAt, bool _quoteOrForward)
{
    GenericBlock* block = nullptr;
    const auto linkString = _link.string().toString();
    const auto metaInCache = SnippetCache::instance()->contains(linkString);

    if ((!deliveredToServer_ && !metaInCache || _linkInText) && !_quoteOrForward)
    {
        if (!_linkInText)
            block = new SnippetBlock(this, _link, _linkInText, _estimatedType);

        SnippetData snippet;
        snippet.link_ = linkString;
        snippet.linkInText_ = _linkInText;
        snippet.textBlock_ = block;
        snippet.estimatedType_ = _estimatedType;
        snippet.insertAt_ = _insertAt;

        snippetsWaitingForInitialization_.push_back(std::move(snippet));
    }
    else
    {
        block = new SnippetBlock(this, _link, _linkInText, _estimatedType);
    }

    return block;
}

void ComplexMessageItem::setSelected(bool _selected)
{
    for (auto block : Blocks_)
        block->onSelectionStateChanged(_selected);

    HistoryControlPageItem::setSelected(_selected);
}

bool ComplexMessageItem::containsText() const
{
    if (!Description_.isEmpty())
        return true;

    for (auto b : Blocks_)
    {
        auto contentType = b->getContentType();
        if (contentType == IItemBlock::ContentType::Quote)
        {
            if (auto quote = dynamic_cast<QuoteBlock*>(b); !quote->containsText())
                return false;
            else
                continue;
        }

        if (contentType != IItemBlock::ContentType::Text && contentType != IItemBlock::ContentType::DebugText)
            return false;
    }

    return true;
}

void ComplexMessageItem::onChainsChanged()
{
    for (auto block : Blocks_)
        block->resetClipping();
    BubbleGeometry_ = QRect();
    update();
}

void ComplexMessageItem::loadAvatar()
{
    auto isDefault = false;

    Avatar_ = Logic::GetAvatarStorage()->GetRounded(
        SenderAimidForDisplay_,
        getSenderFriendly(),
        Utils::scale_bitmap(MessageStyle::getAvatarSize()),
        Out isDefault,
        false,
        false);
}

void ComplexMessageItem::onCopyMenuItem(ComplexMessageItem::MenuItemType type)
{
    const bool isCopy = (type == ComplexMessageItem::MenuItemType::Copy);
    const bool isQuote = (type == ComplexMessageItem::MenuItemType::Quote);
    const bool isForward = (type == ComplexMessageItem::MenuItemType::Forward);

    if (isCopy)
    {
        Data::FString::Builder builder;

        if (isQuote || isForward)
            builder %= getQuoteHeader();

        if (isSelected())
            builder %= getSelectedText(false, TextFormat::Raw);
        else
            builder %= getBlocksText(Blocks_, false, TextFormat::Raw);

        auto text = builder.finalize();
        const auto placeholders = getFilesPlaceholders();
        const auto plainString = text.string();
        auto dt = MimeData::toMimeData(std::move(text), mentions_, placeholders);
        QApplication::clipboard()->setMimeData(dt);

        Q_EMIT copy();

        Q_EMIT Utils::InterConnector::instance().setFocusOnInput(ChatAimid_);

        // show toast if not selected
        // if selected - showCopiedToast will be called from another location
        if (!isSelected())
            Utils::showCopiedToast();
    }
    else if (isQuote)
    {
        Q_EMIT quote(getQuotes(isSelected() && !isAllSelected()));
    }
    else if (isForward)
    {
        Q_EMIT forward(getQuotes(isSelected() && !isAllSelected(), true));
    }
}

bool ComplexMessageItem::onDeveloperMenuItemTriggered(QStringView cmd)
{
    im_assert(!cmd.isEmpty());

    if (!GetAppConfig().IsContextMenuFeaturesUnlocked())
    {
        im_assert(!"developer context menu is not unlocked");
        return true;
    }

    if (cmd == u"copy_message_id")
    {
        const auto idStr = QString::number(getId());

        QApplication::clipboard()->setText(idStr);
        qDebug() << "message id" << idStr;

        return true;
    }

    return false;
}

void ComplexMessageItem::onShareButtonClicked()
{
    if (!hoveredSharingBlock_)
        return;

    Q_EMIT forward({ getQuoteFromBlock(hoveredSharingBlock_, false) });
}

void ComplexMessageItem::trackMenu(const QPoint& _globalPos)
{
    cleanupMenu();

    auto menu = new ContextMenu(this);
    Testing::setAccessibleName(menu, qsl("AS ComplexMessageItem contextMenu"));

    const auto replyOnly = isContextMenuReplyOnly();
    const auto msgSent = getId() != -1;

    const auto role = Logic::getContactListModel()->getYourRole(ChatAimid_);
    const auto isDisabled = role == u"notamember" || role == u"readonly";
    const auto notAMember = Logic::getContactListModel()->areYouNotAMember(ChatAimid_);
    const auto editable = !isDisabled && isEditable();
    const auto isThread = Logic::getContactListModel()->isThread(ChatAimid_);
    const auto hasBlockedFiles = hasBlockedFilesharings();

    if (Utils::InterConnector::instance().isMultiselect(ChatAimid_))
    {
        auto hasSelected = true;
        if (auto scrollArea = qobject_cast<MessagesScrollArea*>(parent()))
            hasSelected = (scrollArea->getSelectedCount() != 0);

        auto addAction = [menu, guard = QPointer(this), this](auto _icon, auto _text, auto _func, auto _enabled)
        {
            auto emitSignal = [guard, this, _func]()
            {
                if (guard)
                    Q_EMIT(Utils::InterConnector::instance().*_func)(ChatAimid_);
            };
            menu->addActionWithIconAndFunc(_icon, _text, emitSignal)->setEnabled(_enabled);
        };

        if (!isDisabled)
            addAction(qsl(":/context_menu/reply"), QT_TRANSLATE_NOOP("context_menu", "Reply"), &Utils::InterConnector::multiselectReply, hasSelected && !notAMember && !isThread);

        addAction(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy"), &Utils::InterConnector::multiselectCopy, hasSelected);

        if (!hasBlockedFiles)
            addAction(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Forward"), &Utils::InterConnector::multiselectForward, hasSelected);

        menu->addSeparator();
        addAction(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Delete"), &Utils::InterConnector::multiselectDelete, hasSelected && !notAMember);
        if (!Favorites::isFavorites(ChatAimid_) && !hasBlockedFiles)
        {
            menu->addSeparator();
            addAction(qsl(":/context_menu/favorites"), QT_TRANSLATE_NOOP("context_menu", "Add to favorites"), &Utils::InterConnector::multiselectFavorites, hasSelected && !isThread);
        }
        connect(menu, &ContextMenu::triggered, this, &ComplexMessageItem::onMenuItemTriggered, Qt::QueuedConnection);
        menu->popup(_globalPos);
        return;
    }

    if (!isDisabled)
        menu->addActionWithIcon(qsl(":/context_menu/reply"), QT_TRANSLATE_NOOP("context_menu", "Reply"), makeData(qsl("quote")))->setEnabled(msgSent && !notAMember);

    if (isChat_ && Logic::getContactListModel()->isPublic(ChatAimid_) && !Logic::getContactListModel()->isChannel(ChatAimid_)
        && Logic::getContactListModel()->areYouAdmin(ChatAimid_) && SenderAimid_ != MyInfo()->aimId() && !isThread)
    {
        menu->addActionWithIcon(qsl(":/context_menu/readonly"), QT_TRANSLATE_NOOP("sidebar", "Ban to write"), makeData(qsl("pre_ban")))->setEnabled(false);
        menu->addActionWithIcon(qsl(":/context_menu/block"), QT_TRANSLATE_NOOP("sidebar", "Delete from group"), makeData(qsl("pre_block")))->setEnabled(false);

        const auto seq = Ui::GetDispatcher()->loadChatMembersInfo(ChatAimid_, { SenderAimid_ });
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatMemberInfo, menu, [menu, seq, chatId = ChatAimid_, senderId = SenderAimid_](const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info)
        {
            if (seq != _seq || chatId != _info->AimId_ || !Logic::getContactListModel()->areYouAdmin(chatId))
                return;

            auto it = std::find_if(_info->Members_.cbegin(), _info->Members_.cend(), [senderId](const auto& _m) { return senderId == _m.AimId_; });
            const auto senderRole = (it == _info->Members_.cend()) ? QString() : it->Role_;

            if (menu && menu->isVisible() && !senderRole.isEmpty())
            {
                if (senderRole == u"member")
                    menu->modifyAction(u"pre_ban", qsl(":/context_menu/readonly"), QT_TRANSLATE_NOOP("sidebar", "Ban to write"), makeData(qsl("make_readonly"), senderId), true);
                else if (senderRole == u"readonly")
                    menu->modifyAction(u"pre_ban", qsl(":/context_menu/readonly_off"), QT_TRANSLATE_NOOP("sidebar", "Allow to write"), makeData(qsl("revoke_readonly"), senderId), true);

                if (senderRole != u"admin" && senderRole != u"moder")
                    menu->modifyAction(u"pre_block", qsl(":/context_menu/block"), QT_TRANSLATE_NOOP("sidebar", "Delete from group"), makeData(qsl("block"), senderId), true);

                menu->update();
            }
        });
    }

    const auto pos = mapFromGlobal(_globalPos);
    MenuBlock_ = findBlockUnder(pos);

    auto testFlag = [](auto flags, auto flag) { return flags && bool(*flags & flag); };

    const auto hasBlock = !!MenuBlock_;
    const auto menuFlags = hasBlock ? std::make_optional(MenuBlock_->getMenuFlags(pos)) : std::nullopt;

    const auto isLinkCopyable = testFlag(menuFlags, IItemBlock::MenuFlagLinkCopyable);
    const auto isFileCopyable = testFlag(menuFlags, IItemBlock::MenuFlagFileCopyable);
    const auto isOpenable = testFlag(menuFlags, IItemBlock::MenuFlagOpenInBrowser);
    const auto isCopyable = testFlag(menuFlags, IItemBlock::MenuFlagCopyable);
    const auto isOpenFolder = testFlag(menuFlags, IItemBlock::MenuFlagOpenFolder);
    const auto isRevokeVote = testFlag(menuFlags, IItemBlock::MenuFlagRevokeVote);
    const auto isStopPoll = testFlag(menuFlags, IItemBlock::MenuFlagStopPoll);
    const auto isSpellSuggestable = testFlag(menuFlags, IItemBlock::MenuFlagSpellItems);
    const auto isEditTask = testFlag(menuFlags, IItemBlock::MenuFlagEditTask);

    bool hasCopyableEmail = false;
    QString link;

    if (hasBlock)
    {
        if (isLinkCopyable && MenuBlock_->getContentType() != IItemBlock::ContentType::Text)
        {
            menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link")))->setEnabled(!replyOnly);
        }
        else
        {
            // check if message has a link without preview
            link = MenuBlock_->linkAtPos(_globalPos).url_;

            bool hasCopyableLink = false;
            auto selectedText = getSelectedText(false);

            if (!link.isEmpty() && (selectedText.string() == link || selectedText.isEmpty()))
            {
                static const QRegularExpression re(
                    qsl("^mailto:"),
                    QRegularExpression::UseUnicodePropertiesOption);

                link.remove(re);
                Utils::UrlParser parser;
                parser.process(link);
                if (parser.hasUrl())
                {
                    if (parser.getUrl().is_email())
                        hasCopyableEmail = true;
                    else
                        hasCopyableLink = true;
                }
            }

            if (hasCopyableLink)
                menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy link"),
                                        makeData(qsl("copy_link"), link))->setEnabled(!replyOnly);
            if (hasCopyableEmail)
                menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy email"),
                                        makeData(qsl("copy_email"), link))->setEnabled(!replyOnly);
            else if (isCopyable)
                menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy"),
                                        makeData(qsl("copy")))->setEnabled(!replyOnly);
        }
    }

    if (hasBlock && isFileCopyable)
        menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy to clipboard"), makeData(qsl("copy_file")))->setEnabled(!replyOnly && msgSent);

    if (isRevokeVote)
        menu->addActionWithIcon(qsl(":/context_menu/cancel"), QT_TRANSLATE_NOOP("context_menu", "Revoke vote"), makeData(qsl("revoke_vote")))->setEnabled(!replyOnly);

    if (isStopPoll)
        menu->addActionWithIcon(qsl(":/context_menu/stop"), QT_TRANSLATE_NOOP("context_menu", "Stop poll"), makeData(qsl("stop_poll")))->setEnabled(!replyOnly);

    if (isEditTask)
        menu->addActionWithIcon(qsl(":/context_menu/edit"), QT_TRANSLATE_NOOP("context_menu", "Edit task"), makeData(qsl("edit_task")))->setEnabled(!replyOnly);

    if (!hasBlockedFiles)
        menu->addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward")))->setEnabled(!replyOnly && msgSent && !isThread);
    menu->addActionWithIcon(qsl(":/context_menu/select"), QT_TRANSLATE_NOOP("context_menu", "Select"), makeData(qsl("select")))->setEnabled(!isThread);

    if (Logic::getContactListModel()->areYouAdmin(ChatAimid_))
    {
        const Data::DlgState dlg = Logic::getRecentsModel()->getDlgState(ChatAimid_);
        if (dlg.pinnedMessage_ && dlg.pinnedMessage_->Id_ == Id_)
            menu->addActionWithIcon(qsl(":/context_menu/unpin"), QT_TRANSLATE_NOOP("context_menu", "Unpin"), makeData(qsl("unpin")))->setEnabled(!replyOnly && msgSent && !isThread);
        else
            menu->addActionWithIcon(qsl(":/context_menu/pin"), QT_TRANSLATE_NOOP("context_menu", "Pin"), makeData(qsl("pin")))->setEnabled(!replyOnly && msgSent && !isThread);
    }

    if (hasBlock && isFileCopyable && !MenuBlock_->isBlockedFileSharing())
        menu->addActionWithIcon(qsl(":/context_menu/download"), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData(qsl("save_as")))->setEnabled(!replyOnly && msgSent);

    if (isOpenable)
        menu->addActionWithIcon(qsl(":/context_menu/browser"), QT_TRANSLATE_NOOP("context_menu", "Open in browser"), makeData(qsl("open_in_browser")))->setEnabled(!replyOnly);

    if (hasBlock && isOpenFolder)
        menu->addActionWithIcon(qsl(":/context_menu/open_folder"), QT_TRANSLATE_NOOP("context_menu", "Show in folder"), makeData(qsl("open_folder")))->setEnabled(!replyOnly);


    if (editable)
        menu->addActionWithIcon(qsl(":/context_menu/edit"), QT_TRANSLATE_NOOP("context_menu", "Edit"), makeData(qsl("edit")))->setEnabled(!replyOnly && !notAMember);

    if (hasCopyableEmail && !link.isEmpty())
        menu->addActionWithIcon(qsl(":/context_menu/goto"), QT_TRANSLATE_NOOP("context_menu", "Go to profile"), makeData(qsl("open_profile"), link))->setEnabled(!replyOnly);

    menu->addSeparator();

    const auto deleteText = Favorites::isFavorites(ChatAimid_) ? QT_TRANSLATE_NOOP("context_menu", "Delete") : QT_TRANSLATE_NOOP("context_menu", "Delete for me");
    menu->addActionWithIcon(qsl(":/context_menu/delete"), deleteText, makeData(qsl("delete")))->setEnabled(!replyOnly && !notAMember && !isThread);

    if ((Logic::getContactListModel()->areYouAdmin(ChatAimid_) || (!isDisabled && isOutgoing())) && !Favorites::isFavorites(ChatAimid_))
        menu->addActionWithIcon(qsl(":/context_menu/deleteall"), QT_TRANSLATE_NOOP("context_menu", "Delete for all"), makeData(qsl("delete_all")))->setEnabled(!replyOnly && !notAMember);

    if (GetAppConfig().IsContextMenuFeaturesUnlocked())
    {
        menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Message ID"), makeData(qsl("dev:copy_message_id")))->setEnabled(!replyOnly);
        if (hasBlock)
        {
            switch (MenuBlock_->getContentType())
            {
            case IItemBlock::ContentType::FileSharing:
            case IItemBlock::ContentType::Sticker:
                menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Filesharing ID"), makeData(qsl("dev:copy_fs_id")))->setEnabled(!replyOnly);
                menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Filesharing full link"), makeData(qsl("dev:copy_fs_link")))->setEnabled(!replyOnly);
                break;
            case IItemBlock::ContentType::Poll:
                menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Poll ID"), makeData(qsl("dev:copy_poll_id")))->setEnabled(!replyOnly);
                break;
            case IItemBlock::ContentType::Task:
                menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Task ID"), makeData(qsl("dev:copy_task_id")))->setEnabled(!replyOnly);
                break;
            default:
                break;
            }
        }
    }

    if (hasBlock && ((MenuBlock_->getContentType() == IItemBlock::ContentType::Sticker && !MenuBlock_->getStickerId().fsId_.value_or(Utils::FileSharingId{}).sourceId) ||
        (isChat_ && SenderAimid_ != MyInfo()->aimId() && !Logic::getContactListModel()->areYouAdmin(ChatAimid_))))
    {
        menu->addSeparator();
        menu->addActionWithIcon(qsl(":/context_menu/spam"), QT_TRANSLATE_NOOP("context_menu", "Report"), makeData(qsl("report")))->setEnabled(!replyOnly);
    }

    if (!hasBlockedFiles && !Favorites::isFavorites(ChatAimid_))
    {
        menu->addSeparator();
        menu->addActionWithIcon(qsl(":/context_menu/favorites"), QT_TRANSLATE_NOOP("context_menu", "Add to favorites"), makeData(qsl("add_to_favorites")))->setEnabled(!replyOnly && msgSent && !isThread);
    }

    connect(menu, &ContextMenu::triggered, this, &ComplexMessageItem::onMenuItemTriggered, Qt::QueuedConnection);
    connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
    connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

    auto showMenuCallback = [menu, _globalPos]()
    {
        menu->popup(_globalPos);
    };

    if (isSpellSuggestable && editable)
    {
        if (const auto word = MenuBlock_->getWordAt(_globalPos); !word)
        {
            showMenuCallback();
        }
        else
        {
            auto onResult = [this, weakThis = QPointer(this), weakMenu = QPointer(menu), _globalPos, word = *word, showMenuCallback = std::move(showMenuCallback), editable](std::optional<spellcheck::Suggests> _suggests, spellcheck::WordStatus _status) mutable
            {
                Q_UNUSED(this); // 'this' is only for VS
                if (!weakThis || !weakMenu)
                    return;

                const auto scopedExit = Utils::ScopeExitT(std::move(showMenuCallback));
                const auto actions = weakMenu->actions();
                QList<QAction*> suggestActions;

                if (_status.isCorrect)
                {
                    if (_status.isAdded)
                    {
                        auto a = new TextAction(QT_TRANSLATE_NOOP("context_menu", "Remove from dictionary"), weakMenu);
                        QObject::connect(a, &QAction::triggered, weakThis.data(), [weakThis, word]()
                        {
                            if (!weakThis)
                                return;
                            spellcheck::Spellchecker::instance().removeWord(word.string());
                        });
                        suggestActions.push_back(a);
                        auto separator = new QAction(weakMenu);
                        separator->setSeparator(true);
                        suggestActions.push_back(separator);
                        weakMenu->insertActions(actions.isEmpty() ? nullptr : actions.front(), suggestActions);
                    }
                    return;
                }

                auto addAction = new TextAction(QT_TRANSLATE_NOOP("context_menu", "Add to dictionary"), weakMenu);
                QObject::connect(addAction, &QAction::triggered, weakThis.data(), [weakThis, word]()
                {
                    if (!weakThis)
                        return;
                    spellcheck::Spellchecker::instance().addWord(word.string());
                });
                suggestActions.push_back(addAction);

                auto ignoreAction = new TextAction(QT_TRANSLATE_NOOP("context_menu", "Ignore"), weakMenu);
                QObject::connect(ignoreAction, &QAction::triggered, weakThis.data(), [weakThis, word]()
                {
                    if (!weakThis)
                        return;
                    spellcheck::Spellchecker::instance().ignoreWord(word.string());
                });
                suggestActions.push_back(ignoreAction);

                auto separator = new QAction(weakMenu);
                separator->setSeparator(true);
                suggestActions.push_back(separator);

                if (!_suggests || _suggests->empty() || !editable)
                {
                    weakMenu->insertActions(actions.isEmpty() ? nullptr : actions.front(), suggestActions);
                    return;
                }
                suggestActions.reserve(_suggests->size() + 1);
                for (auto&& s : std::move(*_suggests))
                {
                    auto a = new TextAction(s, weakMenu);
                    QObject::connect(a, &QAction::triggered, weakThis.data(), [word, s = std::move(s), _globalPos, weakThis]()
                    {
                        if (!weakThis)
                            return;

                        if (weakThis->MenuBlock_ && weakThis->MenuBlock_->replaceWordAt(word, borrowStylesFromWord(word, s), _globalPos))
                            weakThis->sendInstantEdit();
                    });

                    suggestActions.push_back(a);
                }
                if (!actions.isEmpty())
                {
                    auto separator = new QAction(weakMenu);
                    separator->setSeparator(true);
                    suggestActions.push_back(separator);
                }
                weakMenu->insertActions(actions.isEmpty() ? nullptr : actions.front(), suggestActions);
            };

            spellcheck::Spellchecker::instance().getSuggestsIfNeeded(word->string(), onResult);
        }
    }
    else
    {
        showMenuCallback();
    }
}

void ComplexMessageItem::updateSenderControlColor()
{
    if (!Sender_)
        return;

    Sender_->setColor(Utils::getNameColor(SenderAimidForDisplay_));
}

void ComplexMessageItem::updateShareButtonGeometry()
{
    if (!shareButton_)
        return;

    if (!hoveredSharingBlock_ || Utils::InterConnector::instance().isMultiselect(ChatAimid_))
        hideShareButtonAnimated();
    else
        showShareButtonAnimated();
}

void ComplexMessageItem::updateTimeAnimated()
{
    if (!TimeWidget_ || !TimeWidget_->isUnderlayVisible())
        return;

    if (!hoveredBlock_)
        hideTimeAnimated();
    else
        showTimeAnimated();
}

void ComplexMessageItem::showShareButtonAnimated()
{
    if (!hoveredSharingBlock_ || !shareButton_)
        return;

    shareButton_->move(Layout_->getShareButtonPos(*hoveredSharingBlock_, hoveredSharingBlock_->isBubbleRequired()));

    animateShareButton(QAbstractAnimation::Forward);
    shareButton_->show();
}

void ComplexMessageItem::hideShareButtonAnimated()
{
    if (!shareButton_)
        return;

    animateShareButton(QAbstractAnimation::Backward);
}

void ComplexMessageItem::animateShareButton(QAbstractAnimation::Direction _direction)
{
    if (!shareButton_)
        return;

    if (!shareButtonAnimation_)
    {
        shareButtonAnimation_ = new QVariantAnimation(this);
        shareButtonAnimation_->setDuration(MessageStyle::getHiddenControlsAnimationTime().count());
        shareButtonAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        shareButtonAnimation_->setStartValue(0.0);
        shareButtonAnimation_->setEndValue(1.0);

        connect(shareButtonAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& _value)
        {
            shareButtonOpacityEffect_->setOpacity(_value.toDouble());
        });
        connect(shareButtonAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            if (shareButtonAnimation_->direction() == QAbstractAnimation::Backward)
                shareButton_->hide();
        });
    }

    const auto isRunning = shareButtonAnimation_->state() == QAbstractAnimation::Running;
    if (isRunning && shareButtonAnimation_->direction() == _direction)
        return;

    const auto cur = isRunning ? shareButtonAnimation_->currentTime() : 0.;
    shareButtonAnimation_->stop();
    shareButtonAnimation_->setCurrentTime(shareButtonAnimation_->totalDuration() - cur);
    shareButtonAnimation_->setDirection(_direction);
    shareButtonAnimation_->start();
}

void ComplexMessageItem::showTimeAnimated()
{
    if (!TimeWidget_)
        return;

    const auto initialPos = Layout_->getInitialTimePosition();
    const QPoint endPos(initialPos);
    const QPoint startPos(initialPos.x(), TimeWidget_->isVisible() ? TimeWidget_->pos().y() : initialPos.y() + getTimeAnimationDistance());

    TimeWidget_->move(startPos);
    TimeWidget_->show();

    if (timeOpacityEffect_)
        timeOpacityEffect_->setOpacity(0.0);
    animateTime(startPos.y(), endPos.y(), QAbstractAnimation::Forward);
}

void ComplexMessageItem::hideTimeAnimated()
{
    if (!TimeWidget_)
        return;

    const auto initialPos = Layout_->getInitialTimePosition();
    const QPoint startPos(TimeWidget_->pos());
    const QPoint endPos(initialPos.x(), initialPos.y() + getTimeAnimationDistance());

    if (timeOpacityEffect_)
        timeOpacityEffect_->setOpacity(1.0);
    animateTime(startPos.y(), endPos.y(), QAbstractAnimation::Backward);
}

int ComplexMessageItem::getTimeAnimationDistance() const
{
    if (!TimeWidget_)
        return 0;

    return TimeWidget_->getVerMargin() + MessageStyle::getHiddenControlsShift();
}

void ComplexMessageItem::animateTime(const int _startPosY, const int _endPosY, QAbstractAnimation::Direction _direction)
{
    if (!TimeWidget_)
        return;

    if (!timeAnimation_)
        timeAnimation_ = new QVariantAnimation(this);

    timeAnimation_->stop();
    timeAnimation_->setStartValue(_startPosY);
    timeAnimation_->setEndValue(_endPosY);
    timeAnimation_->setDuration(MessageStyle::getHiddenControlsAnimationTime().count());
    timeAnimation_->disconnect(this);
    connect(timeAnimation_, &QVariantAnimation::valueChanged, this, [_startPosY, _endPosY, _direction, this](const QVariant& value)
    {
        const auto val = value.toDouble();
        TimeWidget_->move(TimeWidget_->pos().x(), val);
        const auto denominator = std::max(1, _endPosY - _startPosY);
        const auto opacity = std::abs((val - _startPosY) / denominator);
        if (timeOpacityEffect_)
            timeOpacityEffect_->setOpacity(_direction == QAbstractAnimation::Backward ? 1.0 - opacity : opacity);
    });
    connect(timeAnimation_, &QVariantAnimation::finished, this, [_direction, this]()
    {
        if (_direction == QAbstractAnimation::Backward)
            TimeWidget_->hide();
    });
    timeAnimation_->start();
}

Data::Quote ComplexMessageItem::getQuoteFromBlock(IItemBlock* _block, const bool _selectedTextOnly) const
{
    if (!_block)
        return Data::Quote();

    Data::Quote quote = _block->getQuote();
    if (!quote.isEmpty())
    {
        if (_selectedTextOnly)
            quote.text_ = _block->getSelectedText(false, IItemBlock::TextDestination::quote);

        quote.files_ = files_;
    }
    else
    {
        quote.text_ =
            _selectedTextOnly
            ? _block->getSelectedText(false, IItemBlock::TextDestination::quote)
            : _block->getSourceText();
        quote.senderId_ = isOutgoing() ? MyInfo()->aimId() : _block->getSenderAimid();
        quote.chatId_ = getChatAimid();
        if (Logic::getContactListModel()->isChannel(quote.chatId_))
            quote.senderId_ = quote.chatId_;
        quote.chatStamp_ = Logic::getContactListModel()->getChatStamp(quote.chatId_);
        quote.time_ = getTime();
        quote.msgId_ = getId();
        quote.mentions_ = mentions_;
        quote.mediaType_ = _block->getMediaType();
        quote.sharedContact_ = buddy().sharedContact_;
        quote.geo_ = buddy().geo_;
        quote.files_ = getFilesPlaceholders();
        quote.poll_ = buddy().poll_;
        if (!_selectedTextOnly)
            quote.task_ = buddy().task_;

        if (quote.poll_)
            quote.mediaType_ = Ui::MediaType::mediaTypePoll;
        if (quote.task_)
            quote.mediaType_ = Ui::MediaType::mediaTypeTask;

        if (quote.isEmpty())
            return Data::Quote();

        QString senderFriendly = getSenderFriendly();
        if (senderFriendly.isEmpty())
            senderFriendly = Logic::GetFriendlyContainer()->getFriendly(quote.senderId_);
        if (isOutgoing())
            senderFriendly = MyInfo()->friendly();
        quote.senderFriendly_ = std::move(senderFriendly);

        if (const auto stickerId = _block->getStickerId(); stickerId.obsoleteId_)
        {
            quote.setId_ = stickerId.obsoleteId_->setId_;
            quote.stickerId_ = stickerId.obsoleteId_->id_;
        }

        if (!Url_.isEmpty() && !Description_.isEmpty() && (!_selectedTextOnly || Utils::InterConnector::instance().isMultiselect(ChatAimid_)))
        {
            quote.url_ = Url_;
            quote.description_ = Description_;
        }
    }

    quote.text_ = Utils::setFilesPlaceholders(quote.text_, files_);

    if (_selectedTextOnly)
    {
        quote.type_ = Data::Quote::Type::quote;
        return quote;
    }

    switch (_block->getContentType())
    {
    case IItemBlock::ContentType::FileSharing:
        quote.type_ = Data::Quote::Type::file_sharing;
        break;

    case IItemBlock::ContentType::Link:
        quote.type_ = Data::Quote::Type::link;
        break;

    case IItemBlock::ContentType::Quote:
        quote.type_ = Data::Quote::Type::quote;
        break;

    case IItemBlock::ContentType::Text:
        quote.type_ = Data::Quote::Type::text;
        break;

    default:
        quote.type_ = Data::Quote::Type::other;
        break;
    }

    return quote;
}

void ComplexMessageItem::updateTimeWidgetUnderlay()
{
    if (TimeWidget_ && !Blocks_.empty())
    {
        static constexpr MediaType plainTypes[] =
        {
            MediaType::noMedia,
            MediaType::mediaTypePtt,
            MediaType::mediaTypeFileSharing,
            MediaType::mediaTypeContact,
            MediaType::mediaTypeGeo,
            MediaType::mediaTypePoll,
            MediaType::mediaTypeTask,
        };

        const auto mediaType = Blocks_.back()->getMediaType();
        const auto withUnderlay = getBlockCount() == 1 && std::none_of(std::begin(plainTypes), std::end(plainTypes), [mediaType](const auto _t) { return mediaType == _t; });

        TimeWidget_->setUnderlayVisible(withUnderlay);

        if (timeAnimation_)
            timeAnimation_->stop();

        if (withUnderlay)
        {
            if (!timeOpacityEffect_)
                timeOpacityEffect_ = new QGraphicsOpacityEffect(TimeWidget_);

            TimeWidget_->setGraphicsEffect(timeOpacityEffect_);
            TimeWidget_->hide();
        }
        else
        {
            TimeWidget_->setGraphicsEffect(nullptr);
            TimeWidget_->show();
            TimeWidget_->move(Layout_->getInitialTimePosition());
        }
    }
}

bool ComplexMessageItem::isSpellCheckIsOn()
{
    return Features::isSpellCheckEnabled() && get_gui_settings()->get_value<bool>(settings_spell_check, settings_spell_check_default());
}

QRect ComplexMessageItem::getBubbleGeometry() const
{
    return BubbleGeometry_;
}

QRect ComplexMessageItem::messageRect() const
{
    return isBubbleRequired() ? Layout_->getBubbleRect() : Layout_->getBlocksContentRect();
}

QRect ComplexMessageItem::cornerMenuContentRect() const
{
    if (isSingleSticker())
        return Layout_->getBlocksContentRect();

    return Layout_->getBubbleRect();
}

ReactionsPlateType ComplexMessageItem::reactionsPlateType() const
{
    if (isSingleSticker())
        return ReactionsPlateType::Media;
    return ReactionsPlateType::Regular;

}

void ComplexMessageItem::setSpellErrorsVisible(bool _visible)
{
    for (auto& block : Blocks_)
        block->setSpellErrorsVisible(_visible);
}

void ComplexMessageItem::setProgress(const Utils::FileSharingId& _fsId, const int32_t _value)
{
    for (auto& block : Blocks_)
    {
        if (block->setProgress(_fsId, _value))
            break;
    }
}

QRect ComplexMessageItem::avatarRect() const
{
    return Layout_->getAvatarRect();
}

void ComplexMessageItem::fillGalleryData(Utils::GalleryData& _data)
{
    _data.time_ = Time_;
    _data.sender_ = isChat() ? SenderAimid_ : (isOutgoing() ? Ui::MyInfo()->aimId() : ChatAimid_);
    _data.caption_ = Description_.string();
    _data.fromThreadFeed_ = isThreadFeedMessage();
}

std::optional<Data::FileSharingMeta> ComplexMessageItem::getMeta(const Utils::FileSharingId& _id) const
{
    for (const auto& b : Blocks_)
    {
        auto meta = b->getMeta(_id);
        if (meta)
            return meta;
    }

    return std::nullopt;
}

bool ComplexMessageItem::canBeThreadParent() const
{
    return !isSingleSticker();
}

void ComplexMessageItem::updateFileStatus(FileStatus _status)
{
    for (auto b : Blocks_)
        b->updateFileStatus(_status);
}

void ComplexMessageItem::setTimeWidgetVisible(const bool _visible)
{
    if (TimeWidget_)
        TimeWidget_->setVisible(_visible);
}

bool ComplexMessageItem::needsAvatar() const
{
    if (isSingleSticker())
        return true;

    if (const auto type = getMediaType(); type != MediaType::noMedia && type != MediaType::mediaTypeFileSharing)
        return true;

    return false;
}

void ComplexMessageItem::loadSnippetsMetaInfo()
{
    if (!config::get().is_on(config::features::snippet_in_chat))
        return;

    connect(GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded, this, &ComplexMessageItem::onLinkMetainfoMetaDownloaded, Qt::UniqueConnection);

    for (auto& snippet : std::exchange(snippetsWaitingForInitialization_, {}))
    {
        auto seq = Ui::GetDispatcher()->downloadLinkMetainfo(snippet.link_);
        snippetsWaitingForMeta_[seq] = std::move(snippet);
    }
}

void ComplexMessageItem::sendInstantEdit()
{
    Data::MessageBuddy msg = buddy();

    if (!Description_.isEmpty())
    {
        for (auto it = Blocks_.crbegin(); it != Blocks_.crend(); ++it)
        {
            const auto block = *it;
            const auto type = block->getContentType();
            if (type == IItemBlock::ContentType::DebugText)
                continue;
            if (type == IItemBlock::ContentType::Text)
            {
                msg.SetDescription(block->getTextInstantEdit());
                break;
            }
        }
    }

    msg.SetText(getEditableText(InstantEdit::Yes));
    GetDispatcher()->updateMessage(msg);
}

bool ComplexMessageItem::isSingleBlockOfType(IItemBlock::ContentType _type) const
{
    const auto blockCount = getBlockCount(uint32_t(IItemBlock::ContentType::ExcludeDebug_Mask));
    const auto allBlockCount = getBlockCount();
    return blockCount == 1 && Blocks_[allBlockCount - blockCount]->getContentType() == _type;
}

bool ComplexMessageItem::shareButtonEnabled() const
{
    return !isThreadFeedMessage() && !Logic::getContactListModel()->isThread(ChatAimid_);
}

bool ComplexMessageItem::hasBlockedFilesharings() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](auto b) { return b->isBlockedFileSharing(); });
}

bool ComplexMessageItem::hasFilesharingsWithStatus() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](auto b) { return b->isFileSharingWithStatus(); });
}

ComplexMessageItem::BubbleHorMargins ComplexMessageItem::getBubbleHorMarginAdjust() const
{
    BubbleHorMargins margins;
    if (isSingleFilesharing() && hasFilesharingsWithStatus())
        margins.right_ = Utils::scale_value(-4);

    return margins;
}

MessageTimeWidget* ComplexMessageItem::getTimeWidget() const
{
    return TimeWidget_;
}

void ComplexMessageItem::cancelRequests()
{
    for (auto& b : Blocks_)
        b->cancelRequests();
}

void ComplexMessageItem::setUrlAndDescription(const QString& _url, const QString& _description)
{
    Url_ = _url;
    Description_ = _description;
}

void ComplexMessageItem::setUrlAndDescription(const QString& _url, const Data::FString& _description)
{
    Url_ = _url;
    Description_ = _description;
}

int ComplexMessageItem::getSenderDesiredWidth() const noexcept
{
    return desiredSenderWidth_;
}

void ComplexMessageItem::setQuoteSelection()
{
    for (auto& val : Blocks_)
        val->setQuoteSelection();

    QuoteAnimation_->startQuoteAnimation();
}

void ComplexMessageItem::highlightText(const highlightsV& _highlights)
{
    for (auto& b : Blocks_)
        b->highlight(_highlights);
}

void ComplexMessageItem::resetHighlight()
{
    for (auto& b : Blocks_)
        b->removeHighlight();
}

void ComplexMessageItem::setDeliveredToServer(const bool _isDeliveredToServer)
{
    if (isOutgoing())
        deliveredToServer_ = _isDeliveredToServer;
}

bool ComplexMessageItem::isQuoteAnimation() const
{
    return bQuoteAnimation_;
}

void ComplexMessageItem::setQuoteAnimation()
{
    bQuoteAnimation_ = true;
}

bool ComplexMessageItem::isObserveToSize() const
{
    return bObserveToSize_;
}

void ComplexMessageItem::onObserveToSize()
{
    bObserveToSize_ = true;
}

UI_COMPLEX_MESSAGE_NS_END

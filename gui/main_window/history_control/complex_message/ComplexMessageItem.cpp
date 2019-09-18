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
#include "../../../utils/log/log.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/stat_utils.h"
#include "../../../my_info.h"
#include "../StickerInfo.h"

#include "../../contact_list/ContactList.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/RecentsModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"
#include "../../friendly/FriendlyContainer.h"

#include "../../MainPage.h"
#include "../../ReportWidget.h"

#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"
#include "../MessagesScrollArea.h"

#include "ComplexMessageItemLayout.h"
#include "IItemBlockLayout.h"
#include "IItemBlock.h"
#include "Selection.h"

#include "TextBlock.h"
#include "DebugTextBlock.h"

#include "ComplexMessageItem.h"
#include "../../mediatype.h"

#include "styles/ThemeParameters.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    BlockSelectionType evaluateBlockSelectionType(const QRect &blockGeometry, const QPoint &selectionTop, const QPoint &selectionBottom);

    QMap<QString, QVariant> makeData(const QString& command, const QString& arg = QString())
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;

        if (!arg.isEmpty())
            result[qsl("arg")] = arg;

        return result;
    }
}

ComplexMessageItem::ComplexMessageItem(
    QWidget *parent,
    const int64_t id,
    const int64_t prevId,
    const QString& _internalId,
    const QDate date,
    const QString &chatAimid,
    const QString &senderAimid,
    const QString &senderFriendly,
    const QString &sourceText,
    const Data::MentionMap& _mentions,
    const bool isOutgoing)
    : MessageItemBase(parent)

    , ChatAimid_(chatAimid)
    , Date_(date)
    , FullSelectionType_(BlockSelectionType::Invalid)
    , hoveredBlock_(nullptr)
    , hoveredSharingBlock_(nullptr)
    , Id_(id)
    , PrevId_(prevId)
    , internalId_(_internalId)
    , Initialized_(false)
    , IsOutgoing_(isOutgoing)
    , IsDeliveredToServer_(true)
    , Layout_(nullptr)
    , MenuBlock_(nullptr)
    , MouseLeftPressedOverAvatar_(false)
    , MouseLeftPressedOverSender_(false)
    , MouseRightPressedOverItem_(false)
    , desiredSenderWidth_(0)
    , SenderAimid_(senderAimid)
    , SenderFriendly_(senderFriendly)
    , shareButton_(nullptr)
    , shareButtonOpacityEffect_(nullptr)
    , SourceText_(sourceText)
    , TimeWidget_(nullptr)
    , timeOpacityEffect_(nullptr)
    , Time_(-1)
    , bQuoteAnimation_(false)
    , bObserveToSize_(false)
    , bubbleHovered_(false)
    , hasTrailingLink_(false)
    , mentions_(_mentions)
    , isDrawFullBubbleSelected_(false)
{
    assert(Id_ >= -1);
    assert(!SenderAimid_.isEmpty());
    assert(!ChatAimid_.isEmpty());

    if (!isHeadless())
    {
        assert(Date_.isValid());
    }

    setContact(ChatAimid_);

    Layout_ = new ComplexMessageItemLayout(this);
    setLayout(Layout_);

    connectSignals();
}

ComplexMessageItem::~ComplexMessageItem()
{
    if (!PressPoint_.isNull())
        emit pressedDestroyed();

    shareButtonAnimation_.finish();
    timeAnimation_.finish();
}

void ComplexMessageItem::clearSelection()
{
    assert(!Blocks_.empty());

    FullSelectionType_ = BlockSelectionType::None;

    for (auto block : Blocks_)
        block->clearSelection();
}

QString ComplexMessageItem::formatRecentsText() const
{
    assert(!Blocks_.empty());

    if (Blocks_.empty())
        return qsl("warning: invalid message block");

    QString textOnly;
    textOnly.reserve(1024);

    QString tmp;

    unsigned textBlocks = 0, fileSharingBlocks = 0, linkBlocks = 0, quoteBlocks = 0, otherBlocks = 0;

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
            ++linkBlocks;
            tmp = b->formatRecentsText();
            if (Blocks_.size() - textBlocks - quoteBlocks != 1 || !textOnly.contains(tmp))
            {
                if (linkBlocks)
                    textOnly += QChar::Space;
                textOnly += tmp;
            }
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

        case IItemBlock::ContentType::Other:
        default:
            ++otherBlocks;
            textOnly += b->formatRecentsText();
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
    else if (!textOnly.isEmpty() && ((linkBlocks || quoteBlocks) || (fileSharingBlocks && (linkBlocks || otherBlocks || textBlocks || quoteBlocks))))
    {
        res = textOnly;
    }
    else
    {
        for (const auto& _block : Blocks_)
        {
            if (fileSharingBlocks && _block != Blocks_.front())
                res += QChar::Space;
            res += _block->formatRecentsText();
        }
    }

    if (!mentions_.empty())
        return Utils::convertMentions(res, mentions_);

    return res;
}

MediaType ComplexMessageItem::getMediaType() const
{
    if (!Blocks_.empty())
    {
        for (const auto _block : Blocks_)
        {
            const auto contentType = _block->getContentType();

            if (contentType == IItemBlock::ContentType::Quote || contentType == IItemBlock::ContentType::DebugText)
                continue;

            return _block->getMediaType();
        }
    }

    return MediaType::noMedia;
}

void ComplexMessageItem::updateFriendly(const QString& _aimId, const QString& _friendly)
{
    if (!_aimId.isEmpty() && !_friendly.isEmpty())
    {
        bool needUpdate = false;
        if (isChat() && SenderAimid_ == _aimId && SenderFriendly_ != _friendly)
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
    assert(!Blocks_.empty());

    std::vector<PinPlaceholderType> types;
    types.reserve(Blocks_.size());
    for (const auto b : Blocks_)
    {
        if (b->getContentType() == IItemBlock::ContentType::Quote)
            return PinPlaceholderType::None;

        types.emplace_back(b->getPinPlaceholder());
    }

    types.erase(std::remove_if(types.begin(), types.end(), [](auto _type)
    {
        return _type == PinPlaceholderType::None;
    }), types.end());

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
    for (const auto b : Blocks_)
    {
        switch (b->getContentType())
        {
        case IItemBlock::ContentType::Link:
            return b->getSourceText();

        case IItemBlock::ContentType::FileSharing:
            return b->getSourceText();

        default:
            break;
        }
    }

    return QString();
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
    return getSenderFriendly() % ql1s(" (") % QDateTime::fromTime_t(Time_).toString(qsl("dd.MM.yyyy hh:mm")) % ql1s("):\n");
}

QString ComplexMessageItem::getSelectedText(const bool isQuote) const
{
    QString text;
    const auto isEmptySelection = (FullSelectionType_ == BlockSelectionType::None || FullSelectionType_ == BlockSelectionType::Invalid);
    if (isEmptySelection && !isSelected())
        return text;

    text.reserve(1024);

    if (isQuote)
        text += getQuoteHeader();

    const auto isFullSelection = (FullSelectionType_ == BlockSelectionType::Full);
    if (isFullSelection)
    {
        if (!Blocks_.empty() && Blocks_.front()->getContentType() == IItemBlock::ContentType::DebugText)
            text += Blocks_.front()->getSelectedText(true) % QChar::LineFeed;

        if (!Description_.isEmpty())
            text += Url_ % QChar::LineFeed % Description_;
        else
            text += getBlocksText(Blocks_, true);

        return text;
    }

    const auto selectedText = getBlocksText(Blocks_, true);
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
    assert(Date_.isValid());
    return Date_;
}

const QString& ComplexMessageItem::getSenderAimid() const
{
    assert(!SenderAimid_.isEmpty());
    return SenderAimid_;
}

bool ComplexMessageItem::isOutgoing() const
{
    return IsOutgoing_;
}

bool ComplexMessageItem::isEditable() const
{
    if (MessageItemBase::isEditable())
    {
        assert(!Blocks_.empty());

        auto textBlocks = 0;
        auto fileBlocks = 0;
        auto linkBlocks = 0;
        auto quoteBlocks = 0;
        auto otherBlocks = 0;
        auto forward = false;

        for (const auto block : Blocks_)
        {
            switch (block->getContentType())
            {
            case IItemBlock::ContentType::DebugText:
                continue;

            case IItemBlock::ContentType::Text:
                textBlocks++;
                break;
            case IItemBlock::ContentType::FileSharing:
                fileBlocks++;
                break;
            case IItemBlock::ContentType::Link:
                linkBlocks++;
                break;
            case IItemBlock::ContentType::Quote:
                quoteBlocks++;
                if (!forward && block->getQuote().isForward_)
                    forward = true;
                break;
            default:
                otherBlocks++;
                break;
            }
        }

        const auto textOnly = textBlocks && !(fileBlocks || linkBlocks || otherBlocks);
        const auto textAndMedia = textBlocks && (linkBlocks || fileBlocks);
        const auto linkOnly = linkBlocks;
        const auto quoteAndText = !forward && quoteBlocks && textBlocks && !(fileBlocks || linkBlocks || otherBlocks);
        return textOnly || textAndMedia || linkOnly || quoteAndText;
    }

    return false;
}

bool ComplexMessageItem::isSimple() const
{
    return Blocks_.size() == 1 && Blocks_.front()->isSimple();
}

bool ComplexMessageItem::isSingleSticker() const
{
    return Blocks_.size() == 1 && Blocks_.front()->getContentType() == IItemBlock::ContentType::Sticker;
}

void ComplexMessageItem::onHoveredBlockChanged(IItemBlock *newHoveredBlock)
{
    if (hoveredBlock_ == newHoveredBlock)
        return;

    hoveredBlock_ = newHoveredBlock;
    emit hoveredBlockChanged();

    if (newHoveredBlock && !newHoveredBlock->isSharingEnabled())
    {
        if (newHoveredBlock->containSharingBlock())
            newHoveredBlock = hoveredSharingBlock_;
        else
            newHoveredBlock = nullptr;
    }

    updateTimeAnimated();

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

    for (auto block : Blocks_)
        block->updateFonts();

    if (TimeWidget_)
        TimeWidget_->updateText();

    updateSize();
}

void ComplexMessageItem::onActivityChanged(const bool isActive)
{
    for (auto block : Blocks_)
        block->onActivityChanged(isActive);

    const auto isInit = (isActive && !Initialized_);
    if (isInit)
    {
        Initialized_ = true;
        initialize();
    }
}

void ComplexMessageItem::onVisibilityChanged(const bool isVisible)
{
    for (auto block : Blocks_)
        block->onVisibilityChanged(isVisible);
}

void ComplexMessageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    for (auto block : Blocks_)
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
}


void ComplexMessageItem::replaceBlockWithSourceText(IItemBlock *block)
{
    const auto scopedExit = Utils::ScopeExitT([this]() { emit layoutChanged(QPrivateSignal()); });
    cleanupBlock(block);

    for (auto b : Blocks_)
    {
        if (b->replaceBlockWithSourceText(block))
        {
            Layout_->onBlockSizeChanged();
            return;
        }
    }

    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
    {
        assert(!"block is missing");
        return;
    }

    auto& existingBlock = *iter;
    assert(existingBlock);

    auto textBlock = new TextBlock(this, existingBlock->getSourceText());

    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);

    textBlock->show();

    existingBlock->deleteLater();
    existingBlock = textBlock;

    updateTimeWidgetUnderlay();

    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::removeBlock(IItemBlock *block)
{
    const auto scopedExit = Utils::ScopeExitT([this]() { emit layoutChanged(QPrivateSignal()); });
    cleanupBlock(block);

    for (auto b : Blocks_)
    {
        if (b->removeBlock(block))
        {
            Layout_->onBlockSizeChanged();
            return;
        }
    }

    const auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
    {
        assert(!"block is missing");
        return;
    }

    Blocks_.erase(iter);

    block->deleteLater();
    updateTimeWidgetUnderlay();

    for (auto b : Blocks_)
        b->getBlockLayout()->markDirty();

    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::cleanupBlock(IItemBlock* _block)
{
    assert(_block);
    assert(!Blocks_.empty());

    const auto isMenuBlockReplaced = (MenuBlock_ == _block);
    if (isMenuBlockReplaced)
        cleanupMenu();

    const auto isHoveredBlockReplaced = (hoveredBlock_ == _block || hoveredSharingBlock_ == _block);
    if (isHoveredBlockReplaced)
        resetHover();
}

void ComplexMessageItem::selectByPos(const QPoint& from, const QPoint& to)
{
    assert(!Blocks_.empty());

    const auto isSelectionTopToBottom = (from.y() <= to.y());

    const auto &topPoint = (isSelectionTopToBottom ? from : to);
    const auto &bottomPoint = (isSelectionTopToBottom ? to : from);
    assert(topPoint.y() <= bottomPoint.y());

    const auto &blocksRect = Layout_->getBubbleRect();
    assert(!blocksRect.isEmpty());

    const QRect globalBlocksRect(
        mapToGlobal(blocksRect.topLeft()),
        mapToGlobal(blocksRect.bottomRight()));

    const auto isTopPointAboveBlocks = (topPoint.y() <= globalBlocksRect.top());
    const auto isTopPointBelowBlocks = (topPoint.y() >= globalBlocksRect.bottom());

    const auto isBottomPointAboveBlocks = (bottomPoint.y() <= globalBlocksRect.top());
    const auto isBottomPointBelowBlocks = (bottomPoint.y() >= globalBlocksRect.bottom());

    FullSelectionType_ = BlockSelectionType::Invalid;

    const auto isNotSelected = (
        (isTopPointAboveBlocks && isBottomPointAboveBlocks) ||
        (isTopPointBelowBlocks && isBottomPointBelowBlocks));
    if (isNotSelected)
    {
        FullSelectionType_ = BlockSelectionType::None;
    }

    const auto isFullSelection = (isTopPointAboveBlocks && isBottomPointBelowBlocks);
    if (isFullSelection)
    {
        FullSelectionType_ = BlockSelectionType::Full;
    }

    for (auto block : Blocks_)
    {
        const auto blockLayout = block->getBlockLayout();
        if (!blockLayout)
        {
            continue;
        }

        const auto blockGeometry = blockLayout->getBlockGeometry();

        const QRect globalBlockGeometry(
            mapToGlobal(blockGeometry.topLeft()),
            mapToGlobal(blockGeometry.bottomRight()));

        auto selectionType = BlockSelectionType::Invalid;

        if (isFullSelection)
        {
            selectionType = BlockSelectionType::Full;
        }
        else if (isNotSelected)
        {
            selectionType = BlockSelectionType::None;
        }
        else
        {
            selectionType = evaluateBlockSelectionType(globalBlockGeometry, topPoint, bottomPoint);
        }

        assert(selectionType != BlockSelectionType::Invalid);

        const auto blockNotSelected = (selectionType == BlockSelectionType::None);
        if (blockNotSelected)
        {
            block->clearSelection();
            continue;
        }

        block->selectByPos(topPoint, bottomPoint, selectionType);
    }
}

void ComplexMessageItem::setHasAvatar(const bool _hasAvatar)
{
    HistoryControlPageItem::setHasAvatar(_hasAvatar);

    if (!isOutgoing() && (_hasAvatar || isNeedAvatar()))
        loadAvatar();
    else
        Avatar_.reset();

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

    updateSize();
}

void ComplexMessageItem::setItems(IItemBlocksVec blocks)
{
    assert(Blocks_.empty());
    assert(!blocks.empty());
    assert(!shareButton_);
    assert(
        std::all_of(
            blocks.cbegin(),
            blocks.cend(),
            [](IItemBlock *block) { return block; }));

    Blocks_ = std::move(blocks);

    updateTimeWidgetUnderlay();
}

void ComplexMessageItem::setMchatSender(const QString& _senderName)
{
    if (isOutgoing() || _senderName.isEmpty())
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
    assert(time >= -1);
    assert(Time_ == -1);

    Time_ = time;

    if (!TimeWidget_)
    {
        TimeWidget_ = new MessageTimeWidget(this);
        TimeWidget_->setContact(getContact());
        TimeWidget_->setOutgoing(isOutgoing());
        TimeWidget_->setEdited(isEdited());
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

bool ComplexMessageItem::canBeUpdatedWith(const ComplexMessageItem& _other) const
{
    if (Blocks_.size() != _other.Blocks_.size())
        return false;

    auto iter = Blocks_.begin();
    auto iterOther = _other.Blocks_.begin();
    while (iter != Blocks_.end())
    {
        auto block = *iter;
        auto otherBlock = *iterOther;
        if (block->getContentType() != otherBlock->getContentType())
            return false;

        if (block->getSourceText() != otherBlock->getSourceText() && block->getContentType() != IItemBlock::ContentType::Text)
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
        assert((Id_ == -1) || (Id_ == _update.Id_));
        Id_ = _update.Id_;
        PrevId_ = _update.PrevId_;
        internalId_ = _update.internalId_;
        Url_ = _update.Url_;
        mentions_ = _update.mentions_;
        SourceText_ = _update.SourceText_;
        bool needUpdateSize = false;
        if (_update.isEdited() != isEdited())
        {
            setEdited(_update.isEdited());
            needUpdateSize = true;
        }

        auto iter = Blocks_.begin();
        auto iterOther = _update.Blocks_.begin();
        while (iter != Blocks_.end())
        {
            auto block = *iter;
            auto otherBlock = *iterOther;
            if (block->getSourceText() != otherBlock->getSourceText())
                block->setText(otherBlock->getSourceText());

            ++iter;
            ++iterOther;
        }

        Description_ = _update.Description_;

        if (needUpdateSize)
            updateSize();

        setChainedToPrev(_update.isChainedToPrevMessage());
        setChainedToNext(_update.isChainedToNextMessage());
    }
}

void ComplexMessageItem::leaveEvent(QEvent *event)
{
    event->ignore();

    MouseRightPressedOverItem_ = false;
    MouseLeftPressedOverAvatar_ = false;
    MouseLeftPressedOverSender_ = false;

    bubbleHovered_ = false;

    resetHover();
    emit leave();

    MessageItemBase::leaveEvent(event);
}

void ComplexMessageItem::mouseMoveEvent(QMouseEvent *_event)
{
    _event->ignore();

    const auto mousePos = _event->pos();

    if (isOverAvatar(mousePos) || isOverSender(mousePos))
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);

    auto blockUnderCursor = findBlockUnder(mousePos);
    onHoveredBlockChanged(blockUnderCursor);

    MessageItemBase::mouseMoveEvent(_event);
}

void ComplexMessageItem::mousePressEvent(QMouseEvent *event)
{
    MouseLeftPressedOverAvatar_ = false;
    MouseLeftPressedOverSender_ = false;
    MouseRightPressedOverItem_ = false;

    const auto isLeftButtonPressed = (event->button() == Qt::LeftButton);

    if (isLeftButtonPressed)
    {
        if (isOverAvatar(event->pos()))
            MouseLeftPressedOverAvatar_ = true;
        else if (isOverSender(event->pos()))
            MouseLeftPressedOverSender_ = true;
    }

    const auto isRightButtonPressed = (event->button() == Qt::RightButton);
    if (isRightButtonPressed)
    {
        MouseRightPressedOverItem_ = true;
    }

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
            emit selectionChanged();
            return;
        }
    }

    if (Bubble_.contains(event->pos()))
    {
        if (!isSelected() || !isRightButtonPressed)
        {
            if (auto scrollArea = qobject_cast<MessagesScrollArea*>(parent()))
                scrollArea->clearSelection();
        }
    }

    return MessageItemBase::mousePressEvent(event);
}

void ComplexMessageItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButtonReleased = (event->button() == Qt::LeftButton);

    if (isLeftButtonReleased)
    {
        if ((isOverAvatar(event->pos()) && MouseLeftPressedOverAvatar_) || (isOverSender(event->pos()) && MouseLeftPressedOverSender_))
        {
            Utils::openDialogOrProfile(SenderAimid_);
            event->accept();
        }
    }

    if (isContextMenuEnabled())
    {
        const auto isRightButtonPressed = (event->button() == Qt::RightButton);
        const auto rightButtonClickOnWidget = (isRightButtonPressed && MouseRightPressedOverItem_);
        if (rightButtonClickOnWidget)
        {
            if (isOverAvatar(event->pos()) && !isContextMenuReplyOnly())
            {
                emit avatarMenuRequest(SenderAimid_);
                return;
            }

            if (!Tooltip::isVisible())
            {
                const auto globalPos = event->globalPos();
                trackMenu(globalPos);
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

    return MessageItemBase::mouseReleaseEvent(event);
}

void ComplexMessageItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    const auto isLeftButtonReleased = (event->button() == Qt::LeftButton);

    if (isLeftButtonReleased)
    {
        auto emitQuote = true;
        const auto &bubbleRect = Layout_->getBubbleRect();
        if (bubbleRect.contains(event->pos()))
        {
            for (auto b : Blocks_)
            {
                const auto blockRect = b->getBlockLayout()->getBlockGeometry();
                const auto bockContain = blockRect.contains(event->pos());
                if (bockContain)
                    b->doubleClicked(event->pos(), [&emitQuote](bool result) { if (result) emitQuote = false; });
            }
        }

        if (emitQuote)
        {
            emit quote(getQuotes(false));
            return;
        }


        if (auto scrollArea = qobject_cast<MessagesScrollArea*>(parent()))
            scrollArea->continueSelection(scrollArea->mapFromGlobal(mapToGlobal(event->pos())));

        emit selectionChanged();
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
            assert(dbgBlock);
            if (!dbgBlock || dbgBlock->getSubtype() != DebugTextBlock::Subtype::MessageId)
                continue;

            countDbgMsgs++;

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
            assert(dbgBlock);
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
    MessageItemBase::paintEvent(event);

    if (!Layout_)
        return;

    // NOTE: Doesn't do anything unless debug settings have been changed in runtime
    renderDebugInfo();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawBubble(p, QuoteAnimation_.quoteColor());

    drawAvatar(p);

    if (Sender_ && isSenderVisible())
        Sender_->draw(p, TextRendering::VerPosition::BASELINE);

    if (MessageStyle::isBlocksGridEnabled())
        drawGrid(p);

    if (const auto lastStatus = getLastStatus(); lastStatus != LastStatus::None)
        drawLastStatusIcon(p, lastStatus, SenderAimid_, getSenderFriendly(), 0);

    drawHeads(p);
}

void ComplexMessageItem::onAvatarChanged(const QString& aimId)
{
    assert(!aimId.isEmpty());
    assert(!SenderAimid_.isEmpty());

    if (SenderAimid_ != aimId || (!hasAvatar() && !isNeedAvatar()))
        return;

    loadAvatar();
    update();
}

void ComplexMessageItem::onMenuItemTriggered(QAction *action)
{
    const auto params = action->data().toMap();
    const auto command = params[qsl("command")].toString();

    if (const auto devPrefix = ql1s("dev:"); command.startsWith(devPrefix))
    {
        const auto subCommand = command.midRef(devPrefix.size());

        if (subCommand.isEmpty())
        {
            assert(!"unknown subcommand");
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
        if (command == ql1s("copy_link") || command == ql1s("copy_email") || command == ql1s("copy_file"))
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_copy_action, stat_props);
        return;
    }

    const auto isPendingMessage = (Id_ <= -1);

    if (command == ql1s("copy"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Copy);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_copy_action, stat_props);
    }
    else if (command == ql1s("quote"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Quote);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_reply_action, stat_props);
    }
    else if (command == ql1s("forward"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Forward);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_forward_action, stat_props);
    }
    else if (command == ql1s("report"))
    {
        auto confirm = false;

        if (MenuBlock_ && MenuBlock_->getContentType() != IItemBlock::ContentType::Sticker)
            confirm = ReportMessage(SenderAimid_, ChatAimid_, Id_, SourceText_);
        else
            confirm = ReportSticker(SenderAimid_, isChat_ ? ChatAimid_ : QString(), MenuBlock_ ? MenuBlock_->getStickerId() : QString());

        // delete this message
        if (confirm)
            Ui::GetDispatcher()->deleteMessage(Id_, internalId_, ChatAimid_, false);

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_report_action, stat_props);
    }
    else if (command == ql1s("delete_all") || command == ql1s("delete"))
    {
        const auto is_shared = command == ql1s("delete_all");

        assert(!SenderAimid_.isEmpty());

        const QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?");

        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Delete message"),
            nullptr
        );

        if (confirm)
            GetDispatcher()->deleteMessage(Id_, internalId_, ChatAimid_, is_shared);

        stat_props.emplace_back("delete_target", is_shared ? "all" : "self");
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_delete_action, stat_props);
    }
    else if (command == ql1s("edit"))
    {
        callEditing();
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_edit_action, stat_props);
    }
    else if (!isPendingMessage)
    {
        if (command == ql1s("pin") || command == ql1s("unpin"))
        {
            const bool isUnpin = command == ql1s("unpin");
            emit pin(ChatAimid_, Id_, isUnpin);

            if (isUnpin)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_unpin_action, stat_props);
            else
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_pin_action, stat_props);
        }
    }
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
}

bool ComplexMessageItem::containsShareableBlocks() const
{
    assert(!Blocks_.empty());

     return std::any_of(
         Blocks_.cbegin(),
         Blocks_.cend(),
         []
         (const IItemBlock *block)
         {
             assert(block);
             return block->isSharingEnabled() || block->containSharingBlock();
         });
}

void ComplexMessageItem::drawAvatar(QPainter &p)
{
    if (!Avatar_ || (!hasAvatar() && !isNeedAvatar()))
        return;

    if (const auto &avatarRect = Layout_->getAvatarRect(); avatarRect.isValid())
        Utils::drawAvatarWithBadge(p, avatarRect.topLeft(), *Avatar_, SenderAimid_, true);
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
            flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

        if (isChainedToNextMessage())
            flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);

        auto r = bubbleRect;
        if (!bubbleRequired && headerRequired)
        {
            // draw bubble only for header
            const auto newHeight = r.height() - Blocks_.front()->getBlockLayout()->getBlockGeometry().height();
            if (newHeight > 0)
                r.setHeight(newHeight);
            flags &= ~(Utils::RenderBubbleFlags::RightBottomSmall);
            flags &= ~(Utils::RenderBubbleFlags::LeftBottomSmall);
            flags &= ~(Utils::RenderBubbleFlags::BottomSideRounded);
        }

        Bubble_ = Utils::renderMessageBubble(r, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadiusSmall(), (Utils::RenderBubbleFlags)flags);
        BubbleGeometry_ = bubbleRect;
    }

    const auto needBorder = !isOutgoing() && Styling::getParameters(getContact()).isBorderNeeded();
    if (!needBorder && !isHeaderRequired())
        Utils::drawBubbleShadow(p, Bubble_);

    p.fillPath(Bubble_, MessageStyle::getBodyBrush(isOutgoing(), isDrawFullBubbleSelected_, getContact()));

    if (quote_color.isValid())
        p.fillPath(Bubble_, QBrush(quote_color));

    if (needBorder)
        Utils::drawBubbleRect(p, Bubble_.boundingRect(), MessageStyle::getBorderColor(), MessageStyle::getBorderWidth(), MessageStyle::getBorderRadius());
}

QString ComplexMessageItem::getBlocksText(const IItemBlocksVec& _items, const bool _isSelected) const
{
    QString result;

    // to reduce the number of reallocations
    result.reserve(1024);

    const auto selectedItemCount = _isSelected
        ? std::count_if(_items.cbegin(), _items.cend(), [](auto item) { return item->isSelected(); })
        : 0;

    for (const auto item : _items)
    {
        if (!_isSelected && item->getContentType() == IItemBlock::ContentType::DebugText)
            continue;

        const auto itemText = _isSelected
            ? item->getSelectedText(selectedItemCount > 1)
            : item->getTextForCopy();

        if (itemText.isEmpty())
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

IItemBlock* ComplexMessageItem::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        if (!block)
            continue;

        auto b = block->findBlockUnder(pos);
        if (b)
            return b;

        const auto blockLayout = block->getBlockLayout();
        if (!blockLayout)
            continue;

        auto rc = blockLayout->getBlockGeometry();
        if (block->isSharingEnabled())
        {
            if (isOutgoing())
                rc.setLeft(rc.left() - getSharingAdditionalWidth());
            else
                rc.setRight(rc.right() + getSharingAdditionalWidth());
        }

        if (rc.contains(pos))
            return block;
    }

    return nullptr;
}

QString ComplexMessageItem::getSenderFriendly() const
{
    assert(!SenderFriendly_.isEmpty());
    return SenderFriendly_;
}

Data::QuotesVec ComplexMessageItem::getQuotes(const bool _selectedTextOnly, const bool _isForward) const
{
    Data::QuotesVec result;

    const auto isQuote = [](const auto _block) { return _block->getContentType() == IItemBlock::ContentType::Quote; };

    const auto quotesOnly = std::all_of(Blocks_.begin(), Blocks_.end(), isQuote);
    const auto dropQuotes = !_isForward && !quotesOnly;

    for (const auto block : Blocks_)
    {
        if (block->getContentType() == IItemBlock::ContentType::DebugText)
            continue;

        if (_selectedTextOnly && block->getSelectedText().isEmpty())
            continue;

        if (!_selectedTextOnly && dropQuotes && isQuote(block))
            continue;

        if (Blocks_.size() > 1 &&
            !hasTrailingLink_ &&
            block->getContentType() == IItemBlock::ContentType::Link &&
            !block->isLinkMediaPreview())
            continue;

        auto quote = getQuoteFromBlock(block, _selectedTextOnly);
        if (!quote.isEmpty())
        {
            if (!result.isEmpty())
            {
                auto& prevQuote = result.back();
                const auto canMergeQuotes = prevQuote.msgId_ == quote.msgId_;

                if (canMergeQuotes)
                {
                    prevQuote.text_.reserve(prevQuote.text_.size() + quote.text_.size() + 1);

                    if (!prevQuote.text_.endsWith(QChar::LineFeed) && !prevQuote.text_.endsWith(ql1c('\n')))
                        prevQuote.text_ += QChar::LineFeed;

                    prevQuote.text_ += quote.text_;
                }
                else
                    result.push_back(std::move(quote));
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
    quotes.reserve(Blocks_.size());

    for (const auto b : Blocks_)
    {
        auto q = b->getQuote();
        if (!q.isEmpty())
            quotes.push_back(std::move(q));
    }

    return quotes;
}

void ComplexMessageItem::setSourceText(QString text)
{
    SourceText_ = std::move(text);
}

void ComplexMessageItem::initialize()
{
    assert(Layout_);
    assert(!Blocks_.empty());

    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    updateSize();
}

void ComplexMessageItem::initializeShareButton()
{
    if (shareButton_ || Blocks_.empty())
        return;

    if (!containsShareableBlocks())
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

bool ComplexMessageItem::isBubbleRequired() const
{
    return
        Blocks_.size() > 1 ||
        std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isBubbleRequired(); });
}

bool ComplexMessageItem::isHeaderRequired() const
{
    return isSenderVisible() && Blocks_.size() == 1 && !Blocks_.front()->isBubbleRequired() && !isSingleSticker();
}

bool ComplexMessageItem::isSmallPreview() const
{
    return Blocks_.size() == 1 && Blocks_.front()->isSmallPreview();
}

int ComplexMessageItem::getMaxWidth() const
{
    int maxWidth = -1;

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

QString ComplexMessageItem::getEditableText() const
{
    using editPair = std::pair<IItemBlock::ContentType, QString>;
    std::vector<editPair> pairs;
    pairs.reserve(Blocks_.size());

    for (const auto b : Blocks_)
    {
        const auto ct = b->getContentType();
        switch (ct)
        {
        case IItemBlock::ContentType::Text:
        case IItemBlock::ContentType::FileSharing:
            pairs.emplace_back(ct, b->getSourceText());
            break;
        case IItemBlock::ContentType::Link:
            if (b->isLinkMediaPreview() || (!b->isLinkMediaPreview() && hasTrailingLink_))
                pairs.emplace_back(ct, b->getSourceText());
            break;
        default:
            break;
        }
    }

    auto resSize = 0;
    for (const auto& [_, text] : pairs)
        resSize += text.size();

    QString result;
    result.reserve(resSize);

    for (size_t i = 0; i < pairs.size(); ++i)
    {
        const auto& [type, text] = pairs[i];

        if (i > 0 && type != IItemBlock::ContentType::Text && pairs[i - 1].first != IItemBlock::ContentType::Text)
            result += ql1c(' ');

        result += text;
    }

    return result;
}

int ComplexMessageItem::desiredWidth() const
{
    int result = 0;
    for (auto b : Blocks_)
        result = std::max(result, b->desiredWidth(width()));

    if (isSenderVisible() && !isHeaderRequired())
        result = std::max(result, isSingleSticker() ? MessageStyle::getSenderStickerWidth() : desiredSenderWidth_);

    if (isBubbleRequired())
        result += 2 * MessageStyle::getBubbleHorPadding();

    return result;
}

void ComplexMessageItem::updateSize()
{
    Layout_->onBlockSizeChanged();
    updateGeometry();
    update();
}

void ComplexMessageItem::callEditing()
{
    if (!Description_.isEmpty())
    {
        emit editWithCaption(
            getId(),
            getInternalId(),
            version_,
            Url_,
            Description_);
    }
    else
    {
        emit edit(
            getId(),
            getInternalId(),
            version_,
            getEditableText(),
            getMentions(),
            getQuotesForEdit(),
            getTime());
    }
}

void ComplexMessageItem::setHasTrailingLink(const bool _hasLink)
{
    hasTrailingLink_ = _hasLink;
}

int ComplexMessageItem::getBlockCount() const
{
    return Blocks_.size();
}

IItemBlock* ComplexMessageItem::getHoveredBlock() const
{
    return hoveredBlock_;
}

void ComplexMessageItem::resetHover()
{
    onHoveredBlockChanged(nullptr);
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

bool ComplexMessageItem::isHeadless() const
{
    return parentWidget() == nullptr;
}

bool ComplexMessageItem::isLastBlock(const IItemBlock* _block) const
{
    return !Blocks_.empty() && Blocks_.back() == _block;
}

bool ComplexMessageItem::isOverAvatar(const QPoint &pos) const
{
    assert(Layout_);

    if (hasAvatar() || isNeedAvatar())
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
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isSelected(); });
}

bool ComplexMessageItem::isAllSelected() const
{
    return std::all_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isAllSelected(); });
}

void ComplexMessageItem::setDrawFullBubbleSelected(const bool _selected)
{
    if (isDrawFullBubbleSelected_ != _selected)
    {
        isDrawFullBubbleSelected_ = _selected;
        if (TimeWidget_)
            TimeWidget_->setSelected(_selected);

        updateSenderControlColor();
        update();
    }
}

bool ComplexMessageItem::isSenderVisible() const
{
    return Sender_ && hasSenderName();
}

void ComplexMessageItem::setCustomBubbleHorPadding(const int32_t _val)
{
    Layout_->setCustomBubbleBlockHorPadding(_val);
}

bool ComplexMessageItem::hasSharedContact() const
{
    return buddy().sharedContact_.has_value();
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
        SenderAimid_,
        getSenderFriendly(),
        Utils::scale_bitmap(MessageStyle::getAvatarSize()),
        QString(),
        Out isDefault,
        false,
        false);
}

void ComplexMessageItem::onCopyMenuItem(ComplexMessageItem::MenuItemType type)
{
    bool isCopy = (type == ComplexMessageItem::MenuItemType::Copy);
    bool isQuote = (type == ComplexMessageItem::MenuItemType::Quote);
    bool isForward = (type == ComplexMessageItem::MenuItemType::Forward);

    if (isCopy)
    {
        QString itemText;
        itemText.reserve(1024);

        if (isQuote || isForward)
            itemText += getQuoteHeader();

        if (isSelected())
            itemText += getSelectedText(false);
        else
            itemText += getBlocksText(Blocks_, false);

        emit copy(itemText);
    }
    else if (isQuote)
    {
        emit quote(getQuotes(isSelected() && !isAllSelected()));
    }
    else if (isForward)
    {
        emit forward(getQuotes(isSelected() && !isAllSelected(), true));
    }
}

bool ComplexMessageItem::onDeveloperMenuItemTriggered(const QStringRef &cmd)
{
    assert(!cmd.isEmpty());

    if (!GetAppConfig().IsContextMenuFeaturesUnlocked())
    {
        assert(!"developer context menu is not unlocked");
        return true;
    }

    if (cmd == ql1s("copy_message_id"))
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

    emit forward({ getQuoteFromBlock(hoveredSharingBlock_, false) });
}

void ComplexMessageItem::trackMenu(const QPoint &globalPos)
{
    cleanupMenu();

    auto menu = new ContextMenu(this);
    Testing::setAccessibleName(menu, qsl("AS complexmessageitem menu"));

    const auto replyOnly = isContextMenuReplyOnly();

    const auto role = Logic::getContactListModel()->getYourRole(ChatAimid_);
    const auto isDisabled = role == ql1s("notamember") || role == ql1s("readonly");

    if (!isDisabled)
        menu->addActionWithIcon(qsl(":/context_menu/reply"), QT_TRANSLATE_NOOP("context_menu", "Reply"), makeData(qsl("quote")))->setEnabled(true);

    MenuBlock_ = findBlockUnder(mapFromGlobal(globalPos));

    const auto hasBlock = !!MenuBlock_;
    const auto isLinkCopyable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagLinkCopyable  : false;
    const auto isFileCopyable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagFileCopyable  : false;
    const auto isOpenable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagOpenInBrowser : false;
    const auto isCopyable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagCopyable : false;
    const auto isOpenFolder = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagOpenFolder : false;

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
            link = MenuBlock_->linkAtPos(globalPos);

            bool hasCopyableLink = false;

            if (!link.isEmpty())
            {
                static const QRegularExpression re(
                    qsl("^mailto:"),
                    QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
                );

                link.remove(re);
                Utils::UrlParser parser;
                parser.process(QStringRef(&link));
                if (parser.hasUrl())
                {
                    if (parser.getUrl().is_email())
                        hasCopyableEmail = true;
                    else
                        hasCopyableLink = true;
                }
            }

            if (hasCopyableLink)
                menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link"), link))->setEnabled(!replyOnly);
            if (hasCopyableEmail)
                menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy email"), makeData(qsl("copy_email"), link))->setEnabled(!replyOnly);
            else if (isCopyable)
                menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy"), makeData(qsl("copy")))->setEnabled(!replyOnly);
        }
    }

    if (hasBlock && isFileCopyable)
        menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy to clipboard"), makeData(qsl("copy_file")))->setEnabled(!replyOnly);

    menu->addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward")))->setEnabled(!replyOnly);

    if (Logic::getContactListModel()->isYouAdmin(ChatAimid_))
    {
        const Data::DlgState dlg = Logic::getRecentsModel()->getDlgState(ChatAimid_);
        if (dlg.pinnedMessage_ && dlg.pinnedMessage_->Id_ == Id_)
            menu->addActionWithIcon(qsl(":/context_menu/unpin"), QT_TRANSLATE_NOOP("context_menu", "Unpin"), makeData(qsl("unpin")))->setEnabled(!replyOnly);
        else
            menu->addActionWithIcon(qsl(":/context_menu/pin"), QT_TRANSLATE_NOOP("context_menu", "Pin"), makeData(qsl("pin")))->setEnabled(!replyOnly);
    }

    if (hasBlock && isFileCopyable)
        menu->addActionWithIcon(qsl(":/context_menu/download"), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData(qsl("save_as")))->setEnabled(!replyOnly);

    if (isOpenable)
        menu->addActionWithIcon(qsl(":/context_menu/browser"), QT_TRANSLATE_NOOP("context_menu", "Open in browser"), makeData(qsl("open_in_browser")))->setEnabled(!replyOnly);

    if (hasBlock && isOpenFolder)
        menu->addActionWithIcon(qsl(":/context_menu/open_folder"), QT_TRANSLATE_NOOP("context_menu", "Show in folder"), makeData(qsl("open_folder")))->setEnabled(!replyOnly);

    if (!isDisabled && isEditable())
        menu->addActionWithIcon(qsl(":/context_menu/edit"), QT_TRANSLATE_NOOP("context_menu", "Edit"), makeData(qsl("edit")))->setEnabled(!replyOnly);

    if (hasCopyableEmail && !link.isEmpty())
        menu->addActionWithIcon(qsl(":/context_menu/goto"), QT_TRANSLATE_NOOP("context_menu", "Go to profile"), makeData(qsl("open_profile"), link))->setEnabled(!replyOnly);

    menu->addSeparator();
    menu->addActionWithIcon(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData(qsl("delete")))->setEnabled(!replyOnly);

    if (ChatAimid_ != MyInfo()->aimId() && (isOutgoing() || Logic::getContactListModel()->isYouAdmin(ChatAimid_)))
        menu->addActionWithIcon(qsl(":/context_menu/deleteall"), QT_TRANSLATE_NOOP("context_menu", "Delete for all"), makeData(qsl("delete_all")))->setEnabled(!replyOnly);

    if (GetAppConfig().IsContextMenuFeaturesUnlocked())
        menu->addActionWithIcon(qsl(":/context_menu/copy"), qsl("Copy Message ID"), makeData(qsl("dev:copy_message_id")))->setEnabled(!replyOnly);

    if (hasBlock && (MenuBlock_->getContentType() == IItemBlock::ContentType::Sticker ||
        (isChat_ && SenderAimid_ != MyInfo()->aimId() && !Logic::getContactListModel()->isYouAdmin(ChatAimid_))))
    {
        menu->addSeparator();
        menu->addActionWithIcon(qsl(":/context_menu/spam"), QT_TRANSLATE_NOOP("context_menu", "Report"), makeData(qsl("report")))->setEnabled(!replyOnly);
    }
    connect(menu, &ContextMenu::triggered, this, &ComplexMessageItem::onMenuItemTriggered, Qt::QueuedConnection);
    connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
    connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

    menu->popup(globalPos);
}

void ComplexMessageItem::updateSenderControlColor()
{
    if (!Sender_)
        return;

    QColor color;
    if (isDrawFullBubbleSelected_)
        color = Qt::white;
    else
        color = Utils::getNameColor(SenderAimid_);

    Sender_->setColor(color);
}

void ComplexMessageItem::updateShareButtonGeometry()
{
    if (!shareButton_)
        return;

    if (!hoveredSharingBlock_)
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

    const QPoint endPos(Layout_->getShareButtonPos(*hoveredSharingBlock_, hoveredSharingBlock_->isBubbleRequired()));
    const QPoint startPos(endPos.x() + MessageStyle::getSharingButtonAnimationShift(isOutgoing()), endPos.y());

    shareButton_->move(startPos);
    shareButton_->show();

    shareButtonOpacityEffect_->setOpacity(0.0);
    animateShareButton(startPos.x(), endPos.x(), WidgetAnimationType::show);
}

void ComplexMessageItem::hideShareButtonAnimated()
{
    if (!shareButton_)
        return;

    const QPoint startPos(shareButton_->pos());
    const QPoint endPos(startPos.x() + MessageStyle::getSharingButtonAnimationShift(isOutgoing()), startPos.y());

    shareButtonOpacityEffect_->setOpacity(1.0);
    animateShareButton(startPos.x(), endPos.x(), WidgetAnimationType::hide);
}

void ComplexMessageItem::animateShareButton(const int _startPosX, const int _endPosX, const WidgetAnimationType _anim)
{
    if (!shareButton_)
        return;

    shareButtonAnimation_.finish();
    shareButtonAnimation_.start([_startPosX, _endPosX, _anim, this]()
    {
        const auto val = shareButtonAnimation_.current();
        shareButton_->move(val, shareButton_->pos().y());

        const auto opacity = abs((val - _startPosX) / (_endPosX - _startPosX));
        shareButtonOpacityEffect_->setOpacity(_anim == WidgetAnimationType::hide ? 1.0 - opacity : opacity);

        if (_anim == WidgetAnimationType::hide && val == _endPosX)
            shareButton_->hide();

    }, _startPosX, _endPosX, MessageStyle::getHiddenControlsAnimationTime().count(), anim::linear, 1);
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

    timeOpacityEffect_->setOpacity(0.0);
    animateTime(startPos.y(), endPos.y(), WidgetAnimationType::show);
}

void ComplexMessageItem::hideTimeAnimated()
{
    if (!TimeWidget_)
        return;

    const auto initialPos = Layout_->getInitialTimePosition();
    const QPoint startPos(TimeWidget_->pos());
    const QPoint endPos(initialPos.x(), initialPos.y() + getTimeAnimationDistance());

    timeOpacityEffect_->setOpacity(1.0);
    animateTime(startPos.y(), endPos.y(), WidgetAnimationType::hide);
}

int ComplexMessageItem::getTimeAnimationDistance() const
{
    if (!TimeWidget_)
        return 0;

    return TimeWidget_->getVerMargin() + MessageStyle::getHiddenControlsShift();
}

void ComplexMessageItem::animateTime(const int _startPosY, const int _endPosY, const WidgetAnimationType _anim)
{
    if (!TimeWidget_)
        return;

    timeAnimation_.finish();
    timeAnimation_.start([_startPosY, _endPosY, _anim, this]()
    {
        const auto val = timeAnimation_.current();
        TimeWidget_->move(TimeWidget_->pos().x(), val);

        const auto opacity = abs((val - _startPosY) / (_endPosY - _startPosY));
        timeOpacityEffect_->setOpacity(_anim == WidgetAnimationType::hide ? 1.0 - opacity : opacity);

        if (_anim == WidgetAnimationType::hide && val == _endPosY)
            TimeWidget_->hide();
    }, _startPosY, _endPosY, MessageStyle::getHiddenControlsAnimationTime().count(), anim::linear, 1);
}

Data::Quote ComplexMessageItem::getQuoteFromBlock(IItemBlock* _block, const bool _selectedTextOnly) const
{
    if (!_block)
        return Data::Quote();

    auto q = _block->getQuote();
    if (!q.isEmpty())
    {
        if (_selectedTextOnly)
            q.text_ = _block->getSelectedText(false, IItemBlock::TextDestination::quote);

        return q;
    }

    Data::Quote quote;
    quote.text_ = _selectedTextOnly ? _block->getSelectedText(false, IItemBlock::TextDestination::quote) : _block->getSourceText();
    quote.senderId_ = isOutgoing() ? MyInfo()->aimId() : _block->getSenderAimid();
    quote.chatId_ = getChatAimid();
    quote.time_ = getTime();
    quote.msgId_ = getId();
    quote.mentions_ = mentions_;
    quote.mediaType_ = _block->getMediaType();
    quote.sharedContact_ = buddy().sharedContact_;

    if (quote.isEmpty())
        return Data::Quote();

    QString senderFriendly = getSenderFriendly();
    if (senderFriendly.isEmpty())
        senderFriendly = Logic::GetFriendlyContainer()->getFriendly(quote.senderId_);
    if (isOutgoing())
        senderFriendly = MyInfo()->friendly();
    quote.senderFriendly_ = std::move(senderFriendly);

    if (const auto stickerInfo = _block->getStickerInfo())
    {
        quote.setId_ = stickerInfo->SetId_;
        quote.stickerId_ = stickerInfo->StickerId_;
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

    if (!Url_.isEmpty() && !Description_.isEmpty() && (!_selectedTextOnly || FullSelectionType_ == ComplexMessage::BlockSelectionType::Full))
    {
        quote.url_ = Url_;
        quote.description_ = Description_;
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
        };

        const auto mediaType = Blocks_.back()->getMediaType();
        const auto withUnderlay = Blocks_.size() == 1 && std::none_of(std::begin(plainTypes), std::end(plainTypes), [mediaType](const auto _t) { return mediaType == _t; });

        TimeWidget_->setUnderlayVisible(withUnderlay);

        timeAnimation_.finish();

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

void ComplexMessageItem::setTimeWidgetVisible(const bool _visible)
{
    if (TimeWidget_)
        TimeWidget_->setVisible(_visible);
}

bool ComplexMessageItem::isNeedAvatar() const
{
    if (isSingleSticker())
        return true;

    if (const auto type = getMediaType(); type != MediaType::noMedia && type != MediaType::mediaTypeFileSharing)
        return true;

    return false;
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

int ComplexMessageItem::getSenderDesiredWidth() const noexcept
{
    return desiredSenderWidth_;
}

void ComplexMessageItem::setQuoteSelection()
{
    for (auto& val : Blocks_)
        val->setQuoteSelection();

    QuoteAnimation_.startQuoteAnimation();
}

void ComplexMessageItem::setDeliveredToServer(const bool _isDeliveredToServer)
{
    if (!isOutgoing())
        return;

    if (IsDeliveredToServer_ != _isDeliveredToServer)
        IsDeliveredToServer_ = _isDeliveredToServer;
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

namespace
{
    BlockSelectionType evaluateBlockSelectionType(const QRect &blockGeometry, const QPoint &selectionTop, const QPoint &selectionBottom)
    {
        const auto isBlockAboveSelection = (blockGeometry.bottom() < selectionTop.y());
        const auto isBlockBelowSelection = (blockGeometry.top() > selectionBottom.y());
        if (isBlockAboveSelection || isBlockBelowSelection)
        {
            return BlockSelectionType::None;
        }

        const auto isTopPointAboveBlock = (blockGeometry.top() >= selectionTop.y());
        const auto isBottomPointBelowBlock = (blockGeometry.bottom() <= selectionBottom.y());

        if (isTopPointAboveBlock && isBottomPointBelowBlock)
        {
            return BlockSelectionType::Full;
        }

        if (isTopPointAboveBlock)
        {
            return BlockSelectionType::FromBeginning;
        }

        if (isBottomPointBelowBlock)
        {
            return BlockSelectionType::TillEnd;
        }

        return BlockSelectionType::PartialInternal;
    }
}

UI_COMPLEX_MESSAGE_NS_END

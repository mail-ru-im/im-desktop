#include "stdafx.h"

#include "ComplexMessageItem.h"
#include "ComplexMessageUtils.h"
#include "../MessageStyle.h"
#include "../../../controls/TextUnit.h"
#include "../../../controls/LabelEx.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../utils/PainterPath.h"
#include "../../../my_info.h"
#include "../../../fonts.h"
#include "../../contact_list/ContactListModel.h"
#include "../../containers/FriendlyContainer.h"

#include "QuoteBlockLayout.h"

#include "QuoteBlock.h"
#include "TextBlock.h"

#include "../../../styles/ThemeParameters.h"

#include "main_window/MainWindow.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    QString labelLink(const TextRendering::TextUnitPtr& _label, const QPoint& _pos)
    {
        if (_label)
            return _label->getLink(_pos).url_;

        return QString();
    }

    auto forwardLabelColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
    }
}

QuoteBlock::QuoteBlock(ComplexMessageItem* _parent, const Data::Quote& _quote)
    : GenericBlock(_parent, _quote.senderFriendly_, MenuFlagCopyable, false)
    , Quote_(std::move(_quote))
    , Layout_(new QuoteBlockLayout())
    , desiredSenderWidth_(0)
    , Parent_(_parent)
    , ReplyBlock_(nullptr)
    , MessagesCount_(0)
    , MessageIndex_(0)
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageQuote " % QString::number(_parent->getId()));

    setLayout(Layout_);
    setMouseTracking(true);

    if (!getParentComplexMessage()->isHeadless())
        initSenderName();

    connect(this, &QuoteBlock::observeToSize, _parent, &ComplexMessageItem::onObserveToSize);

    if (isQuoteInteractive())
    {
        setCursor(Qt::PointingHandCursor);
        connect(_parent, &ComplexMessageItem::hoveredBlockChanged, this, qOverload<>(&QuoteBlock::update));
    }
}

QuoteBlock::~QuoteBlock() = default;

void QuoteBlock::clearSelection()
{
    for (auto b : Blocks_)
        b->clearSelection();

    update();
}

void QuoteBlock::releaseSelection()
{
    for (auto b : Blocks_)
        b->releaseSelection();

    update();
}

IItemBlockLayout* QuoteBlock::getBlockLayout() const
{
    return Layout_;
}

bool QuoteBlock::updateFriendly(const QString& _aimId, const QString& _friendly)
{
    bool needUpdate = false;
    if (!_aimId.isEmpty() && !_friendly.isEmpty())
    {
        if (Quote_.senderId_ == _aimId && Quote_.senderFriendly_ != _friendly)
        {
            Quote_.senderFriendly_ = _friendly;
            GenericBlock::setSourceText(Quote_.senderFriendly_);
            initSenderName();
            needUpdate = true;
        }

        for (const auto block : Blocks_)
            needUpdate |= block->updateFriendly(_aimId, _friendly);
    }
    return needUpdate;
}

Data::FString QuoteBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
{
    Data::FString result;
    for (auto b : Blocks_)
    {
        if (b->isSelected())
        {
            const auto selectedText = b->getSelectedText(false, _dest);
            if (b->getContentType() == IItemBlock::ContentType::Link && result.string().contains(selectedText.string()))
                continue;
            result += selectedText;
            result += QChar::LineFeed;
        }
    }

    if (_isFullSelect && !result.isEmpty())
        return Data::FString(getQuoteHeader()) += result;

    return result;
}

QString QuoteBlock::getTextForCopy() const
{
    QString result;
    for (auto b : Blocks_)
    {
        const auto textForCopy = !b->hasSourceText() ? b->getTextForCopy() : b->getSourceText().string();
        if (b->getContentType() == IItemBlock::ContentType::Link && result.contains(textForCopy))
            continue;

        result += textForCopy;
        result += QChar::LineFeed;
    }

    if (!result.isEmpty())
        return getQuoteHeader() % result;

    return result;
}

Data::FString QuoteBlock::getSourceText() const
{
    Data::FString result;
    for (auto b : Blocks_)
    {
        const auto contentType = b->getContentType();
        const Data::FString sourceText = b->hasSourceText() ? b->getSourceText() : b->getTextForCopy();
        if (contentType == IItemBlock::ContentType::Link && result.contains(sourceText.string()))
            continue;

        if (contentType == IItemBlock::ContentType::Text && sourceText.startsWith(QChar::LineFeed) && result.endsWith(QChar::LineFeed))
            result.chop(1);

        result += sourceText;
        if (!result.endsWith(QChar::LineFeed) && contentType != IItemBlock::ContentType::Code)
            result += QChar::LineFeed;
    }

    if (!result.isEmpty())
        return Data::FString(getQuoteHeader()) += result;

    return result;
}

QString QuoteBlock::formatRecentsText() const
{
    if (standaloneText())
        return ReplyBlock_ ? ReplyBlock_->formatRecentsText() : QString();

    if (!Blocks_.empty())
    {
        const auto senderFriendly = Logic::GetFriendlyContainer()->getFriendly2(Quote_.senderId_);
        const auto friendly = senderFriendly.default_ ? QString() : senderFriendly.name_;
        const auto separator = senderFriendly.default_ ? QStringView() : QStringView(u": ");
        if (!Quote_.description_.isEmpty())
            return friendly % separator % Blocks_.front()->formatRecentsText() % QChar::Space % Quote_.description_.string();

        return friendly % separator % Blocks_.front()->formatRecentsText();
    }

    im_assert(false);
    return QString();
}

bool QuoteBlock::standaloneText() const
{
    return !quoteOnly() && !Quote_.isForward_;
}

Data::Quote QuoteBlock::getQuote() const
{
    return Quote_;
}

IItemBlock* QuoteBlock::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        if (!block)
            continue;

        auto rc = block->getBlockGeometry();
        if (block->isSharingEnabled())
        {
            const auto add = getParentComplexMessage()->getSharingAdditionalWidth();
            auto extendedGeometry = getParentComplexMessage()->getBubbleGeometry();
            if (extendedGeometry.isEmpty())
                extendedGeometry = rc;

            if (isOutgoing())
                rc.setLeft(extendedGeometry.left() - add);
            else
                rc.setRight(extendedGeometry.right() + add);
        }

        if (rc.contains(pos))
            return block;
    }

    return nullptr;
}

bool QuoteBlock::isSelected() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isSelected() && !b->getSelectedText().isEmpty(); });
}

bool QuoteBlock::isAllSelected() const
{
    return std::all_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isAllSelected() && !b->getSelectedText().isEmpty(); });
}

void QuoteBlock::selectByPos(const QPoint& from, const QPoint& to, bool topToBottom)
{
    for (auto b : Blocks_)
        b->selectByPos(from, to, topToBottom);
}

void QuoteBlock::selectAll()
{
    for (auto b : Blocks_)
        b->selectAll();
}

void QuoteBlock::setReplyBlock(GenericBlock* block)
{
    if (!ReplyBlock_)
        ReplyBlock_ = block;
}

void QuoteBlock::onVisibleRectChanged(const QRect& _visibleRect)
{
    GenericBlock::onVisibleRectChanged(_visibleRect);

    for (auto block : Blocks_)
        block->onVisibleRectChanged(_visibleRect.intersected(block->getBlockGeometry()));
}

void QuoteBlock::onSelectionStateChanged(const bool isSelected)
{
    for (auto block : Blocks_)
        block->onSelectionStateChanged(isSelected);
}

void QuoteBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    GenericBlock::onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);

    for (auto block : Blocks_)
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
}

QRect QuoteBlock::setBlockGeometry(const QRect& _boundingRect)
{
    auto contentRect = _boundingRect.adjusted(getLeftOffset(), 0, -MessageStyle::Quote::getQuoteOffsetRight(), 0);
    const auto senderRect = GenericBlock::setBlockGeometry(contentRect);

    int totalHeight = senderRect.height();

    contentRect.translate(0, senderRect.height());
    for (auto childBlock : Blocks_)
    {
        QRect childRect = childBlock->setBlockGeometry(contentRect);

        const auto childHeight = childRect.height() + MessageStyle::Quote::getQuoteBlockInsideSpacing();
        totalHeight += childHeight;
        contentRect.translate(0, childHeight);
    }

    for (auto block : Blocks_)
    {
        if (block->needStretchToOthers())
            block->stretchToWidth(contentRect.width());
    }

    const auto availableWidth = contentRect.width();
    if (availableWidth < desiredSenderWidth_ || (senderLabel_->isElided() && availableWidth >= desiredSenderWidth_))
        senderLabel_->elide(availableWidth);

    Geometry_ = QRect(_boundingRect.topLeft(), QSize(_boundingRect.width(), totalHeight));
    setGeometry(Geometry_);

    return Geometry_;
}

void QuoteBlock::onActivityChanged(const bool isVisible)
{
    GenericBlock::onActivityChanged(isVisible);
    for (auto block : Blocks_)
        block->onActivityChanged(isVisible);
}

void QuoteBlock::addBlock(GenericBlock* block)
{
    if (!getParentComplexMessage()->isHeadless())
    {
        block->setBubbleRequired(false);
        block->setInsideQuote(!Quote_.isForward_);
        block->setInsideForward(Quote_.isForward_);

        if (!Quote_.isForward_)
            block->setGalleryId(Quote_.msgId_);

        if (block->getContentType() != IItemBlock::ContentType::Text)
            block->raise();
        block->setEmojiSizeType(TextRendering::EmojiSizeType::REGULAR);
    }

    Blocks_.push_back(block);
}

bool QuoteBlock::quoteOnly() const
{
    return !ReplyBlock_;
}

bool QuoteBlock::isQuoteInteractive() const
{
    return Quote_.isInteractive();
}

QString QuoteBlock::getQuoteHeader() const
{
    return u"> " % Quote_.senderFriendly_ % u" (" % QDateTime::fromSecsSinceEpoch(Quote_.time_).toString(u"dd.MM.yyyy hh:mm") % u"): ";
}

QColor QuoteBlock::getSenderColor() const
{
    return Utils::getNameColor(getSenderAimid());
}

int32_t QuoteBlock::getLeftOffset() const
{
    return Quote_.isForward_ ? MessageStyle::Quote::getFwdOffsetLeft() : MessageStyle::Quote::getQuoteOffsetLeft();
}

int32_t QuoteBlock::getLeftAdditional() const
{
    return Quote_.isForward_ ? MessageStyle::Quote::getFwdLeftOverflow() : 0;
}

void QuoteBlock::initSenderName()
{
    QPoint offsets;
    if (senderLabel_)
        offsets = senderLabel_->offsets();

    auto text = [](const QString& _text, const QFont& _font, const Styling::ColorParameter& _color)
    {
        auto unit = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS,
            TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS, TextRendering::EmojiSizeType::REGULAR,
            ParseBackticksPolicy::ParseSingles);
        TextRendering::TextUnit::InitializeParameters params{ _font, _color };
        params.maxLinesCount_ = 1;
        unit->init(params);
        return unit;
    };

    const auto senderFriendly = Logic::GetFriendlyContainer()->getFriendly2(Quote_.senderId_);

    if (Quote_.isForward_)
    {
        const auto withChatName = !Quote_.chatName_.isEmpty() && Quote_.chatName_ != senderFriendly.name_;

        QString hdrText;
        if (withChatName)
            hdrText = QT_TRANSLATE_NOOP("quote", "Forwarded from `%1` from `%2`").arg(senderFriendly.name_, Quote_.chatName_);
        else if (!senderFriendly.default_)
            hdrText = QT_TRANSLATE_NOOP("quote", "Forwarded from `%1`").arg(senderFriendly.name_);
        else
            hdrText = QT_TRANSLATE_NOOP("quote", "Forwarded message");

        senderLabel_ = text(hdrText, MessageStyle::getQuoteSenderFont(), forwardLabelColor());
    }
    else
    {
        senderLabel_ = text(Quote_.senderFriendly_, MessageStyle::getSenderFont(), Styling::ColorParameter{ getSenderColor() });
    }

    senderLabel_->applyLinks({
        { senderFriendly.name_, Quote_.senderId_ },
        { Quote_.chatName_, Quote_.chatId_ },
    });

    senderLabel_->evaluateDesiredSize();
    desiredSenderWidth_ = senderLabel_->desiredWidth();

    if (!offsets.isNull())
        senderLabel_->setOffsets(offsets.x(), offsets.y());
}

void QuoteBlock::updateFonts()
{
    for (auto block : Blocks_)
        block->updateFonts();

    initSenderName();
}

void QuoteBlock::blockClicked()
{
    if (!isQuoteInteractive())
        return;

    const auto& id = Quote_.chatId_.isEmpty() ? getChatAimid() : Quote_.chatId_;
    if (!Quote_.isForward_ || Logic::getContactListModel()->contains(id))
    {
        if (getParentComplexMessage()->isThreadFeedMessage())
        {
            if (getParentComplexMessage()->isTopicStarter())
                Utils::InterConnector::instance().openDialog(id, Quote_.msgId_);
            else
                Q_EMIT Utils::InterConnector::instance().scrollThreadFeedToMsg(id, Quote_.msgId_);
        }
        else
        {
            auto& interConnector  = Utils::InterConnector::instance();
            const auto isFeedPage = interConnector.getMainWindow()->isFeedAppPage();
            if (const auto& selected = Logic::getContactListModel()->selectedContact(); !isFeedPage && selected != id)
                Q_EMIT interConnector.addPageToDialogHistory(selected);
            interConnector.openDialog(id, Quote_.msgId_, true, isFeedPage ? PageOpenedAs::FeedPageScrollable : PageOpenedAs::MainPage);
        }
    }
    else if (!Quote_.chatStamp_.isEmpty())
    {
        Logic::getContactListModel()->joinLiveChat(Quote_.chatStamp_, false);
    }
}

void QuoteBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
    if (hoveredBlock && isQuoteInteractive())
    {
        if (hoveredBlock == this || std::any_of(Blocks_.begin(), Blocks_.end(), [hoveredBlock](const auto _block) { return hoveredBlock == _block; }))
        {
            if (hoverRect_.isEmpty() || hoverRect_ != rect())
            {
                const auto leftMargin = Quote_.isForward_ ? 0 : MessageStyle::Quote::getLineWidth();
                const auto hoverRect = rect().adjusted(leftMargin, 0, 0, 0);

                const auto flags = Quote_.isForward_
                    ? Utils::RenderBubbleFlags::AllRounded
                    : Utils::RenderBubbleFlags::RightSideRounded;
                hoverClipPath_ = Utils::renderMessageBubble(hoverRect, MessageStyle::getBorderRadiusSmall(), 0, flags);

                hoverRect_ = rect();
            }

            Utils::PainterSaver ps(p);
            p.setRenderHint(QPainter::Antialiasing);
            p.setClipPath(hoverClipPath_);

            p.setPen(Qt::NoPen);
            p.fillRect(rect(), MessageStyle::Quote::getHoverColor(isOutgoing()));
        }
    }

    if (senderLabel_)
    {
        if (Quote_.isForward_ && Quote_.isSelectable_ && getParentComplexMessage()->containsShareableBlocks())
            senderLabel_->setColor(forwardLabelColor());

        senderLabel_->draw(p);
    }

    if (!Quote_.isForward_)
    {
        const auto left = rect().left() + MessageStyle::Quote::getLineWidth() / 2;
        const QPoint lineStart(left, rect().top());
        const QPoint lineEnd(left, rect().bottom() + 1);

        Utils::PainterSaver ps(p);
        p.setPen(QPen(getSenderColor(), MessageStyle::Quote::getLineWidth(), Qt::SolidLine, Qt::FlatCap));
        p.drawLine(lineStart, lineEnd);
    }
}

void QuoteBlock::initialize()
{
    GenericBlock::initialize();
}

void QuoteBlock::mouseMoveEvent(QMouseEvent * _e)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()) || isQuoteInteractive())
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        const auto mousePos = _e->pos();
        const auto showHand =
            !labelLink(senderLabel_, mousePos).isEmpty() ||
            std::any_of(Blocks_.begin(), Blocks_.end(), [globPos = mapToGlobal(mousePos)](const auto b) { return b->isOverLink(globPos); });

        if (showHand)
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);
    }

    GenericBlock::mouseMoveEvent(_e);
}

bool QuoteBlock::replaceBlockWithSourceText(IItemBlock *block)
{
    if (std::find(Blocks_.begin(), Blocks_.end(), block) == Blocks_.end())
        return false;

    if (!mergeReplaceByText(Blocks_, block, Parent_))
        return false;

    Q_EMIT observeToSize();
    return true;
}

bool QuoteBlock::removeBlock(IItemBlock *block)
{
    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
        return false;

    Blocks_.erase(iter);

    block->deleteLater();

    return true;
}

bool QuoteBlock::isSharingEnabled() const
{
    return false;
}

bool QuoteBlock::containsSharingBlock() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](const auto& b) { return b->isSharingEnabled(); });
}

void QuoteBlock::setMessagesCountAndIndex(int count, int index)
{
    MessagesCount_ = count;
    MessageIndex_ = index;
}

int QuoteBlock::desiredWidth(int _width) const
{
    int result = senderLabel_ ? desiredSenderWidth_ : 0;

    for (auto b : Blocks_)
        result = std::max(result, b->desiredWidth(_width));

    return (getLeftOffset() - getLeftAdditional()) + std::max(result + Ui::MessageStyle::Quote::getQuoteOffsetRight(), Ui::MessageStyle::getQuoteBlockDesiredWidth());
}

Data::LinkInfo QuoteBlock::linkAtPos(const QPoint& pos) const
{
    for (auto b : Blocks_)
    {
        auto link = b->linkAtPos(pos);
        if (!link.isEmpty())
            return link;
    }

    return {};
}

bool QuoteBlock::isOverLink(const QPoint& _mousePosGlobal) const
{
    for (auto b : Blocks_)
    {
        if (b->isOverLink(_mousePosGlobal))
            return true;
    }

    return false;
}

QStringList QuoteBlock::messageLinks() const
{
    if (!build::is_testing())
        return QStringList();

    QStringList result;
    for (const auto& b : Blocks_)
        result += b->messageLinks();
    return result;
}

bool QuoteBlock::clicked(const QPoint& _p)
{
    const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
    if (hoveredBlock == this)
    {
        QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());

        if (const auto link = labelLink(senderLabel_, mappedPoint); !link.isEmpty())
        {
            if (link == Quote_.senderId_)
            {
                const auto fromChannel = Quote_.senderId_ == Quote_.chatId_;
                if (fromChannel && !Quote_.chatStamp_.isEmpty())
                    Utils::openDialogOrProfile(Quote_.chatStamp_, Utils::OpenDOPParam::stamp);
                else
                    Utils::openDialogOrProfile(Quote_.senderId_);
            }
            else
            {
                if (!Quote_.chatStamp_.isEmpty())
                    Utils::openDialogOrProfile(Quote_.chatStamp_, Utils::OpenDOPParam::stamp);
                else
                    Utils::openDialogOrProfile(Quote_.chatId_);
            }
        }
        else
        {
            blockClicked();
        }
    }
    else
    {
        const auto isChildBlock = std::any_of(Blocks_.begin(), Blocks_.end(), [hoveredBlock](const auto _block) { return hoveredBlock == _block; });
        if (isChildBlock)
        {
            if (hoveredBlock->getContentType() == ContentType::Text)
            {
                const auto globalPoint = getParentComplexMessage()->mapToGlobal(_p);
                if (!hoveredBlock->linkAtPos(globalPoint).isEmpty() || !isQuoteInteractive())
                    hoveredBlock->clicked(_p);
                else
                    blockClicked();
            }
            else
            {
                if (!hoveredBlock->clicked(_p))
                    blockClicked();
            }
        }
    }

    return true;
}

void QuoteBlock::doubleClicked(const QPoint & _p, std::function<void(bool)> _callback)
{
    if (!isQuoteInteractive())
    {
        const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
        const auto isChildBlock = std::any_of(Blocks_.begin(), Blocks_.end(), [hoveredBlock](const auto _block) { return hoveredBlock == _block; });
        if (isChildBlock && hoveredBlock->getBlockGeometry().contains(_p))
            hoveredBlock->doubleClicked(_p, _callback);
    }
    else
    {
        if (_callback)
            _callback(true);
    }
}

bool QuoteBlock::pressed(const QPoint & _p)
{
    if (!isQuoteInteractive())
    {
        const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
        const auto isChildBlock = std::any_of(Blocks_.begin(), Blocks_.end(), [hoveredBlock](const auto _block) { return hoveredBlock == _block; });
        if (isChildBlock)
            return hoveredBlock->pressed(_p);
    }
    else if (Geometry_.contains(_p))
    {
        auto pressed = false;
        for (auto b : Blocks_)
            pressed |= b->pressed(_p);

        return pressed;
    }

    return false;
}

QString QuoteBlock::getSenderAimid() const
{
    return Quote_.senderId_;
}

bool QuoteBlock::isNeedCheckTimeShift() const
{
    if (isQuoteInteractive())
        return true;

    if (!Blocks_.empty())
        return Blocks_.back()->getContentType() != IItemBlock::ContentType::Link;

    return false;
}

void QuoteBlock::highlight(const highlightsV& _hl)
{
    for (auto b : Blocks_)
        b->highlight(_hl);
}

void QuoteBlock::removeHighlight()
{
    for (auto b : Blocks_)
        b->removeHighlight();
}

Data::FilesPlaceholderMap QuoteBlock::getFilePlaceholders()
{
    Data::FilesPlaceholderMap files;
    for (const auto& b : Blocks_)
    {
        if (const auto bf = b->getFilePlaceholders(); !bf.empty())
            files.insert(bf.begin(), bf.end());
    }
    return files;
}

bool QuoteBlock::containsText() const
{
    static const uint32_t TextMask = uint32_t(IItemBlock::ContentType::Text) | uint32_t(IItemBlock::ContentType::DebugText);
    return (findBlock(Blocks_.begin(), Blocks_.end(), TextMask) != nullptr);
}

int QuoteBlock::getMaxWidth() const
{
    auto maxWidth = 0;
    for (auto block : Blocks_)
        maxWidth = std::max(block->getMaxWidth(), maxWidth);

    if (maxWidth)
        return maxWidth + (getLeftOffset() - getLeftAdditional()) + Ui::MessageStyle::Quote::getQuoteOffsetRight();
    else
        return 0;
}

int QuoteBlock::effectiveBlockWidth() const
{
    auto result = 0;
    for (auto block : Blocks_)
    {
        result = std::max(result, block->effectiveBlockWidth());
    }

    return result;
}

void QuoteBlock::markTrustRequired()
{
    for (auto block : Blocks_)
        block->markTrustRequired();
}

bool QuoteBlock::isBlockedFileSharing() const
{
    return std::any_of(Blocks_.begin(), Blocks_.end(), [](auto b) { return b->isBlockedFileSharing(); });
}

bool QuoteBlock::needToExpand() const
{
    if (!Quote_.isForward_)
        return false;
    bool extendableBlocksOnly = true;
    bool hasCodeBlocks = false;
    for (auto block : Blocks_)
    {
        const auto contentType = block->getContentType();
        const bool codeBlock = contentType == IItemBlock::ContentType::Code;
        const auto textBlock = contentType == IItemBlock::ContentType::Text;
        hasCodeBlocks |= codeBlock;
        extendableBlocksOnly &= textBlock || codeBlock;
        if (!extendableBlocksOnly)
            return false;
    }
    return extendableBlocksOnly && hasCodeBlocks;
}

const std::vector<GenericBlock *>& QuoteBlock::getBlocks() const
{
    return Blocks_;
}

UI_COMPLEX_MESSAGE_NS_END

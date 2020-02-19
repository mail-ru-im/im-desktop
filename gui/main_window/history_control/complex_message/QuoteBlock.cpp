#include "stdafx.h"

#include "ComplexMessageItem.h"
#include "../MessageStyle.h"
#include "../../../controls/TextUnit.h"
#include "../../../controls/LabelEx.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/PainterPath.h"
#include "../../../my_info.h"
#include "../../../fonts.h"
#include "../../contact_list/ContactListModel.h"
#include "../../friendly/FriendlyContainer.h"

#include "QuoteBlockLayout.h"

#include "QuoteBlock.h"
#include "TextBlock.h"

#include "../../../styles/ThemeParameters.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    QString labelLink(const TextRendering::TextUnitPtr& _label, const QPoint& _pos)
    {
        if (_label)
            return _label->getLink(_pos);

        return QString();
    }

    QColor forwardLabelColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }
}

QuoteBlock::QuoteBlock(ComplexMessageItem *parent, const Data::Quote& quote)
    : GenericBlock(parent, quote.senderFriendly_, MenuFlagCopyable, false)
    , Quote_(std::move(quote))
    , Layout_(new QuoteBlockLayout())
    , desiredSenderWidth_(0)
    , Parent_(parent)
    , ReplyBlock_(nullptr)
    , MessagesCount_(0)
    , MessageIndex_(0)
{
    setLayout(Layout_);
    setMouseTracking(true);

    if (!getParentComplexMessage()->isHeadless())
        initSenderName();

    connect(this, &QuoteBlock::observeToSize, parent, &ComplexMessageItem::onObserveToSize);

    if (Quote_.isInteractive())
    {
        setCursor(Qt::PointingHandCursor);
        connect(parent, &ComplexMessageItem::hoveredBlockChanged, this, Utils::QOverload<>::of(&QuoteBlock::update));
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

        if (needUpdate)
        {
            update();
            updateGeometry();
        }
    }
    return needUpdate;
}

QString QuoteBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
{
    QString result;
    for (auto b : Blocks_)
    {
        if (b->isSelected())
            result += b->getSelectedText(false, _dest) % QChar::LineFeed;
    }

    if (_isFullSelect && !result.isEmpty())
        return getQuoteHeader() % result;

    return result;
}

QString QuoteBlock::getTextForCopy() const
{
    QString result;
    for (auto b : Blocks_)
    {
        result += b->getTextForCopy();
        result += QChar::LineFeed;
    }

    if (!result.isEmpty())
        return getQuoteHeader() % result;

    return result;
}

QString QuoteBlock::getSourceText() const
{
    QString result;
    for (auto b : Blocks_)
    {
        result += b->getSourceText();
        result += QChar::LineFeed;
    }

    if (!result.isEmpty())
        return getQuoteHeader() % result;

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
        const auto separator = senderFriendly.default_ ? QLatin1String() : ql1s(": ");
        if (!Quote_.description_.isEmpty())
            return friendly % separator % Blocks_.front()->formatRecentsText() % QChar::Space % Quote_.description_;

        return friendly % separator % Blocks_.front()->formatRecentsText();
    }

    assert(false);
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
            if (isOutgoing())
                rc.setLeft(rc.left() - add);
            else
                rc.setRight(rc.right() + add);
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

void QuoteBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);

    for (auto block : Blocks_)
        block->onVisibilityChanged(isVisible);
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

QString QuoteBlock::getQuoteHeader() const
{
    return ql1s("> ") % Quote_.senderFriendly_ % ql1s(" (") % QDateTime::fromTime_t(Quote_.time_).toString(qsl("dd.MM.yyyy hh:mm")) % ql1s("): ");
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

    auto text = [](const QString& _text, const QFont& _font, const QColor& _color)
    {
        auto unit = TextRendering::MakeTextUnit(_text, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        unit->init(_font, _color, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
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
        senderLabel_->markdown(MessageStyle::getQuoteSenderFontMarkdown(), forwardLabelColor(), Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    }
    else
    {
        senderLabel_ = text(Quote_.senderFriendly_, MessageStyle::getSenderFont(), getSenderColor());
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

void QuoteBlock::updateStyle()
{
    if (senderLabel_ && Quote_.isForward_ && Quote_.isSelectable_ && getParentComplexMessage()->containsShareableBlocks())
        senderLabel_->setColor(forwardLabelColor());

    for (auto block : Blocks_)
        block->updateStyle();

    update();
}

void QuoteBlock::updateFonts()
{
    for (auto block : Blocks_)
        block->updateFonts();

    initSenderName();
}

void QuoteBlock::blockClicked()
{
    if (!Quote_.isInteractive())
        return;

    if (Quote_.isForward_)
    {
        if (Logic::getContactListModel()->contains(Quote_.chatId_))
        {
            const auto selectedContact = Logic::getContactListModel()->selectedContact();
            if (selectedContact != Quote_.chatId_)
                emit Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

            emit Logic::getContactListModel()->select(Quote_.chatId_, Quote_.msgId_, Logic::UpdateChatSelection::Yes);
        }
        else if (!Quote_.chatStamp_.isEmpty())
        {
            Logic::getContactListModel()->joinLiveChat(Quote_.chatStamp_, false);
        }
    }
    else
    {
        Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), Quote_.msgId_, true, nullptr);
    }
}

void QuoteBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
    if (hoveredBlock && Quote_.isInteractive())
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
    if (Utils::InterConnector::instance().isMultiselect() || Quote_.isInteractive())
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
    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
        return false;

    auto &existingBlock = *iter;
    if (!existingBlock)
        return false;

    auto textBlock = new TextBlock(Parent_, existingBlock->getSourceText());

    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);
    textBlock->show();

    existingBlock->deleteLater();
    existingBlock = textBlock;

    emit observeToSize();

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

QString QuoteBlock::linkAtPos(const QPoint& pos) const
{
    for (auto b : Blocks_)
    {
        auto link = b->linkAtPos(pos);
        if (!link.isEmpty())
            return link;
    }

    return QString();
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
                if (!hoveredBlock->linkAtPos(globalPoint).isEmpty() || !Quote_.isInteractive())
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
    if (!Quote_.isInteractive())
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
    if (!Quote_.isInteractive())
    {
        const auto hoveredBlock = getParentComplexMessage()->getHoveredBlock();
        const auto isChildBlock = std::any_of(Blocks_.begin(), Blocks_.end(), [hoveredBlock](const auto _block) { return hoveredBlock == _block; });
        if (isChildBlock)
            return hoveredBlock->pressed(_p);
    }
    return false;
}

QString QuoteBlock::getSenderAimid() const
{
    return Quote_.senderId_;
}

bool QuoteBlock::isNeedCheckTimeShift() const
{
    if (Quote_.isInteractive())
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
    for (auto b : Blocks_)
    {
        if (b->getContentType() != IItemBlock::ContentType::Text && b->getContentType() != IItemBlock::ContentType::DebugText)
            return false;
    }

    return true;
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

const std::vector<GenericBlock *>& QuoteBlock::getBlocks() const
{
    return Blocks_;
}

UI_COMPLEX_MESSAGE_NS_END

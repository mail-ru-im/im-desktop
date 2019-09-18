#include "stdafx.h"

#include "../../../controls/TextUnit.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/utils.h"
#include "../../../gui_settings.h"

#include "../MessageStyle.h"
#include "../MessageStatusWidget.h"

#include "../../../styles/ThemesContainer.h"

#include "ComplexMessageItem.h"
#include "TextBlockLayout.h"
#include "Selection.h"

#include "TextBlock.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

TextBlock::TextBlock(ComplexMessageItem* _parent, const QString &text, const Ui::TextRendering::EmojiSizeType _emojiSizeType)
    : GenericBlock(_parent, text, MenuFlagCopyable, false)
    , Layout_(new TextBlockLayout())
    , Selection_(BlockSelectionType::None)
    , TripleClickTimer_(new QTimer(this))
    , emojiSizeType_(_emojiSizeType)
{
    assert(!text.isEmpty());

    adjustEmojiSize();

    setLayout(Layout_);

    connect(this, &TextBlock::selectionChanged, _parent, &ComplexMessageItem::selectionChanged);

    if (!_parent->isHeadless())
        initTextUnit();

    TripleClickTimer_->setInterval(QApplication::doubleClickInterval());
    TripleClickTimer_->setSingleShot(true);
}

TextBlock::~TextBlock() = default;

void TextBlock::clearSelection()
{
    if (textUnit_)
        textUnit_->clearSelection();

    Selection_ = BlockSelectionType::None;

    update();
}

IItemBlockLayout* TextBlock::getBlockLayout() const
{
    return Layout_;
}

QString TextBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
{
    if (textUnit_)
    {
        const auto textType = _dest == IItemBlock::TextDestination::quote ? TextRendering::TextType::SOURCE : TextRendering::TextType::VISIBLE;

        if (textUnit_->isAllSelected())
            return textUnit_->sourceModified() ? GenericBlock::getSourceText() : textUnit_->getSelectedText(textType);

        return textUnit_->getSelectedText(textType);
    }

    return QString();
}

bool TextBlock::updateFriendly(const QString& _aimId, const QString& _friendly)
{
    if (textUnit_ && !_aimId.isEmpty() && !_friendly.isEmpty())
    {
        const auto& mentions = textUnit_->getMentions();
        if (auto it = mentions.find(_aimId); it != mentions.end() && it->second != _friendly)
        {
            textUnit_->setTextAndMentions(GenericBlock::getSourceText(), getParentComplexMessage()->getMentions());
            notifyBlockContentsChanged();
            update();
            return true;
        }
    }

    return false;
}

bool TextBlock::isDraggable() const
{
    return false;
}

bool TextBlock::isSelected() const
{
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (textUnit_)
        return textUnit_->isSelected();

    return false;
}

bool TextBlock::isAllSelected() const
{
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (textUnit_)
        return textUnit_->isAllSelected();

    return false;
}

bool TextBlock::isSharingEnabled() const
{
    return false;
}

void TextBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    assert(selection > BlockSelectionType::Min);
    assert(selection < BlockSelectionType::Max);
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (textUnit_)
    {
        const auto localFrom = mapFromGlobal(from);
        const auto localTo = mapFromGlobal(to);

        textUnit_->select(localFrom, localTo);
    }

    Selection_ = selection;

    update();
}

void TextBlock::drawBlock(QPainter& p, const QRect&, const QColor&)
{
    if (!textUnit_)
        return;

    textUnit_->setOffsets(rect().topLeft());
    textUnit_->draw(p);
}

void TextBlock::initialize()
{
    return GenericBlock::initialize();
}

void TextBlock::mouseMoveEvent(QMouseEvent *e)
{
    if (isOverLink(mapToGlobal(e->pos())))
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);

    return GenericBlock::mouseMoveEvent(e);
}

void TextBlock::leaveEvent(QEvent *e)
{
    QApplication::restoreOverrideCursor();
    return GenericBlock::leaveEvent(e);
}

void TextBlock::initTextUnit()
{
    adjustEmojiSize();

    textUnit_ = TextRendering::MakeTextUnit(
        GenericBlock::getSourceText(),
        getParentComplexMessage()->getMentions(),
        TextRendering::LinksVisible::SHOW_LINKS,
        TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS,
        emojiSizeType_
    );

    textUnit_->init(MessageStyle::getTextFont()
                  , MessageStyle::getTextColor()
                  , MessageStyle::getLinkColor(isOutgoing())
                  , MessageStyle::getTextSelectionColor(getChatAimid())
                  , QColor()
                  , TextRendering::HorAligment::LEFT
                  , -1
                  , TextRendering::LineBreakType::PREFER_WIDTH
                  , emojiSizeType_
                  , Styling::getThemesContainer().getCurrentTheme()->linksUnderlined());

    textUnit_->markdown(MessageStyle::getMarkdownFont(), MessageStyle::getTextColor());
    textUnit_->setLineSpacing(MessageStyle::getTextLineSpacing());
}

void TextBlock::adjustEmojiSize()
{
    if (!Ui::get_gui_settings()->get_value<bool>(settings_allow_big_emoji, settings_allow_big_emoji_deafult()))
        emojiSizeType_ = Ui::TextRendering::EmojiSizeType::REGULAR;
    else
        emojiSizeType_ = Ui::TextRendering::EmojiSizeType::ALLOW_BIG;
}

bool TextBlock::isBubbleRequired() const
{
    return true;
}

bool TextBlock::pressed(const QPoint& _p)
{
    if (textUnit_ && TripleClickTimer_->isActive())
    {
        TripleClickTimer_->stop();
        textUnit_->selectAll();
        textUnit_->fixSelection();
        update();

        return true;
    }

    return false;
}

bool TextBlock::clicked(const QPoint& _p)
{
    if (textUnit_)
    {
        QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());
        textUnit_->clicked(mappedPoint);
    }

    return true;
}

void TextBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
{
    if (textUnit_)
    {
        if (!textUnit_->isAllSelected())
        {
            QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());

            auto tripleClick = [this, callback = std::move(_callback)](bool result)
            {
                if (callback)
                    callback(result);
                if (result)
                    TripleClickTimer_->start();
            };

            textUnit_->doubleClicked(mappedPoint, true, std::move(tripleClick));

            update();
        }
        else if (_callback)
        {
            // here after quad click (click after triple click)
            // fix quote creation - emulate "text is selected"
            _callback(true);
        }
    }
}

QString TextBlock::linkAtPos(const QPoint& pos) const
{
    if (textUnit_)
        return textUnit_->getLink(mapFromGlobal(pos));

    return GenericBlock::linkAtPos(pos);
}

QString TextBlock::getTrimmedText() const
{
    if (textUnit_ && textUnit_->sourceModified())
        return textUnit_->getText();

    return getSourceText();
}

int TextBlock::desiredWidth(int _w) const
{
    if (textUnit_)
    {
        auto desired = textUnit_->desiredWidth();

        const auto cm = getParentComplexMessage();
        if (cm && cm->isLastBlock(this) && textUnit_->getLineCount() > 0)
        {
            if (const auto timeWidget = cm->getTimeWidget())
            {
                const auto timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();
                if (textUnit_->needsEmojiMargin())
                {
                    desired = std::max(timeWidth, desired);
                }
                else
                {
                    if (textUnit_->getLineCount() == 1)
                        desired += timeWidth;
                    else
                        desired = std::max(textUnit_->getLastLineWidth() + timeWidth, desired);
                }
            }
        }

        return desired;
    }

    return GenericBlock::desiredWidth(_w);
}

QString TextBlock::getSourceText() const
{
    auto result = GenericBlock::getSourceText();
    if (result.startsWith(QChar::LineFeed))
        result.remove(0, 1);

    return result;
}

QString TextBlock::getTextForCopy() const
{
    if (!textUnit_)
        return GenericBlock::getTextForCopy();

    if (textUnit_->sourceModified())
        return textUnit_->getSourceText();
    else
        return textUnit_->getText();
}

bool TextBlock::getTextStyle() const
{
    if (textUnit_)
        return textUnit_->sourceModified();
    return false;
}

void TextBlock::releaseSelection()
{
    if (textUnit_)
        textUnit_->releaseSelection();
}

bool TextBlock::isOverLink(const QPoint& _mousePosGlobal) const
{
    return textUnit_ && textUnit_->isOverLink(mapFromGlobal(_mousePosGlobal));
}

void TextBlock::setText(const QString& _text)
{
    setSourceText(_text);
    updateStyle();
}

void TextBlock::setEmojiSizeType(const TextRendering::EmojiSizeType & _emojiSizeType)
{
    if (textUnit_)
        textUnit_->setEmojiSizeType(_emojiSizeType);
}

void TextBlock::updateStyle()
{
    Layout_->markDirty();
    initTextUnit();
    notifyBlockContentsChanged();
}

void TextBlock::updateFonts()
{
    updateStyle();
}

UI_COMPLEX_MESSAGE_NS_END

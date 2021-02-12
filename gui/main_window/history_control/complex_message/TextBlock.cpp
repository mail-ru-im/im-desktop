#include "stdafx.h"

#include "main_window/containers/LastseenContainer.h"
#include "controls/TextUnit.h"
#include "controls/textrendering/TextRendering.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/async/AsyncTask.h"
#include "styles/ThemesContainer.h"
#include "gui_settings.h"

#include "../MessageStyle.h"
#include "../MessageStatusWidget.h"

#include "ComplexMessageItem.h"
#include "TextBlockLayout.h"
#include "TextBlock.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

TextBlock::TextBlock(ComplexMessageItem* _parent, const QString& _text, const Ui::TextRendering::EmojiSizeType _emojiSizeType)
    : GenericBlock(_parent, _text, MenuFlags(MenuFlagCopyable), false)
    , Layout_(new TextBlockLayout())
    , emojiSizeType_(_emojiSizeType)
{
    assert(!_text.isEmpty());

    Testing::setAccessibleName(this, u"AS HistoryPage messageText " % QString::number(_parent->getId()));

    setLayout(Layout_);

    connect(this, &TextBlock::selectionChanged, _parent, &ComplexMessageItem::selectionChanged);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::emojiSizeChanged, this, &TextBlock::adjustEmojiSize);

    if (!_parent->isHeadless())
        initTextUnit();
}

TextBlock::~TextBlock() = default;

void TextBlock::clearSelection()
{
    if (textUnit_)
        textUnit_->clearSelection();

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
            onTextUnitChanged();
            startSpellCheckingIfNeeded();
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
    return textUnit_ && textUnit_->isSelected();
}

bool TextBlock::isAllSelected() const
{
    return textUnit_ && textUnit_->isAllSelected();
}

bool TextBlock::isSharingEnabled() const
{
    return false;
}

void TextBlock::selectByPos(const QPoint& from, const QPoint& to, bool)
{
    if (textUnit_)
    {
        const auto localFrom = mapFromGlobal(from);
        const auto localTo = mapFromGlobal(to);

        textUnit_->select(localFrom, localTo);
    }

    update();
}

void TextBlock::selectAll()
{
    if (textUnit_)
        textUnit_->selectAll();

    update();
}

IItemBlock::MenuFlags TextBlock::getMenuFlags(QPoint p) const
{
    auto flags = GenericBlock::getMenuFlags(p);
    if (!textUnit_ || !ComplexMessageItem::isSpellCheckIsOn())
        return flags;

    if (textUnit_->sourceModified())
        return flags;

    if (const auto w = textUnit_->getWordAt(mapPoint(p)); w && (!textUnit_->isSkippableWordForSpellChecking(*w) || (w->syntaxWord && w->syntaxWord->spellError)))
        return MenuFlags(flags | MenuFlags::MenuFlagSpellItems);

    return flags;
}

void TextBlock::startSpellChecking()
{
    needSpellCheck_ = true;
    if (!textUnit_ || textUnit_->sourceModified() || !ComplexMessageItem::isSpellCheckIsOn())
        return;

    textUnit_->startSpellChecking([guard = QPointer(this)]()
    {
        return guard && guard->textUnit_;
    }, [weak_this = QPointer(this)](bool res) mutable
    {
        if (res && weak_this)
            Async::runInMain([weak_this = std::move(weak_this)]()
        {
            if (weak_this && weak_this->textUnit_)
            {
                weak_this->textUnit_->getHeight(weak_this->textUnit_->cachedSize().width(), Ui::TextRendering::CallType::FORCE);
                weak_this->onTextUnitChanged();
            }
        });
    });
}

void TextBlock::setSpellErrorsVisible(bool _visible)
{
    if (std::exchange(needSpellCheck_, _visible) != _visible)
        reinit();
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
    if (!Utils::InterConnector::instance().isMultiselect())
    {
        if (isOverLink(mapToGlobal(e->pos())))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }

    return GenericBlock::mouseMoveEvent(e);
}

void TextBlock::leaveEvent(QEvent *e)
{
    QApplication::restoreOverrideCursor();
    return GenericBlock::leaveEvent(e);
}

void TextBlock::reinit()
{
    initTextUnit();
    onTextUnitChanged();
}

void TextBlock::initTextUnit()
{
    textUnit_ = TextRendering::MakeTextUnit(
        GenericBlock::getSourceText(),
        getParentComplexMessage()->getMentions(),
        TextRendering::LinksVisible::SHOW_LINKS,
        TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS,
        emojiSizeType_);

    const auto linkStyle = Styling::getThemesContainer().getCurrentTheme()->isLinksUnderlined() ? TextRendering::LinksStyle::UNDERLINED : TextRendering::LinksStyle::PLAIN;
    textUnit_->init(MessageStyle::getTextFont()
                  , MessageStyle::getTextColor()
                  , MessageStyle::getLinkColor()
                  , MessageStyle::getTextSelectionColor(getChatAimid())
                  , MessageStyle::getHighlightColor()
                  , TextRendering::HorAligment::LEFT
                  , -1
                  , TextRendering::LineBreakType::PREFER_WIDTH
                  , emojiSizeType_
                  , linkStyle);

    textUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
    textUnit_->markdown(MessageStyle::getTextMonospaceFont(), MessageStyle::getTextColor());
    textUnit_->setLineSpacing(MessageStyle::getTextLineSpacing());

    const auto& contact = getChatAimid();
    const auto isCommandsEnabled = Logic::GetLastseenContainer()->isBot(contact) || Utils::isChat(contact);
    if (!isCommandsEnabled)
        textUnit_->disableCommands();

    startSpellCheckingIfNeeded();
}

void TextBlock::adjustEmojiSize()
{
    if (isInsideForward() || isInsideQuote())
        return;

    const auto bigEmojiAllowed = Ui::get_gui_settings()->get_value<bool>(settings_allow_big_emoji, settings_allow_big_emoji_default());
    setEmojiSizeType(bigEmojiAllowed ? TextRendering::EmojiSizeType::ALLOW_BIG : TextRendering::EmojiSizeType::REGULAR);
}

void TextBlock::initTripleClickTimer()
{
    if (TripleClickTimer_)
        return;
    TripleClickTimer_ = new QTimer(this);
    TripleClickTimer_->setInterval(QApplication::doubleClickInterval());
    TripleClickTimer_->setSingleShot(true);
}

void TextBlock::onTextUnitChanged()
{
    Layout_->markDirty();
    notifyBlockContentsChanged();
    update();
}

bool TextBlock::isBubbleRequired() const
{
    return true;
}

bool TextBlock::pressed(const QPoint& _p)
{
    if (textUnit_)
    {
        if (TripleClickTimer_ && TripleClickTimer_->isActive())
        {
            TripleClickTimer_->stop();
            textUnit_->selectAll();
            textUnit_->fixSelection();
            update();

            return true;
        }
        else if (textUnit_->isOverLink(mapPoint(_p)))
        {
            return true;
        }
    }

    return false;
}

bool TextBlock::clicked(const QPoint& _p)
{
    if (textUnit_)
        textUnit_->clicked(mapPoint(_p));

    return true;
}

void TextBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
{
    if (textUnit_)
    {
        if (!textUnit_->isAllSelected())
        {
            auto tripleClick = [this, callback = std::move(_callback)](bool result)
            {
                if (callback)
                    callback(result);
                if (result)
                {
                    initTripleClickTimer();
                    TripleClickTimer_->start();
                }
            };

            textUnit_->doubleClicked(mapPoint(_p), true, std::move(tripleClick));

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

std::optional<QString> TextBlock::getWordAt(QPoint pos) const
{
    if (textUnit_)
    {
        if (auto w = textUnit_->getWordAt(mapFromGlobal(pos)); w)
        {
            auto text = w->word.getText();
            if (w->syntaxWord)
                return std::move(text).mid(w->syntaxWord->pos, w->syntaxWord->size);
            return text;
        }
    };
    return {};
}

bool TextBlock::replaceWordAt(const QString& _old, const QString& _new, QPoint pos)
{
    return textUnit_ && textUnit_->replaceWordAt(_old, _new, mapFromGlobal(pos));
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

QString TextBlock::getTextForCopy() const
{
    if (!textUnit_)
        return GenericBlock::getTextForCopy();

    if (textUnit_->sourceModified())
        return textUnit_->getSourceText();
    else
        return textUnit_->getText();
}

QString TextBlock::getTextInstantEdit() const
{
    if (!textUnit_)
        return GenericBlock::getTextInstantEdit();
    return textUnit_->getTextInstantEdit();
}

bool TextBlock::getTextStyle() const
{
    return textUnit_ && textUnit_->sourceModified();
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
    reinit();
}

void TextBlock::setEmojiSizeType(const TextRendering::EmojiSizeType _emojiSizeType)
{
    if (textUnit_ && emojiSizeType_ != _emojiSizeType)
    {
        emojiSizeType_ = _emojiSizeType;

        if (textUnit_ && textUnit_->hasEmoji())
            reinit();
    }
}

void TextBlock::highlight(const highlightsV& _hl)
{
    if (textUnit_)
    {
        textUnit_->setHighlighted(_hl);
        update();
    }
}

void TextBlock::removeHighlight()
{
    if (textUnit_)
    {
        textUnit_->setHighlighted(false);
        update();
    }
}

void TextBlock::updateStyle()
{
    reinit();
}

void TextBlock::updateFonts()
{
    reinit();
}

QPoint TextBlock::mapPoint(const QPoint& _complexMsgPoint) const
{
    return mapFromParent(_complexMsgPoint, Layout_->getBlockGeometry());
}

void TextBlock::startSpellCheckingIfNeeded()
{
    if (needSpellCheck_)
        startSpellChecking();
}

UI_COMPLEX_MESSAGE_NS_END

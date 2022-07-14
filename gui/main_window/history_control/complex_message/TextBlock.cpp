#include "stdafx.h"

#include "main_window/containers/LastseenContainer.h"
#include "controls/TextUnit.h"
#include "controls/textrendering/TextRendering.h"
#include "controls/TooltipWidget.h"
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

TextBlock::TextBlock(ComplexMessageItem* _parent, const QString& _text, TextRendering::EmojiSizeType _emojiSizeType)
    : GenericBlock(_parent, _text, MenuFlags(MenuFlagCopyable), false)
    , Layout_(new TextBlockLayout())
    , emojiSizeType_(_emojiSizeType)
{
    init(_parent);
}

TextBlock::TextBlock(ComplexMessageItem* _parent, Data::FStringView _text, TextRendering::EmojiSizeType _emojiSizeType)
    : GenericBlock(_parent, _text, MenuFlags(MenuFlagCopyable), false)
    , Layout_(new TextBlockLayout())
    , emojiSizeType_(_emojiSizeType)
{
    init(_parent);
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

Data::FString TextBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
{
    if (textUnit_)
    {
        const auto textType = _dest == IItemBlock::TextDestination::quote ? TextRendering::TextType::SOURCE : TextRendering::TextType::VISIBLE;
        if (textUnit_->isAllSelected() && textType == TextRendering::TextType::SOURCE)
            return getSourceText();

        return textUnit_->getSelectedText(textType);
    }

    return {};
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

IItemBlock::MenuFlags TextBlock::getMenuFlags(QPoint _p) const
{
    auto flags = GenericBlock::getMenuFlags(_p);
    if (!textUnit_ || !ComplexMessageItem::isSpellCheckIsOn())
        return flags;

    if (const auto w = textUnit_->getWordAt(mapPoint(_p)); w && (!textUnit_->isSkippableWordForSpellChecking(*w) || (w->syntaxWord && w->syntaxWord->spellError)))
        return MenuFlags(flags | MenuFlags::MenuFlagSpellItems);

    return flags;
}

void TextBlock::startSpellChecking()
{
    needSpellCheck_ = true;
    if (!textUnit_ || !ComplexMessageItem::isSpellCheckIsOn())
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

bool TextBlock::needStretchToOthers() const
{
    if (!textUnit_)
        return false;

    return textUnit_->mightStretchForLargerWidth();
}

void TextBlock::stretchToWidth(const int _width)
{
    auto rect = geometry();
    if (_width == rect.width())
        return;

    rect.setWidth(_width);
    setGeometry(rect);
    if (textUnit_)
        textUnit_->getHeight(_width, Ui::TextRendering::CallType::FORCE);
}

bool TextBlock::managesTime() const
{
    return !needStretchToOthers();
}

bool TextBlock::isNeedCheckTimeShift() const
{
    return false;
}

void TextBlock::anyMouseButtonReleased()
{
    hideTooltip();
}

void TextBlock::updateWith(IItemBlock* _other)
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageText " % QString::number(getParentComplexMessage()->getId()));
    GenericBlock::updateWith(_other);
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

void TextBlock::showTooltip(const QString& _text, QRect _rect)
{
    GenericBlock::showTooltip(_text, _rect, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top);
}

void TextBlock::botCommandsDisabledChatsUpdated()
{
    const auto& contact = getChatAimid();
    if (!Utils::isChat(contact))
        return;

    if (commandsDisabledForThisChat_ != Utils::InterConnector::instance().areBotCommandsDisabled(contact))
        reinit();
}

void TextBlock::mouseMoveEvent(QMouseEvent *e)
{
    if (!Utils::InterConnector::instance().isMultiselect(getChatAimid()))
    {
        const auto globalPos = mapToGlobal(e->pos());
        if (const auto link = linkAtPos(globalPos); !link.isEmpty())
        {
            setCursor(Qt::PointingHandCursor);
            if (!isTooltipActivated() && link.isFormattedLink())
            {
                const auto linkTopLeft = mapToGlobal(rect().topLeft()) + link.rect_.topLeft();
                const auto linkGlobalRect = QRect{linkTopLeft, link.rect_.size()};

                if (!Utils::isMentionLink(link.url_))
                    showTooltip(link.url_, linkGlobalRect);
                else
                    showMentionTooltip(Utils::getEmailFromMentionLink(link.url_), link.displayName_, linkGlobalRect, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top);
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor);
            hideTooltip();
        }
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }

    return GenericBlock::mouseMoveEvent(e);
}

void TextBlock::leaveEvent(QEvent *e)
{
    Q_EMIT Utils::InterConnector::instance().stopTooltipTimer();
    if (isTooltipActivated())
        hideTooltip();

    QApplication::restoreOverrideCursor();
    return GenericBlock::leaveEvent(e);
}

void TextBlock::init(ComplexMessageItem* _parent)
{
    im_assert(!GenericBlock::getSourceText().isEmpty());

    Testing::setAccessibleName(this, u"AS HistoryPage messageText " % QString::number(_parent->getId()));

    setLayout(Layout_);

    connect(this, &TextBlock::selectionChanged, _parent, &ComplexMessageItem::selectionChanged);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::emojiSizeChanged, this, &TextBlock::adjustEmojiSize);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::botCommandsDisabledChatsUpdated, this, &TextBlock::botCommandsDisabledChatsUpdated);
    connect(_parent, &ComplexMessageItem::contextMenuOpened, this, &TextBlock::hideTooltip);

    if (!_parent->isHeadless())
        initTextUnit();
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
        emojiSizeType_,
        ParseBackticksPolicy::ParseSinglesAndTriples);

    const auto linkStyle = Styling::getThemesContainer().getCurrentTheme()->isLinksUnderlined() ? TextRendering::LinksStyle::UNDERLINED : TextRendering::LinksStyle::PLAIN;
    TextRendering::TextUnit::InitializeParameters params{ MessageStyle::getTextFont(), MessageStyle::getTextColorKey() };
    params.monospaceFont_ = MessageStyle::getTextMonospaceFont();
    params.linkColor_ = MessageStyle::getLinkColorKey();
    params.selectionColor_ = MessageStyle::getTextSelectionColorKey(getChatAimid());
    params.highlightColor_ = MessageStyle::getHighlightColorKey();
    params.emojiSizeType_ = emojiSizeType_;
    params.linksStyle_ = linkStyle;
    textUnit_->init(params);

    textUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColorKey());
    textUnit_->setLineSpacing(MessageStyle::getTextLineSpacing());

    const auto& contact = getChatAimid();
    const auto isChat = Utils::isChat(contact);
    if (isChat)
        commandsDisabledForThisChat_ = Utils::InterConnector::instance().areBotCommandsDisabled(contact);

    const auto areCommandsEnabled = Logic::GetLastseenContainer()->isBot(contact) || (isChat && !commandsDisabledForThisChat_);
    if (!areCommandsEnabled)
        textUnit_->disableCommands();

    startSpellCheckingIfNeeded();
    if (build::is_testing())
        updateLinkMap();
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

Data::LinkInfo TextBlock::linkAtPos(const QPoint& pos) const
{
    if (textUnit_)
        return textUnit_->getLink(mapFromGlobal(pos));

    return GenericBlock::linkAtPos(pos);
}

std::optional<Data::FString> TextBlock::getWordAt(QPoint pos) const
{
    if (textUnit_)
    {
        if (auto w = textUnit_->getWordAt(mapFromGlobal(pos)); w)
        {
            auto text = Data::FString();
            if (auto view = w->word.view(); view.isEmpty())
                text = w->word.getText();
            else
                text = view.toFString();

            if (w->syntaxWord)
                return Data::FStringView(text).mid(w->syntaxWord->offset_, w->syntaxWord->size_).toFString();
            return text;
        }
    };
    return {};
}

bool TextBlock::replaceWordAt(const Data::FString& _old, const Data::FString& _new, QPoint pos)
{
    return textUnit_ && textUnit_->replaceWordAt(_old, _new, mapFromGlobal(pos));
}

QString TextBlock::getTrimmedText() const
{
    return getSourceText().string();
}

int TextBlock::desiredWidth(int _w) const
{
    if (textUnit_)
    {
        if (textUnit_->cachedSize().isEmpty())
            textUnit_->getHeight(_w);

        auto desired = textUnit_->desiredWidth();
        const auto cm = getParentComplexMessage();
        if (cm && managesTime() && cm->isLastBlock(this) && !textUnit_->isEmpty())
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
                    if (desired < _w)
                        desired += timeWidth;
                    else if (desired != 0 && desired != _w)
                        desired = _w % desired;
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

    return textUnit_->getText();
}

Data::FString TextBlock::getTextInstantEdit() const
{
    if (!textUnit_)
        return GenericBlock::getTextInstantEdit();
    return textUnit_->getTextInstantEdit();
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

void TextBlock::updateLinkMap()
{
    linkWordMap_.clear();
    if (!textUnit_)
        return;

    ExtractLinkWords(textUnit_, linkWordMap_);
}

void TextBlock::setText(const Data::FString& _text)
{
    setSourceText(_text);
    reinit();
}

QStringList TextBlock::messageLinks() const
{
    QStringList result;
    for (const auto& link : linkWordMap_)
        result += link->getText();
    return result;
}

bool TextBlock::activateLink(size_t _linkIndex) const
{
    if (_linkIndex < 0 || _linkIndex >= linkWordMap_.size())
        return false;

    linkWordMap_[_linkIndex]->clicked();
    return true;
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

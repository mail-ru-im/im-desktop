#include "stdafx.h"
#include "TextUnit.h"
#include "textrendering/TextRenderingUtils.h"
#include "../utils/features.h"

#include "../styles/ThemeParameters.h"
#include "../spellcheck/Spellchecker.h"
#include "../utils/utils.h"

namespace
{
    auto counter() noexcept
    {
        static int64_t v = 0;
        return ++v;
    }

    void extractBlockLinks(std::vector<const Ui::TextRendering::TextWord*>& _mapping, const Ui::TextRendering::BaseDrawingBlockPtr& block);

    void extractLinks(std::vector<const Ui::TextRendering::TextWord*>& _mapping, const Ui::TextRendering::TextDrawingBlock* _text)
    {
        if (!_text)
            return;

        for (const auto& word : _text->getWords())
            if (word.isLink())
                _mapping.emplace_back(&word);
    }

    void extractLinks(std::vector<const Ui::TextRendering::TextWord*>& _mapping, const Ui::TextRendering::TextDrawingParagraph* _paragraph)
    {
        if (!_paragraph)
            return;

        for (const auto& b : _paragraph->getBlocks())
            extractBlockLinks(_mapping, b);
    }

    void extractBlockLinks(std::vector<const Ui::TextRendering::TextWord*>& _mapping, const Ui::TextRendering::BaseDrawingBlockPtr& block)
    {
        using namespace Ui::TextRendering;
        switch (block->getType())
        {
        case BlockType::Text:
            extractLinks(_mapping, static_cast<TextDrawingBlock*>(block.get()));
            break;
        case BlockType::Paragraph:
            extractLinks(_mapping, static_cast<TextDrawingParagraph*>(block.get()));
            break;
        default:
            break;
        }
    }
}

namespace Ui
{
namespace TextRendering
{

    TextUnit::TextUnit()
        : showLinks_(LinksVisible::SHOW_LINKS)
        , processLineFeeds_(ProcessLineFeeds::KEEP_LINE_FEEDS)
        , emojiSizeType_(EmojiSizeType::REGULAR)
        , linksStyle_(isLinksUnderlined() ? LinksStyle::UNDERLINED : LinksStyle::PLAIN)
        , blockId_(counter())
    {
    }

    TextUnit::TextUnit(std::vector<BaseDrawingBlockPtr>&& _blocks, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType)
        : blocks_(std::move(_blocks))
        , mentions_(_mentions)
        , showLinks_(_showLinks)
        , processLineFeeds_(_processLineFeeds)
        , emojiSizeType_(_emojiSizeType)
        , linksStyle_(isLinksUnderlined() ? LinksStyle::UNDERLINED : LinksStyle::PLAIN)
        , blockId_(counter())
    {
    }

    void TextUnit::setSourceText(const Data::FString& _text, ProcessLineFeeds _processLineFeeds)
    {
        sourceText_ = _text;
        sourceText_.removeMentionFormats();
        if (_processLineFeeds == ProcessLineFeeds::REMOVE_LINE_FEEDS)
            sourceText_.replace(QChar::LineFeed, QChar::Space);
    }


    int TextUnit::sourceTextWidth() const
    {
        QFontMetrics fm(getFont());
        return fm.width(getSourceText().string());
    }

    int TextUnit::textWidth() const
    {
        QFontMetrics fm(getFont());
        return fm.width(getText());
    }

    void TextUnit::appendToSourceText(TextUnit& _suffix)
    {
        const auto viewShift = sourceText_.size();
        sourceText_ += _suffix.sourceText_;
        TextRendering::updateWordsView(_suffix.blocks_, sourceText_, viewShift);
    }

    void TextUnit::initializeBlocks()
    {
        initBlocks(blocks_, font_, monospaceFont_, color_.actualColor(), linkColor_.actualColor(), selectionColor_.actualColor(), highlightColor_.actualColor(), align_, emojiSizeType_, linksStyle_);
    }

    void TextUnit::initializeBlocksAndSelectedTextColor()
    {
        initializeBlocks();
        if (highlightTextColor_.isValid())
            setBlocksHighlightedTextColor(blocks_, highlightTextColor_.actualColor());
    }

    TextUnit::TextUnit(const Data::FString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType)
        : TextUnit(std::vector<BaseDrawingBlockPtr>(), _mentions, _showLinks, _processLineFeeds, _emojiSizeType)
    {
        setSourceText(_text, _processLineFeeds);
        blocks_ = parseForParagraphs(sourceText_, _mentions, _showLinks);
    }

    void TextUnit::setText(const QString& _text, const Styling::ThemeColorKey& _color)
    {
        setText(Data::FString(_text), _color);
    }

    void TextUnit::setText(const Data::FString& _text, const Styling::ThemeColorKey& _color)
    {
        if (_color.isValid())
            color_ = _color;

        setSourceText(_text, processLineFeeds_);
        blocks_ = parseForParagraphs(sourceText_, mentions_, showLinks_);

        blockId_ = counter();
        initializeBlocks();
        setBlocksMaxLinesCount(blocks_, maxLinesCount_, lineBreak_);
        evaluateDesiredSize();
    }

    void TextUnit::setMentions(const Data::MentionMap& _mentions)
    {
        if (mentions_ == _mentions)
            return;

        mentions_ = _mentions;
        setText(getSourceText());
    }

    const Data::MentionMap& TextUnit::getMentions() const
    {
        return mentions_;
    }

    void TextUnit::setTextAndMentions(const QString& _text, const Data::MentionMap& _mentions)
    {
        if (mentions_ != _mentions)
            mentions_ = _mentions;
        setText(_text);
    }

    void TextUnit::setTextAndMentions(const Data::FString& _text, const Data::MentionMap& _mentions)
    {
        if (mentions_ != _mentions)
            mentions_ = _mentions;
        setText(_text);
    }

    void TextUnit::elide(int _width)
    {
        return elideBlocks(blocks_, _width);
    }

    bool TextUnit::isElided() const
    {
        return isBlocksElided(blocks_);
    }

    void TextUnit::forEachBlockOfType(BlockType _blockType, const modify_callback_t& _modifyCallback)
    {
        if (!_modifyCallback)
            return;

        for (size_t i = 0; i < blocks_.size(); ++i)
        {
            if (blocks_[i]->getType() == _blockType)
                _modifyCallback(blocks_[i], i);
        }
    }

    bool TextUnit::removeBlocks(BlockType _blockType, const condition_callback_t& _removeConditionCallback)
    {
        bool result = false;
        auto checkRemoveLambda = [&_removeConditionCallback, _blockType](BaseDrawingBlockPtr &block) {
            if (block->getType() != _blockType)
                return false;

            return !_removeConditionCallback || _removeConditionCallback(block);
        };

        const auto it = std::remove_if(blocks_.begin(), blocks_.end(), checkRemoveLambda);
        result = it != blocks_.end();
        blocks_.erase(it, blocks_.end());

        return result;
    }

    bool TextUnit::replaceBlock(size_t _atIndex, BaseDrawingBlockPtr&& _newBlock, const condition_callback_t &_replaceConditionCallback)
    {
        if (_atIndex >= blocks_.size())
            return false;

        if (_replaceConditionCallback && _replaceConditionCallback(blocks_[_atIndex]) == false)
            return false;

        blocks_[_atIndex] = std::move(_newBlock);
        return true;
    }

    bool TextUnit::insertBlock(BaseDrawingBlockPtr&& _block, size_t _atIndex)
    {
        if (!blocks_.empty() && _atIndex > blocks_.size())
            return false;

        blocks_.insert(std::begin(blocks_) + _atIndex, std::move(_block));
        return true;
    }

    void TextUnit::assignBlock(BaseDrawingBlockPtr&& _block)
    {
        blocks_.push_back(std::move(_block));
    }

    void TextUnit::assignBlocks(std::vector<BaseDrawingBlockPtr>&& _blocks)
    {
        blocks_.insert(blocks_.end(), std::make_move_iterator(_blocks.begin()), std::make_move_iterator(_blocks.end()));
    }

    void TextUnit::setOffsets(int _horOffset, int _verOffset)
    {
        horOffset_ = _horOffset;
        verOffset_ = _verOffset;
    }

    int TextUnit::horOffset() const
    {
        return horOffset_;
    }

    int TextUnit::verOffset() const
    {
        return verOffset_;
    }

    QPoint TextUnit::offsets() const
    {
        return QPoint(horOffset(), verOffset());
    }

    void TextUnit::draw(QPainter& _painter, VerPosition _pos)
    {
        lastVerPosition_ = _pos;
        updateColors();
        return drawBlocks(blocks_, _painter, QPoint(horOffset_, verOffset_), _pos);
    }

    void TextUnit::drawSmart(QPainter& _painter, int _center)
    {
        return drawBlocksSmart(blocks_, _painter, QPoint(horOffset_, verOffset_), _center);
    }

    int TextUnit::getHeight(int _width, CallType _calltype)
    {
        cachedSize_ = QSize(_width, getBlocksHeight(blocks_, _width, lineSpacing_, _calltype));

        if (blocks_.size() == 1 && blocks_.front()->needsEmojiMargin())
            needsEmojiMargin_ = true;

        return cachedSize_.height();
    }

    void TextUnit::evaluateDesiredSize()
    {
        getHeight(desiredWidth());
    }

    void TextUnit::init(const InitializeParameters& _parameters)
    {
        font_ = _parameters.font_;
        monospaceFont_ = _parameters.monospaceFont_;
        color_ = Styling::ColorContainer{ _parameters.color_ };
        linkColor_ = _parameters.linkColor_;
        selectionColor_ = _parameters.selectionColor_;
        highlightColor_ = _parameters.highlightColor_;
        align_ = _parameters.align_;
        maxLinesCount_ = _parameters.maxLinesCount_;
        lineBreak_ = _parameters.lineBreak_;
        linksStyle_ = _parameters.linksStyle_;
        initializeBlocks();
        setBlocksMaxLinesCount(blocks_, maxLinesCount_, lineBreak_);
    }

    void TextUnit::setLastLineWidth(int _width)
    {
        setBlocksLastLineWidth(blocks_, _width);
    }

    size_t TextUnit::getLinesCount() const
    {
        return std::accumulate(blocks_.begin(), blocks_.end(), 0, [](auto currentValue, const auto& b) { return b->getLinesCount() + currentValue; });
    }

    void TextUnit::select(QPoint _from, QPoint _to)
    {
        return selectBlocks(blocks_, mapPoint(_from), mapPoint(_to));
    }

    void TextUnit::selectAll()
    {
        return selectAllBlocks(blocks_);
    }

    void TextUnit::fixSelection()
    {
        return fixBlocksSelection(blocks_);
    }

    void TextUnit::clearSelection()
    {
        return clearBlocksSelection(blocks_);
    }

    void TextUnit::releaseSelection()
    {
        return releaseBlocksSelection(blocks_);
    }

    bool TextUnit::isSelected() const
    {
        return isBlocksSelected(blocks_);
    }

    void TextUnit::clicked(const QPoint& _p)
    {
        return blocksClicked(blocks_, mapPoint(_p));
    }

    void TextUnit::doubleClicked(const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback)
    {
        return blocksDoubleClicked(blocks_, mapPoint(_p), _fixSelection, std::move(_callback));
    }

    bool TextUnit::isOverLink(const QPoint& _p) const
    {
        return TextRendering::isAnyBlockOverLink(blocks_, mapPoint(_p));
    }

    Data::LinkInfo TextUnit::getLink(const QPoint& _p) const
    {
        return getBlocksLink(blocks_, mapPoint(_p));
    }

    std::optional<TextWordWithBoundary> TextUnit::getWordAt(QPoint _p) const
    {
        return getWord(blocks_, mapPoint(_p));
    }

    bool TextUnit::replaceWordAt(const Data::FString& _old, const Data::FString& _new, QPoint _p)
    {
        if (auto w = getWordAt(_p))
        {
            if (auto view = w->word.view().trimmed(); !view.isEmpty())
            {
                const auto sourceView = view.sourceView();
                const auto sourceRange = view.getRangeOf(_old);

                if(sourceRange != core::data::range())
                {
                    Data::FString::Builder builder;
                    builder %= sourceView.left(sourceRange.offset_);
                    builder %= _new;
                    builder %= sourceView.mid(sourceRange.end());
                    setText(builder.finalize());
                    return true;
                }

            }
        }
        return false;
    }

    Data::FString TextUnit::getSelectedText(TextType _type) const
    {
        if (sourceText_.hasFormatting())
            return getBlocksSelectedText(blocks_);
        else
            return getBlocksSelectedPlainText(blocks_, _type);
    }

    QString TextUnit::getText() const
    {
        return getBlocksText(blocks_);
    }

    Data::FString TextUnit::getTextInstantEdit() const
    {
        return getBlocksTextForInstantEdit(blocks_);
    }

    Data::FString TextUnit::getSourceText() const
    {
        return sourceText_;
    }

    bool TextUnit::isAllSelected() const
    {
        return isAllBlocksSelected(blocks_);
    }

    int TextUnit::desiredWidth() const
    {
        return getBlocksDesiredWidth(blocks_);
    }

    void TextUnit::applyLinks(const std::map<QString, QString>& _links)
    {
        return applyBlocksLinks(blocks_, _links);
    }

    void TextUnit::applyFontToLinks(const QFont& _font)
    {
        return applyLinksFont(blocks_, _font);
    }

    bool TextUnit::contains(const QPoint& _p) const
    {
        return QRect(horOffset_, verOffset_, cachedSize_.width(), cachedSize_.height()).contains(_p);
    }

    QSize TextUnit::cachedSize() const
    {
        return cachedSize_;
    }

    void TextUnit::setColor(const Styling::ColorParameter& _color)
    {
        color_ = Styling::ColorContainer{ _color };
        setBlocksColor(blocks_, color_.cachedColor());
    }

    void TextUnit::setLinkColor(const Styling::ThemeColorKey& _color)
    {
        linkColor_ = _color;
        setBlocksLinkColor(blocks_, linkColor_.cachedColor());
    }

    void TextUnit::setSelectionColor(const Styling::ThemeColorKey& _color)
    {
        selectionColor_ = _color;
        setBlocksSelectionColor(blocks_, selectionColor_.cachedColor());
    }

    void TextUnit::setHighlightedTextColor(const Styling::ThemeColorKey& _color)
    {
        highlightTextColor_ = _color;
        setBlocksHighlightedTextColor(blocks_, highlightTextColor_.cachedColor());
    }

    void TextUnit::setHighlightColor(const Styling::ThemeColorKey& _color)
    {
        highlightColor_ = _color;
        setBlocksHighlightColor(blocks_, highlightColor_.cachedColor());
    }

    Styling::ThemeColorKey TextUnit::getColorKey() const
    {
        return color_.key();
    }

    QColor TextUnit::getColor() const
    {
        return color_.cachedColor();
    }

    Styling::ThemeColorKey TextUnit::getLinkColorKey() const
    {
        return linkColor_.key();
    }

    HorAligment TextUnit::getAlign() const
    {
        return align_;
    }

    void TextUnit::setColorForAppended(const Styling::ThemeColorKey& _color)
    {
        return setBlocksColorForAppended(blocks_, Styling::getColor(_color), appended_);
    }

    const QFont& TextUnit::getFont() const
    {
        return font_;
    }

    void TextUnit::append(std::unique_ptr<TextUnit>&& _other)
    {
        appendToSourceText(*_other);
        TextRendering::appendBlocks(blocks_, _other->blocks_);
    }

    void TextUnit::appendBlocks(std::unique_ptr<TextUnit>&& _other)
    {
        appendToSourceText(*_other);
        appendWholeBlocks(blocks_, _other->blocks_);
        appended_ = blocks_.size();
    }

    void TextUnit::setLineSpacing(int _spacing)
    {
        lineSpacing_ = _spacing;
    }

    void TextUnit::setHighlighted(const bool _isHighlighted)
    {
        highlightBlocks(blocks_, _isHighlighted);
    }

    void TextUnit::setHighlighted(const highlightsV& _entries)
    {
        highlightParts(blocks_, _entries);
    }

    void TextUnit::setUnderline(const bool _enabled)
    {
        return underlineBlocks(blocks_, _enabled);
    }

    int TextUnit::getLastLineWidth() const
    {
        return getBlocksLastLineWidth(blocks_);
    }

    int TextUnit::getMaxLineWidth() const
    {
        return getBlocksMaxLineWidth(blocks_);
    }

    void TextUnit::setEmojiSizeType(const EmojiSizeType _emojiSizeType)
    {
        if (emojiSizeType_ != _emojiSizeType)
        {
            emojiSizeType_ = _emojiSizeType;

            if (hasEmoji())
                initializeBlocksAndSelectedTextColor();
        }
    }

    LinksVisible TextUnit::linksVisibility() const
    {
        return showLinks_;
    }

    bool TextUnit::needsEmojiMargin() const
    {
        return needsEmojiMargin_;
    }

    size_t TextUnit::getEmojiCount() const
    {
        if (isEmpty() || blocks_.size() > 1)
            return 0;

        return TextRendering::getEmojiCount(blocks_.front()->getWords());
    }

    bool TextUnit::isEmpty() const
    {
        return blocks_.empty();
    }

    void TextUnit::disableCommands()
    {
        disableCommandsInBlocks(blocks_);
    }

    void TextUnit::startSpellChecking(std::function<bool()> isAlive, std::function<void(bool)> onFinish)
    {
        auto sharedActual = std::make_shared<spellcheck::ActualGuard>(std::move(onFinish));

        if (!guard_)
            guard_ = std::make_shared<bool>(false);

        const auto weak = std::weak_ptr<bool>(guard_);
        const auto id = blockId();
        for (size_t blockIdx = 0; blockIdx < blocks_.size(); ++blockIdx)
        {
            const auto& words = blocks_[blockIdx]->getWords();
            for (size_t wordIdx = 0; wordIdx < words.size(); ++wordIdx)
            {
                auto& w = words[wordIdx];
                if (w.skipSpellCheck())
                    continue;

                const auto text = w.getText();
                for (const auto& syntaxWord : w.getSyntaxWords())
                {
                    auto s = text.mid(syntaxWord.offset_, syntaxWord.size_);
                    spellcheck::Spellchecker::instance().hasMisspelling(std::move(s), [this, weak, id, isAlive, wordIdx, blockIdx, text, sharedActual, syntaxWord](bool _res)
                    {
                        if (!_res || !sharedActual->isActual())
                            return;

                        const auto lock = weak.lock();
                        if (!lock || id != blockId() || !isAlive || !isAlive())
                        {
                            sharedActual->ignore();
                            return;
                        }

                        if (blockIdx >= blocks_.size() || wordIdx >= blocks_[blockIdx]->getWords().size())
                        {
                            sharedActual->ignore();
                            return;
                        }
                        auto& w = blocks_[blockIdx]->getWords()[wordIdx];
                        if (w.skipSpellCheck() || text != w.getText())
                        {
                            sharedActual->ignore();
                            return;
                        }

                        if (w.markAsSpellError(syntaxWord))
                        {
                            w.setSpellError(true);
                            sharedActual->setError();
                        }
                    });
                }
            }
        }
    }

    bool TextUnit::isSkippableWordForSpellChecking(const TextWordWithBoundary& w)
    {
        if (w.word.skipSpellCheck())
            return true;
        const auto text = w.word.getText();
        QStringView word(text);
        if (w.syntaxWord)
            word = word.mid(w.syntaxWord->offset_, w.syntaxWord->size_);

        return spellcheck::Spellchecker::isWordSkippable(word);
    }

    bool TextUnit::hasEmoji() const
    {
        return std::any_of(blocks_.begin(), blocks_.end(), [](const auto& block)
        {
            const auto& words = block->getWords();
            return std::any_of(words.begin(), words.end(), [](const auto& word) { return word.isEmoji(); });
        });;
    }

    void TextUnit::setShadow(const int _offsetX, const int _offsetY, const QColor& _color)
    {
        return setShadowForBlocks(blocks_, _offsetX, _offsetY, _color);
    }

    std::vector<QRect> TextUnit::getLinkRects() const
    {
        std::vector<QRect> res;
        int blockOffset = 0;
        for (const auto& b : blocks_)
        {
            for (auto r : b->getLinkRects())
                res.emplace_back(r.translated(horOffset_, verOffset_ + blockOffset));

            blockOffset += b->getCachedHeight();
        }

        return res;
    }

    bool TextUnit::mightStretchForLargerWidth() const
    {
        return std::any_of(blocks_.cbegin(), blocks_.cend(),
            [](const auto& _b) { return _b->mightStretchForLargerWidth(); });
    }

    void TextUnit::updateColors()
    {
        if (color_.canUpdateColor())
            setBlocksColor(blocks_, color_.actualColor());
        if (linkColor_.canUpdateColor())
            setBlocksLinkColor(blocks_, linkColor_.actualColor());
        if (selectionColor_.canUpdateColor())
            setBlocksSelectionColor(blocks_, selectionColor_.actualColor());
        if (highlightColor_.canUpdateColor())
            setBlocksHighlightColor(blocks_, highlightColor_.actualColor());
        if (highlightTextColor_.canUpdateColor())
            setBlocksHighlightedTextColor(blocks_, highlightTextColor_.actualColor());
    }

    QPoint TextUnit::mapPoint(QPoint _p) const
    {
        auto p = QPoint(_p.x() - horOffset_, _p.y() - verOffset_);
        if (lastVerPosition_ == VerPosition::BASELINE)
            p.ry() += textAscent(getFont());
        return p;
    }

    void TextUnit::setAlign(HorAligment _align)
    {
        align_ = _align;
        initializeBlocksAndSelectedTextColor();
    }

    TextUnitPtr MakeTextUnit(
        const Data::FString& _text,
        const Data::MentionMap& _mentions,
        LinksVisible _showLinks,
        ProcessLineFeeds _processLineFeeds,
        EmojiSizeType _emojiSizeType,
        ParseBackticksPolicy _backticksPolicy)
    {
        if (_text.string().contains(Data::singleBackTick()) && _backticksPolicy != ParseBackticksPolicy::KeepBackticks)
        {
            auto text = _text;
            Utils::convertOldStyleMarkdownToFormats(text, _backticksPolicy);
            return std::make_unique<TextUnit>(text,  _mentions, _showLinks, _processLineFeeds, _emojiSizeType);
        }
        return std::make_unique<TextUnit>(_text,  _mentions, _showLinks, _processLineFeeds, _emojiSizeType);
    }

    TextUnitPtr MakeTextUnit(const QString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType)
    {
        return MakeTextUnit(Data::FString(_text), _mentions, _showLinks, _processLineFeeds, _emojiSizeType);
    }

    void ExtractLinkWords(TextUnitPtr& _textUnit, std::vector<const TextWord*>& _words)
    {
        _words.clear();
        if (!_textUnit)
            return;

        for (auto& block : _textUnit->blocks())
            extractBlockLinks(_words, block);
    }
}

QString getEllipsis()
{
    return qsl("...");
}
}

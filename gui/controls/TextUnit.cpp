#include "stdafx.h"
#include "TextUnit.h"
#include "textrendering/TextRenderingUtils.h"

#include "../styles/ThemeParameters.h"
#include "../spellcheck/Spellchecker.h"

namespace
{
    auto counter() noexcept
    {
        static int64_t v = 0;
        return ++v;
    }
}

namespace Ui
{
namespace TextRendering
{
    TextUnit::TextUnit()
        : horOffset_(0)
        , verOffset_(0)
        , showLinks_(LinksVisible::SHOW_LINKS)
        , processLineFeeds_(ProcessLineFeeds::KEEP_LINE_FEEDS)
        , emojiSizeType_(EmojiSizeType::REGULAR)
        , align_(HorAligment::LEFT)
        , maxLinesCount_(-1)
        , appended_(-1)
        , lineBreak_(LineBreakType::PREFER_WIDTH)
        , linksStyle_(isLinksUnderlined() ? LinksStyle::UNDERLINED : LinksStyle::PLAIN)
        , lineSpacing_(0)
        , sourceModified_(false)
        , needsEmojiMargin_(false)
        , blockId_(counter())
    {
    }

    TextUnit::TextUnit(const QString& _text, std::vector<BaseDrawingBlockPtr>&& _blocks, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType)
        : originText_(_text)
        , blocks_(std::move(_blocks))
        , horOffset_(0)
        , verOffset_(0)
        , mentions_(_mentions)
        , showLinks_(_showLinks)
        , processLineFeeds_(_processLineFeeds)
        , emojiSizeType_(_emojiSizeType)
        , align_(HorAligment::LEFT)
        , maxLinesCount_(-1)
        , appended_(-1)
        , lineBreak_(LineBreakType::PREFER_WIDTH)
        , linksStyle_(isLinksUnderlined() ? LinksStyle::UNDERLINED : LinksStyle::PLAIN)
        , lineSpacing_(0)
        , sourceModified_(false)
        , needsEmojiMargin_(false)
        , blockId_(counter())
    {
    }

    void TextUnit::setText(const QString& _text, const QColor& _color)
    {
        if (_color.isValid())
            color_ = _color;
        originText_ = _text;
        blocks_ = parseForBlocks(_text, mentions_, showLinks_, processLineFeeds_);
        blockId_ = counter();
        initBlocks(blocks_, font_, color_, linkColor_, selectionColor_, highlightColor_, align_, emojiSizeType_, linksStyle_);
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

    void TextUnit::elide(int _width, ELideType _type)
    {
        return elideBlocks(blocks_, _width, _type);
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
        return drawBlocks(blocks_, _painter, QPoint(horOffset_, verOffset_), _pos);
    }

    void TextUnit::drawSmart(QPainter& _painter, int _center)
    {
        return drawBlocksSmart(blocks_, _painter, QPoint(horOffset_, verOffset_), _center);
    }

    int TextUnit::getHeight(int width, CallType _calltype)
    {
        cachedSize_ = QSize(width, getBlocksHeight(blocks_, width, lineSpacing_, _calltype));

        if (blocks_.size() == 1 && blocks_.front()->needsEmojiMargin())
            needsEmojiMargin_ = true;

        return cachedSize_.height();
    }

    void TextUnit::evaluateDesiredSize()
    {
        getHeight(desiredWidth());
    }

    void TextUnit::init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, int _maxLinesCount, LineBreakType _lineBreak, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        font_ = _font;
        color_ = _color;
        linkColor_ = _linkColor;
        selectionColor_ = _selectionColor;
        highlightColor_ = _highlightColor;
        align_ = _align;
        maxLinesCount_ = _maxLinesCount;
        lineBreak_ = _lineBreak;
        linksStyle_ = _linksStyle;
        initBlocks(blocks_, _font, _color, _linkColor, _selectionColor, _highlightColor, _align, _emojiSizeType, _linksStyle);
        setBlocksMaxLinesCount(blocks_, _maxLinesCount, lineBreak_);
    }

    void TextUnit::setLastLineWidth(int _width)
    {
        setBlocksLastLineWidth(blocks_, _width);
    }

    int TextUnit::getLineCount() const
    {
        return std::accumulate(blocks_.begin(), blocks_.end(), 0, [](auto currentValue, const auto& b) { return b->getLineCount() + currentValue; });
    }

    void TextUnit::select(const QPoint& _from, const QPoint& _to)
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
        return TextRendering::isOverLink(blocks_, mapPoint(_p));
    }

    QString TextUnit::getLink(const QPoint& _p) const
    {
        return getBlocksLink(blocks_, mapPoint(_p));
    }

    std::optional<TextWordWithBoundary> TextUnit::getWordAt(QPoint _p) const
    {
        return getWord(blocks_, mapPoint(_p));
    }

    bool TextUnit::replaceWordAt(const QString& _old, const QString& _new, QPoint _p)
    {
        return replaceWord(blocks_, _old, _new, mapPoint(_p));
    }

    QString TextUnit::getSelectedText(TextType _type) const
    {
        return getBlocksSelectedText(blocks_, _type);
    }

    QString TextUnit::getText() const
    {
        return getBlocksText(blocks_);
    }

    QString TextUnit::getTextInstantEdit() const
    {
        return getBlocksTextForInstantEdit(blocks_);
    }

    QString TextUnit::getSourceText() const
    {
        return sourceModified() ? getBlocksSourceText(blocks_) : originText_;
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
        return applyBLocksLinks(blocks_, _links);
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

    void TextUnit::setColor(const QColor& _color)
    {
        color_ = _color;
        setBlocksColor(blocks_, _color);
    }

    void TextUnit::setColor(const Styling::StyleVariable _var, const QString& _aimid)
    {
        setColor(Styling::getParameters(_aimid).getColor(_var));
    }

    void TextUnit::setLinkColor(const QColor& _color)
    {
        setBlocksLinkColor(blocks_, _color);
    }

    void TextUnit::setSelectionColor(const Styling::StyleVariable _var, const QString & _aimid)
    {
        selectionColor_ = Styling::getParameters(_aimid).getColor(_var);
        setBlocksSelectionColor(blocks_, selectionColor_);
    }

    void TextUnit::setSelectionColor(const QColor _color)
    {
        selectionColor_ = _color;
        setBlocksSelectionColor(blocks_, selectionColor_);
    }

    void TextUnit::setHighlightedTextColor(const QColor& _color)
    {
        highlightTextColor_ = _color;
        setBlocksHighlightedTextColor(blocks_, highlightTextColor_);
    }

    QColor TextUnit::getColor() const
    {
        return color_;
    }

    HorAligment TextUnit::getAlign() const
    {
        return align_;
    }

    void TextUnit::setColorForAppended(const QColor& _color)
    {
        return setBlocksColorForAppended(blocks_, _color, appended_);
    }

    const QFont& TextUnit::getFont() const
    {
        return font_;
    }

    void TextUnit::append(std::unique_ptr<TextUnit>&& _other)
    {
        return TextRendering::appendBlocks(blocks_, _other->blocks_);
    }

    void TextUnit::appendBlocks(std::unique_ptr<TextUnit>&& _other)
    {
        appended_ = blocks_.size();
        return appendWholeBlocks(blocks_, _other->blocks_);
    }

    void TextUnit::setLineSpacing(int _spacing)
    {
        lineSpacing_ = _spacing;
    }

    void TextUnit::markdown(const QFont& _font, const QColor& _color, ProcessLineFeeds _linefeeds)
    {
        sourceModified_ = markdownBlocks(blocks_, _font, _color, _linefeeds);
    }

    bool TextUnit::sourceModified() const
    {
        return sourceModified_;
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
            {
                initBlocks(blocks_, font_, color_, linkColor_, selectionColor_, highlightColor_, align_, emojiSizeType_, linksStyle_);
                setBlocksHighlightedTextColor(blocks_, highlightTextColor_);
            }
        }
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
                for (auto syntaxWord : w.getSyntaxWords())
                {
                    auto s = text.mid(syntaxWord.pos, syntaxWord.size);
                    spellcheck::Spellchecker::instance().hasMisspelling(std::move(s), [this, weak, id, isAlive, wordIdx, blockIdx, text, sharedActual, syntaxWord](bool res)
                    {
                        if (!res || !sharedActual->isActual())
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
            word = word.mid(w.syntaxWord->pos, w.syntaxWord->size);

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

    QPoint TextUnit::mapPoint(QPoint _p) const
    {
        return QPoint(_p.x() - horOffset_, _p.y() - verOffset_);
    }

    void TextUnit::setAlign(HorAligment _align)
    {
        align_ = _align;
        initBlocks(blocks_, font_, color_, linkColor_, selectionColor_, highlightColor_, align_, emojiSizeType_, isLinksUnderlined() ? LinksStyle::UNDERLINED : LinksStyle::PLAIN);
        setBlocksHighlightedTextColor(blocks_, highlightTextColor_);
    }

    TextUnitPtr MakeTextUnit(const QString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType)
    {
        return std::make_unique<TextUnit>(_text, parseForBlocks(_text, _mentions, _showLinks, _processLineFeeds), _mentions, _showLinks, _processLineFeeds, _emojiSizeType);
    }
}
QString getEllipsis()
{
    return qsl("...");
}
}

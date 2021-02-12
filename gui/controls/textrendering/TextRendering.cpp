#include "stdafx.h"

#include "TextRenderingUtils.h"
#include "TextRendering.h"
#include "TextWordRenderer.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/UrlParser.h"
#include "utils/InterConnector.h"

namespace
{
    void drawLine(Ui::TextRendering::TextWordRenderer& _renderer, QPoint _point, const std::vector<Ui::TextRendering::TextWord>& _line, const Ui::TextRendering::HorAligment _align, const int _width)
    {
        if (_line.empty())
            return;

        QPointF p(_point);
        if (_align != Ui::TextRendering::HorAligment::LEFT)
        {
            if (const auto lineWidth = getLineWidth(_line); lineWidth < _width)
                p.rx() += (_align == Ui::TextRendering::HorAligment::CENTER) ? ((_width - lineWidth) / 2) : (_width - lineWidth);
        }
        _renderer.setPoint(p);

        for (const auto& w : _line)
        {
            const auto& subwords = w.getSubwords();
            if (subwords.empty())
            {
                _renderer.draw(w);
            }
            else
            {
                for (auto s = subwords.begin(), end = std::prev(subwords.end()); s != end; ++s)
                    _renderer.draw(*s, false);

                _renderer.draw(subwords.back(), true);
            }
        }
    }

    constexpr int maxCommandSize() noexcept { return 32; }
}

namespace Ui
{
namespace TextRendering
{
    TextWord::TextWord(QString _text, Space _space, WordType _type, LinksVisible _showLinks, EmojiSizeType _emojiSizeType)
        : code_(0)
        , space_(_space)
        , type_(_type)
        , text_(std::move(_text))
        , emojiSize_(defaultEmojiSize())
        , cachedWidth_(0)
        , selectedFrom_(-1)
        , selectedTo_(-1)
        , spaceWidth_(0)
        , highlight_({ 0, 0 })
        , isSpaceSelected_(false)
        , isSpaceHighlighted_(false)
        , isTruncated_(false)
        , linkDisabled_(false)
        , selectionFixed_(false)
        , hasSpellError_(false)
        , showLinks_(_showLinks)
        , emojiSizeType_(_emojiSizeType)
    {
        trimmedText_ = text_;
        trimmedText_.remove(QChar::Space);

        if (type_ != WordType::LINK && _showLinks == LinksVisible::SHOW_LINKS)
        {
            Utils::UrlParser p;
            p.process(trimmedText_);
            if (p.hasUrl())
            {
                type_ = WordType::LINK;
                originalUrl_ = QString::fromStdString(p.getUrl().original_);
            }
        }
        assert(type_ != WordType::EMOJI);

        checkSetClickable();
    }

    TextWord::TextWord(Emoji::EmojiCode _code, Space _space, EmojiSizeType _emojiSizeType)
        : code_(std::move(_code))
        , space_(_space)
        , type_(WordType::EMOJI)
        , emojiSize_(defaultEmojiSize())
        , cachedWidth_(0)
        , selectedFrom_(-1)
        , selectedTo_(-1)
        , spaceWidth_(0)
        , highlight_({ 0, 0 })
        , isSpaceSelected_(false)
        , isSpaceHighlighted_(false)
        , isTruncated_(false)
        , linkDisabled_(false)
        , selectionFixed_(false)
        , hasSpellError_(false)
        , showLinks_(LinksVisible::SHOW_LINKS)
        , emojiSizeType_(_emojiSizeType)
    {
        assert(!code_.isNull());
    }

    QString TextWord::getText(TextType _text_type) const
    {
        if (isEmoji())
            return Emoji::EmojiCode::toQString(code_);

        if (_text_type == TextType::SOURCE && type_ == WordType::MENTION)
            return originalMention_;

        if (subwords_.empty())
            return text_;

        QString result;
        for (const auto& s : subwords_)
        {
            result += s.getText(_text_type);
            if (s.isSpaceAfter())
                result += QChar::Space;
        }

        return result;
    }

    void TextWord::setText(QString _text)
    {
        text_ = std::move(_text);
        subwords_ = {};
    }

    bool TextWord::equalTo(QStringView _sl) const
    {
        return text_ == _sl;
    }

    bool TextWord::equalTo(QChar _c) const
    {
        return text_.size() == 1 && text_.at(0) == _c;
    }

    QString TextWord::getLink() const
    {
        if (type_ == WordType::MENTION)
            return originalMention_;
        else if (type_ == WordType::NICK)
            return text_;
        else if (type_ == WordType::COMMAND)
            return trimmedText_;
        else if (type_ != WordType::LINK)
            return QString();

        return originalLink_.isEmpty() ? text_ : originalLink_;
    }

    double TextWord::width(WidthMode _force, int _emojiCount)
    {
        if (_force == WidthMode::FORCE)
            cachedWidth_ = 0;

        if (!qFuzzyIsNull(cachedWidth_) && _emojiCount == 0)
            return cachedWidth_;

        if (emojiSizeType_ == EmojiSizeType::SMARTREPLY)
        {
            emojiSize_ = getEmojiSizeSmartreply(font_, _emojiCount);
        }
        else
        {
            emojiSize_ = getEmojiSize(font_, isResizable() ? _emojiCount : 0);
            if (isResizable() && _emojiCount == 0)
                emojiSize_ += Utils::scale_value(1);

            if constexpr (platform::is_apple())
                if (isInTooltip())
                    emojiSize_ -= Utils::scale_value(1);
        }

        if (subwords_.empty())
        {
            cachedWidth_ = isEmoji() ? emojiSize_ : textWidth(font_, text_);
        }
        else
        {
            cachedWidth_ = 0;
            for (auto& w : subwords_)
            {
                cachedWidth_ += w.width(_force, _emojiCount);
                if (w.isSpaceAfter())
                    cachedWidth_ += w.spaceWidth();
            }
        }
        spaceWidth_ = textSpaceWidth(font_);

        return cachedWidth_;
    }

    void TextWord::setWidth(double _width, int _emojiCount)
    {
        emojiSize_ = getEmojiSize(font_, isResizable() ? _emojiCount : 0);
        spaceWidth_ = textSpaceWidth(font_);
        cachedWidth_ = isResizable() ? std::max(_width, static_cast<double>(emojiSize_)) : _width;
    }

    int TextWord::getPosByX(int _x) const
    {
        if (_x <= 0)
            return 0;

        if (isEmoji())
        {
            if (_x >= cachedWidth_ / 2)
                return 1;
            return 0;
        }

        const auto text = subwords_.empty() ? text_ : getText(TextType::VISIBLE);
        if (_x >= cachedWidth_)
            return text.size();

        for (int i = 1; i < text.size(); ++i)
        {
            if (textWidth(font_, text.left(i)) >= _x)
                return i;
        }

        return text.size();
    }

    void TextWord::select(int _from, int _to, SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = ((selectedTo_ == text_.size() || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        if (type_ == WordType::MENTION  && (_from != 0 || _to != 0))
            return selectAll((_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter() && _to == getText().size()) ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);

        if (_from == _to)
        {
            if (_from == 0)
                clearSelection();

            return;
        }

        selectedFrom_ = _from;
        selectedTo_ = isEmoji() ? 1 : _to;
        isSpaceSelected_ = ((selectedTo_ == text_.size() || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
    }

    void TextWord::selectAll(SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = (_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        selectedFrom_ = 0;
        selectedTo_ = isEmoji() ? 1 : text_.size();
        isSpaceSelected_ = (_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
        for (auto& s : subwords_)
            s.selectAll(SelectionType::WITH_SPACE_AFTER);
    }

    void TextWord::clearSelection()
    {
        if (selectionFixed_)
            return;

        selectedFrom_ = -1;
        selectedTo_ = -1;
        isSpaceSelected_ = false;
        for (auto& s : subwords_)
            s.clearSelection();
    }

    void TextWord::fixSelection()
    {
        selectionFixed_ = true;
    }

    void TextWord::releaseSelection()
    {
        selectionFixed_ = false;
    }

    void TextWord::clicked() const
    {
        if (type_ == WordType::MENTION)
        {
            Utils::openUrl(originalMention_);
        }
        else if (type_ == WordType::NICK)
        {
            Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(trimmedText_);
        }
        else if (type_ == WordType::COMMAND)
        {
            Q_EMIT Utils::InterConnector::instance().sendBotCommand(trimmedText_);
        }
        else if (type_ == WordType::LINK || !originalLink_.isEmpty())
        {
            if (Utils::isMentionLink(originalLink_))
            {
                Utils::openUrl(originalLink_);
                return;
            }

            Utils::UrlParser parser;
            parser.process(originalLink_.isEmpty() ? trimmedText_ : originalLink_);
            if (parser.hasUrl())
            {
                if (const auto& url = parser.getUrl(); url.is_email())
                    Utils::openUrl(QString(u"mailto:" % QString::fromStdString(url.url_)));
                else
                    Utils::openUrl(QString::fromStdString(url.url_));
            }
        }
    }

    double TextWord::cachedWidth() const noexcept
    {
        return cachedWidth_;
    }

    void TextWord::setHighlighted(const bool _isHighlighted) noexcept
    {
        highlight_ = { 0, _isHighlighted ? (isEmoji() ? 1 : text_.size()) : 0 };
        for (auto& sw : subwords_)
            sw.setHighlighted(_isHighlighted);
    }

    void TextWord::setHighlighted(const int _from, const int _to) noexcept
    {
        const auto maxPos = isEmoji() ? 1 : text_.size();
        highlight_ = { std::clamp(_from, 0, maxPos), std::clamp(_to, 0, maxPos) };
    }

    void TextWord::setHighlighted(const highlightsV& _entries)
    {
        for (const auto& hl : _entries)
        {
            if (isEmoji())
            {
                qsizetype pos = 0;
                const auto code = Emoji::getEmoji(hl, pos);
                if (code == getCode())
                {
                    setHighlighted(true);
                    return;
                }
            }
            else if (text_.size() >= hl.size())
            {
                if (const auto idx = text_.indexOf(hl, 0, Qt::CaseInsensitive); idx != -1)
                {
                    highlight_ = { idx, idx + hl.size() };
                    return;
                }
            }
        }
    }

    int TextWord::spaceWidth() const
    {
        return ceilToInt(spaceWidth_);
    }

    static bool isValidMention(QStringView _text, const QString& _mention)
    {
        if (!Utils::isMentionLink(_text))
            return false;

        const auto mention = _text.mid(2, _text.size() - 3);
        return mention == _mention;
    }

    bool TextWord::applyMention(const std::pair<QString, QString>& _mention)
    {
        if (!isValidMention(text_, _mention.first))
            return false;

        originalMention_ = std::exchange(text_, _mention.second);
        type_ = WordType::MENTION;
        return true;
    }

    bool TextWord::applyMention(const QString& _mention, const std::vector<TextWord>& _mentionWords)
    {
        if (!isValidMention(text_, _mention))
            return false;

        originalMention_ = std::exchange(text_, {});
        setSubwords(_mentionWords);
        if (isSpaceAfter() && hasSubwords())
            subwords_.back().setSpace(Space::WITH_SPACE_AFTER);

        for (auto& s : subwords_)
            s.applyFontColor(*this);

        type_ = WordType::MENTION;
        return true;
    }

    void TextWord::setOriginalLink(const QString& _link)
    {
        originalLink_ = _link;
        if(type_ == WordType::TEXT)
            type_ = WordType::LINK;
    }

    void TextWord::disableLink()
    {
        linkDisabled_ = true;
        if (type_ == WordType::LINK || type_ == WordType::NICK || type_ == WordType::COMMAND)
        {
            type_ = WordType::TEXT;
            originalLink_.clear();
            showLinks_ = LinksVisible::DONT_SHOW_LINKS;
        }
    }

    void TextWord::disableCommand()
    {
        if (type_ == WordType::COMMAND)
        {
            type_ = WordType::TEXT;
            if (underline())
                setUnderline(false);
        }
    }

    void TextWord::setFont(const QFont& _font)
    {
        font_ = _font;
        for (auto& s : subwords_)
            s.setFont(_font);

        cachedWidth_ = 0;
    }

    void TextWord::setColor(QColor _color)
    {
        color_ = _color;
        for (auto& w : subwords_)
            w.setColor(_color);
    }

    void TextWord::applyFontColor(const TextWord& _other)
    {
        setFont(_other.getFont());
        setColor(_other.getColor());
        setUnderline(_other.underline());
        setSpellError(_other.hasSpellError());
    }

    void TextWord::setSubwords(std::vector<TextWord> _subwords)
    {
        assert(subwords_.empty());
        subwords_ = std::move(_subwords);
        cachedWidth_ = 0;
    }

    std::vector<TextWord> TextWord::splitByWidth(int _width)
    {
        std::vector<TextWord> result;
        if (subwords_.empty())//todo implement for common words
            return result;

        auto curwidth = 0;
        std::vector<TextWord> subwordsFirst;

        auto finalize = [&result, &subwordsFirst, this](auto iter) mutable
        {
            auto tw = *this;
            tw.setSubwords(subwordsFirst);
            tw.width(WidthMode::FORCE);
            result.push_back(tw);
            tw.setSubwords(std::vector<TextWord>(iter, subwords_.end()));
            tw.width(WidthMode::FORCE);
            result.push_back(tw);
        };

        for (auto iter = subwords_.begin(); iter != subwords_.end(); ++iter)
        {
            auto wordWidth = iter->cachedWidth();
            if (curwidth + wordWidth <= _width)
            {
                curwidth += wordWidth;
                if (iter->isSpaceAfter())
                    curwidth += iter->spaceWidth();

                subwordsFirst.push_back(*iter);
                continue;
            }

            if (iter->isEmoji())
            {
                finalize(iter);
                break;
            }

            const auto cmpWidth = (_width - curwidth);
            const auto curWordText = iter->getText();
            auto tmpWord = elideText(font_, curWordText, cmpWidth);

            if (tmpWord.isEmpty())
            {
                finalize(++iter);
                break;
            }

            auto tw = TextWord(tmpWord, Space::WITHOUT_SPACE_AFTER, getType(), linkDisabled_ ? LinksVisible::DONT_SHOW_LINKS : getShowLinks());
            tw.applyFontColor(*this);
            tw.width();
            if (tw.isLink())
                tw.setOriginalLink(getText());
            tw.setTruncated();

            subwordsFirst.push_back(tw);
            if (tmpWord == curWordText)
            {
                finalize(++iter);
                break;
            }

            tw = TextWord(curWordText.mid(tmpWord.size()), getSpace(), getType(), linkDisabled_ ? LinksVisible::DONT_SHOW_LINKS : getShowLinks());
            tw.applyFontColor(*this);
            tw.width();
            if (tw.isLink())
                tw.setOriginalLink(getText());

            std::vector<TextWord> subwordsSecond = { std::move(tw) };
            if (++iter != subwords_.end())
                subwordsSecond.insert(subwordsSecond.end(), iter, subwords_.end());

            auto temp = *this;
            temp.setSubwords(std::move(subwordsFirst));
            temp.width(WidthMode::FORCE);
            result.push_back(temp);
            temp.setSubwords(std::move(subwordsSecond));
            temp.width(WidthMode::FORCE);
            result.push_back(std::move(temp));
            break;
        }

        if (result.empty())
        {
            auto temp = *this;
            temp.setSubwords(std::move(subwordsFirst));
            temp.width(WidthMode::FORCE);
            result.push_back(std::move(temp));
        }

        return result;
    }

    void TextWord::setUnderline(bool _enabled)
    {
        font_.setUnderline(_enabled);
        for (auto& w : subwords_)
            w.setUnderline(_enabled);
    }

    bool TextWord::underline() const
    {
        return font_.underline();
    }

    void TextWord::setEmojiSizeType(EmojiSizeType _emojiSizeType) noexcept
    {
        emojiSizeType_ = _emojiSizeType;
    }

    void TextWord::checkSetClickable()
    {
        if (showLinks_ == LinksVisible::SHOW_LINKS && type_ != WordType::EMOJI && trimmedText_.size() > 1)
        {
            if (trimmedText_.startsWith(u'@'))
            {
                if (Utils::isNick(&trimmedText_))
                    type_ = WordType::NICK;
            }
            else if (trimmedText_.startsWith(u'/'))
            {
                if (const auto cmd = trimmedText_.midRef(1); cmd.size() <= maxCommandSize())
                {
                    static const auto isLetter = [](char c) { return c && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); };
                    static const auto isDigit = [](char c) { return c && c >= '0' && c <= '9'; };
                    static const auto check = [](QChar _c)
                    {
                        const auto sym = _c.toLatin1();
                        return isLetter(sym) || isDigit(sym) || sym == '_';
                    };

                    if (isLetter(cmd.at(0).toLatin1()))
                    {
                        if (const auto posAfterCmd = std::find_if_not(std::next(cmd.begin()), cmd.end(), check); posAfterCmd == cmd.end())
                        {
                            type_ = WordType::COMMAND;
                        }
                        else
                        {
                            static const auto isAllowedPunct = [](QChar _c)
                            {
                                const auto sym = _c.toLatin1();
                                return sym && std::ispunct(static_cast<unsigned char>(sym)) && sym != '_' && sym != '/';
                            };
                            if (std::all_of(posAfterCmd, cmd.end(), isAllowedPunct))
                            {
                                type_ = WordType::COMMAND;
                                trimmedText_.chop(std::distance(posAfterCmd, cmd.end()));
                            }
                        }
                    }
                }
            }
        }
    }

    const std::vector<WordBoundary>& TextWord::getSyntaxWords() const
    {
        return const_cast<TextWord*>(this)->getSyntaxWords();
    }

    std::vector<WordBoundary>& TextWord::getSyntaxWords()
    {
        if (syntaxWords_.empty())
            syntaxWords_ = parseForSyntaxWords(getText());
        return syntaxWords_;
    }

    bool TextWord::markAsSpellError(WordBoundary _w)
    {
        auto& words = getSyntaxWords();
        const auto it = std::find_if(words.begin(), words.end(), [_w](auto x) { return _w.pos == x.pos && _w.size == x.size; });
        if (it != words.end())
        {
            it->spellError = true;
            return true;
        }
        return false;
    }

    std::optional<WordBoundary> TextWord::getSyntaxWordAt(int _x) const
    {
        const auto& words = getSyntaxWords();
        if (words.empty())
            return {};

        const auto text = getText();
        if (words.size() == 1 && words.front().pos == 0 && words.front().size == text.size())
            return words.front();

        const auto pos = getPosByX(_x);
        for (auto w : words)
            if (pos >= w.pos && pos < (w.pos + w.size))
                return w;

        return {};
    }

    void TextWord::setShadow(const int _offsetX, const int _offsetY, const QColor& _color)
    {
        shadow_.offsetX = _offsetX;
        shadow_.offsetY = _offsetY;
        shadow_.color = _color;
    }

    std::vector<WordBoundary> TextWord::parseForSyntaxWords(QStringView text)
    {
        const auto textSize = int(text.size());
        auto finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, text.data(), textSize);

        auto atEnd = [&finder]
        {
            return finder.toNextBoundary() == -1;
        };

        std::vector<WordBoundary> res;

        while (finder.position() < textSize)
        {
            if (!finder.boundaryReasons().testFlag(QTextBoundaryFinder::StartOfItem))
            {
                if (atEnd())
                    break;
                continue;
            }

            const auto start = finder.position();
            const auto end = finder.toNextBoundary();
            if (end == -1)
                break;
            const auto length = end - start;
            if (length < 1)
                continue;

            res.push_back({ start, length, false });

            if (atEnd())
                break;
        }
        return res;
    }

    TextDrawingBlock::TextDrawingBlock(QStringView _text, LinksVisible _showLinks, BlockType _blockType)
        : BaseDrawingBlock(_blockType)
        , lineHeight_(0)
        , desiredWidth_(0)
        , originalDesiredWidth_(0)
        , maxLinesCount_(std::numeric_limits<size_t>::max())
        , appended_(-1)
        , lineBreak_(LineBreakType::PREFER_WIDTH)
        , lineSpacing_(0)
        , selectionFixed_(false)
        , lastLineWidth_(-1)
        , needsEmojiMargin_(false)
    {
        parseForWords(_text, _showLinks, words_);
    }

    TextDrawingBlock::TextDrawingBlock(const std::vector<TextWord>& _words, BaseDrawingBlock* _other)
        : BaseDrawingBlock(BlockType::Text)
        , words_(_words)
        , lineHeight_(0)
        , desiredWidth_(0)
        , originalDesiredWidth_(0)
        , cachedMaxLineWidth_(-1)
        , maxLinesCount_(std::numeric_limits<size_t>::max())
        , appended_(-1)
        , lineBreak_(LineBreakType::PREFER_WIDTH)
        , lineSpacing_(0)
        , selectionFixed_(false)
        , lastLineWidth_(-1)
        , needsEmojiMargin_(false)
    {

        if (auto textBlock = dynamic_cast<TextDrawingBlock*>(_other); textBlock)
        {
            type_ = textBlock->type_;
            linkColor_ = textBlock->linkColor_;
            selectionColor_ = textBlock->selectionColor_;
            highlightColor_ = textBlock->highlightColor_;
            highlightTextColor_ = textBlock->highlightTextColor_;
            align_ = textBlock->align_;
        }

        calcDesired();
    }

    void TextDrawingBlock::init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        auto needUnderline = [_linksStyle](const auto& w)
        {
            return w.getShowLinks() == LinksVisible::SHOW_LINKS && w.isLink() && _linksStyle == LinksStyle::UNDERLINED;
        };

        for (auto& words : { boost::make_iterator_range(words_), boost::make_iterator_range(originalWords_) })
        {
            for (auto& w : words)
            {
                w.setFont(_font);
                w.setColor(_color);
                w.setEmojiSizeType(_emojiSizeType);

                if (needUnderline(w))
                    w.setUnderline(true);
            }
        }

        linkColor_ = _linkColor;
        selectionColor_ = _selectionColor;
        highlightColor_ = _highlightColor;
        cachedMaxLineWidth_ = -1;
        cachedSize_ = QSize();
        elided_ = false;
        align_ = _align;

        calcDesired();
    }

    void TextDrawingBlock::setMargins(const QMargins& _margins)
    {
        margins_ = _margins;
    }

    void TextDrawingBlock::draw(QPainter& _p, QPoint& _point, VerPosition _pos)
    {
        if (_pos != VerPosition::TOP)
            assert(lines_.size() == 1);

        Utils::PainterSaver ps(_p);

        TextWordRenderer renderer(
            &_p,
            QPoint(),
            lineHeight_,
            lineSpacing_,
            0,
            _pos,
            selectionColor_,
            highlightColor_,
            highlightTextColor_,
            linkColor_
        );

        _point = QPoint(_point.x() + margins_.left(), _point.y() + margins_.top());
        if (lines_.size() > 1)
            _point.ry() -= lineSpacing_ / 2;

        for (const auto& l : lines_)
        {
            drawLine(renderer, _point, l, align_, cachedSize_.width());
            _point.ry() += lineHeight_;
        }

        if (lines_.size() > 1)
        {
            _point.ry() += lineSpacing_ / 2;
        }
        else if (platform::is_apple() && !lines_.empty())
        {
            const auto& line = lines_.front();
            if (std::any_of(line.begin(), line.end(), [](const auto &w) { return w.isEmoji() && w.underline(); }))
                needsEmojiMargin_ = true;
        }

        _point.ry() += margins_.bottom();
    }

    void TextDrawingBlock::drawSmart(QPainter& _p, QPoint& _point, int _center)
    {
        if (lines_.size() != 1)
            return draw(_p, _point, VerPosition::TOP);

        Utils::PainterSaver ps(_p);
        _point = QPoint(_point.x() + margins_.left(), _center);

        TextWordRenderer renderer(
            &_p,
            QPoint(),
            lineHeight_,
            lineSpacing_,
            lineHeight_ / 2,
            VerPosition::MIDDLE,
            selectionColor_,
            highlightColor_,
            highlightTextColor_,
            linkColor_
        );
        drawLine(renderer, _point, lines_.front(), align_, cachedSize_.width());

        _point.ry() += lineHeight_ + margins_.bottom();
    }

    int TextDrawingBlock::getHeight(int _width, int _lineSpacing, CallType _type, bool _isMultiline)
    {
        if (_width == 0 || words_.empty())
            return 0;

        if (cachedSize_.width() == _width && _type == CallType::USUAL)
            return cachedSize_.height();

        lineSpacing_ = _lineSpacing;

        lines_.clear();
        linkRects_.clear();
        links_.clear();
        std::vector<TextWord> curLine;
        _width -= (margins_.left() + margins_.right());
        double curWidth = 0;
        lineHeight_ = 0;

        const size_t emojiCount = _isMultiline ? 0 : getEmojiCount(words_);

        if (emojiCount != 0)
            needsEmojiMargin_ = true;

        const auto& last = *(words_.rbegin());
        auto fillLines = [_width, &curLine, &curWidth, emojiCount, last, this]()
        {
            for (auto& w : words_)
            {
                const auto limited = (maxLinesCount_ != std::numeric_limits<size_t>::max() && (lineBreak_ == LineBreakType::PREFER_WIDTH || lines_.size() == maxLinesCount_ - 1));

                lineHeight_ = std::max(lineHeight_, textHeight(w.getFont()));

                auto wordWidth = w.width(WidthMode::FORCE, emojiCount);

                if (w.isSpaceAfter())
                    wordWidth += w.spaceWidth();

                auto curWord = w;
                double cmpWidth = _width;
                if (limited && !curLine.empty())
                    cmpWidth -= curWidth;

                auto wordLen = 0;

                while (!curWord.isEmoji() && wordWidth > cmpWidth)
                {
                    if (!curLine.empty() && !limited)
                    {
                        lines_.push_back(curLine);
                        curLine.clear();
                        if (lines_.size() == maxLinesCount_)
                            return true;
                    }

                    auto splitedWords = curWord.splitByWidth(cmpWidth);

                    const auto curWordText = curWord.getText();
                    auto tmpWord = splitedWords.empty() ? elideText(w.getFont(), curWordText, cmpWidth) : QString();

                    if ((splitedWords.empty() && tmpWord.isEmpty()) || (!splitedWords.empty() && !splitedWords.front().hasSubwords()) || splitedWords.size() == 1)
                    {
                        if (!curLine.empty())
                        {
                            lines_.push_back(curLine);
                            curLine.clear();
                            if (lines_.size() == maxLinesCount_)
                                return true;

                            cmpWidth = _width;
                            curWidth = 0;
                            continue;
                        }
                        break;
                    }

                    auto tw = splitedWords.empty() ? TextWord(tmpWord, Space::WITHOUT_SPACE_AFTER, curWord.getType(), (curWord.isLinkDisabled() || curWord.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : curWord.getShowLinks()) : splitedWords[0];
                    tw.setSpellError(w.hasSpellError());
                    if (splitedWords.empty())
                    {
                        tw.applyFontColor(w);
                        tw.setHighlighted(w.highlightedFrom() - wordLen, w.highlightedTo() - wordLen);
                        tw.setWidth(cmpWidth, emojiCount);

                        wordLen += tmpWord.size();
                    }

                    if (tw.isLink())
                    {
                        linkRects_.emplace_back(limited ? curWidth : 0, lines_.size(), tw.cachedWidth(), 0);
                        tw.setOriginalLink(w.getText());
                        links_[linkRects_.size() - 1] = tw.getLink();
                    }
                    tw.setTruncated();
                    if (!limited || curLine.empty())
                    {
                        lines_.push_back({ std::move(tw) });
                        curWidth = 0;
                    }
                    else
                    {
                        curLine.push_back(std::move(tw));
                        lines_.push_back(curLine);
                        if (lines_.size() == maxLinesCount_)
                            return true;

                        curLine.clear();
                        curWidth = 0;
                        cmpWidth = _width;
                    }

                    if (lines_.size() == maxLinesCount_)
                        return true;

                    curWord = splitedWords.size() <= 1 ? TextWord(curWordText.mid(tmpWord.size()), curWord.getSpace(), curWord.getType(), (curWord.isLinkDisabled() || curWord.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : curWord.getShowLinks()) : splitedWords[1];
                    curWord.setSpellError(w.hasSpellError());
                    if (curWord.isLink())
                        curWord.setOriginalLink(w.getText());

                    if (splitedWords.size() <= 1)
                    {
                        curWord.applyFontColor(w);
                        curWord.setHighlighted(w.highlightedFrom() - wordLen, w.highlightedTo() - wordLen);
                        curWord.setSpaceHighlighted(w.isSpaceHighlighted());

                        if (wordWidth - tw.cachedWidth() < cmpWidth)
                            curWord.width(WidthMode::PLAIN, emojiCount);
                        else
                            curWord.setWidth(wordWidth - tw.cachedWidth(), emojiCount);
                    }

                    wordWidth = curWord.cachedWidth();
                }

                if (curWidth + wordWidth <= _width)
                {
                    if (curWord.isLink())
                    {
                        linkRects_.emplace_back(curWidth, lines_.size(), wordWidth, 0);
                        links_[linkRects_.size() - 1] = curWord.getLink();
                    }
                    curLine.push_back(std::move(curWord));
                    curWidth += wordWidth;
                }
                else
                {
                    if (!curLine.empty())
                    {
                        lines_.push_back(curLine);
                        if (lines_.size() == maxLinesCount_)
                            return true;
                    }

                    if (curWord.isLink())
                    {
                        linkRects_.emplace_back(0, lines_.size(), wordWidth, 0);
                        links_[linkRects_.size() - 1] = curWord.getLink();
                    }
                    curLine.clear();
                    curLine.push_back(std::move(curWord));

                    curWidth = wordWidth;
                }
            }

            if (!curLine.empty() && lines_.size() != maxLinesCount_)
                lines_.push_back(std::move(curLine));

            return false;
        };

        auto elided = fillLines();
        if (lines_.size() == maxLinesCount_)
        {
            if (lines_ [maxLinesCount_ - 1].rbegin()->isTruncated() || (elided && !words_.empty()))
            {
                lines_[maxLinesCount_ - 1] = elideWords(lines_[maxLinesCount_ - 1], lastLineWidth_ == -1 ? _width : lastLineWidth_, QWIDGETSIZE_MAX, true);
                elided_ = true;
            }
        }

        lineHeight_ = std::max(lineHeight_, words_.back().emojiSize());
        lineHeight_ += lineSpacing_;
        lineHeight_ += getHeightCorrection(words_.back(), emojiCount);

        const auto underlineOffset = getMetrics(words_[0].getFont()).underlinePos();
        for (auto& r : linkRects_)
        {
            r.setY(r.y() * lineHeight_);
            r.setHeight(lineHeight_ + underlineOffset);
        }

        auto cachedHeight = lineHeight_ * lines_.size() + margins_.top() + margins_.bottom();
        if (lines_.size() == 1 && !_isMultiline)
        {
            if (!linkRects_.empty() || Features::isSpellCheckEnabled())
                cachedHeight += underlineOffset;
        }

        cachedSize_ = QSize(_width, cachedHeight);
        cachedMaxLineWidth_ = -1;

        return cachedSize_.height();
    }

    int TextDrawingBlock::getCachedHeight() const
    {
        return cachedSize_.height();
    }

    void TextDrawingBlock::setMaxHeight(int)
    {

    }

    void TextDrawingBlock::select(const QPoint& _from, const QPoint& _to)
    {
        if (selectionFixed_)
            return;

        auto lineSelected = [this](int i)
        {
            if (i < 0 || i >= static_cast<int>(lines_.size()))
                return false;

            for (auto& w : lines_[i])
            {
                if (w.isSelected())
                    return true;
            }

            return false;
        };

        int curLine = 0;
        for (auto& l : lines_)
        {
            auto y1 = curLine * lineHeight_;
            auto y2 = (curLine + 1) * lineHeight_;

            auto minY = std::min(_from.y(), _to.y());
            auto maxY = std::max(_from.y(), _to.y());

            auto topToBottom = (minY < y1 && maxY < y2) || lineSelected(curLine - 1);
            auto bottomToTop = (minY > y1 && maxY > y2) || lineSelected(curLine + 1);
            auto ordinary = (!topToBottom && !bottomToTop);

            if (topToBottom)
                minY = y1;
            else if (bottomToTop)
                maxY = y2;

            if (minY <= y1 && maxY >= y2)
            {
                for (auto& w : l)
                {
                    w.selectAll(SelectionType::WITH_SPACE_AFTER);
                }
            }
            else if ((ordinary && minY >= y1 - 1 && maxY <= y2 + 1) || (topToBottom && maxY > minY && maxY < y2) || (bottomToTop && minY < maxY && minY > y1))
            {
                auto x1 = std::max(0, std::min(_to.x(), _from.x()));
                auto x2 = std::min(std::max(_to.x(), _from.x()), cachedSize_.width());

                if (topToBottom)
                {
                    x2 = std::min(_to.x(), cachedSize_.width());
                    x1 = 0;
                }
                else if (bottomToTop)
                {
                    x1 = std::max(_from.x(), 0);
                    x2 = cachedSize_.width();
                }

                if (x1 == x2)
                {
                    ++curLine;
                    continue;
                }

                int curWidth = 0;
                for (auto& w : l)
                {
                    curWidth += w.cachedWidth();
                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();

                    auto diff = curWidth - w.cachedWidth();
                    const auto selectSpace = bottomToTop || x2 > curWidth;

                    if (diff < x1 && curWidth > x1)
                        w.select(w.getPosByX(x1 - diff), (x2 >= curWidth) ? w.getText().size() : w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else if (diff < x1)
                        w.clearSelection();
                    else if (diff >= x1 && curWidth <= x2)
                        w.selectAll((x2 - curWidth >= 0) ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else if (diff < x2)
                        w.select(0, w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else
                        w.clearSelection();
                }
            }
            else if (ordinary && ((minY <= 0 && curLine == 0) || (maxY >= cachedSize_.height() && curLine == int(lines_.size() - 1))))
            {
                auto x1 = std::max(0, std::min(_to.x(), _from.x()));
                auto x2 = std::min(std::max(_to.x(), _from.x()), cachedSize_.width());

                if (minY < 0)
                {
                    x2 = std::min(_to.x(), cachedSize_.width());
                    x1 = 0;
                }
                else if (maxY > cachedSize_.height())
                {
                    x1 = std::max(_from.x(), 0);
                    x2 = cachedSize_.width();
                }

                if (x1 == x2)
                {
                    ++curLine;
                    continue;
                }

                int curWidth = 0;
                for (auto& w : l)
                {
                    curWidth += w.cachedWidth();
                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();

                    auto diff = curWidth - w.cachedWidth();
                    const auto selectSpace = (x2 > curWidth);

                    if (diff < x1 && curWidth > x1)
                        w.select(w.getPosByX(x1 - diff), (x2 >= curWidth) ? w.getText().size() : w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else if (diff < x1)
                        w.clearSelection();
                    else if (diff >= x1 && curWidth <= x2)
                        w.selectAll((x2 - curWidth >= 0) ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else if (diff < x2)
                        w.select(0, w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
                    else
                        w.clearSelection();
                }
            }
            else
            {
                for (auto& w : l)
                    if (w.isSelected())
                        w.clearSelection();
            }
            ++curLine;
        }
    }

    void TextDrawingBlock::selectAll()
    {
        if (selectionFixed_)
            return;

        for (auto& l : lines_)
            for (auto& w : l)
                w.selectAll(SelectionType::WITH_SPACE_AFTER);
    }

    void TextDrawingBlock::fixSelection()
    {
        selectionFixed_ = true;
    }

    void TextDrawingBlock::clearSelection()
    {
        if (selectionFixed_)
            return;

        for (auto& l : lines_)
            for (auto& w : l)
                w.clearSelection();
    }

    void TextDrawingBlock::releaseSelection()
    {
        selectionFixed_ = false;
        for (auto& l : lines_)
            for (auto& w : l)
                w.releaseSelection();
    }

    bool TextDrawingBlock::isSelected() const
    {
        return std::any_of(lines_.begin(), lines_.end(), [](const auto& l) {
            return std::any_of(l.begin(), l.end(), [](const auto& w) { return w.isSelected(); });
        });
    }

    bool TextDrawingBlock::isFullSelected() const
    {
        return std::all_of(lines_.begin(), lines_.end(), [](const auto& l) {
            return std::all_of(l.begin(), l.end(), [](const auto& w) { return w.isFullSelected(); });
        });
    }

    QString TextDrawingBlock::selectedText(TextType _type) const
    {
        QString result;
        for (const auto& l : lines_)
        {
            for (const auto& w : l)
            {
                if (w.isSelected())
                {
                    if (w.isFullSelected() && w.getType() == WordType::MENTION)
                    {
                        result += w.getText(_type);
                    }
                    else
                    {
                        const auto text = w.getText();
                        if (w.isEmoji() && w.isSelected())
                            result += text;
                        else
                            result += text.midRef(w.selectedFrom(), w.selectedTo() - w.selectedFrom());
                    }
                    if (w.isSpaceAfter() && w.isSpaceSelected())
                        result += QChar::Space;
                }
            }
        }

        if (!result.isEmpty())
            result += QChar::LineFeed;

        return result;
    }

    QString TextDrawingBlock::textForInstantEdit() const
    {
        QString result;
        for (const auto& l : lines_)
        {
            for (const auto& w : l)
            {
                result += w.getText(TextRendering::TextType::SOURCE);
                if (w.isSpaceAfter())
                    result += QChar::Space;
            }
        }

        if (!result.isEmpty())
            result += QChar::LineFeed;

        return result;
    }

    QString TextDrawingBlock::getText() const
    {
        QString result;
        for (const auto& w : words_)
        {
            result += w.getText();
            if (w.isSpaceAfter())
                result += QChar::Space;
        }

        if (!result.isEmpty())
            result += QChar::LineFeed;

        return result;
    }

    QString TextDrawingBlock::getSourceText() const
    {
        QString result;
        for (const auto& w : words_)
        {
            result += w.getText(TextRendering::TextType::SOURCE);
            if (w.isSpaceAfter())
                result += QChar::Space;
        }

        if (!result.isEmpty())
            result += QChar::LineFeed;

        return result;
    }

    void TextDrawingBlock::clicked(const QPoint& _p) const
    {
        if (const auto w = getWordAt(_p, WithBoundary::No); w)
            w->word.clicked();
    }

    bool TextDrawingBlock::doubleClicked(const QPoint& _p, bool _fixSelection)
    {
        bool selected = false;

        if (_p.x() < 0)
            return selected;

        int i = 0;
        bool selectLinked = false;
        std::vector<TextWord*> linkedWords;
        for (auto& l : lines_)
        {
            ++i;
            auto y1 = (i - 1) * lineHeight_;
            auto y2 = i * lineHeight_;

            if ((_p.y() > y1 && _p.y() < y2) || selectLinked)
            {
                int curWidth = 0;
                auto x = _p.x();

                if (align_ != Ui::TextRendering::HorAligment::LEFT)
                {
                    const auto lineWidth = getLineWidth(l);
                    if (lineWidth < cachedSize_.width())
                        x -= (align_ == Ui::TextRendering::HorAligment::CENTER) ? ((cachedSize_.width() - lineWidth) / 2) : (cachedSize_.width() - lineWidth);
                }

                for (auto& w : l)
                {
                    if (selectLinked)
                    {
                        w.selectAll(SelectionType::WITHOUT_SPACE_AFTER);
                        selected = true;
                        if (_fixSelection)
                            w.fixSelection();

                        if (w.isSpaceAfter())
                            return selected;
                    }

                    curWidth += w.cachedWidth();
                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();

                    if (curWidth > x)
                    {
                        if (w.isLink())
                            return selected;

                        w.selectAll(SelectionType::WITHOUT_SPACE_AFTER);
                        selected = true;
                        if (_fixSelection)
                            w.fixSelection();

                        for (auto& linked : linkedWords)
                        {
                            linked->selectAll(SelectionType::WITHOUT_SPACE_AFTER);
                            selected = true;
                            if (_fixSelection)
                                linked->fixSelection();
                        }

                        if (w.isSpaceAfter())
                            return selected;

                        selectLinked = true;
                    }
                    else
                    {
                        if (w.isSpaceAfter())
                            linkedWords.clear();
                        else
                            linkedWords.push_back(&w);
                    }
                }
            }
            else
            {
                for (auto& w : l)
                {
                    if (w.isSpaceAfter())
                        linkedWords.clear();
                    else
                        linkedWords.push_back(&w);
                }
            }
        }

        return selected;
    }

    bool TextDrawingBlock::isOverLink(const QPoint& _p) const
    {
        const auto rects = getLinkRects();
        return std::any_of(rects.cbegin(), rects.cend(), [_p](auto r) { return r.contains(_p); });
    }

    QString TextDrawingBlock::getLink(const QPoint& _p) const
    {
        int i = 0;
        for (auto r : getLinkRects())
        {
            if (r.contains(_p))
            {
                const auto it = links_.find(i);
                return it != links_.end() ? it->second : QString();
            }

            ++i;
        }

        return {};
    }

    void TextDrawingBlock::applyMentions(const Data::MentionMap& _mentions)
    {
        if (_mentions.empty())
            return;

        for (const auto& m : _mentions)
        {
            const QString mention = u"@[" % m.first % u']';
            auto iter = words_.begin();
            while (iter != words_.end())
            {
                const auto text = iter->getText();
                const auto mentionIdx = text.indexOf(mention);
                if (mentionIdx == -1 || text == mention)
                {
                    ++iter;
                    continue;
                }

                const auto space = iter->getSpace();
                const auto type = iter->getType();
                const auto links = iter->getShowLinks();

                auto func = [space, type, &iter, links, this](QString _left, QString _right)
                {
                    auto w = TextWord(std::move(_left), Space::WITHOUT_SPACE_AFTER, type, links);
                    w.applyFontColor(*iter);
                    *iter = w;
                    ++iter;

                    auto newWord = TextWord(std::move(_right), space, type, links);
                    newWord.applyFontColor(w);

                    std::vector<TextWord> linkParts;
                    if (newWord.getType() == WordType::LINK)
                    {
                        parseWordLink(newWord, linkParts);
                        std::for_each(linkParts.begin(), linkParts.end(), [&newWord](TextWord& w)
                        {
                            w.applyFontColor(newWord);
                        });
                    }

                    const auto atEnd = iter == words_.end();
                    if (linkParts.empty())
                        iter = words_.insert(iter, std::move(newWord));
                    else
                        iter = words_.insert(iter, std::make_move_iterator(linkParts.begin()), std::make_move_iterator(linkParts.end()));

                    if (atEnd)
                        iter = std::prev(words_.end());
                };

                const auto isLengthEqual = text.size() == mention.size();
                if (isLengthEqual || !text.startsWith(mention))
                {
                    if (isLengthEqual || !text.endsWith(mention))
                    {
                        func(text.left(mentionIdx), text.mid(mentionIdx));
                        continue;
                    }

                    func(text.left(mentionIdx), mention);
                    continue;
                }

                func(mention, text.mid(mention.size()));
            }
        }

        std::vector<TextWord> mentionWords;
        for (const auto& m : _mentions)
        {
            parseForWords(m.second, LinksVisible::SHOW_LINKS, mentionWords, WordType::MENTION);
            if (mentionWords.size() == 1 && !mentionWords.front().isEmoji())
            {
                for (auto& w : words_)
                    w.applyMention(m);
            }
            else
            {
                for (auto& w : words_)
                    w.applyMention(m.first, mentionWords);
            }
            mentionWords.clear();
        }
    }

    int TextDrawingBlock::desiredWidth() const
    {
        return ceilToInt(desiredWidth_);
    }

    void TextDrawingBlock::elide(int _width, ElideType _type, bool _prevElided)
    {
        if (_prevElided)
        {
            if (originalWords_.empty())
            {
                originalWords_ = words_;
                originalDesiredWidth_ = desiredWidth_;
            }

            words_.clear();
            lines_.clear();
            elided_ = true;
            desiredWidth_ = _width;

            return;
        }

        if (words_.empty() || (_width == desiredWidth()))
            return;

        if (originalWords_.empty())
        {
            originalWords_ = words_;
            originalDesiredWidth_ = desiredWidth_;
        }

        words_ = elideWords(originalWords_, _width, ceilToInt(originalDesiredWidth_), false, _type);
        if (words_ != originalWords_)
        {
            elided_ = true;
            desiredWidth_ = _width;
        }
        else
        {
            elided_ = false;
            calcDesired();
        }

        if (words_.empty())
        {
            words_ = originalWords_;
            elided_ = false;
            calcDesired();
        }

        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);
    }

    void TextDrawingBlock::disableCommands()
    {
        for (auto& w : words_)
            w.disableCommand();
    }

    std::optional<TextWordWithBoundary> TextDrawingBlock::getWordAt(QPoint _p, WithBoundary _mode) const
    {
        if (auto res = const_cast<TextDrawingBlock*>(this)->getWordAtImpl(_p, _mode); res)
            return TextWordWithBoundary{ res->word.get(), res->syntaxWord };
        return {};
    }

    bool TextDrawingBlock::replaceWordAt(const QString& _old, const QString& _new, QPoint _p)
    {
        if (auto word = getWordAtImpl(_p, WithBoundary::Yes))
        {
            auto text = word->word.get().getText();
            QStringRef syntaxWord(&text);
            if (word->syntaxWord)
                syntaxWord = syntaxWord.mid(word->syntaxWord->pos, word->syntaxWord->size);

            if (syntaxWord == _old)
            {
                if (syntaxWord.size() == text.size())
                    word->word.get().setText(_new);
                else
                    word->word.get().setText(text.leftRef(word->syntaxWord->pos) % _new % text.midRef(word->syntaxWord->pos + word->syntaxWord->size));
                return true;
            }
        }
        return false;
    }

    void TextDrawingBlock::setShadow(const int _offsetX, const int _offsetY, const QColor& _color)
    {
        const auto addShadow = [_offsetX, _offsetY, _color](auto& _words)
        {
            for (auto& w : _words)
                w.setShadow(_offsetX, _offsetY, _color);
        };
        addShadow(words_);
        for (auto& l : lines_)
            addShadow(l);
    }

    std::optional<TextDrawingBlock::TextWordWithBoundaryInternal> TextDrawingBlock::getWordAtImpl(QPoint _p, WithBoundary _mode)
    {
        if (_p.x() < 0)
            return {};

        int i = 0;
        for (auto& l : lines_)
        {
            ++i;
            auto y1 = (i - 1) * lineHeight_;
            auto y2 = i * lineHeight_;

            if (_p.y() > y1 && _p.y() < y2)
            {
                int curWidth = 0;
                auto x = _p.x();

                if (align_ != Ui::TextRendering::HorAligment::LEFT)
                {
                    const auto lineWidth = getLineWidth(l);
                    if (lineWidth < cachedSize_.width())
                        x -= (align_ == Ui::TextRendering::HorAligment::CENTER) ? ((cachedSize_.width() - lineWidth) / 2) : (cachedSize_.width() - lineWidth);
                }

                if (x < 0)
                    return {};

                for (auto& w : l)
                {
                    curWidth += w.cachedWidth();
                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();

                    if (curWidth > x)
                    {
                        if (_mode == WithBoundary::No)
                            return TextWordWithBoundaryInternal{ w };

                        const auto diff = curWidth - w.cachedWidth();
                        return TextWordWithBoundaryInternal{ w, w.getSyntaxWordAt(x - diff) };
                    }
                }
            }
        }
        return {};
    }

    void TextDrawingBlock::calcDesired()
    {
        desiredWidth_ = 0;

        size_t emojiCount = getEmojiCount(words_);

        if (emojiCount != 0)
            needsEmojiMargin_ = true;

        for (auto& w : words_)
        {
            desiredWidth_ += w.width(WidthMode::FORCE, emojiCount);
            if (w.isSpaceAfter())
                desiredWidth_ += w.spaceWidth();
        }
    }

    void TextDrawingBlock::parseForWords(QStringView _text, LinksVisible _showLinks, std::vector<TextWord>& _words, WordType _type)
    {
        const auto textSize = _text.size();

        QString tmp;
        const auto S_MARK = singleBackTick();
        for (qsizetype i = 0; i < textSize;)
        {
            const auto isSpace = _text.at(i).isSpace();
            if ((!tmp.isEmpty() && (isSpace || (tmp.endsWith(S_MARK) && _text.at(i) != S_MARK))) || (!tmp.endsWith(S_MARK) && _text.at(i) == S_MARK))
            {
                while (tmp.startsWith(QChar::Space) && !tmp.isEmpty())
                {
                    _words.emplace_back(spaceAsString(), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                    tmp.remove(0, 1);
                }
                if (!tmp.isEmpty())
                {
                    auto word = TextWord(std::move(tmp), isSpace ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                    if (!parseWordNick(word, _words) && (word.getType() != WordType::LINK || !parseWordLink(word, _words)))
                    {
                        if (word.getType() != WordType::COMMAND)
                            _words.emplace_back(std::move(word));
                        else
                            correctCommandWord(std::move(word), _words);
                    }
                }
                else if (!_words.empty() && isSpace)
                {
                    _words.back().setSpace(Space::WITH_SPACE_AFTER);
                }
                tmp.clear();

                if (isSpace)
                {
                    ++i;
                    continue;
                }
            }

            if (i < textSize)
            {
                auto prev = i;
                auto emoji = Emoji::getEmoji(_text, i);
                if (!emoji.isNull())
                {
                    const auto space = (i < textSize && _text.at(i) == QChar::Space) ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER;
                    if (!tmp.isEmpty())
                    {
                        _words.emplace_back(std::move(tmp), space, _type, _showLinks);
                        tmp.clear();
                    }

                    _words.emplace_back(std::move(emoji), space);
                    if (space == Space::WITH_SPACE_AFTER)
                        ++i;
                    continue;
                }
                else
                {
                    i = prev;
                }
            }

            if (i < textSize)
                tmp += _text.at(i);

            ++i;
        }

        if (!tmp.isEmpty())
        {
            while (tmp.startsWith(QChar::Space) && !tmp.isEmpty())
            {
                _words.emplace_back(spaceAsString(), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                tmp.remove(0, 1);
            }
            if (!tmp.isEmpty())
            {
                auto word = TextWord(std::move(tmp), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                if (!parseWordNick(word, _words) && (word.getType() != WordType::LINK || !parseWordLink(word, _words)))
                {
                    if (word.getType() != WordType::COMMAND)
                        _words.emplace_back(std::move(word));
                    else
                        correctCommandWord(std::move(word), _words);
                }
            }
        }
    }

    bool TextDrawingBlock::parseWordLink(const TextWord& _wordWithLink, std::vector<TextWord>& _words)
    {
        const auto text = _wordWithLink.getText();
        const auto url = _wordWithLink.getOriginalUrl();

        const auto idx = text.indexOf(url);
        if (idx == -1)
            return false;

        const auto beforeUrl = text.leftRef(idx);
        const auto afterUrl = text.midRef(idx + url.size());
        const auto showLinks = _wordWithLink.getShowLinks();
        const auto wordSpace = _wordWithLink.getSpace();

        if (!beforeUrl.isEmpty())
            _words.emplace_back(beforeUrl.toString(), Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

        _words.emplace_back(url, afterUrl.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);

        if (!afterUrl.isEmpty())
            _words.emplace_back(afterUrl.toString(), wordSpace, WordType::TEXT, showLinks);

        return true;
    }

    static int getNickStartIndex(QStringView _text)
    {
        int index = 0;
        while (1)
        {
            index = _text.indexOf(u'@', index);
            if (index == -1 || index == 0)
                return index;
            const auto prevChar = _text.at(index - 1);
            if (!prevChar.isLetterOrNumber())
                return index;
            ++index;
        }

    }

    bool TextDrawingBlock::parseWordNickImpl(const TextWord& _word, std::vector<TextWord>& _words, const QStringRef& _text)
    {
        const auto showLinks = _word.getShowLinks();
        const auto wordSpace = _word.getSpace();

        if (const auto index = getNickStartIndex(_text); index >= 0)
        {
            const auto before = _text.left(index);
            auto nick = _text.mid(index, _text.size() - before.size());
            while (!nick.isEmpty() && !Utils::isNick(nick))
                nick = _text.mid(index, nick.size() - 1);

            if (!nick.isEmpty())
            {
                const auto after = _text.mid(index + nick.size(), _text.size() - index - nick.size());
                if (after.startsWith(u'@'))
                    return false;

                if (!before.isEmpty())
                    _words.emplace_back(before.toString(), Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

                _words.emplace_back(nick.toString(), after.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);
                if (!after.isEmpty() && !parseWordNick(_word, _words, after))
                    _words.emplace_back(after.toString(), wordSpace, WordType::TEXT, showLinks);

                return true;
            }
        }
        return false;
    }

    bool TextDrawingBlock::parseWordNick(const TextWord& _word, std::vector<TextWord>& _words, const QStringRef& _text)
    {
        if (_text.isEmpty())
        {
            const auto wordText = _word.getText();
            return parseWordNickImpl(_word, _words, QStringRef(&wordText));
        }
        return parseWordNickImpl(_word, _words, _text);
    }

    void TextDrawingBlock::correctCommandWord(TextWord _word, std::vector<TextWord>& _words)
    {
        const auto fullText = _word.getText();
        auto cmdText = _word.getLink();
        if (fullText.size() == cmdText.size())
        {
            _words.emplace_back(std::move(_word));
        }
        else
        {
            assert(fullText.size() > cmdText.size());
            auto tailText = fullText.right(fullText.size() - cmdText.size());
            _words.emplace_back(std::move(cmdText), Space::WITHOUT_SPACE_AFTER, WordType::COMMAND, _word.getShowLinks());
            _words.emplace_back(std::move(tailText), _word.getSpace(), WordType::TEXT, _word.getShowLinks());
        }
    }

    std::vector<TextWord> TextDrawingBlock::elideWords(const std::vector<TextWord>& _original, int _width, int _desiredWidth, bool _forceElide, ElideType _type)
    {
        if (_original.empty())
            return _original;

        static const auto dots = getEllipsis();
        auto f = _original.front().getFont();
        auto dotsWidth = textWidth(f, dots);

        if (_width <= 0 || (_width >= _desiredWidth))
            return _original;

        std::vector<TextWord> result;

        _width -= (dotsWidth + _original.front().spaceWidth());

        int curWidth = 0;
        bool elided = false;
        for (const auto& w : _original)
        {
            if (curWidth > _width)
            {
                if (!result.empty() && result.back().getType() == WordType::EMOJI)
                    result.pop_back();

                elided = true;
                break;
            }

            auto wordWidth = w.cachedWidth();
            if (w.isSpaceAfter())
                wordWidth += w.spaceWidth();

            if (curWidth + wordWidth <= _width || w.getType() == WordType::EMOJI)
            {
                result.push_back(w);
                curWidth += wordWidth;
                continue;
            }

            auto tmp = w.getText();
            const auto wordTextSize = tmp.size();

            if (_type == ElideType::ACCURATE)
            {
                while (!tmp.isEmpty() && textWidth(f, tmp) > (_width - curWidth))
                    tmp.chop(1);
            }
            else if (_type == ElideType::FAST)
            {
                auto left = 0, right = tmp.size();
                const auto chWidth = averageCharWidth(f);
                while (left != right)
                {
                    const auto mid = left + (right - left) / 2;
                    if (mid == left || mid == right)
                        break;

                    auto t = tmp.left(mid);
                    const auto width = textWidth(f, t);
                    const auto cmp = _width - curWidth - width;
                    const auto cmpCh = chWidth - abs(cmp);

                    if (cmpCh >= 0 && cmpCh <= chWidth)
                    {
                        tmp = std::move(t);
                        break;
                    }
                    else if (width > (_width - curWidth))
                    {
                        right = mid;
                    }
                    else
                    {
                        left = mid;
                    }
                }

                while (!tmp.isEmpty() && textWidth(f, tmp) > (_width - curWidth))
                    tmp.chop(1);
            }
            else
            {
                assert(!"Unexpected elide type");
            }

            if (tmp.size() == wordTextSize)
            {
                result.push_back(w);
                curWidth += wordWidth;
                continue;
            }

            if (!tmp.isEmpty())
            {
                const auto wordSize = tmp.size();
                auto word = TextWord(std::move(tmp), Space::WITHOUT_SPACE_AFTER, w.getType(), (w.isLinkDisabled() || w.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : w.getShowLinks()); //todo: implement method makeFrom()
                word.applyFontColor(w);
                if (w.isHighlighted() && w.highlightedFrom() < wordSize)
                    word.setHighlighted(w.highlightedFrom(), std::min(w.highlightedTo(), wordSize));
                word.width();
                result.push_back(word);
            }

            elided = true;
            break;
        }

        if (curWidth > _width && !result.empty() && result.rbegin()->getType() == WordType::EMOJI)
        {
            result.pop_back();
            elided = true;
        }

        if ((elided && !result.empty()) || _forceElide)
        {
            auto w = TextWord(dots, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, LinksVisible::DONT_SHOW_LINKS);

            if (!result.empty())
            {
                w.applyFontColor(result.back());
                w.setHighlighted(result.back().isRightHighlighted());
            }
            else if (!words_.empty())
            {
                w.applyFontColor(words_.back());
                w.setHighlighted(words_.back().isRightHighlighted());
            }
            w.width();
            result.push_back(std::move(w));
        }

        return result;
    }

    void TextDrawingBlock::setMaxLinesCount(size_t _count, LineBreakType _lineBreak)
    {
        maxLinesCount_ = _count;
        lineBreak_ = _lineBreak;
    }

    void TextDrawingBlock::setLastLineWidth(int _width)
    {
        lastLineWidth_ = _width;
    }

    int TextDrawingBlock::getLineCount() const
    {
        return lines_.size();
    }

    void TextDrawingBlock::applyLinks(const std::map<QString, QString>& _links)
    {
        if (_links.empty())
            return;

        for (const auto& [_key, _link] : _links)
        {
            if (_key.isEmpty())
                continue;

            size_t i = 0;
            std::vector<size_t> indexes_;
            QString link;
            for (auto& w : words_)
            {
                auto found = false;
                const auto text = w.getText();
                QStringRef textRef(&text);
                if (textRef.size() > 1 && (textRef.endsWith(u',') || textRef.endsWith(u'.')))
                    textRef.chop(1);

                if ((link.isEmpty() && _key.startsWith(textRef)) || (!link.isEmpty() && _key.contains(textRef)))
                {
                    link += textRef;
                    indexes_.push_back(i);
                    found = true;
                }
                else
                {
                    link.clear();
                    indexes_.clear();
                }

                if (!link.isEmpty() && link == _key)
                {
                    for (auto j : indexes_)
                        words_[j].setOriginalLink(_link);

                    link.clear();
                    indexes_.clear();

                    ++i;
                    continue;
                }
                else if (found && w.isSpaceAfter())
                {
                    link += QChar::Space;
                }

                ++i;
            }
        }

        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);
    }

    void TextDrawingBlock::applyLinksFont(const QFont & _font)
    {
        for (auto& w : words_)
        {
            if (w.isLink())
                w.setFont(_font);
        }

        calcDesired();
        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);
    }

    void TextDrawingBlock::setColor(const QColor& _color)
    {
        for (auto& w : words_)
            w.setColor(_color);

        for (auto& w : originalWords_)
            w.setColor(_color);

        for (auto& l : lines_)
        {
            for (auto& w : l)
                w.setColor(_color);
        }
    }

    void TextDrawingBlock::setLinkColor(const QColor& _color)
    {
        linkColor_ = _color;
    }

    void TextDrawingBlock::setSelectionColor(const QColor & _color)
    {
        selectionColor_ = _color;
    }

    void TextDrawingBlock::setHighlightedTextColor(const QColor& _color)
    {
        highlightTextColor_ = _color;
    }

    void TextDrawingBlock::setColorForAppended(const QColor& _color)
    {
        auto i = 0;
        for (auto& w : words_)
        {
            if (i < appended_)
            {
                if (!w.isTruncated())
                    ++i;
                continue;
            }

            w.setColor(_color);
            ++i;
        }

        i = 0;
        for (auto& l : lines_)
        {
            for (auto& w : l)
            {
                if (i < appended_)
                {
                    if (!w.isTruncated())
                        ++i;
                    continue;
                }

                w.setColor(_color);
                ++i;
            }
        }
    }

    void TextDrawingBlock::appendWords(const std::vector<TextWord>& _words)
    {
        appended_ = words_.size();

        cachedMaxLineWidth_ = -1;
        if (desiredWidth_ == 0)
            desiredWidth_ = 1;

        if (!words_.empty() && _words.size() == 1 && _words.front().getText() == spaceAsString())
        {
            words_.back().setSpace(Space::WITH_SPACE_AFTER);

            desiredWidth_ += words_.back().spaceWidth();
        }
        else
        {
            if (!words_.empty() && words_.back().isRightHighlighted() && !_words.empty() && _words.front().isLeftHighlighted())
                words_.back().setSpaceHighlighted(true);

            for (auto it = words_.insert(words_.end(), _words.begin(), _words.end()); it != words_.end(); ++it)
            {
                desiredWidth_ += it->width();
                if (it->isSpaceAfter())
                    desiredWidth_ += it->spaceWidth();
            }
        }
    }

    bool TextDrawingBlock::markdownSingle(const QFont& _font, const QColor& _color)
    {
        std::vector<int> indexes;
        auto iter = words_.begin(), first = words_.end();
        auto found = false;
        const auto S_MARK = singleBackTick();
        while (iter != words_.end())
        {
            if (iter->getFont() == _font)//already markdowned
            {
                ++iter;
                continue;
            }

            if (iter->equalTo(S_MARK))
            {
                if (first == words_.end())
                {
                    first = iter;
                }
                else if (iter->equalTo(S_MARK))
                {
                    auto i = std::next(first);
                    while (i != iter)
                    {
                        i->setFont(_font);
                        if (i->isMention())
                            i->setUnderline(isLinksUnderlined());
                        i->setColor(_color);
                        i->disableLink();
                        ++i;
                    }

                    indexes.push_back(std::distance(words_.begin(), first));
                    indexes.push_back(std::distance(words_.begin(), iter));

                    first = words_.end();
                    found = true;
                }
            }
            ++iter;
        }

        if (!found)
            return false;

        auto offset = 0;
        auto j = 1;
        for (const auto i : indexes)
        {
            auto ind = i - offset;
            auto space_after = words_[ind].isSpaceAfter();
            auto erased = words_.erase(words_.begin() + ind);
            if (j % 2 == 0 && erased != words_.begin() && space_after)
            {
                auto prev = std::prev(erased);
                prev->setSpace(Space::WITH_SPACE_AFTER);
            }
            ++offset;
            ++j;
        }

        calcDesired();
        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);

        return true;
    }

    bool TextDrawingBlock::markdownMulti(const QFont& _font, const QColor& _color, bool _split, std::pair<std::vector<TextWord>, std::vector<TextWord>>& _splitted)
    {
        auto begin = -1, end = -1, i = 0;
        const auto M_MARK = tripleBackTick();
        for (const auto& w : words_)
        {
            if (w.equalTo(M_MARK))
            {
                if (begin == -1)
                {
                    begin = i;
                }
                else if (end == -1)
                {
                    end = i;
                    break;
                }
            }
            ++i;
        }

        if (begin == -1 || end == -1)
            return false;

        i = 0;
        auto iter = words_.begin();
        while (iter != words_.end())
        {
            if (i < begin)
            {
                if (_split)
                {
                    _splitted.first.push_back(*iter);
                    iter = words_.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            else if (i > end)
            {
                if (_split)
                {
                    _splitted.second.push_back(*iter);
                    iter = words_.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            else
            {
                if (i == begin || i == end)
                {
                    iter = words_.erase(iter);
                }
                else
                {
                    iter->setFont(_font);
                    if (iter->isMention())
                        iter->setUnderline(isLinksUnderlined());
                    iter->setColor(_color);
                    iter->disableLink();
                    if (!_split)
                        iter->setSpace(Ui::TextRendering::Space::WITH_SPACE_AFTER);
                    ++iter;
                }
            }

            ++i;
        }

        calcDesired();
        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);

        return true;
    }

    int TextDrawingBlock::findForMarkdownMulti() const
    {
        auto result = 0;
        const auto M_MARK = tripleBackTick();
        for (const auto& w : words_)
        {
            if (w.equalTo(M_MARK))
                ++result;
        }

        return result;
    }

    std::vector<TextWord> TextDrawingBlock::markdown(MarkdownType _type, const QFont& _font, const QColor& _color, bool _split)
    {
        std::vector<TextWord> splitted;
        const auto M_MARK = tripleBackTick();
        switch (_type)
        {
        case Ui::TextRendering::MarkdownType::ALL:
        case Ui::TextRendering::MarkdownType::FROM:
        {
            auto iter = words_.begin();
            auto found = _type == Ui::TextRendering::MarkdownType::ALL;
            while (iter != words_.end())
            {
                if (iter->equalTo(M_MARK))
                {
                    iter = words_.erase(iter);
                    found = true;
                    continue;
                }

                if (found)
                {
                    iter->setFont(_font);
                    if (iter->isMention())
                        iter->setUnderline(isLinksUnderlined());
                    iter->setColor(_color);
                    iter->disableLink();
                    ++iter;
                }
                else if (_split)
                {
                    splitted.push_back(*iter);
                    iter = words_.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            break;
        }
        case Ui::TextRendering::MarkdownType::TILL:
        {
            auto iter = words_.rbegin();
            auto found = words_.rend();
            while (iter != words_.rend())
            {
                if (iter->equalTo(M_MARK))
                {
                    found = iter;
                    ++iter;
                    continue;
                }

                if (found != words_.rend())
                {
                    iter->setFont(_font);
                    if (iter->isMention())
                        iter->setUnderline(isLinksUnderlined());
                    iter->setColor(_color);
                    iter->disableLink();
                }

                ++iter;
            }

            if (found == words_.rend())
                break;

            auto i = words_.erase(std::next(found).base());
            if (_split && i != words_.end())
            {
                splitted.insert(splitted.end(), i, words_.end());
                words_.erase(i, words_.end());
            }
            break;
        }
        default:
            return splitted;
        }

        calcDesired();
        lines_.clear();
        getHeight(desiredWidth(), lineSpacing_, CallType::FORCE);
        return splitted;
    }

    void TextDrawingBlock::setHighlighted(const bool _isHighlighted)
    {
        const auto highlight = [_isHighlighted](auto& _words)
        {
            for (auto& w : _words)
            {
                w.setHighlighted(_isHighlighted);
                w.setSpaceHighlighted(_isHighlighted);
            }
        };
        highlight(words_);
        if (!words_.empty())
            words_.back().setSpaceHighlighted(false);

        for (auto& l : lines_)
            highlight(l);

        if (!lines_.empty() && !lines_.back().empty())
            lines_.back().back().setSpaceHighlighted(false);
    }

    void TextDrawingBlock::setHighlighted(const highlightsV& _entries)
    {
        const auto highlight = [&_entries](auto& _words)
        {
            for (size_t i = 0; i < _words.size(); ++i)
            {
                _words[i].setHighlighted(_entries);
                if (i > 0 && _words[i].isLeftHighlighted() && _words[i - 1].isRightHighlighted())
                    _words[i - 1].setSpaceHighlighted(true);
            }

            if (!_words.empty())
                _words.back().setSpaceHighlighted(false);
        };

        highlight(words_);

        for (size_t i = 0; i < lines_.size(); ++i)
        {
            highlight(lines_[i]);

            if (i > 0 && !lines_[i - 1].empty() && !lines_[i].empty())
            {
                auto& prevLast = lines_[i - 1].back();
                auto& curFirst = lines_[i].front();

                if (prevLast.isHighlighted() && curFirst.isHighlighted())
                {
                    if (prevLast.isTruncated() && !(prevLast.isRightHighlighted() && curFirst.isLeftHighlighted()))
                        curFirst.setHighlighted(false);

                    // if has space - highlight was possibly changed, so recheck
                    if (prevLast.isSpaceAfter() && prevLast.isRightHighlighted() && curFirst.isLeftHighlighted())
                        prevLast.setSpaceHighlighted(true);
                }
                else if (prevLast.isTruncated() && !prevLast.isHighlighted() && curFirst.isHighlighted())
                {
                    curFirst.setHighlighted(false);
                }
            }
        }
    }

    void TextDrawingBlock::setUnderline(const bool _enabled)
    {
        const auto underline = [_enabled](auto& _words)
        {
            for (auto& w : _words)
                w.setUnderline(_enabled);
        };
        underline(words_);
        for (auto& l : lines_)
            underline(l);
    }

    double TextDrawingBlock::getLastLineWidth() const
    {
        if (lines_.empty())
            return 0.0;

        return getLineWidth(lines_.back());
    }

    double TextDrawingBlock::getMaxLineWidth() const
    {
        if (cachedMaxLineWidth_ != -1)
            return cachedMaxLineWidth_;

        if (lines_.empty())
            return 0.0;

        std::vector<double> lineSizes;
        lineSizes.reserve(lines_.size());

        for (const auto& l : lines_)
            lineSizes.push_back(getLineWidth(l));

        cachedMaxLineWidth_ = *std::max_element(lineSizes.begin(), lineSizes.end());

        return cachedMaxLineWidth_;
    }

    std::vector<QRect> TextDrawingBlock::getLinkRects() const
    {
        auto res = linkRects_;
        if (!res.empty() && align_ != Ui::TextRendering::HorAligment::LEFT)
        {
            int i = 0;
            for (const auto& l : lines_)
            {
                ++i;
                const auto y1 = (i - 1) * lineHeight_;
                const auto y2 = i * lineHeight_;

                auto shift = 0;
                if (const auto lineWidth = getLineWidth(l); lineWidth < cachedSize_.width())
                {
                    if (align_ == HorAligment::CENTER)
                        shift = (cachedSize_.width() - lineWidth) / 2;
                    else
                        shift = cachedSize_.width() - lineWidth;
                }

                for (auto& r : res)
                {
                    if (const auto c = r.center(); c.y() > y1 && c.y() < y2)
                        r.translate(shift, 0);
                }
            }
        }
        return res;
    }

    NewLineBlock::NewLineBlock()
        : BaseDrawingBlock(BlockType::NewLine)
        , cachedHeight_(0)
    {
    }

    void NewLineBlock::init(const QFont& f, const QColor&, const QColor&, const QColor&, const QColor&, HorAligment, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        font_ = f;
    }

    void NewLineBlock::draw(QPainter&, QPoint& _point, VerPosition)
    {
        _point.ry() += cachedHeight_;
    }

    void NewLineBlock::drawSmart(QPainter&, QPoint& _point, int)
    {
        _point.ry() += cachedHeight_;
    }

    int NewLineBlock::getHeight(int, int lineSpacing, CallType, bool)
    {
        cachedHeight_ = textHeight(font_) + lineSpacing;
        return cachedHeight_;
    }

    int NewLineBlock::getCachedHeight() const
    {
        return cachedHeight_;
    }

    void NewLineBlock::setMaxHeight(int)
    {

    }

    void NewLineBlock::select(const QPoint&, const QPoint&)
    {

    }

    void NewLineBlock::selectAll()
    {

    }

    void NewLineBlock::fixSelection()
    {

    }

    void NewLineBlock::clearSelection()
    {

    }

    void NewLineBlock::releaseSelection()
    {

    }

    bool NewLineBlock::isSelected() const
    {
        return false;
    }

    bool NewLineBlock::isFullSelected() const
    {
        return true;
    }

    QString NewLineBlock::selectedText(TextType) const
    {
        return getText();
    }

    QString NewLineBlock::getText() const
    {
        const static auto lineFeed = QString(QChar::LineFeed);
        return lineFeed;
    }

    QString NewLineBlock::getSourceText() const
    {
        return getText();
    }

    QString NewLineBlock::textForInstantEdit() const
    {
        return getText();
    }

    void NewLineBlock::clicked(const QPoint&) const
    {

    }

    bool NewLineBlock::doubleClicked(const QPoint&, bool)
    {
        return false;
    }

    bool NewLineBlock::isOverLink(const QPoint&) const
    {
        return false;
    }

    QString NewLineBlock::getLink(const QPoint&) const
    {
        return QString();
    }

    void NewLineBlock::applyMentions(const Data::MentionMap&)
    {

    }

    int NewLineBlock::desiredWidth() const
    {
        return 0;
    }

    void NewLineBlock::elide(int, ElideType, bool)
    {

    }

    const std::vector<TextWord>& NewLineBlock::getWords() const
    {
        return const_cast<NewLineBlock*>(this)->getWords();
    }

    std::vector<TextWord>& NewLineBlock::getWords()
    {
        static auto empty = std::vector<TextWord>{};
        return empty;
    }


    std::vector<BaseDrawingBlockPtr> parseForBlocks(const QString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds)
    {
        std::vector<BaseDrawingBlockPtr> blocks;
        if (_text.isEmpty())
            return blocks;
        QString text;
        QStringView textView(_text);
        if (_processLineFeeds == ProcessLineFeeds::REMOVE_LINE_FEEDS)
        {
            text = _text;
            text.replace(QChar::LineFeed, QChar::Space);
            textView = text;
        }

        auto i = textView.indexOf(QChar::LineFeed);
        bool skipNewLine = true;
        while (i != -1 && !textView.isEmpty())
        {
            const auto t = textView.left(i);
            if (!t.isEmpty())
                blocks.push_back(std::make_unique<TextDrawingBlock>(t, _showLinks));

            if (!skipNewLine)
                blocks.push_back(std::make_unique<NewLineBlock>());

            textView = textView.mid(i + 1, textView.size() - i - 1);
            i = textView.indexOf(QChar::LineFeed);
            skipNewLine = (i != 0);
        }

        if (!textView.isEmpty())
            blocks.push_back(std::make_unique<TextDrawingBlock>(textView, _showLinks));

        if (!_mentions.empty())
        {
            for (auto& b : blocks)
                b->applyMentions(_mentions);
        }

        return blocks;
    }


    int getBlocksHeight(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width, int _lineSpacing, CallType _calltype)
    {
        if (_width <= 0)
            return 0;

        int h = 0;
        const bool isMultiline = _blocks.size() > 1;
        for (auto& b : _blocks)
            h += b->getHeight(_width, _lineSpacing, _calltype, isMultiline);

        return h;
    }

    void drawBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, QPainter& _p, QPoint _point, VerPosition _pos)
    {
        for (auto& b : _blocks)
            b->draw(_p, _point, _pos);
    }

    void drawBlocksSmart(const std::vector<BaseDrawingBlockPtr>& _blocks, QPainter& _p, QPoint _point, int _center)
    {
        if (_blocks.size() != 1)
            return drawBlocks(_blocks, _p, _point, VerPosition::TOP);

        _blocks[0]->drawSmart(_p, _point, _center);
    }

    void initBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        for (auto& b : _blocks)
            b->init(_font, _color, _linkColor, _selectionColor, _highlightColor, _align, _emojiSizeType, _linksStyle);
    }

    void selectBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& from, const QPoint& to)
    {
        QPoint f = from;
        QPoint t = to;
        auto y1 = 0, y2 = 0;;
        for (auto& b : _blocks)
        {
            auto h = b->getCachedHeight();
            y2 += h;

            if (y2 < from.y() || y1 > to.y())
                b->clearSelection();
            else
                b->select(f, t);

            f.ry() -= h;
            t.ry() -= h;

            y1 += h;
        }
    }

    void selectAllBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        for (auto& b : _blocks)
            b->selectAll();
    }

    void fixBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        for (auto& b : _blocks)
            b->fixSelection();
    }

    void clearBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        for (auto& b : _blocks)
            b->clearSelection();
    }

    void releaseBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        for (auto& b : _blocks)
            b->releaseSelection();
    }

    bool isBlocksSelected(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        return std::any_of(_blocks.begin(), _blocks.end(), [](const auto& b) { return b->isSelected(); });
    }

    void blocksClicked(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        QPoint p = _p;
        for (auto& b : _blocks)
        {
            b->clicked(p);
            p.ry() -= b->getCachedHeight();
        }
    }

    void blocksDoubleClicked(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback)
    {
        QPoint p = _p;
        bool result = false;
        for (auto& b : _blocks)
        {
            result |= b->doubleClicked(p, _fixSelection);
            p.ry() -= b->getCachedHeight();
        }

        if (_callback)
            _callback(result);
    }

    bool isOverLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        QPoint p = _p;
        return std::any_of(_blocks.begin(), _blocks.end(), [&p](const auto& b) {
            if (b->isOverLink(p))
                return true;
            p.ry() -= b->getCachedHeight();
            return false;
        });
    }

    QString getBlocksLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        QPoint p = _p;
        for (auto& b : _blocks)
        {
            auto l = b->getLink(p);
            if (!l.isEmpty())
                return l;

            p.ry() -= b->getCachedHeight();
        }

        return QString();
    }

    std::optional<TextWordWithBoundary> getWord(const std::vector<BaseDrawingBlockPtr>& _blocks, QPoint _p)
    {
        for (auto& b : _blocks)
        {
            if (auto e = b->getWordAt(_p, WithBoundary::Yes); e)
                return e;

            _p.ry() -= b->getCachedHeight();
        }
        return {};
    }

    bool replaceWord(const std::vector<BaseDrawingBlockPtr>& _blocks, const QString& _old, const QString& _new, QPoint _p)
    {
        for (auto& b : _blocks)
        {
            if( b->replaceWordAt(_old, _new, _p))
                return true;

            _p.ry() -= b->getCachedHeight();
        }
        return false;
    }

    static void trimLineFeeds(QString& str)
    {
        if (str.isEmpty())
            return;

        QStringView strView(str);
        int atStart = 0;
        int atEnd = 0;
        while (strView.startsWith(QChar::LineFeed))
        {
            strView = strView.mid(1);
            ++atStart;
        }
        while (strView.endsWith(QChar::LineFeed))
        {
            strView.chop(1);
            ++atEnd;
        }

        if (strView.isEmpty())
        {
            str.clear();
        }
        else
        {
            if (atStart > 0)
                str.remove(0, atStart);
            if (atEnd > 0)
                str.chop(atEnd);
        }
    }

    QString getBlocksSelectedText(const std::vector<BaseDrawingBlockPtr>& _blocks, TextType _type)
    {
        QString result;
        for (auto& b : _blocks)
            result += b->selectedText(_type);

        trimLineFeeds(result);

        return result;
    }

    QString getBlocksTextForInstantEdit(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        QString result;
        for (auto& b : _blocks)
        {
            if (b->getType() == BlockType::DebugText)
                continue;
            result += b->textForInstantEdit();
        }

        trimLineFeeds(result);

        return result;
    }

    QString getBlocksText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        QString result;
        for (auto& b : _blocks)
            result += b->getText();

        trimLineFeeds(result);

        return result;
    }

    QString getBlocksSourceText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        QString result;
        for (auto& b : _blocks)
        {
            if (b->getType() == BlockType::DebugText)
                continue;
            result += b->getSourceText();
        }

        trimLineFeeds(result);

        return result;
    }

    bool isAllBlocksSelected(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        return std::all_of(_blocks.begin(), _blocks.end(), [](const auto& b) { return b->isFullSelected(); });
    }

    int getBlocksDesiredWidth(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        int result = 0;
        for (auto& b : _blocks)
            result = std::max(result, b->desiredWidth());

        return result;
    }

    void elideBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width, ElideType _type)
    {
        bool prevElided = false;
        for (auto& b : _blocks)
        {
            b->elide(_width, _type, prevElided);
            prevElided |= b->isElided();
        }
    }

    bool isBlocksElided(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        return std::any_of(_blocks.begin(), _blocks.end(), [](const auto& b) { return b->isElided(); });
    }

    void setBlocksMaxLinesCount(const std::vector<BaseDrawingBlockPtr>& _blocks, int _count, LineBreakType _lineBreak)
    {
        for (auto& b : _blocks)
            b->setMaxLinesCount(_count, _lineBreak);
    }

    void setBlocksLastLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width)
    {
        for (auto& b : _blocks)
            b->setLastLineWidth(_width);
    }

    void applyBLocksLinks(const std::vector<BaseDrawingBlockPtr>& _blocks, const std::map<QString, QString>& _links)
    {
        for (auto& b : _blocks)
            b->applyLinks(_links);
    }

    void applyLinksFont(const std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font)
    {
        for (auto& b : _blocks)
            b->applyLinksFont(_font);
    }

    void setBlocksColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color)
    {
        for (auto& b : _blocks)
            b->setColor(_color);
    }

    void setBlocksLinkColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color)
    {
        for (auto& b : _blocks)
            b->setLinkColor(_color);
    }

    void setBlocksSelectionColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor & _color)
    {
        for (auto& b : _blocks)
            b->setSelectionColor(_color);
    }

    void setBlocksColorForAppended(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color, int _appended)
    {
        auto i = 0;
        for (auto& b : _blocks)
        {
            if (i < _appended)
            {
                ++i;
                continue;
            }

            b->setColorForAppended(_color);
            ++i;
        }
    }

    void appendBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from)
    {
        if (_from.empty())
            return;

        if (_to.empty())
            appendWholeBlocks(_to, _from);
        else
            _to.back()->appendWords(_from.front()->getWords());
    }

    void appendWholeBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from)
    {
        std::move(_from.begin(), _from.end(), std::back_inserter(_to));
        _from.clear();
    }

    bool markdownBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font, const QColor& _color, ProcessLineFeeds _processLineFeeds)
    {
        auto multi = [&_blocks, _font, _color, _processLineFeeds]()
        {
            bool modified = false;
            auto begin = -1, end = -1, i = 0;
            for (const auto& b : _blocks)
            {
                auto count = b->findForMarkdownMulti();
                if (begin == -1 && count == 1)
                    begin = i;
                else if (begin != -1 && count > 0)
                    end = i;

                if (begin != -1 && end != -1)
                {
                    if (end > begin)
                    {
                        auto splittedAtBegin = _blocks[begin]->markdown(MarkdownType::FROM, _font, _color, _processLineFeeds == ProcessLineFeeds::KEEP_LINE_FEEDS);

                        for (auto j = begin + 1; j < end; ++j)
                            _blocks[j]->markdown(MarkdownType::ALL, _font, _color, _processLineFeeds == ProcessLineFeeds::KEEP_LINE_FEEDS);

                        auto splittedAtEnd = _blocks[end]->markdown(MarkdownType::TILL, _font, _color, _processLineFeeds == ProcessLineFeeds::KEEP_LINE_FEEDS);

                        if (_processLineFeeds == ProcessLineFeeds::KEEP_LINE_FEEDS)
                        {
                            auto add = 0;
                            if (!splittedAtBegin.empty())
                            {
                                _blocks.emplace(_blocks.begin() + begin, std::make_unique<TextDrawingBlock>(splittedAtBegin, _blocks[begin].get()));
                                ++add;
                            }

                            if (!splittedAtEnd.empty())
                            {
                                if (end == (int)_blocks.size() + add)
                                    _blocks.emplace_back(std::make_unique<TextDrawingBlock>(splittedAtEnd, _blocks[end + add].get()));
                                else
                                    _blocks.emplace(_blocks.begin() + end + 1 + add, std::make_unique<TextDrawingBlock>(splittedAtEnd, _blocks[end + add].get()));
                            }
                        }
                    }

                    return true;
                }

                else if (count > 1)
                {
                    std::pair<std::vector<TextWord>, std::vector<TextWord>> splitted;
                    _blocks[i]->markdownMulti(_font, _color, _processLineFeeds == ProcessLineFeeds::KEEP_LINE_FEEDS, splitted);

                    auto add = 0;
                    if (!splitted.first.empty())
                    {
                        _blocks.emplace(_blocks.begin() + i, std::make_unique<TextDrawingBlock>(splitted.first, _blocks[i].get()));
                        ++add;
                    }

                    if (!splitted.second.empty())
                    {
                        if (i == (int)_blocks.size() + add)
                            _blocks.emplace_back(std::make_unique<TextDrawingBlock>(splitted.second, _blocks[i + add].get()));
                        else
                            _blocks.emplace(_blocks.begin() + i + 1 + add, std::make_unique<TextDrawingBlock>(splitted.second, _blocks[i + add].get()));
                    }

                    return true;
                }

                ++i;
            }

            return modified;
        };

        auto modified = multi();
        if (modified)
        {
            do { ; } while (multi());
        }

        for (auto & b : _blocks)
            modified |= b->markdownSingle(_font, _color);

        return modified;
    }

    void highlightBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const bool _isHighlighted)
    {
        for (auto & b : _blocks)
            b->setHighlighted(_isHighlighted);
    }

    void highlightParts(std::vector<BaseDrawingBlockPtr>& _blocks, const highlightsV& _entries)
    {
        for (auto & b : _blocks)
            b->setHighlighted(_entries);
    }

    void underlineBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const bool _enabled)
    {
        for (auto & b : _blocks)
            b->setUnderline(_enabled);
    }

    int getBlocksLastLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        if (_blocks.empty())
            return 0;

        const auto iter = std::find_if(_blocks.rbegin(), _blocks.rend(), [](const auto& _block) { return !_block->isEmpty(); });
        if (iter == _blocks.rend())
            return 0;

        return ceilToInt((*iter)->getLastLineWidth());
    }

    int getBlocksMaxLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        if (_blocks.empty())
            return 0;

        std::vector<double> blockSizes;
        blockSizes.reserve(_blocks.size());

        for (const auto& b : _blocks)
            blockSizes.push_back(b->getMaxLineWidth());

        return ceilToInt(*std::max_element(blockSizes.begin(), blockSizes.end()));
    }

    void setBlocksHighlightedTextColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color)
    {
        for (auto& b : _blocks)
            b->setHighlightedTextColor(_color);
    }

    void disableCommandsInBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        for (auto& b : _blocks)
            b->disableCommands();
    }

    void setShadowForBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const int _offsetX, const int _offsetY, const QColor& _color)
    {
        for (auto& b : _blocks)
            b->setShadow(_offsetX, _offsetY, _color);
    }

    std::optional<TextWordWithBoundary> BaseDrawingBlock::getWordAt(QPoint, WithBoundary) const
    {
        return {};
    }

    bool BaseDrawingBlock::replaceWordAt(const QString&, const QString&, QPoint)
    {
        return false;
    }
}
}

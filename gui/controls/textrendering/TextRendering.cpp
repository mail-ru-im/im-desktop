#include "stdafx.h"

#include "TextRenderingUtils.h"
#include "TextRendering.h"
#include "FormattedTextRendering.h"
#include "TextWordRenderer.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/UrlParser.h"
#include "utils/InterConnector.h"
#include "fonts.h"

#include "../common.shared/message_processing/message_tokenizer.h"
#include "../common.shared/config/config.h"

using ftype = core::data::format_type;

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

        for (auto it = _line.cbegin(); it != _line.cend(); ++it)
        {
            const auto& w = *it;
            const auto& subwords = w.getSubwords();
            const auto isLast = it == _line.cend() - 1;
            if (subwords.empty())
            {
                _renderer.draw(w, true, isLast);
            }
            else
            {
                for (auto s = subwords.begin(), end = std::prev(subwords.end()); s != end; ++s)
                    _renderer.draw(*s, false, false);

                _renderer.draw(subwords.back(), true, isLast);
            }
        }
    }

    constexpr int maxCommandSize() noexcept { return 32; }

    bool isBrailleSpace(QChar _c) noexcept
    {
        return _c == 0x2800;
    }
}

namespace Ui::TextRendering
{
    ParagraphType toParagraphType(core::data::format_type _type)
    {
        switch (_type)
        {
        case core::data::format_type::ordered_list:
            return ParagraphType::OrderedList;
        case core::data::format_type::unordered_list:
            return ParagraphType::UnorderedList;
        case core::data::format_type::pre:
            return ParagraphType::Pre;
        case core::data::format_type::quote:
            return ParagraphType::Quote;
        default:
            return ParagraphType::Regular;
        }
    };

    core::data::format_type toFormatType(ParagraphType _type)
    {
        switch (_type)
        {
        case ParagraphType::OrderedList:
            return core::data::format_type::ordered_list;
        case ParagraphType::UnorderedList:
            return core::data::format_type::unordered_list;
        case ParagraphType::Pre:
            return core::data::format_type::pre;
        case ParagraphType::Quote:
            return core::data::format_type::quote;
        default:
            return core::data::format_type::none;
        }
    };

    void emplace_word_back(std::vector<TextWord>& _words, Data::FStringView _text, Space _space, WordType _type, LinksVisible _showLinks, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR)
    {
        if (!_words.empty())
            im_assert(_words.back().view().sourceRange().end() == _text.sourceRange().offset_);

        _words.emplace_back(_text, _space, _type, _showLinks, _emojiSizeType);
    }

    void emplace_word_back(std::vector<TextWord>& _words, Data::FStringView _text, Emoji::EmojiCode _code, Space _space, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR)
    {
        if (!_words.empty())
            im_assert(_words.back().view().sourceRange().end() == _text.sourceRange().offset_);

        _words.emplace_back(_text, _code, _space, _emojiSizeType);
    }

    void emplace_word_back(std::vector<TextWord>& _words, TextWord&& _word)
    {
        if (!_words.empty())
            im_assert(_words.back().view().sourceRange().end() == _word.view().sourceRange().offset_);

        _words.emplace_back(std::move(_word));
    }

    TextWord::TextWord(Data::FStringView _text, Space _space, WordType _type, LinksVisible _showLinks, EmojiSizeType _emojiSizeType)
        : code_(0)
        , space_(_space)
        , type_(_type)
        , view_(_text)
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
        im_assert(type_ != WordType::EMOJI);

        if ((type_ == WordType::LINK || type_ == WordType::HASHTAG) && _showLinks == LinksVisible::SHOW_LINKS)
            originalUrl_ = _text.toString();

        checkSetClickable();
        initFormat();

        if (!view_.isEmpty())
            im_assert(space_ != Space::WITH_SPACE_AFTER || view_.back().isSpace());
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
        im_assert(!code_.isNull());
    }

    TextWord::TextWord(Data::FStringView _view, Emoji::EmojiCode _code, Space _space, EmojiSizeType _emojiSizeType)
        : TextWord(_code, _space, _emojiSizeType)
    {
        view_ = _view;
        initFormat();
    }

    void TextWord::initFormat()
    {
        if (view_.isEmpty())
            return;

        const auto applyFormattedLink = [this](const std::string& _data)
        {
            originalUrl_ = QString::fromStdString(_data);
            setOriginalLink(originalUrl_);
        };

        std::vector<int> cutPositions;
        auto addCutPosition = [&cutPositions, this](int _pos)
        {
            if (_pos != 0 && _pos != view_.size() && _pos != viewNoEndSpace().size())
                cutPositions.push_back(_pos);
        };

        Data::FormatTypes styles;
        const auto textStyles = viewNoEndSpace().getStyles();
        for (const auto& styleInfo : textStyles)
        {
            const auto type = styleInfo.type_;
            const auto& range = styleInfo.range_;

            styles.setFlag(type);
            if (type != ftype::link)
            {
                addCutPosition(range.offset_);
                addCutPosition(range.offset_ + range.size_);
            }
            else
            {
                if (const auto& data = styleInfo.data_)
                    applyFormattedLink(*data);
            }
        }
        setStyles(styles);

        if (isLink())
        {   // Calc cut positions for emoji subword which might appear in links
            const auto trimmedText = plainViewNoEndSpace();
            auto i = qsizetype(0);
            while (i < trimmedText.size())
            {
                auto iAfterEmoji = i;
                if (trimmedText.at(i) == QChar::Space)
                {
                    addCutPosition(i);
                    ++i;
                }
                else if (const auto emoji = Emoji::getEmoji(trimmedText, iAfterEmoji); !emoji.isNull())
                {
                    addCutPosition(i);
                    addCutPosition(iAfterEmoji);
                    i = iAfterEmoji;
                }
                else
                {
                    ++i;
                }
            }
        }

        cutPositions.push_back(view_.size());
        std::sort(cutPositions.begin(), cutPositions.end());
        cutPositions.erase(std::unique(cutPositions.begin(), cutPositions.end()), cutPositions.end());
        if (cutPositions.size() > 1)
        {
            auto start = 0;
            for (auto end : cutPositions)
            {
                const auto spaceAfter = end == view_.size() ? space_ : Space::WITHOUT_SPACE_AFTER;
                const auto subView = view_.mid(start, end - start);
                if (isLink())
                {
                    auto emojiLength = qsizetype(0);
                    if (auto emoji = Emoji::getEmoji(subView.string(), emojiLength); !emoji.isNull())
                    {
                        emplace_word_back(subwords_, subView.mid(0, emojiLength), emoji, spaceAfter);
                        start += emojiLength;
                        continue;
                    }
                }
                const auto showLinks = isLink() ? showLinks_ : LinksVisible::DONT_SHOW_LINKS;
                emplace_word_back(subwords_, subView, spaceAfter, type_, showLinks, emojiSizeType_);

                if (type_ == WordType::LINK)
                    subwords_.back().setOriginalUrl(originalUrl_);

                start = end;
            }
        }
    }

    QString TextWord::getText(TextType _text_type) const
    {
        if (isEmoji())
            return Emoji::EmojiCode::toQString(code_);

        if (_text_type == TextType::VISIBLE && !substitution_.isEmpty())
            return substitution_.string();

        if (subwords_.empty() || _text_type == TextType::SOURCE)
            return viewNoEndSpace().toString();

        QString result;
        for (const auto& s : subwords_)
        {
            result += s.getText(_text_type);
            if (s.isSpaceAfter())
                result += QChar::Space;
        }

        return result;
    }

    QString TextWord::getVisibleText() const
    {
        return getText(TextType::VISIBLE);
    }

    QStringView TextWord::plainViewNoEndSpace() const
    {
        return viewNoEndSpace().string();
    }

    Data::FStringView TextWord::viewNoEndSpace() const
    {
        return space_ == Space::WITH_SPACE_AFTER && !view_.isEmpty() && view_.back().isSpace() ? view_.left(view_.size() - 1) : view_;
    }

    QStringView TextWord::plainVisibleTextNoEndSpace() const
    {
        return isSubstitutionUsed() ? substitution_.string() : plainViewNoEndSpace();
    }

    Data::FStringView TextWord::getVisibleTextView() const
    {
        return substitution_.isEmpty() ? view_ : substitution_;
    }

    bool TextWord::equalTo(QStringView _sl) const
    {
        return view_.string() == _sl;
    }

    bool TextWord::equalTo(QChar _c) const
    {
        return view_.string() == _c;
    }

    Data::LinkInfo TextWord::getLink() const
    {
        const auto displayText = getVisibleText();

        if (type_ == WordType::MENTION)
            return { plainViewNoEndSpace().toString(), displayText };
        else if (type_ == WordType::NICK)
            return { plainViewNoEndSpace().toString(), displayText };
        else if (type_ == WordType::COMMAND)
            return { originalLink_, displayText };
        else if (type_ == WordType::HASHTAG)
            return { originalLink_, displayText };
        else if (type_ == WordType::EMOJI && !originalUrl_.isEmpty())
            return { originalLink_, displayText };
        else if (type_ != WordType::LINK)
            return {};

        return { (originalLink_.isEmpty() ? plainViewNoEndSpace().toString() : originalLink_), displayText };
    }

    double TextWord::width(WidthMode _force, int _emojiCount, bool _isLastWord)
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
            {
                if (isInTooltip())
                    emojiSize_ -= Utils::scale_value(1);
            }
        }

        if (subwords_.empty())
        {
            spaceWidth_ = textSpaceWidth(font_);
            if (isEmoji())
                cachedWidth_ = emojiSize_;
            else
                cachedWidth_ = _isLastWord && italic()
                    ? textVisibleWidth(font_, plainVisibleTextNoEndSpace().toString())
                    : textWidth(font_, plainVisibleTextNoEndSpace().toString());
        }
        else
        {
            cachedWidth_ = 0;
            for (auto it = subwords_.begin(); it != subwords_.end(); ++it)
            {
                auto& w = *it;
                cachedWidth_ += w.width(_force, _emojiCount, (_isLastWord && it == subwords_.cend() - 1));
                if (w.isSpaceAfter())
                {
                    if (std::next(it) == subwords_.end())
                        spaceWidth_ = textSpaceWidth(font_);
                    else if (std::next(it) != subwords_.end())
                        cachedWidth_ += w.spaceWidth();
                }
            }
        }

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

        const auto text = subwords_.empty() ? getVisibleTextView().toString() : getText(TextType::VISIBLE);
        if (_x >= cachedWidth_)
            return text.size();

        const auto averageWidth = averageCharWidth(font_);
        const auto letter_space = font_.letterSpacing();

        auto currentWidth = 0.0;
        for (int i = 0; i < text.size(); ++i)
        {
            const auto symbolWidth = textWidth(font_, text.at(i));
            if(symbolWidth < 0.7 * averageWidth)
            {
                if (currentWidth + ceil(symbolWidth) >= _x)
                    return i;
            }
            else
            {
                if (currentWidth + ceil(symbolWidth / 2) >= _x)
                    return i;
            }

            currentWidth += symbolWidth;
        }

        return text.size();
    }

    void TextWord::select(int _from, int _to, SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = ((selectedTo_ == view_.size() || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        if (type_ == WordType::MENTION  && (_from != 0 || _to != 0))
            return selectAll((_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter() && _to == getText().size())
                ? SelectionType::WITH_SPACE_AFTER
                : SelectionType::WITHOUT_SPACE_AFTER);

        if (_from == _to)
        {
            if (_from == 0)
                clearSelection();

            return;
        }

        selectedFrom_ = _from;
        selectedTo_ = isEmoji() ? 1 : _to;
        const auto textSize = getText().size();
        isSpaceSelected_ = ((selectedTo_ == textSize || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());

        if (!isMention() && !subwords_.empty())
        {
            auto offset = 0;
            for (auto& w : subwords_)
            {
                w.clearSelection();
                auto size = w.getVisibleTextView().size();

                if (w.type_ == WordType::EMOJI && size > 1)
                    size = 1;

                if (auto begin = qBound(0, selectedFrom_ - offset, size); begin < size)
                {
                    w.selectedFrom_ = begin;
                    w.selectedTo_ = qBound(begin, selectedTo_ - offset, size);
                }
                offset += size;
            }
            subwords_.back().isSpaceSelected_ = isSpaceSelected_;
        }
    }

    void TextWord::selectAll(SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = (_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        selectedFrom_ = 0;
        selectedTo_ = isEmoji() ? 1 : getVisibleTextView().size();
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
        const auto isFormattedLink = styles_.testFlag(core::data::format_type::link);
        const auto openLink = [this, &isFormattedLink]()
        {
            const auto sourceUrl = (originalLink_.isEmpty() ? plainViewNoEndSpace() : originalLink_).trimmed();

            QString url;
            Utils::UrlParser parser;

            auto isUrlValid = !sourceUrl.contains(u' ');
            if (isUrlValid)
            {
                parser.process(sourceUrl);
                isUrlValid = parser.hasUrl();
                if (isUrlValid)
                    url = parser.formattedUrl();
            }

            const auto urlString = isUrlValid ? url : qsl("http://%1/").arg(sourceUrl);
            if (isFormattedLink)
                Utils::openUrl(urlString, Utils::OpenUrlConfirm::Yes);
            else if (isUrlValid)
                Utils::openUrl(urlString);
        };

        if (type_ == WordType::MENTION)
        {
            Utils::openUrl(plainViewNoEndSpace());
        }
        else if (type_ == WordType::NICK)
        {
            Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(plainViewNoEndSpace().toString());
        }
        else if (type_ == WordType::COMMAND)
        {
            if (isFormattedLink)
                openLink();
            else if (!originalLink_.isEmpty())
                Q_EMIT Utils::InterConnector::instance().sendBotCommand(originalLink_);
        }
        else if (type_ == WordType::HASHTAG)
        {
            if (isFormattedLink)
                openLink();
            else if (!originalLink_.isEmpty())
                Q_EMIT Utils::InterConnector::instance().startSearchHashtag(originalLink_);
        }
        else if (type_ == WordType::LINK || !originalLink_.isEmpty())
        {
            if (Utils::isMentionLink(originalLink_))
            {
                Utils::openUrl(originalLink_);
                return;
            }
            openLink();
        }
    }

    double TextWord::cachedWidth() const noexcept
    {
        return cachedWidth_;
    }

    void TextWord::setHighlighted(const bool _isHighlighted) noexcept
    {
        highlight_ = { 0, _isHighlighted ? (isEmoji() ? 1 : getVisibleTextView().size()) : 0 };
        for (auto& sw : subwords_)
            sw.setHighlighted(_isHighlighted);
    }

    void TextWord::setHighlighted(const int _from, const int _to) noexcept
    {
        const auto maxPos = isEmoji() ? 1 : getVisibleTextView().size();
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
            else if (getVisibleTextView().size() >= hl.size())
            {
                if (const auto idx = getVisibleTextView().string().indexOf(hl, 0, Qt::CaseInsensitive); idx != -1)
                {
                    highlight_ = { idx, idx + hl.size() };
                    return;
                }
            }
        }
    }

    double TextWord::spaceWidth() const
    {
        if (subwords_.empty())
            return spaceWidth_;
        return subwords_.back().spaceWidth();
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
        if (!isValidMention(view_.string(), _mention.first))
            return false;

        //originalMention_ = std::exchange(text_, _mention.second);
        substitution_ = _mention.second;
        type_ = WordType::MENTION;
        showLinks_ = LinksVisible::SHOW_LINKS;
        return true;
    }

    bool TextWord::applyMention(const QString& _mention, const std::vector<TextWord>& _mentionWords)
    {
        if (!isValidMention(view_.string(), _mention))
            return false;

        //originalMention_ = std::exchange(text_, {});
        substitution_ = {};
        type_ = WordType::MENTION;
        showLinks_ = LinksVisible::SHOW_LINKS;
        setSubwords(_mentionWords);
        if (isSpaceAfter() && hasSubwords())
            subwords_.back().setSpace(Space::WITH_SPACE_AFTER);

        for (auto& s : subwords_)
        {
            s.applyFontParameters(*this);
            s.setStyles(getStyles());
        }

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
        if (type_ == WordType::LINK || type_ == WordType::NICK || type_ == WordType::COMMAND || type_ == WordType::HASHTAG)
        {
            type_ = WordType::TEXT;
            originalLink_.clear();
            showLinks_ = LinksVisible::DONT_SHOW_LINKS;
        }
    }

    QString TextWord::disableCommand()
    {
        for (auto& s : subwords_)
            s.disableCommand();

        if (isBotCommand())
        {
            type_ = WordType::TEXT;
            showLinks_ = LinksVisible::DONT_SHOW_LINKS;
            if (underline())
                setUnderline(false);

            return std::exchange(originalLink_, {});
        }

        static const QString empty;
        return empty;
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

    void TextWord::applyFontParameters(const TextWord& _other)
    {
        setFont(_other.getFont());
        setColor(_other.getColor());

        if (isEmoji())
            setUnderline(_other.isMention() ? _other.underline() : false);

        setItalic(_other.italic());
        setBold(_other.bold());
        setStrikethrough(_other.strikethrough());
        setSpellError(_other.hasSpellError());
    }

    void TextWord::applyCachedStyles(const QFont& _font, const QFont& _monospaceFont)
    {
        const auto& styles = getStyles();

        if (styles.testFlag(ftype::monospace))
            setFont(_monospaceFont);
        else
            setFont(_font);

        if (styles.testFlag(ftype::bold))
            setBold(true);

        if (styles.testFlag(ftype::italic))
            setItalic(true);

        if (styles.testFlag(ftype::strikethrough))
            setStrikethrough(true);

        if (styles.testFlag(ftype::underline))
            setUnderline(true);

        for (auto& w : subwords_)
            w.applyCachedStyles(_font, _monospaceFont);
    }

    void TextWord::setSubwords(std::vector<TextWord> _subwords)
    {
        subwords_ = std::move(_subwords);
        cachedWidth_ = 0;

        if (isMention())
        {
            for (auto& w : subwords_)
            {
                w.view_ = view_;
                im_assert(!w.substitution_.isEmpty());
            }
        }
    }

    std::vector<TextWord> TextWord::splitByWidth(int _widthAvailable)
    {
        std::vector<TextWord> wordsAfterSplit;
        if (subwords_.empty())//todo implement for common words
            return wordsAfterSplit;

        auto widthUsed = 0;
        std::vector<TextWord> subwordsFirst;

        auto finalize = [&wordsAfterSplit, &subwordsFirst, this](auto _iter) mutable
        {
            auto tw = *this;
            tw.setSubwords(subwordsFirst);
            tw.width(WidthMode::FORCE);
            wordsAfterSplit.push_back(tw);

            if (auto leftovers = std::vector<TextWord>(_iter, subwords_.end()); !leftovers.empty())
            {
                tw.setSubwords(std::move(leftovers));
                tw.width(WidthMode::FORCE);
                wordsAfterSplit.push_back(tw);
            }
        };

        auto makeWord = [this](const TextWord& _subword, auto _textPartThatFits, Space _space)
        {
            const auto fittedWordView = _subword.isSubstitutionUsed() ? _subword.view() : _textPartThatFits;
            auto word = TextWord(fittedWordView, Space::WITHOUT_SPACE_AFTER, getType(), linkDisabled_ ? LinksVisible::DONT_SHOW_LINKS : getShowLinks());
            if (_subword.isSubstitutionUsed())
                word.setSubstitution(_textPartThatFits);

            word.applyFontParameters(_subword);
            word.width();
            if (word.isLink())
                word.setOriginalLink(word.getOriginalUrl());
            return word;
        };

        for (auto subwordIt = subwords_.begin(); subwordIt != subwords_.end(); ++subwordIt)
        {
            auto wordWidth = subwordIt->cachedWidth();
            if (widthUsed + wordWidth <= _widthAvailable)
            {
                widthUsed += wordWidth;
                if (subwordIt->isSpaceAfter())
                    widthUsed += subwordIt->spaceWidth();

                subwordsFirst.push_back(*subwordIt);
                continue;
            }

            if (subwordIt->isEmoji())
            {
                finalize(subwordIt);
                break;
            }

            const auto curSubwordText = subwordIt->getVisibleTextView();
            auto textPartThatFits = elideText(subwordIt->getFont(), curSubwordText, _widthAvailable - widthUsed);

            if (textPartThatFits.isEmpty())
            {
                finalize(subwordIt);
                break;
            }

            auto tw = makeWord(*subwordIt, textPartThatFits, Space::WITHOUT_SPACE_AFTER);
            tw.setTruncated();
            subwordsFirst.push_back(tw);

            if (textPartThatFits.string() == curSubwordText.string())
            {
                finalize(++subwordIt);
                break;
            }

            tw = makeWord(*subwordIt, curSubwordText.mid(textPartThatFits.size()), getSpace());

            std::vector<TextWord> subwordsSecond = { std::move(tw) };
            if (++subwordIt != subwords_.end())
                subwordsSecond.insert(subwordsSecond.end(), subwordIt, subwords_.end());

            auto temp = *this;
            temp.setSubwords(std::move(subwordsFirst));
            temp.width(WidthMode::FORCE);
            wordsAfterSplit.push_back(temp);

            if (!subwordsSecond.empty())
            {
                temp.setSubwords(std::move(subwordsSecond));
                temp.width(WidthMode::FORCE);
                wordsAfterSplit.push_back(std::move(temp));
            }

            break;
        }

        if (wordsAfterSplit.empty())
        {
            auto temp = *this;
            temp.setSubwords(std::move(subwordsFirst));
            temp.width(WidthMode::FORCE);
            wordsAfterSplit.push_back(std::move(temp));
        }

        return wordsAfterSplit;
    }

    void TextWord::setUnderline(bool _enabled)
    {
        if (isEmoji() && isLink())
            return;

        if (font_.underline() != _enabled)
            font_.setUnderline(_enabled);

        for (auto& w : subwords_)
            w.setUnderline(_enabled);
    }

    void TextWord::setBold(bool _enabled)
    {
        if (isEmoji())
            return;

        if (font_.bold() != _enabled)
            font_.setBold(_enabled);
        for (auto& w : subwords_)
            w.setBold(_enabled);
    }

    void TextWord::setItalic(bool _enabled)
    {
        if (isEmoji())
            return;

        if (font_.italic() != _enabled)
            font_.setItalic(_enabled);
        for (auto& w : subwords_)
            w.setItalic(_enabled);
    }

    void TextWord::setStrikethrough(bool _enabled)
    {
        if (isEmoji())
            return;

        if (font_.strikeOut() != _enabled)
            font_.setStrikeOut(_enabled);
        for (auto& w : subwords_)
            w.setStrikethrough(_enabled);
    }

    void TextWord::setEmojiSizeType(EmojiSizeType _emojiSizeType) noexcept
    {
        emojiSizeType_ = _emojiSizeType;
    }

    void TextRendering::TextWord::setSpellError(bool _value)
    {
        hasSpellError_ = _value;

        core::data::range subwordRange;
        for (auto& w : subwords_)
        {
            subwordRange.size_ = w.plainViewNoEndSpace().size();
            for (auto& sw : getSyntaxWords())
            {
                w.syntaxWords_.clear();
                if (const auto [offset, size, startIndex] = subwordRange.intersected(sw); size > 0)
                {
                    w.syntaxWords_.emplace_back(offset - subwordRange.offset_, size, sw.spellError);
                    w.setSpellError(sw.spellError);
                }
            }

            subwordRange.offset_ += subwordRange.size_;
        }
    }

    void TextWord::checkSetClickable()
    {
        const auto text = plainViewNoEndSpace();
        if (showLinks_ == LinksVisible::SHOW_LINKS && type_ != WordType::EMOJI && text.size() > 1)
        {
            if (text.startsWith(u'@'))
            {
                if (Utils::isNick(text))
                    type_ = WordType::NICK;
            }
            else if (text.startsWith(u'/'))
            {
                if (const auto cmd = text.mid(1); cmd.size() <= maxCommandSize())
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
                            originalLink_ = text.toString();
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
                                originalLink_ = text.chopped(std::distance(posAfterCmd, cmd.end())).toString();
                            }
                        }
                    }
                }
            }
        }
    }

    void TextRendering::TextWord::setSubstitution(Data::FStringView _substitution)
    {
        substitution_ = _substitution.toFString();
    }

    void TextRendering::TextWord::setSubstitution(QStringView _substitution)
    {
        substitution_ = _substitution.toString();
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
        const auto it = std::find_if(words.begin(), words.end(), [_w](auto x) { return _w.offset_ == x.offset_ && _w.size_ == x.size_; });
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

        if (words.size() == 1 && words.front().offset_ == 0 && words.front().size_ == text.size())
            return words.front();

        const auto pos = getPosByX(_x);
        for (auto w : words)
        {
            if (pos >= w.offset_ && pos < w.end())
                return w;
        }

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

    TextDrawingBlock::TextDrawingBlock(Data::FStringView _text, LinksVisible _showLinks, BlockType _blockType)
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

    void TextDrawingBlock::init(const QFont& _font, const QFont& _monospaceFont, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        auto needUnderline = [_linksStyle](const auto& w)
        {
            return w.getShowLinks() == LinksVisible::SHOW_LINKS && w.isLink() && w.getType() != WordType::HASHTAG
                && _linksStyle == LinksStyle::UNDERLINED;
        };

        for (auto& words : { boost::make_iterator_range(words_), boost::make_iterator_range(originalWords_) })
        {
            for (auto& w : words)
            {
                w.setColor(_color);
                w.setEmojiSizeType(_emojiSizeType);

                w.applyCachedStyles(_font, _monospaceFont);

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
            im_assert(lines_.size() == 1);

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

        for (const auto& line : lines_)
        {
            drawLine(renderer, _point, line, align_, cachedSize_.width());
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
        _width -= margins_.left() + margins_.right();
        double curWidth = 0;
        lineHeight_ = 0;

        const size_t emojiCount = _isMultiline ? 0 : getEmojiCount(words_);

        if (emojiCount != 0)
            needsEmojiMargin_ = true;

        const auto& last = *(words_.rbegin());
        auto fillLines = [_width, &curLine, &curWidth, emojiCount, last, this]()
        {
            for (auto it = words_.begin(); it != words_.end(); ++it)
            {
                auto& w = *it;
                const auto limited = (maxLinesCount_ != std::numeric_limits<size_t>::max() && (lineBreak_ == LineBreakType::PREFER_WIDTH || lines_.size() == maxLinesCount_ - 1));

                lineHeight_ = std::max(lineHeight_, textHeight(w.getFont()));

                auto wordWidth = w.width(WidthMode::FORCE, emojiCount, (it == words_.end() - 1));
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

                    const auto curWordText = curWord.view();
                    const auto elidedText = splitedWords.empty() ? elideText(w.getFont(), curWordText, cmpWidth) : Data::FStringView();

                    if ((splitedWords.empty() && elidedText.isEmpty()) || (!splitedWords.empty() && !splitedWords.front().hasSubwords()) || splitedWords.size() == 1)
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

                    auto makeWord = [&curWord](auto _elided, bool _useWordsSpace)
                    {
                        return TextWord(
                            _elided,
                            _useWordsSpace ? curWord.getSpace() : Space::WITHOUT_SPACE_AFTER,
                            curWord.getType(),
                            (curWord.isLinkDisabled() || curWord.getType() == WordType::TEXT)
                                ? LinksVisible::DONT_SHOW_LINKS
                                : curWord.getShowLinks());
                    };

                    auto tw = splitedWords.empty() ? makeWord(elidedText, false) : splitedWords.front();

                    tw.setSpellError(w.hasSpellError());
                    if (splitedWords.empty())
                    {
                        tw.applyFontParameters(w);
                        tw.setHighlighted(w.highlightedFrom() - wordLen, w.highlightedTo() - wordLen);
                        tw.setWidth(cmpWidth, emojiCount);

                        wordLen += elidedText.size();
                    }

                    if (tw.isLink())
                    {
                        linkRects_.emplace_back(limited ? curWidth : 0, lines_.size(), tw.cachedWidth(), 0);
                        tw.setOriginalLink(w.getOriginalUrl());
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

                    curWord = splitedWords.size() <= 1
                        ? makeWord(curWordText.mid(elidedText.size()), true)
                        : splitedWords[1];
                    curWord.setSpellError(w.hasSpellError());
                    if (curWord.isLink())
                        curWord.setOriginalLink(w.getOriginalUrl());

                    if (splitedWords.size() <= 1)
                    {
                        curWord.applyFontParameters(w);
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

        if (maxLinesCount_ > 0)
        {
            auto elided = fillLines();
            if (lines_.size() == maxLinesCount_)
            {
                if (lines_[maxLinesCount_ - 1].rbegin()->isTruncated() || (elided && !words_.empty()))
                {
                    lines_[maxLinesCount_ - 1] = elideWords(lines_[maxLinesCount_ - 1], lastLineWidth_ == -1 ? _width : lastLineWidth_, QWIDGETSIZE_MAX, true);
                    elided_ = true;
                }
            }
        }
        else
        {
            elided_ = !words_.empty();
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

    void TextDrawingBlock::select(QPoint _from, QPoint _to)
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
            const auto y1 = curLine * lineHeight_;
            const auto y2 = (curLine + 1) * lineHeight_;

            auto minY = std::min(_from.y(), _to.y());
            auto maxY = std::max(_from.y(), _to.y());

            const auto topToBottom = (minY < y1 && maxY < y2) || lineSelected(curLine - 1);
            const auto bottomToTop = (minY > y1 && maxY > y2) || lineSelected(curLine + 1);
            const auto ordinary = (!topToBottom && !bottomToTop);

            if (topToBottom)
                minY = y1;
            else if (bottomToTop)
                maxY = y2;

            if (minY <= y1 && maxY >= y2)
            {
                for (auto& w : l)
                    w.selectAll(SelectionType::WITH_SPACE_AFTER);
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

                auto toSelectionType = [](bool _needSpace)
                {
                    return _needSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER;
                };

                int curWidth = 0;
                for (auto& w : l)
                {
                    curWidth += w.cachedWidth();

                    auto diff = curWidth - w.cachedWidth();
                    const auto selectSpace = bottomToTop || x2 > curWidth;

                    if (diff < x1 && curWidth > x1)
                    { // "cachedSize.width() - curWidth < 0" condition for too large lines included in the link
                        w.select(w.getPosByX(x1 - diff),
                            (x2 >= curWidth) || cachedSize_.width() - curWidth < 0
                                ? w.getText().size()
                                : w.getPosByX(x2 - diff),
                            toSelectionType(selectSpace));
                    }
                    else if (diff < x1)
                    {
                        w.clearSelection();
                    }
                    else if (diff >= x1 && curWidth <= x2)
                    {
                        w.selectAll(toSelectionType(x2 - curWidth >= 0));
                    }
                    else if (diff < x2)
                    {
                        w.select(0, w.getPosByX(x2 - diff), toSelectionType(selectSpace));
                    }
                    else
                    {
                        w.clearSelection();
                    }

                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();
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

    QString TextDrawingBlock::selectedPlainText(TextType _type) const
    {
        QString result;
        for (const auto& line : lines_)
        {
            for (const auto& w : line)
            {
                if (!w.isSelected())
                    continue;

                if (w.isFullSelected() && w.isMention())
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

        result += QChar::LineFeed;

        return result;
    }

    Data::FStringView TextDrawingBlock::selectedTextView() const
    {
        Data::FStringView result;
        int offsetMultiLineLinkWithEmoji = 0;
        for (const auto& line : lines_)
        {
            for (const auto& w : line)
            {
                auto view = w.view();
                const auto lengthView = view.toString().length();
                const auto isMultiLineLinkWithEmoji = [&lengthView, &w]() -> bool
                {
                    return lengthView > w.getText().length() && w.isLink();
                };
                if (!isMultiLineLinkWithEmoji())
                    offsetMultiLineLinkWithEmoji = 0;
                if (!w.isSelected() || view.isEmpty())
                {
                    if (isMultiLineLinkWithEmoji())
                        offsetMultiLineLinkWithEmoji += w.getText().length();
                    continue;
                }
                int selectFromView = w.selectedFrom();
                int sizeView = w.selectedTo() - w.selectedFrom();
                if (isMultiLineLinkWithEmoji())
                {
                    selectFromView += offsetMultiLineLinkWithEmoji;
                    if (w.selectedTo() >= w.getText().length())
                        sizeView = w.getText().length() - w.selectedFrom();
                }

                if (!((w.isEmoji() && w.isSelected()) || (w.isFullSelected() && w.isMention())))
                    view = view.mid(selectFromView, sizeView);

                result.tryToAppend(view);

                if (isMultiLineLinkWithEmoji())
                    offsetMultiLineLinkWithEmoji += w.getText().length();
            }
        }

        result.tryToAppend(QChar::LineFeed);

        return result;
    }

    Data::FString TextDrawingBlock::textForInstantEdit() const
    {
        Data::FString::Builder result;
        for (const auto& line : lines_)
        {
            for (const auto& w : line)
            {
                if (auto view = w.view(); view.isEmpty())
                {
                    auto text = w.getText(TextType::SOURCE);
                    if (w.isSpaceAfter())
                        text.append(QChar::Space);
                    result %= std::move(text);
                }
                else
                {
                    if (w.isSpaceAfter())
                        view.tryToAppend(QChar::Space);
                    result %= view;
                }
            }
        }

        if (!result.isEmpty())
            result %= qsl("\n");

        return result.finalize();
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

    Data::FString TextDrawingBlock::getSourceText() const
    {
        Data::FStringView view;
        for (const auto& w : words_)
        {
            if (!view.tryToAppend(w.view()))
                im_assert(false);
        }
        view.tryToAppend(QChar::Space);

        auto result = view.toFString();
        if (!result.isEmpty())
            result += QChar::LineFeed;
        return result;
    }

    void TextDrawingBlock::clicked(QPoint _p) const
    {
        if (const auto w = getWordAt(_p, WithBoundary::No); w)
            w->word.clicked();
    }

    bool TextDrawingBlock::doubleClicked(QPoint _p, bool _fixSelection)
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
                        if (w.isLink() && w.getShowLinks() == Ui::TextRendering::LinksVisible::SHOW_LINKS)
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

    bool TextDrawingBlock::isOverLink(QPoint _p) const
    {
        const auto rects = getLinkRects();
        return std::any_of(rects.cbegin(), rects.cend(), [_p](auto r) { return r.contains(_p); });
    }

    Data::LinkInfo TextDrawingBlock::getLink(QPoint _p) const
    {
        int i = 0;
        for (auto r : getLinkRects())
        {
            if (r.contains(_p))
            {
                if (const auto it = links_.find(i); it != links_.end())
                {
                    if (Utils::isMentionLink(it->second.url_))
                    {
                        QString displayName;
                        for (const auto& link : links_)
                        {
                            if (link.second.url_ == it->second.url_)
                                displayName += link.second.displayName_;
                        }
                        return {it->second.url_, displayName.replace(qsl("\n"), qsl(" ")), r};
                    }
                    return {it->second.url_, it->second.displayName_, r};
                }
                return {};
            }

            ++i;
        }

        return {};
    }

    template <typename T>
    void TextDrawingBlock::applyMentionsImpl(const Data::MentionMap& _mentions, std::function<T(const TextWord&)> _func)
    {
        if (_mentions.empty())
            return;

        for (const auto& m : _mentions)
        {
            const auto mention = QString(u"@[" % m.first % u']');
            auto iter = words_.begin();
            while (iter != words_.end())
            {
                const T text = _func(*iter);
                const auto mentionIdx = text.indexOf(mention);
                if (mentionIdx == -1 || text == mention)
                {
                    ++iter;
                    continue;
                }

                const auto space = iter->getSpace();
                const auto type = iter->getType();
                const auto links = iter->getShowLinks();

                auto func = [space, type, &iter, links, this](auto _left, auto _right)
                {
                    auto w = TextWord(std::move(_left), Space::WITHOUT_SPACE_AFTER, type, links);
                    w.applyFontParameters(*iter);
                    *iter = w;
                    ++iter;

                    auto newWord = TextWord(std::move(_right), space, type, links);
                    newWord.applyFontParameters(w);

                    std::vector<TextWord> linkParts;
                    if (newWord.getType() == WordType::LINK)
                    {
                        parseWordLink(newWord, linkParts);
                        std::for_each(linkParts.begin(), linkParts.end(), [&newWord](TextWord& w)
                        {
                            w.applyFontParameters(newWord);
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

                    func(text.left(mentionIdx), text.mid(mentionIdx, mention.size()));
                    continue;
                }

                func(text.left(mention.size()), text.mid(mention.size()));
            }
        }

        std::vector<TextWord> mentionWords;
        for (const auto& m : _mentions)
        {
            const auto friendlyText = Data::FString(m.second);
            parseForWords(friendlyText, LinksVisible::SHOW_LINKS, mentionWords, WordType::MENTION);
            for (auto& w : mentionWords)
            {
                w.setSubstitution(w.viewNoEndSpace());
                w.clearView();
            }

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
    void TextDrawingBlock::applyMentionsForView(const Data::MentionMap& _mentions)
    {
        applyMentionsImpl<Data::FStringView>(_mentions, &TextWord::getVisibleTextView);
    }

    void TextDrawingBlock::applyMentions(const Data::MentionMap& _mentions)
    {
        applyMentionsImpl<Data::FStringView>(_mentions, &TextWord::getVisibleTextView);
    }

    int TextDrawingBlock::desiredWidth() const
    {
        return ceilToInt(desiredWidth_);
    }

    void TextDrawingBlock::elide(int _width, bool _prevElided)
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

        words_ = elideWords(originalWords_, _width, ceilToInt(originalDesiredWidth_), false);

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

        for (auto& line : lines_)
        {
            for (auto& w : line)
            {
                if (auto cmd = w.disableCommand(); !cmd.isEmpty())
                {
                    auto it = std::find_if(links_.begin(), links_.end(), [&cmd](const auto& x) { return x.second.url_ == cmd; });
                    if (it != links_.end())
                    {
                        auto rect_idx = it->first;
                        if (rect_idx >= 0 && rect_idx < int(linkRects_.size()))
                            linkRects_[rect_idx] = {};
                        links_.erase(it);
                    }
                }
            }
        }
        if (links_.empty())
            linkRects_.clear();
    }

    std::optional<TextWordWithBoundary> TextDrawingBlock::getWordAt(QPoint _p, WithBoundary _mode) const
    {
        if (auto res = const_cast<TextDrawingBlock*>(this)->getWordAtImpl(_p, _mode); res)
            return TextWordWithBoundary{ res->word.get(), res->syntaxWord };
        return {};
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

    void TextDrawingBlock::updateWordsView(std::function<Data::FStringView(Data::FStringView)> _updater)
    {
        for (auto& word : words_)
            word.updateView(_updater);

        for (auto& line : lines_)
        {
            for (auto& word : line)
                word.updateView(_updater);
        }

        for (auto& word : originalWords_)
            word.updateView(_updater);
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

        for (auto it = words_.begin(); it != words_.end(); ++it)
        {
            auto& w = *it;
            desiredWidth_ += w.width(WidthMode::FORCE, emojiCount, (it == words_.end() - 1));
            if (w.isSpaceAfter())
                desiredWidth_ += w.spaceWidth();
        }
    }

    void TextDrawingBlock::parseForWords(Data::FStringView _text, LinksVisible _showLinks, std::vector<TextWord>& _words, WordType _type)
    {
        const QStringView plainView = _text.string();

        const auto getLinkRanges = [_type](Data::FStringView _text) -> std::vector<core::data::range>
        {
            const auto styles = _text.getStyles();
            std::vector<core::data::range> result;
            result.reserve(styles.size());
            for (const auto& s : styles)
            {
                if (s.type_ == ftype::link)
                    result.emplace_back(s.range_);
            }

            if (_type == WordType::MENTION)
                return result;

            std::u16string_view sv((const char16_t*)_text.string().utf16(), (size_t)_text.string().size());
            const QString profile = Features::getProfileDomain();
            config::get_mutable().set_profile_link(profile.toStdWString());
            common::tools::message_tokenizer tokenizer(sv, styles);
            for (; tokenizer.has_token(); tokenizer.next())
            {
                const auto& token = tokenizer.current();
                if (token.type_ == common::tools::message_token::type::url)
                {
                    const auto& url = boost::get<common::tools::url_view>(token.data_);
                    const size_t length = url.text_.size();
                    const size_t first = token.formatted_source_offset_;
                    const size_t last = first + length;
                    if (first > 1 && last != sv.size())
                    {
                        if (sv.substr(first - 2, 2) == u"@[" && sv[last] == u']')
                            continue; // mention/nick - discard and continue
                    }
                    result.emplace_back(first, length);
                }
            }
            std::sort(result.begin(), result.end());

            im_assert(std::is_sorted(result.cbegin(), result.cend()));
            return result;
        };

        const auto isSpaceOnTheRight = [plainView](auto _i)
        {
            if (_i < plainView.size())
                return plainView.at(_i).isSpace();
            return false;
        };

        const auto toSpaceParam = [](bool _needSpace)
        {
            return _needSpace ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER;
        };

        const auto addSpace = [&_words, _text, _type, _showLinks](auto _begin)
        {
            emplace_word_back(_words, _text.mid(_begin, 1), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
        };

        const auto addRegularWord = [_text, &_words, this, _type, _showLinks, &addSpace](auto _begin, auto _end, bool _needSpace)
        {
            im_assert(_end > _begin);
            im_assert(_begin >= 0 && _end <= _text.size());

            auto noTralingSpaceView = _text.mid(_begin, _end - _begin);
            const auto isMention = Utils::isMentionLink(noTralingSpaceView.string());
            const auto endWithSpace = _end + static_cast<int>(_needSpace) - _begin;
            auto wordView = _text.mid(_begin, endWithSpace);
            const auto spaceView = _needSpace ? wordView.right(1) : Data::FStringView();
            const auto needSeparateSpace = isMention
                || spaceView.isAnyOf({ ftype::underline, ftype::strikethrough });
            const auto spaceParam = (_needSpace && !needSeparateSpace) ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER;
            const auto showLinks = wordView.isAnyOf({ ftype::pre }) ? LinksVisible::DONT_SHOW_LINKS : _showLinks;
            auto word = TextWord(needSeparateSpace ? noTralingSpaceView : wordView, spaceParam, _type, showLinks, EmojiSizeType::ALLOW_BIG);
            if (!parseWordNick(word, _words) && !parseWordHashtag(word, _words) && (word.getType() != WordType::LINK || !parseWordLink(word, _words)))
            {
                if (word.getType() == WordType::COMMAND)
                    correctCommandWord(std::move(word), _words);
                else
                    emplace_word_back(_words, std::move(word));
            }
            if (_needSpace && needSeparateSpace)
                addSpace(_end);
        };

        const auto addEmoji = [_text, &_words, &toSpaceParam](Emoji::EmojiCode&& _emoji, auto _begin, auto _end, bool _needSpace)
        {
            emplace_word_back(_words, _text.mid(_begin, _end + static_cast<int>(_needSpace) - _begin), std::move(_emoji), toSpaceParam(_needSpace));
        };

        const auto addFormattedLink = [_text, &_words, _showLinks, &addEmoji, &toSpaceParam](auto _begin, auto _end, bool _needSpace)
        {
            auto iAfterEmoji = _begin;
            if (auto emoji = Emoji::getEmoji(_text.string(), iAfterEmoji); !emoji.isNull() && iAfterEmoji == _end)
                addEmoji(std::move(emoji), _begin, iAfterEmoji, _needSpace);
            else
                emplace_word_back(_words, _text.mid(_begin, _end + static_cast<int>(_needSpace) - _begin), toSpaceParam(_needSpace), WordType::LINK, _showLinks);
        };

        _words.reserve(_text.string().count(QChar::Space));

        const auto links = LinksVisible::SHOW_LINKS == _showLinks ? getLinkRanges(_text) : std::vector<core::data::range>{};
        auto linksIt = links.cbegin();
        auto wordStart = qsizetype(0);
        auto i = qsizetype(0);
        auto iAfterEmoji = i;
        while (i < _text.size())
        {
            if (linksIt != links.cend() && i == linksIt->offset_)
            {
                if (i > wordStart)
                    addRegularWord(wordStart, i, false);
                wordStart = i;
                i += linksIt->size_;
                const auto needSpace = isSpaceOnTheRight(i);
                addFormattedLink(wordStart, i, needSpace);
                i += static_cast<decltype(i)>(needSpace);
                wordStart = i;
                ++linksIt;
            }
            else if (isBrailleSpace(plainView.at(i)))
            {
                if (i > wordStart)
                    addRegularWord(wordStart, i, false);
                addSpace(i);
                ++i;
                wordStart = i;
            }
            else if (plainView.at(i).isSpace())
            {
                if (i > wordStart)
                    addRegularWord(wordStart, i, true);
                else
                    addSpace(i);
                ++i;
                wordStart = i;
            }
            else if (auto emoji = Emoji::getEmoji(_text.string(), iAfterEmoji = i); !emoji.isNull())
            {
                if (i > wordStart)
                    addRegularWord(wordStart, i, false);
                const auto needSpace = isSpaceOnTheRight(iAfterEmoji);
                addEmoji(std::move(emoji), i, iAfterEmoji, needSpace);
                wordStart = iAfterEmoji + static_cast<bool>(needSpace);
                i = wordStart;
            }
            else
            {
                ++i;
            }
        }
        if (wordStart < _text.size())
            addRegularWord(wordStart, i, false);
    }

    bool TextDrawingBlock::parseWordLink(const TextWord& _wordWithLink, std::vector<TextWord>& _words)
    {
        const auto text = _wordWithLink.getText();
        const auto url = _wordWithLink.getOriginalUrl();

        const auto idx = text.indexOf(url);
        if (idx == -1)
            return false;

        const auto showLinks = _wordWithLink.getShowLinks();
        const auto wordSpace = _wordWithLink.getSpace();

        if (const auto& view = _wordWithLink.view(); !view.isEmpty())
        {
            const auto beforeUrlView = view.mid(0, idx);
            const auto urlView = view.mid(idx, url.size());
            const auto afterUrlView = view.mid(idx + url.size());

            if (!beforeUrlView.isEmpty())
                _words.emplace_back(beforeUrlView, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

            _words.emplace_back(urlView, afterUrlView.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);
            _words.back().setOriginalUrl(url);

            if (!afterUrlView.isEmpty())
                _words.emplace_back(afterUrlView, wordSpace, WordType::TEXT, showLinks);
        }

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
            if (!prevChar.isLetterOrNumber() && prevChar.isSpace())
                return index;
            ++index;
        }

    }

    bool TextDrawingBlock::parseWordNickImpl(const TextWord& _word, std::vector<TextWord>& _words, Data::FStringView _text)
    {
        const auto showLinks = _word.getShowLinks();
        const auto wordSpace = _word.getSpace();

        if (const auto index = getNickStartIndex(_text.string()); index >= 0)
        {
            const auto before = _text.left(index);
            auto nick = _text.mid(index, _text.size() - before.size());
            while (!nick.isEmpty() && !Utils::isNick(nick.string().toString()))
                nick = _text.mid(index, nick.size() - 1);

            if (!nick.isEmpty())
            {
                const auto after = _text.mid(index + nick.size(), _text.size() - index - nick.size());
                if (after.string().startsWith(u'@'))
                    return false;

                if (!before.isEmpty())
                    _words.emplace_back(before, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

                _words.emplace_back(nick, after.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, showLinks == LinksVisible::SHOW_LINKS ? WordType::LINK : WordType::TEXT, showLinks);
                if (!after.isEmpty() && !parseWordNickImpl(_word, _words, after))
                    _words.emplace_back(after, wordSpace, WordType::TEXT, showLinks);

                return true;
            }
        }
        return false;
    }

    bool TextDrawingBlock::parseWordNick(const TextWord& _word, std::vector<TextWord>& _words)
    {
        if (auto view = _word.view(); !view.isEmpty())
            return parseWordNickImpl(_word, _words, view);
        return false;
    }

    static int getHashtagStartIndex(QStringView _text)
    {
        int index = 0;
        static const auto tag = u'#';
        for(;;)
        {
            index = _text.indexOf(tag, index);

            if (index == -1)
                return index;

            QChar nextChar(-1);
            if (index + 1 < _text.size())
                nextChar = _text.at(index + 1);

            if (index == 0)
            {
                if ((nextChar.isLetterOrNumber() || nextChar == u'_'))
                {
                    return index;
                }
                else
                {
                    if (nextChar == u'@')
                    {
                        const auto startMention = _text.indexOf(u"@[", index);
                        const auto endMention = _text.indexOf(u']', startMention);
                        if (startMention != -1 && endMention != -1)
                        {
                            if (Utils::isHashtag(_text.mid(index + startMention + 2, endMention - startMention - index - 2)))
                                return index + startMention + 2;
                        }
                    }
                }
            }

            if (index + 2 >= _text.size())
                return index;

            if (index == 0)
            {
                ++index;
                nextChar = _text.at(index + 1);
            }

            if (const auto prevChar = _text.at(index - 1); !prevChar.isLetterOrNumber())
            {
                if ((prevChar.isSpace() || prevChar.isPunct())
                && (index + 1 < _text.size() && !nextChar.isPunct()))
                {
                    return index;
                }

                while(index + 1 < _text.size() && _text.at(index).isPunct())
                    ++index;
                return --index;
            }
            ++index;
        }

    }

    bool TextDrawingBlock::parseWordHashtagImpl(const TextWord& _word, std::vector<TextWord>& _words, Data::FStringView _text)
    {
        const auto showLinks = _word.getShowLinks();
        const auto wordSpace = _word.getSpace();

        if (const auto index = getHashtagStartIndex(_text.string()); index >= 0)
        {
            auto before = _text.left(index);
            auto hashtag = _text.mid(index, _text.size() - before.size());
            while (!hashtag.isEmpty() && !Utils::isHashtag(hashtag.string().toString()))
                hashtag = _text.mid(index, hashtag.size() - 1);

            if (!hashtag.isEmpty())
            {
                auto after = _text.mid(index + hashtag.size(), _text.size() - index - hashtag.size());
                const auto isTagAfter = after.string().startsWith(u'#');
                if (isTagAfter && !(after.size() > 1 && after.at(1).isPunct() || after.string().contains(u"##")))
                    return false;

                if (!before.isEmpty())
                    _words.emplace_back(before, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

                const auto isText = isTagAfter || before.string().endsWith(u'_') || _word.getStyles().testFlag(core::data::format_type::pre);
                const auto isSpaceAfter = after.string().startsWith(QChar::Space);
                if (isSpaceAfter && !after.isEmpty())
                {
                    hashtag.tryToAppend(after.front());
                    after = after.mid(1);
                }
                _words.emplace_back(hashtag, after.isEmpty() || isSpaceAfter ? wordSpace : Space::WITHOUT_SPACE_AFTER, isText ? WordType::TEXT : WordType::HASHTAG, showLinks);
                if (!isText)
                    _words.back().setOriginalLink(hashtag.trimmed().toString());

                if (!after.isEmpty() && !parseWordHashtagImpl(_word, _words, after))
                    _words.emplace_back(after, wordSpace, WordType::TEXT, showLinks);

                return true;
            }
            else if (const auto tail = _text.mid(index + 1); tail.size() > 1 && tail.string().contains(u'#'))
            {
                before = _text.left(index + 1);
                
                const auto lastChar = before.lastChar();

                if (!before.isEmpty() && lastChar != u'#' && !lastChar.isPunct() && !lastChar.isSymbol())
                    _words.emplace_back(before, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);
                return parseWordHashtagImpl(_word, _words, tail);
            }
        }
        return false;
    }

    bool TextDrawingBlock::parseWordHashtag(const TextWord& _word, std::vector<TextWord>& _words)
    {
        if (auto view = _word.view(); !view.isEmpty())
            return parseWordHashtagImpl(_word, _words, view);
        return false;
    }

    void TextDrawingBlock::correctCommandWord(TextWord _word, std::vector<TextWord>& _words)
    {
        const auto fullText = _word.getVisibleTextView();
        auto cmdText = _word.getLink().url_;
        if (fullText.size() == cmdText.size())
        {
            _words.emplace_back(std::move(_word));
        }
        else
        {
            im_assert(fullText.size() > cmdText.size());
            auto tailText = fullText.right(fullText.size() - cmdText.size());
            _words.emplace_back(fullText.left(cmdText.size()), Space::WITHOUT_SPACE_AFTER, WordType::COMMAND, _word.getShowLinks());
            _words.emplace_back(std::move(tailText), _word.getSpace(), WordType::TEXT, _word.getShowLinks());
        }
    }

    std::vector<TextWord> TextDrawingBlock::elideWords(const std::vector<TextWord>& _original, int _width, int _desiredWidth, bool _forceElide)
    {
        if (_original.empty())
            return _original;

        static const auto dots = getEllipsis();
        auto f = _original.front().getFont();
        const auto dotsWidth = textWidth(f, dots);

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

            auto partLeft = w.getVisibleTextView();
            const auto wordTextSize = partLeft.size();

            auto left = 0, right = partLeft.size();
            const auto chWidth = averageCharWidth(f);
            while (left != right)
            {
                const auto mid = left + (right - left) / 2;
                if (mid == left || mid == right)
                    break;

                auto t = partLeft.left(mid);
                const auto width = textWidth(f, t.toString());
                const auto cmp = _width - curWidth - width;
                const auto cmpCh = chWidth - abs(cmp);

                if (cmpCh >= 0 && cmpCh <= chWidth)
                {
                    partLeft = std::move(t);
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

            while (!partLeft.isEmpty() && textWidth(f, partLeft.toString()) > (_width - curWidth))
                partLeft.chop(1);

            if (partLeft.size() == wordTextSize)
            {
                result.push_back(w);
                curWidth += wordWidth;
                continue;
            }

            if (!partLeft.isEmpty())
            {
                const auto wordSize = partLeft.size();
                auto newWordView = w.isSubstitutionUsed() ? w.view() : partLeft;
                auto word = TextWord(newWordView, Space::WITHOUT_SPACE_AFTER, w.getType(), (w.isLinkDisabled() || w.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : w.getShowLinks()); //todo: implement method makeFrom()
                if (w.isSubstitutionUsed())
                    word.setSubstitution(partLeft);
                word.applyFontParameters(w);
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
            auto w = TextWord({}, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, LinksVisible::DONT_SHOW_LINKS);
            w.setSubstitution(dots);

            if (!result.empty())
            {
                w.applyFontParameters(result.back());
                w.setHighlighted(result.back().isRightHighlighted());
            }
            else if (!words_.empty())
            {
                w.applyFontParameters(words_.back());
                w.setHighlighted(words_.back().isRightHighlighted());
            }
            w.width();
            result.push_back(std::move(w));
        }

        return result;
    }

    void TextDrawingBlock::setMaxLinesCount(size_t _count)
    {
        maxLinesCount_ = _count;
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

    size_t TextDrawingBlock::getLinesCount() const
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
                bool chopped = false;
                if (textRef.size() > 1 && (textRef.endsWith(u',') || textRef.endsWith(u'.')))
                {
                    textRef.chop(1);
                    chopped = true;
                }

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

                if (found && chopped && _key.size() > link.size())
                {
                    const auto c = _key.at(link.size());
                    if (c == u'.' || c == u',')
                        link += c;
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

    void Ui::TextRendering::TextDrawingBlock::setHighlightColor(const QColor& _color)
    {
        highlightColor_ = _color;
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

    double TextDrawingBlock::getFirstLineHeight() const
    {
        if (lines_.empty())
            return 0.0;

        return std::accumulate(lines_.front().cbegin(), lines_.front().cend(), 0,
            [](auto _val, auto _w) { return std::max(_val, textHeight(_w.getFont())); }
        );
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

    NewLineBlock::NewLineBlock(Data::FStringView _view)
        : BaseDrawingBlock(BlockType::NewLine)
        , cachedHeight_(0)
        , view_(_view)
        , hasHeight_(true)
    {
    }

    void NewLineBlock::init(const QFont& f, const QFont&, const QColor&, const QColor&, const QColor&, const QColor&, HorAligment, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
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
        cachedHeight_ = hasHeight_ ? textHeight(font_) + lineSpacing : 0;
        return cachedHeight_;
    }

    int NewLineBlock::getCachedHeight() const
    {
        return cachedHeight_;
    }

    void NewLineBlock::select(QPoint, QPoint)
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

    QString NewLineBlock::selectedPlainText(TextType) const
    {
        return getText();
    }

    Data::FStringView NewLineBlock::selectedTextView() const
    {
        return view_;
    }

    QString NewLineBlock::getText() const
    {
        const static auto lineFeed = QString(QChar::LineFeed);
        return lineFeed;
    }

    Data::FString NewLineBlock::getSourceText() const
    {
        return getText();
    }

    Data::FString NewLineBlock::textForInstantEdit() const
    {
        return getText();
    }

    void NewLineBlock::clicked(QPoint) const
    {

    }

    bool NewLineBlock::doubleClicked(QPoint, bool)
    {
        return false;
    }

    bool NewLineBlock::isOverLink(QPoint) const
    {
        return false;
    }

    Data::LinkInfo NewLineBlock::getLink(QPoint) const
    {
        return {};
    }

    void NewLineBlock::applyMentions(const Data::MentionMap&)
    {

    }

    void NewLineBlock::applyMentionsForView(const Data::MentionMap&)
    {

    }

    int NewLineBlock::desiredWidth() const
    {
        return 0;
    }

    void NewLineBlock::elide(int, bool)
    {
    }

    void NewLineBlock::setMaxLinesCount(size_t _count)
    {
        hasHeight_ = _count > 0;
    }

    void NewLineBlock::setMaxLinesCount(size_t _count, LineBreakType)
    {
        hasHeight_ = _count > 0;
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

    std::vector<BaseDrawingBlockPtr> parseForBlocks(Data::FStringView _text, const Data::MentionMap& _mentions, LinksVisible _showLinks)
    {
        std::vector<BaseDrawingBlockPtr> blocks;
        if (_text.isEmpty())
            return blocks;

        std::unique_ptr<Data::FString> textAsSingleLine;
        auto view = Data::FStringView(_text);

        auto i = view.indexOf(QChar::LineFeed);
        auto skipNewLine = true;
        while (i != -1 && !view.isEmpty())
        {
            if (const auto t = view.mid(0, i); !t.isEmpty())
                blocks.push_back(std::make_unique<TextDrawingBlock>(t, _showLinks));

            if (!skipNewLine)
            {
                const auto lineFeedView = view.mid(i, 1);
                im_assert(lineFeedView.string() == u"\n");
                blocks.push_back(std::make_unique<NewLineBlock>(lineFeedView));
            }

            view = view.mid(i + 1, view.size() - i - 1);
            i = view.indexOf(QChar::LineFeed);
            skipNewLine = (i != 0);
        }

        if (!view.isEmpty())
            blocks.push_back(std::make_unique<TextDrawingBlock>(view.mid(0, view.size()), _showLinks));

        if (!_mentions.empty())
        {
            for (auto& b : blocks)
                b->applyMentionsForView(_mentions);
        }

        return blocks;
    }

    std::vector<BaseDrawingBlockPtr> parseForParagraphs(const Data::FString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks)
    {
        const auto extractBlocksSortedAndFillGaps = [text_size = _text.size()](const std::vector<core::data::range_format>& _blocks)
        {
            const auto block_cmp = [](const core::data::range_format& _b1, const core::data::range_format& _b2)
            {
                return _b1.range_.offset_ < _b2.range_.offset_;
            };

            auto newBlocks = std::vector<core::data::range_format>();
            std::copy_if(_blocks.cbegin(), _blocks.cend(), std::back_inserter(newBlocks),
                [](const auto& _v) { return !core::data::is_style(_v.type_); });

            std::sort(newBlocks.begin(), newBlocks.end(), block_cmp);

            auto result = std::vector<core::data::range_format>();
            auto offset = 0;
            auto length = text_size;
            for (const auto& b : newBlocks)
            {
                im_assert(b.type_ != core::data::format_type::none);
                length = std::max(0, b.range_.offset_ - offset);
                if (length > 0 && b.range_.offset_ != offset)
                {
                    result.emplace_back(core::data::format_type::none, core::data::range{ offset, length });
                    im_assert(offset >= 0);
                }
                offset = b.range_.offset_ + b.range_.size_;
                length = std::max(text_size - offset, 0);
                im_assert(offset + length <= text_size);
            }

            if (length > 0)
                result.emplace_back(core::data::format_type::none, core::data::range{ offset, length });

            std::copy(newBlocks.cbegin(), newBlocks.cend(), std::back_inserter(result));
            std::sort(result.begin(), result.end(), block_cmp);
            return result;
        };

        const auto sortedBlocks = extractBlocksSortedAndFillGaps(_text.formatting().formats());
        auto view = Data::FStringView(_text);
        std::vector<BaseDrawingBlockPtr> paragraphs;
        auto wasPreviousBlockAParagraph = false;
        for (auto it = sortedBlocks.cbegin(); it != sortedBlocks.cend(); ++it)
        {
            const auto& block = *it;
            const auto paragraphType = toParagraphType(block.type_);
            const auto isBlockAParagraph = paragraphType != ParagraphType::Regular;
            const auto needsTopMargin = (it != sortedBlocks.cbegin() && isBlockAParagraph)
                || (wasPreviousBlockAParagraph && !isBlockAParagraph)
                || (it == sortedBlocks.cbegin() && (paragraphType == ParagraphType::Pre || paragraphType == ParagraphType::Quote));

            const auto paragraphText = view.mid(block.range_.offset_, block.range_.size_);

            switch (paragraphType)
            {
            case ParagraphType::UnorderedList:
            case ParagraphType::OrderedList:
            case ParagraphType::Quote:
                paragraphs.emplace_back(std::make_unique<TextDrawingParagraph>(
                    parseForBlocks(paragraphText, _mentions, _showLinks), paragraphType, needsTopMargin, block.range_.start_index_));
                break;
            case ParagraphType::Pre:
                paragraphs.emplace_back(std::make_unique<TextDrawingParagraph>(
                    parseForBlocks(paragraphText, _mentions, LinksVisible::DONT_SHOW_LINKS), paragraphType, needsTopMargin));
                break;
            case ParagraphType::Regular:
            {
                auto regularBlocks = parseForBlocks(paragraphText, _mentions, _showLinks);
                if (needsTopMargin && !regularBlocks.empty())
                {
                    if (regularBlocks.front()->getType() == BlockType::Text)
                        regularBlocks.front()->setMargins({ 0, getParagraphVMargin(), 0, 0 });
                }
                for (auto& b : regularBlocks)
                    paragraphs.emplace_back(std::move(b));
                break;
            }
            }
            wasPreviousBlockAParagraph = isBlockAParagraph;
        }

        return paragraphs;
    }

    TextDrawingParagraph::TextDrawingParagraph(std::vector<BaseDrawingBlockPtr>&& _blocks, ParagraphType _paragraphType, bool _needsTopMargin, int _startIndex)
        : BaseDrawingBlock(BlockType::Paragraph)
        , blocks_(std::move(_blocks))
        , topIndent_(_needsTopMargin ? getParagraphVMargin() : 0)
        , paragraphType_(_paragraphType)
        , startIndex_(_startIndex)
    {
        switch (paragraphType_)
        {
        case ParagraphType::Pre:
            margins_ = QMargins(getPreHPadding(), getPreVPadding(), getPreHPadding(), getPreVPadding());
            for (auto& b : blocks_)
            {
                for (auto& w : b->getWords())
                {
                    auto styles = w.getStyles();
                    styles.setFlag(core::data::format_type::monospace);
                    w.setStyles(styles);
                }
            }
            break;
        case ParagraphType::Quote:
            margins_ = QMargins(getQuoteTextLeftMargin(), 0, 0, 0);
            break;
        case ParagraphType::OrderedList:
            margins_ = QMargins(getOrderedListLeftMargin(blocks_.size()), 0, 0, 0);
            break;
        case ParagraphType::UnorderedList:
            margins_ = QMargins(getOrderedListLeftMargin(), 0, 0, 0);
            break;
        default:
            break;
        }
    }

    void TextDrawingParagraph::init(const QFont& _font, const QFont& _monospaceFont, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        font_ = _font;
        textColor_ = _color;
        for (auto& block : blocks_)
            block->init(_font, _monospaceFont, _color, _linkColor, _selectionColor, _highlightColor, _align, _emojiSizeType, _linksStyle);
    }

    void TextDrawingParagraph::draw(QPainter& _p, QPoint& _point, VerPosition _pos)
    {
        _point.ry() += topIndent_;
        const auto textTopLeft = _point + QPoint{ margins_.left(), + margins_.top() };
        auto textBottomLeft = textTopLeft;
        const auto descent = QFontMetrics(font_).descent();
        for (auto i = 0u; i < blocks_.size(); ++i)
        {
            // Our custom text rendering line height is larger QFont.lineHeight() which is expected
            constexpr auto lineHeightFactor = 0.85;

            auto& block = blocks_.at(i);
            const auto lineHeight = block->getFirstLineHeight();
            const auto baselineLeft = QPoint(_point.x(), qRound(textBottomLeft.y() + lineHeight - descent));
            if (paragraphType_ == ParagraphType::UnorderedList)
                drawUnorderedListBullet(_p, baselineLeft, margins_.left(), lineHeightFactor * lineHeight, textColor_);
            else if (paragraphType_ == ParagraphType::OrderedList)
                drawOrderedListBullet(_p, baselineLeft + QPoint(0, descent + 1), margins_.left(), font_, startIndex_ + i, textColor_);

            block->draw(_p, textBottomLeft, _pos);
        }

        switch (paragraphType_)
        {
        case ParagraphType::Quote:
            drawQuoteBar(_p, QPointF(_point.x(), textTopLeft.y()), textBottomLeft.y() - textTopLeft.y());
            break;
        case ParagraphType::Pre:
        {
            const auto w = getCachedWidth();
            const auto h = textBottomLeft.y() + margins_.bottom() - _point.y();
            drawPreBlockSurroundings(_p, { _point, QSize(w, h) });
            break;
        }
        default:
            break;
        }

        _point.ry() = textBottomLeft.y() + margins_.bottom();
    }

    void TextDrawingParagraph::drawSmart(QPainter& _p, QPoint& _point, int _center)
    {
        im_assert(!"not implemented properly as it is not used for formatted text actually");
        for (auto& block : blocks_)
            block->drawSmart(_p, _point, _center);
    }

    int TextDrawingParagraph::getHeight(int _width, int _lineSpacing, CallType _type, bool _isMultiline)
    {
        if (cachedSize_.width() == _width && _type == CallType::USUAL)
            return cachedSize_.height();

        const auto contentWidth = _width - (margins_.left() + margins_.right());
        const auto height = getTopTotalIndentation() + margins_.bottom()
            + getBlocksHeight(blocks_, contentWidth, _lineSpacing, _type);
        cachedSize_ = { _width, height };
        return height;
    }

    int TextDrawingParagraph::getCachedHeight() const
    {
        return cachedSize_.height();
    }

    int TextRendering::TextDrawingParagraph::getCachedWidth() const
    {
        return cachedSize_.width();
    }

    void TextDrawingParagraph::select(QPoint _from, QPoint _to)
    {
        _from.rx() -= margins_.left();
        _to.rx() -= margins_.left();
        _from.ry() -= getTopTotalIndentation();
        _to.ry() -= getTopTotalIndentation();
        for (auto& block : blocks_)
        {
            const auto height = block->getCachedHeight();
            block->select(_from, _to);
            _from.ry() -= height;
            _to.ry() -= height;
        }
    }

    void TextDrawingParagraph::selectAll()
    {
        for (auto& block : blocks_)
            block->selectAll();
    }

    void TextDrawingParagraph::fixSelection()
    {
        for (auto& block : blocks_)
            block->fixSelection();
    }

    void TextDrawingParagraph::clearSelection()
    {
        for (auto& block : blocks_)
            block->clearSelection();
    }

    void TextDrawingParagraph::releaseSelection()
    {
        for (auto& block : blocks_)
            block->releaseSelection();
    }

    bool TextDrawingParagraph::isSelected() const
    {
        return isBlocksSelected(blocks_);
    }

    bool TextDrawingParagraph::isFullSelected() const
    {
        return isAllBlocksSelected(blocks_);
    }

    QString TextDrawingParagraph::selectedPlainText(TextType _type) const
    {
        return getBlocksSelectedPlainText(blocks_, _type);
    }

    Data::FStringView TextDrawingParagraph::selectedTextView() const
    {
        int startIndex = 1;
        return getBlocksSelectedView(blocks_, startIndex);
    }

    Data::FString TextDrawingParagraph::textForInstantEdit() const
    {
        return getBlocksTextForInstantEdit(blocks_);
    }

    QString TextDrawingParagraph::getText() const
    {
        return getBlocksText(blocks_) % u'\n';
    }

    Data::FString TextDrawingParagraph::getSourceText() const
    {
        return getBlocksSourceText(blocks_);
    }

    void TextDrawingParagraph::clicked(QPoint _p) const
    {
        _p -= {margins_.left(), getTopTotalIndentation()};
        blocksClicked(blocks_, _p);
    }

    bool TextDrawingParagraph::doubleClicked(QPoint _p, bool _fixSelection)
    {
        _p -= {margins_.left(), getTopTotalIndentation()};
        for (auto& b : blocks_)
        {
            if (b->doubleClicked(_p, _fixSelection))
                return true;
            _p.ry() -= b->getCachedHeight();
        }
        return false;
    }

    bool TextDrawingParagraph::isOverLink(QPoint _p) const
    {
        _p -= {margins_.left(), getTopTotalIndentation()};
        return isAnyBlockOverLink(blocks_, _p);
    }

    Data::LinkInfo TextDrawingParagraph::getLink(QPoint _p) const
    {
        _p -= {margins_.left(), getTopTotalIndentation()};
        return getBlocksLink(blocks_, _p);
    }

    void TextDrawingParagraph::applyMentions(const Data::MentionMap& _mentions)
    {
        for (auto& b : blocks_)
            b->applyMentions(_mentions);
    }

    void TextDrawingParagraph::applyMentionsForView(const Data::MentionMap& _mentions)
    {
        for (auto& b : blocks_)
            b->applyMentionsForView(_mentions);
    }

    int TextDrawingParagraph::desiredWidth() const
    {
        auto result = margins_.left() + margins_.right() + getBlocksDesiredWidth(blocks_);
        if (paragraphType_ == ParagraphType::Pre)
            result = std::max(getPreMinWidth(), result);
        return result;
    }

    void TextDrawingParagraph::elide(int _width, bool _prevElided)
    {
        _width -= margins_.left() + margins_.right();
        elideBlocks(blocks_, _width);
    }

    bool TextRendering::TextDrawingParagraph::isElided() const
    {
        return isBlocksElided(blocks_);
    }

    void TextDrawingParagraph::disableCommands()
    {
        for (auto& b : blocks_)
            b->disableCommands();
    }

    void TextDrawingParagraph::setShadow(const int _offsetX, const int _offsetY, const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setShadow(_offsetX, _offsetY, _color);
    }

    void TextDrawingParagraph::updateWordsView(std::function<Data::FStringView(Data::FStringView)> _updater)
    {
        for (auto& block : blocks_)
            block->updateWordsView(_updater);
    }

    int TextDrawingParagraph::getSelectedStartIndex() const
    {
        auto i = 1;
        for (const auto& block : blocks_)
        {
            if (block->isSelected())
                return i + startIndex_ - 1;

            ++i;
        }

        return BaseDrawingBlock::getSelectedStartIndex();
    }

    void TextDrawingParagraph::setMaxLinesCount(size_t _count, LineBreakType _lineBreak)
    {
        for (auto& b : blocks_)
            b->setMaxLinesCount(_count, _lineBreak);
    }

    void TextRendering::TextDrawingParagraph::setMaxLinesCount(size_t _count)
    {
        for (auto& b : blocks_)
            b->setMaxLinesCount(_count);
    }

    void TextDrawingParagraph::setLastLineWidth(int _width)
    {
        if (blocks_.empty())
            return;

        _width -= margins_.left() + margins_.right();
        blocks_.back()->setLastLineWidth(_width);
    }

    size_t TextDrawingParagraph::getLinesCount() const
    {
        size_t result = 0;
        for (const auto& b : blocks_)
            result += b->getLinesCount();
        return result;
    }

    void TextDrawingParagraph::applyLinks(const std::map<QString, QString>& _links)
    {
        for (auto& b : blocks_)
            b->applyLinks(_links);
    }

    void TextDrawingParagraph::applyLinksFont(const QFont& _font)
    {
        for (auto& b : blocks_)
            b->applyLinksFont(_font);
    }

    void TextDrawingParagraph::setColor(const QColor& _color)
    {
        textColor_ = _color;
        for (auto& b : blocks_)
            b->setColor(_color);
    }

    void TextDrawingParagraph::setLinkColor(const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setLinkColor(_color);
    }

    void TextDrawingParagraph::setSelectionColor(const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setSelectionColor(_color);
    }

    void TextDrawingParagraph::setHighlightedTextColor(const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setHighlightedTextColor(_color);
    }

    void Ui::TextRendering::TextDrawingParagraph::setHighlightColor(const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setHighlightColor(_color);
    }

    void TextDrawingParagraph::setColorForAppended(const QColor& _color)
    {
        for (auto& b : blocks_)
            b->setColorForAppended(_color);
    }

    void TextDrawingParagraph::appendWords(const std::vector<TextWord>& _words)
    {
        if (blocks_.empty())
            return;
        blocks_.back()->appendWords(_words);
    }

    const std::vector<TextWord>& TextRendering::TextDrawingParagraph::getWords() const
    {
        // REF-IMPLEMENT
        static const std::vector<TextWord> emptyResult;
        //if (blocks_.size() > 0)
        //{
        //    std::vector<TextWord> words;
        //    for (const auto& block : blocks_)
        //        std::copy(block->getWords().cbegin(), block->getWords().cend(), std::back_inserter(words));
        //    return words;
        //}
        //return emptyResult;
        return emptyResult;
    }

    std::vector<TextWord>& TextRendering::TextDrawingParagraph::getWords()
    {
        // REF-IMPLEMENT
        static std::vector<TextWord> emptyResult;
        if (blocks_.size() > 0)
            return blocks_.front()->getWords();
        return emptyResult;
    }

    void TextDrawingParagraph::setHighlighted(const bool _isHighlighted)
    {
        for (auto& block : blocks_)
            block->setHighlighted(_isHighlighted);
    }

    void TextDrawingParagraph::setHighlighted(const highlightsV& _entries)
    {
        for (auto& block : blocks_)
            block->setHighlighted(_entries);
    }

    void TextDrawingParagraph::setUnderline(const bool _enabled)
    {
        for (auto& block : blocks_)
            block->setUnderline(_enabled);
    }

    double TextDrawingParagraph::getLastLineWidth() const
    {
        if (blocks_.empty())
            return 0.0;
        return blocks_.back()->getLastLineWidth() + margins_.left() + margins_.right();
    }

    double TextRendering::TextDrawingParagraph::getFirstLineHeight() const
    {
        if (blocks_.empty())
            return 0.0;

        return blocks_.front()->getFirstLineHeight();
    }

    double TextDrawingParagraph::getMaxLineWidth() const
    {
        auto result = 0.0;
        for (const auto& b : blocks_)
            result = std::max(result, b->getMaxLineWidth());
        result += static_cast<qreal>(margins_.left()) + margins_.right();
        if (paragraphType_ == ParagraphType::Pre && getCachedWidth() > result)
            result = getCachedWidth();
        return result;
    }

    int getBlocksHeight(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width, int _lineSpacing, CallType _calltype)
    {
        if (_width <= 0)
            return 0;

        int h = 0;
        const bool isMultiline = _blocks.size() > 1;

        constexpr auto max = std::numeric_limits<size_t>::max();

        auto maxLinesCount = _blocks.empty() ? max : _blocks.front()->getMaxLinesCount();
        if (maxLinesCount != max && isMultiline)
            _calltype = CallType::FORCE;

        for (auto& b : _blocks)
        {
            if (maxLinesCount != max)
                b->setMaxLinesCount(maxLinesCount);

            h += b->getHeight(_width, _lineSpacing, _calltype, isMultiline);

            if (maxLinesCount != max)
            {
                auto linesCount = b->getLinesCount();
                maxLinesCount = maxLinesCount > linesCount ? maxLinesCount - linesCount : 0;
            }
        }

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

    void initBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font, const QFont& _monospaceFont, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        for (auto& b : _blocks)
            b->init(_font, _monospaceFont, _color, _linkColor, _selectionColor, _highlightColor, _align, _emojiSizeType, _linksStyle);
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
        for (const auto& b : _blocks)
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

    bool isAnyBlockOverLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        auto p = _p;
        return std::any_of(_blocks.begin(), _blocks.end(), [&p](const auto& b) {
            if (b->isOverLink(p))
                return true;
            p.ry() -= b->getCachedHeight();
            return false;
        });
    }

    Data::LinkInfo getBlocksLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        QPoint p = _p;
        for (auto& b : _blocks)
        {
            if (auto info = b->getLink(p); !info.isEmpty())
            {
                info.rect_.translate(0, _p.y() - p.y());
                return info;
            }

            p.ry() -= b->getCachedHeight();
        }

        return {};
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

    static void trimLineFeeds(Data::FString& _str)
    {
        const auto& s = _str.string();

        auto startIt = s.cbegin();
        while (startIt != s.cend() && *startIt == QChar::LineFeed)
            ++startIt;
        const auto startPos = static_cast<int>(std::distance(s.cbegin(), startIt));

        auto endIt = s.crbegin();
        while ((endIt != s.crend() - startPos) && (*endIt == QChar::LineFeed))
            ++endIt;
        const auto endPos = s.size() - static_cast<int>(std::distance(s.crbegin(), endIt));

        if (auto size = qMax(0, endPos - startPos); size < s.size())
            _str = Data::FStringView(_str, startPos, size).toFString();
    }

    Data::FStringView getBlocksSelectedView(const std::vector<BaseDrawingBlockPtr>& _blocks, int& _startIndex)
    {
        Data::FStringView result;
        for (const auto& b : _blocks)
        {
            const auto text = b->selectedTextView();
            if (text.isEmpty())
                continue;

            auto startIndex = b->getSelectedStartIndex();
            if (startIndex != 1 && startIndex != _startIndex)
                _startIndex = startIndex;

            if (b->getType() != BlockType::NewLine)
            {
                result.tryToAppend(text);
            }
            result.tryToAppend(QChar::LineFeed);
        }
        return result;
    }

    Data::FString getBlocksSelectedText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        int startIndex = 1;
        auto result = getBlocksSelectedView(_blocks, startIndex).toFString();
        result.formatting().add_start_index_to_ordered_list(startIndex);
        trimLineFeeds(result);
        return result;
    }

    QString getBlocksSelectedPlainText(const std::vector<BaseDrawingBlockPtr>& _blocks, TextType _type)
    {
        QString result;

        for (const auto& b : _blocks)
            result += b->selectedPlainText(_type);

        trimLineFeeds(result);
        return result;
    }

    Data::FString getBlocksTextForInstantEdit(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        Data::FString::Builder resultBuilder;
        for (const auto& b : _blocks)
        {
            if (b->getType() == BlockType::DebugText)
                continue;
            resultBuilder %= b->textForInstantEdit();
        }

        auto result = resultBuilder.finalize();
        trimLineFeeds(result);
        return result;
    }

    QString getBlocksText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        QString result;
        for (const auto& b : _blocks)
            result += b->getText();

        trimLineFeeds(result);

        return result;
    }

    Data::FString getBlocksSourceText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        Data::FString result;
        for (const auto& b : _blocks)
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
        for (const auto& b : _blocks)
            result = std::max(result, b->desiredWidth());

        return result;
    }

    void elideBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width)
    {
        bool prevElided = false;
        for (auto& b : _blocks)
        {
            b->elide(_width, prevElided);
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

    void applyBlocksLinks(const std::vector<BaseDrawingBlockPtr>& _blocks, const std::map<QString, QString>& _links)
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

    void setBlocksHighlightColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color)
    {
        for (auto& b : _blocks)
            b->setHighlightColor(_color);
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

    void updateWordsView(std::vector<BaseDrawingBlockPtr>& _blocks, const Data::FString& _newSource, int _offset)
    {
        auto updateWordView = [&_newSource, _offset](Data::FStringView _view) -> Data::FStringView
        {
            if (_view.isEmpty())
                return _view;

            return { _newSource, _offset + _view.sourceRange().offset_, _view.size() };
        };

        for (auto& block : _blocks)
            block->updateWordsView(updateWordView);
    }

    void appendBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from)
    {
        if (_to.empty())
            appendWholeBlocks(_to, _from);
        else
            _to.back()->appendWords(_from.empty() ? std::vector<TextWord>() : _from.front()->getWords());
    }

    void appendWholeBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from)
    {
        std::move(_from.begin(), _from.end(), std::back_inserter(_to));
        _from.clear();
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
}

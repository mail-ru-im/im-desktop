#include "stdafx.h"

#include "TextRenderingUtils.h"
#include "TextRendering.h"
#include "TextWordRenderer.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/UrlParser.h"
#include "utils/InterConnector.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"


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

    int getQuoteBarWidth() noexcept { return Utils::scale_value(4); }

    int getQuoteTextLeftMargin() noexcept { return Utils::scale_value(16); }

    int getPreHPadding() noexcept { return Utils::scale_value(12); }

    int getPreVPadding() noexcept { return Utils::scale_value(4); }

    int getPreBorderRadius() noexcept { return Utils::scale_value(4); }

    int getPreBorderThickness() noexcept { return Utils::scale_value(1); }

    int getPreMinWidth() noexcept { return Utils::scale_value(30); }

    int getParagraphVMargin() noexcept { return Utils::scale_value(6); }

    int getOrderedListLeftMargin(int _numItems = -1) noexcept { return _numItems < 1000 ? Utils::scale_value(32) : Utils::scale_value(40) ; }

    int getUnorderedListLeftMargin() noexcept { return getOrderedListLeftMargin(); }

    int getUnorderedListBulletRadius() { return Utils::scale_value(2); }

    constexpr int getUrlInDialogElideSize() noexcept { return 140; }

    void drawUnorderedListBullet(QPainter& _painter, QPoint _lineTopLeft, qreal _leftMargin, qreal _lineHeight, QColor _color)
    {
        const auto radius = getUnorderedListBulletRadius();
        const auto rect = QRectF(_lineTopLeft.x() + 0.5 * _leftMargin - radius, _lineTopLeft.y() + 0.5 * (_lineHeight - radius) + 0.5,
                                 2.0 * radius, 2.0 * radius);

        const auto painterSaver = Utils::PainterSaver(_painter);
        _painter.setPen(Qt::NoPen);
        _painter.setBrush(_color);
        _painter.drawEllipse(rect);
    }

    void drawOrderedListNumber(QPainter& _painter, QPoint _lineTopLeft, qreal _leftMargin, qreal _lineHeight, const QFont& _font, int _number, QColor _color)
    {
        static const auto textOption = QTextOption(Qt::AlignmentFlag::AlignHCenter| Qt::AlignmentFlag::AlignBottom);
        const auto rect = QRectF(_lineTopLeft, QSizeF(_leftMargin + 1, _lineHeight + 1));

        const auto painterSaver = Utils::PainterSaver(_painter);
        _painter.setPen(_color);
        _painter.setFont(_font);
        _painter.drawText(rect, QString::number(_number) % u'.', textOption);
    }
}

namespace Ui::TextRendering
{
    namespace fmt = core::data::format;

    void drawPreBlockSurroundings(QPainter& _painter, QRectF _rect)
    {
        constexpr const auto backgroundAlpha = 0.08;
        const auto thickness = getPreBorderThickness();
        const auto radius = getPreBorderRadius();

        const auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        auto backgroundColor = borderColor;
        backgroundColor.setAlphaF(backgroundAlpha);

        const auto painterSaver = Utils::PainterSaver(_painter);

        _painter.setBrush(backgroundColor);
        _painter.setPen(Qt::NoPen);
        _painter.drawRoundedRect(_rect, radius, radius);

        _painter.setBrush(Qt::NoBrush);
        _painter.setPen(QPen(QBrush(borderColor), thickness));
        _painter.drawRoundedRect(_rect.adjusted(thickness, thickness, -2*thickness, -thickness), radius, radius);
    }

    void drawQuoteBar(QPainter& _painter, QPointF _topLeft, qreal _height)
    {
        const auto painterSaver = Utils::PainterSaver(_painter);

        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY_INVERSE);
        _painter.setBrush(color);
        _painter.setPen(Qt::NoPen);
        const auto barRect = QRect(_topLeft.x(), _topLeft.y(), getQuoteBarWidth(), _height);
        _painter.drawRect(barRect);
    }

    ParagraphType toParagraphType(fmt::format_type _type)
    {
        im_assert(!fmt::is_style(_type));
        switch (_type)
        {
        case fmt::format_type::ordered_list:
            return ParagraphType::OrderedList;
        case fmt::format_type::unordered_list:
            return ParagraphType::UnorderedList;
        case fmt::format_type::pre:
            return ParagraphType::Pre;
        case fmt::format_type::quote:
            return ParagraphType::Quote;
        default:
            return ParagraphType::Regular;
        }
    };

    fmt::format_type toFormatType(ParagraphType _type)
    {
        switch (_type)
        {
        case ParagraphType::OrderedList:
            return fmt::format_type::ordered_list;
        case ParagraphType::UnorderedList:
            return fmt::format_type::unordered_list;
        case ParagraphType::Pre:
            return fmt::format_type::pre;
        case ParagraphType::Quote:
            return fmt::format_type::quote;
        default:
            return fmt::format_type::none;
        }
    };

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
        im_assert(type_ != WordType::EMOJI);

        checkSetClickable();
    }

    TextWord::TextWord(const Data::FormattedStringView& _text, Space _space, WordType _type, LinksVisible _showLinks, EmojiSizeType _emojiSizeType)
        : TextWord(_text.string().toString(), _space, _type, _showLinks, _emojiSizeType)
    {
        view_ = _text;
        initFormat();
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

    TextWord::TextWord(const Data::FormattedStringView& _view, Emoji::EmojiCode _code, Space _space, EmojiSizeType _emojiSizeType)
        : TextWord(_code, _space, _emojiSizeType)
    {
        view_ = _view;
        initFormat();
    }

    void TextWord::initFormat()
    {
        if (view_.isEmpty())
            return;

        const auto trimmed = view_.trimmed();
        const auto fullRange = fmt::format_range{ 0, trimmed.size() };
        const auto textStyles = trimmed.getStyles();

        const auto applyFormattedLink = [this](const std::string_view _data)
        {
            auto parser = common::tools::url_parser(std::vector<std::string>());
            for (const auto ch : _data)
                parser.process(ch);
            parser.finish();
            if (parser.has_url())
            {
                originalUrl_ = QString::fromStdString(parser.get_url().original_);
                setOriginalLink(originalUrl_);
            }
        };

        FormatStyles styles;
        std::vector<int> cutPositions;
        for (const auto& styleInfo : textStyles)
        {
            const auto type = styleInfo.type_;
            const auto& range = styleInfo.range_;
            if (styleInfo.range_ == fullRange)
            {
                const auto& data = styleInfo.data_;
                styles.setFlag(type);
                im_assert(type != fmt::format_type::link || data);
                if (type == fmt::format_type::link && data)
                    applyFormattedLink(*data);
            }
            else
            {
                im_assert(type != fmt::format_type::link);
                if (type != fmt::format_type::link)
                {
                    if (range.offset_ > 0)
                        cutPositions.push_back(range.offset_);
                    if (const auto pos = range.offset_ + range.length_; pos != fullRange.length_)
                        cutPositions.push_back(pos);
                }
            }
        }
        setStyles(styles);

        if (isLink())
        {   // Calc cut positions for emoji subword which might appear in links
            auto i = qsizetype(0);
            while (i < trimmedText_.size())
            {
                auto iAfterEmoji = i;
                if (const auto emoji = Emoji::getEmoji(trimmedText_, iAfterEmoji); emoji.isNull())
                {
                    i += 1;
                }
                else
                {
                    if (i > 0)
                        cutPositions.push_back(i);
                    if (iAfterEmoji < trimmedText_.size())
                        cutPositions.push_back(iAfterEmoji);
                    i = iAfterEmoji;
                }
            }
        }

        cutPositions.push_back(fullRange.length_);
        std::sort(cutPositions.begin(), cutPositions.end());
        cutPositions.erase(std::unique(cutPositions.begin(), cutPositions.end()), cutPositions.end());
        if (cutPositions.size() > 1)
        {
            auto start = 0;
            for (auto end : cutPositions)
            {
                const auto subView = trimmed.mid(start, end - start);
                const auto spaceAfter = end == fullRange.length_ ? space_ : Space::WITHOUT_SPACE_AFTER;
                if (isLink())
                {
                    auto emojiLength = qsizetype(0);
                    if (auto emoji = Emoji::getEmoji(subView.string(), emojiLength); !emoji.isNull())
                    {
                        subwords_.emplace_back(subView.mid(0, emojiLength), emoji, spaceAfter);
                        start += emojiLength;
                        continue;
                    }
                }
                subwords_.emplace_back(subView, spaceAfter, type_, showLinks_);
                start = end;
            }
        }
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
                if (isInTooltip())
                    emojiSize_ -= Utils::scale_value(1);
        }

        if (subwords_.empty())
        {
            spaceWidth_ = textSpaceWidth(font_);
            if (isEmoji())
                cachedWidth_ = emojiSize_;
            else
                cachedWidth_ = _isLastWord && italic() ? textVisibleWidth(font_, text_) : textWidth(font_, text_);
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
        const auto textSize = view_.isEmpty() ? text_.size() : getText().size();
        isSpaceSelected_ = ((selectedTo_ == textSize || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());

        if (!isMention() && !subwords_.empty())
        {
            auto offset = 0;
            for (auto& w : subwords_)
            {
                w.clearSelection();
                const auto size = w.text_.size();
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
                {
                    Utils::openUrl(QString(u"mailto:" % QString::fromStdString(url.url_)));
                }
                else if (styles_.testFlag(core::data::format::format_type::link))
                {
                    auto elidedUrl = QString::fromStdString(url.url_);
                    if (elidedUrl.size() > getUrlInDialogElideSize())
                        elidedUrl = elidedUrl.mid(0, getUrlInDialogElideSize()) % u"...";
                    const auto confirm = Utils::GetConfirmationWithTwoButtons(
                        QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                        QT_TRANSLATE_NOOP("popup_window", "OK"),
                        QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to open external link %1?").arg(elidedUrl),
                        QString(),
                        nullptr
                    );
                    if (confirm)
                        Utils::openUrl(QString::fromStdString(url.url_));
                }
                else
                {
                    Utils::openUrl(QString::fromStdString(url.url_));
                }
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
        if (subwords_.empty())
            return ceilToInt(spaceWidth_);
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
            s.applyFontParameters(*this);

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

    const QString& TextWord::disableCommand()
    {
        if (type_ == WordType::COMMAND)
        {
            type_ = WordType::TEXT;
            showLinks_ = LinksVisible::DONT_SHOW_LINKS;
            if (underline())
                setUnderline(false);

            return trimmedText_;
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
        setUnderline(_other.underline());
        setItalic(_other.italic());
        setBold(_other.bold());
        setStrikethrough(_other.strikethrough());
        setSpellError(_other.hasSpellError());
    }

    void TextWord::applyCachedStyles(const QFont& _font, const QFont& _monospaceFont)
    {
        const auto& styles = getStyles();

        if (styles.testFlag(core::data::format::format_type::inline_code))
            setFont(_monospaceFont);
        else
            setFont(_font);

        if (styles.testFlag(core::data::format::format_type::bold))
            setBold(true);

        if (styles.testFlag(core::data::format::format_type::italic))
            setItalic(true);

        if (styles.testFlag(core::data::format::format_type::strikethrough))
            setStrikethrough(true);

        if (styles.testFlag(core::data::format::format_type::underline))
            setUnderline(true);

        for (auto& w : subwords_)
            w.applyCachedStyles(_font, _monospaceFont);
    }

    void TextWord::setSubwords(std::vector<TextWord> _subwords)
    {
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
            tw.applyFontParameters(*this);
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
            tw.applyFontParameters(*this);
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

    void TextWord::setBold(bool _enabled)
    {
        if (isEmoji())
            return;

        font_.setBold(_enabled);
        for (auto& w : subwords_)
            w.setBold(_enabled);
    }

    void TextWord::setItalic(bool _enabled)
    {
        if (isEmoji())
            return;

        font_.setItalic(_enabled);
        for (auto& w : subwords_)
            w.setItalic(_enabled);
    }

    void TextWord::setStrikethrough(bool _enabled)
    {
        if (isEmoji())
            return;

        font_.setStrikeOut(_enabled);
        for (auto& w : subwords_)
            w.setStrikethrough(_enabled);
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
                if (Utils::isNick(trimmedText_))
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

    TextDrawingBlock::TextDrawingBlock(const Data::FormattedStringView& _text, LinksVisible _showLinks, BlockType _blockType)
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
            return w.getShowLinks() == LinksVisible::SHOW_LINKS && w.isLink()
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

                    const auto curWordView = curWord.getView();
                    const auto curWordText = curWord.getText();
                    const auto isViewUsed = !curWordView.isEmpty();

                    auto elidedText = decltype(curWordText)();
                    auto elidedView = decltype(curWordView)();

                    if (isViewUsed)
                         elidedView = splitedWords.empty() ? elideText(w.getFont(), curWordView, cmpWidth) : decltype(curWordView)();
                    else
                         elidedText = splitedWords.empty() ? elideText(w.getFont(), curWordText, cmpWidth) : decltype(curWordText)();

                    const auto isElidedEmpty = isViewUsed ? elidedView.isEmpty() : elidedText.isEmpty();
                    const auto elidedSize = isViewUsed ? elidedView.size() : elidedText.size();

                    if ((splitedWords.empty() && isElidedEmpty) || (!splitedWords.empty() && !splitedWords.front().hasSubwords()) || splitedWords.size() == 1)
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

                    auto tw = splitedWords.empty()
                        ? (isViewUsed
                            ? makeWord(elidedView, false)
                            : makeWord(elidedText, false))
                        : splitedWords[0];

                    tw.setSpellError(w.hasSpellError());
                    if (splitedWords.empty())
                    {
                        tw.applyFontParameters(w);
                        tw.setHighlighted(w.highlightedFrom() - wordLen, w.highlightedTo() - wordLen);
                        tw.setWidth(cmpWidth, emojiCount);

                        wordLen += elidedSize;
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

                    curWord = splitedWords.size() <= 1 ?
                        isViewUsed
                            ? makeWord(curWordView.mid(elidedSize), true) : makeWord(curWordText.mid(elidedSize), true)
                            : splitedWords[1];
                    curWord.setSpellError(w.hasSpellError());
                    if (curWord.isLink())
                        curWord.setOriginalLink(w.getText());

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
                    {
                        w.select(w.getPosByX(x1 - diff),
                            (x2 >= curWidth)
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

    Data::FormattedStringView TextDrawingBlock::selectedTextView() const
    {
        Data::FormattedStringView result;
        for (const auto& line : lines_)
        {
            for (const auto& w : line)
            {
                auto view = w.getView();
                im_assert(w.getText().isEmpty() || !view.isEmpty());
                if (!w.isSelected() || view.isEmpty())
                    continue;

                if (!((w.isEmoji() && w.isSelected()) || (w.isFullSelected() && w.isMention())))
                    view = view.mid(w.selectedFrom(), w.selectedTo() - w.selectedFrom());

                if (!result.tryToAppend(view))
                    im_assert(false);
                if (w.isSpaceAfter() && w.isSpaceSelected())
                    result.tryToAppend(QChar::Space);
            }
        }

        result.tryToAppend(QChar::LineFeed);

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

    Data::FormattedString TextDrawingBlock::getSourceText() const
    {
        Data::FormattedString result;
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

    bool TextDrawingBlock::isOverLink(QPoint _p) const
    {
        const auto rects = getLinkRects();
        return std::any_of(rects.cbegin(), rects.cend(), [_p](auto r) { return r.contains(_p); });
    }

    QString TextDrawingBlock::getLink(QPoint _p) const
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

        for (auto& line : lines_)
        {
            for (auto& w : line)
            {
                if (auto cmd = w.disableCommand(); !cmd.isEmpty())
                {
                    auto it = std::find_if(links_.begin(), links_.end(), [&cmd](const auto& x) { return x.second == cmd; });
                    if (it != links_.end())
                    {
                        if (it->first >= 0 && it->first < int(linkRects_.size()))
                            linkRects_.erase(std::next(linkRects_.begin(), it->first));
                        links_.erase(it);
                    }
                }
            }
        }

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

        for (auto it = words_.begin(); it != words_.end(); ++it)
        {
            auto& w = *it;
            desiredWidth_ += w.width(WidthMode::FORCE, emojiCount, (it == words_.end() - 1));
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


    void TextDrawingBlock::parseForWords(Data::FormattedStringView _text, LinksVisible _showLinks, std::vector<TextWord>& _words, WordType _type)
    {
        namespace fmt = core::data::format;
        const auto plainView = _text.string();

        const auto getLinkRanges = [](Data::FormattedStringView _text)
        {
            const auto styles = _text.getStyles();
            auto result = std::vector<fmt::format_range>();
            for (const auto& s : styles)
            {
                if (s.type_ == fmt::format_type::link)
                    result.emplace_back(s.range_);
            }
            im_assert(std::is_sorted(result.cbegin(), result.cend()));
            return result;
        };

        const auto isNextSpace = [plainView](auto _i)
        {
            if (_i + 1 < plainView.size())
                return plainView.at(_i + 1).isSpace();
            return false;
        };

        const auto addRegularWord = [_text, &_words, this, _type, _showLinks](auto _begin, auto _end, auto _isSpaceNeeded)
        {
            im_assert(_end > _begin);
            im_assert(_begin >= 0 && _end <= _text.size());

            const auto spaceParam = _isSpaceNeeded ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER;
            auto word = TextWord(_text.mid(_begin, _end - _begin), spaceParam, _type, _showLinks);
            if (!parseWordNick(word, _words) && (word.getType() != WordType::LINK || !parseWordLink(word, _words)))
            {
                if (word.getType() == WordType::COMMAND)
                    correctCommandWord(std::move(word), _words);
                else
                    _words.emplace_back(std::move(word));
            }
        };

        const auto addEmoji = [_text, &_words](Emoji::EmojiCode&& _emoji, auto _begin, auto _end)
        {
            _words.emplace_back(_text.mid(_begin, _end - _begin), std::move(_emoji), Space::WITHOUT_SPACE_AFTER);
        };

        const auto addFormattedLink = [_text, &_words, _type, _showLinks, &addEmoji](auto _begin, auto _end, auto _isSpaceNeeded)
        {
            const auto spaceParam = _isSpaceNeeded ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER;
            auto iAfterEmoji = _begin;
            if (auto emoji = Emoji::getEmoji(_text.string(), iAfterEmoji); !emoji.isNull() && iAfterEmoji == _end)
                addEmoji(std::move(emoji), _begin, iAfterEmoji);
            else
                _words.emplace_back(_text.mid(_begin, _end - _begin), spaceParam, _type, _showLinks);
        };

        const auto addSpace = [&_words, _text, _type, _showLinks](auto _begin)
        {
            _words.emplace_back(_text.mid(_begin, 1), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
        };

        const auto links = getLinkRanges(_text);
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
                i += linksIt->length_;
                const auto needSpace = isNextSpace(i);
                addFormattedLink(wordStart, i, needSpace);
                i += static_cast<decltype(i)>(needSpace);
                wordStart = i;
                ++linksIt;
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
                addEmoji(std::move(emoji), i, iAfterEmoji);
                wordStart = iAfterEmoji;
                i = iAfterEmoji;
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

        if (const auto& view = _wordWithLink.getView(); !view.isEmpty())
        {
            const auto beforeUrlView = view.mid(0, idx);
            const auto urlView = view.mid(idx, url.size());
            const auto afterUrlView = view.mid(idx + url.size());

            if (!beforeUrlView.isEmpty())
                _words.emplace_back(beforeUrlView, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

            _words.emplace_back(urlView, afterUrlView.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);

            if (!afterUrlView.isEmpty())
                _words.emplace_back(afterUrlView, wordSpace, WordType::TEXT, showLinks);
        }
        else
        {
            const auto beforeUrl = text.leftRef(idx);
            const auto afterUrl = text.midRef(idx + url.size());

            if (!beforeUrl.isEmpty())
                _words.emplace_back(beforeUrl.toString(), Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

            _words.emplace_back(url, afterUrl.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);

            if (!afterUrl.isEmpty())
                _words.emplace_back(afterUrl.toString(), wordSpace, WordType::TEXT, showLinks);
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
            if (!prevChar.isLetterOrNumber())
                return index;
            ++index;
        }

    }

    bool TextDrawingBlock::parseWordNickImpl(const TextWord& _word, std::vector<TextWord>& _words, QStringView _text)
    {
        const auto showLinks = _word.getShowLinks();
        const auto wordSpace = _word.getSpace();

        if (const auto index = getNickStartIndex(_text); index >= 0)
        {
            const auto before = _text.left(index);
            auto nick = _text.mid(index, _text.size() - before.size());
            while (!nick.isEmpty() && !Utils::isNick(nick.toString()))
                nick = _text.mid(index, nick.size() - 1);

            if (!nick.isEmpty())
            {
                const auto after = _text.mid(index + nick.size(), _text.size() - index - nick.size());
                if (after.startsWith(u'@'))
                    return false;

                if (!before.isEmpty())
                    _words.emplace_back(before.toString(), Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

                _words.emplace_back(nick.toString(), after.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, showLinks == LinksVisible::SHOW_LINKS ? WordType::LINK : WordType::TEXT, showLinks);
                if (!after.isEmpty() && !parseWordNickImpl(_word, _words, after))
                    _words.emplace_back(after.toString(), wordSpace, WordType::TEXT, showLinks);

                return true;
            }
        }
        return false;
    }

    bool TextDrawingBlock::parseWordNickImpl(const TextWord& _word, std::vector<TextWord>& _words, Data::FormattedStringView _text)
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
        if (auto view = _word.getView(); !view.isEmpty())
            return parseWordNickImpl(_word, _words, view);
        else if (auto text = _word.getText(); !text.isEmpty())
            return parseWordNickImpl(_word, _words, QStringView(text));

        im_assert(false);
        return false;
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
            im_assert(fullText.size() > cmdText.size());
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
                im_assert(!"Unexpected elide type");
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
            auto w = TextWord(dots, Space::WITHOUT_SPACE_AFTER, WordType::TEXT, LinksVisible::DONT_SHOW_LINKS);

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

    NewLineBlock::NewLineBlock(Data::FormattedStringView _view)
        : BaseDrawingBlock(BlockType::NewLine)
        , cachedHeight_(0)
        , view_(_view)
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
        cachedHeight_ = textHeight(font_) + lineSpacing;
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

    Data::FormattedStringView NewLineBlock::selectedTextView() const
    {
        return view_;
    }

    QString NewLineBlock::getText() const
    {
        const static auto lineFeed = QString(QChar::LineFeed);
        return lineFeed;
    }

    Data::FormattedString NewLineBlock::getSourceText() const
    {
        return getText();
    }

    QString NewLineBlock::textForInstantEdit() const
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

    QString NewLineBlock::getLink(QPoint) const
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

    std::vector<BaseDrawingBlockPtr> parseForBlocks(const Data::FormattedStringView& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds)
    {
        std::vector<BaseDrawingBlockPtr> blocks;
        if (_text.isEmpty())
            return blocks;

        std::unique_ptr<Data::FormattedString> textAsSingleLine;
        auto view = Data::FormattedStringView(_text);
        if (_processLineFeeds == ProcessLineFeeds::REMOVE_LINE_FEEDS)
        {
            im_assert(!"It is not supposed to get here and it makes expensive copy, but just in case");
            textAsSingleLine = std::make_unique<Data::FormattedString>();
            *textAsSingleLine = _text.toFormattedString();
            textAsSingleLine->replace(QChar::LineFeed, QChar::Space);
            view = Data::FormattedStringView(*textAsSingleLine.get());
        }

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
                b->applyMentions(_mentions);
        }

        return blocks;
    }

    std::vector<BaseDrawingBlockPtr> parseForParagraphs(const Data::FormattedString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds)
    {
        if (_processLineFeeds == ProcessLineFeeds::REMOVE_LINE_FEEDS)
            return parseForBlocks(_text.string(), _mentions, _showLinks, _processLineFeeds);

        const auto extractBlocksSortedAndFillGaps = [text_size = _text.size()](const std::vector<fmt::format>& _blocks)
        {
            const auto block_cmp = [](const fmt::format& _b1, const fmt::format& _b2)
            {
                return _b1.range_.offset_ < _b2.range_.offset_;
            };

            auto newBlocks = std::vector<fmt::format>();
            std::copy_if(_blocks.cbegin(), _blocks.cend(), std::back_inserter(newBlocks),
                [](const auto& _v) { return !fmt::is_style(_v.type_); });

            std::sort(newBlocks.begin(), newBlocks.end(), block_cmp);

            auto result = std::vector<fmt::format>();
            auto offset = 0;
            auto length = text_size;
            for (const auto& b : newBlocks)
            {
                im_assert(b.type_ != fmt::format_type::none);
                length = std::max(0, b.range_.offset_ - offset);
                if (length > 0 && b.range_.offset_ != offset)
                {
                    result.emplace_back(fmt::format_type::none, fmt::format_range{ offset, length });
                    im_assert(offset >= 0);
                }
                offset = b.range_.offset_ + b.range_.length_;
                length = std::max(text_size - offset, 0);
                im_assert(offset + length <= text_size);
            }

            if (length > 0)
                result.emplace_back(fmt::format_type::none, fmt::format_range{ offset, length });

            std::copy(newBlocks.cbegin(), newBlocks.cend(), std::back_inserter(result));
            std::sort(result.begin(), result.end(), block_cmp);
            return result;
        };

        const auto sortedBlocks = extractBlocksSortedAndFillGaps(_text.formatting().formats());
        auto view = Data::FormattedStringView(_text);
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

            const auto paragraphText = view.mid(block.range_.offset_, block.range_.length_);

            switch (paragraphType)
            {
            case ParagraphType::UnorderedList:
            case ParagraphType::OrderedList:
            case ParagraphType::Quote:
                paragraphs.emplace_back(std::make_unique<TextDrawingParagraph>(
                    parseForBlocks(paragraphText, _mentions, _showLinks, _processLineFeeds), paragraphType, needsTopMargin));
                break;
            case ParagraphType::Pre:
                paragraphs.emplace_back(std::make_unique<TextDrawingParagraph>(
                    parseForBlocks(paragraphText, _mentions, LinksVisible::DONT_SHOW_LINKS, _processLineFeeds), paragraphType, needsTopMargin));
                break;
            case ParagraphType::Regular:
            {
                auto regularBlocks = parseForBlocks(paragraphText, _mentions, _showLinks, _processLineFeeds);
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

    TextDrawingParagraph::TextDrawingParagraph(std::vector<BaseDrawingBlockPtr>&& _blocks, ParagraphType _paragraphType, bool _needsTopMargin)
        : BaseDrawingBlock(BlockType::Paragraph)
        , blocks_(std::move(_blocks))
        , topIndent_(_needsTopMargin ? getParagraphVMargin() : 0)
        , paragraphType_(_paragraphType)
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
                    styles.setFlag(core::data::format::format_type::inline_code);
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
        for (auto i = 0u; i < blocks_.size(); ++i)
        {
            auto& block = blocks_.at(i);
            const auto lineTopLeft = QPoint(_point.x(), textBottomLeft.y());
            const auto lineHeight = block->getFirstLineHeight();
            if (paragraphType_ == ParagraphType::UnorderedList)
                drawUnorderedListBullet(_p, lineTopLeft, margins_.left(), lineHeight, textColor_);
            else if (paragraphType_ == ParagraphType::OrderedList)
                drawOrderedListNumber(_p, lineTopLeft, margins_.left(), lineHeight, font_, i + 1, textColor_);

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

    int Ui::TextRendering::TextDrawingParagraph::getCachedWidth() const
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

    Data::FormattedStringView TextDrawingParagraph::selectedTextView() const
    {
        return getBlocksSelectedView(blocks_);
    }

    QString TextDrawingParagraph::textForInstantEdit() const
    {
        return getBlocksTextForInstantEdit(blocks_);
    }

    QString TextDrawingParagraph::getText() const
    {
        return getBlocksText(blocks_) % u'\n';
    }

    Data::FormattedString TextDrawingParagraph::getSourceText() const
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
        return isBlocksOverLink(blocks_, _p);
    }

    QString TextDrawingParagraph::getLink(QPoint _p) const
    {
        _p -= {margins_.left(), getTopTotalIndentation()};
        return getBlocksLink(blocks_, _p);
    }

    void TextDrawingParagraph::applyMentions(const Data::MentionMap& _mentions)
    {
        for (auto& b : blocks_)
            b->applyMentions(_mentions);
    }

    int TextDrawingParagraph::desiredWidth() const
    {
        auto result = margins_.left() + margins_.right() + getBlocksDesiredWidth(blocks_);
        if (paragraphType_ == ParagraphType::Pre)
            result = std::max(getPreMinWidth(), result);
        return result;
    }

    void TextDrawingParagraph::elide(int _width, ElideType _type, bool _prevElided)
    {
        _width -= margins_.left() + margins_.right();
        elideBlocks(blocks_, _width, _type);
    }

    bool Ui::TextRendering::TextDrawingParagraph::isElided() const
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

    void TextDrawingParagraph::setMaxLinesCount(size_t _count, LineBreakType _lineBreak)
    {
        for (auto& b : blocks_)
            b->setMaxLinesCount(_count, _lineBreak);
    }

    void Ui::TextRendering::TextDrawingParagraph::setMaxLinesCount(size_t _count)
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

    bool TextDrawingParagraph::markdownSingle(const QFont& _font, const QColor& _color)
    {
        im_assert(false);
        return false;
    }

    bool TextDrawingParagraph::markdownMulti(const QFont& _font, const QColor& _color, bool _split, std::pair<std::vector<TextWord>, std::vector<TextWord>>& _splitted)
    {
        im_assert(false);
        return false;
    }

    int TextDrawingParagraph::findForMarkdownMulti() const
    {
        im_assert(false);
        return 0;
    }

    std::vector<TextWord> TextDrawingParagraph::markdown(MarkdownType _type, const QFont& _font, const QColor& _color, bool _split)
    {
        im_assert(false);
        return {};
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

    bool isBlocksOverLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p)
    {
        auto p = _p;
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

    static void trimLineFeeds(Data::FormattedString& _str)
    {
        const auto& s = _str.string();

        auto startIt = s.cbegin();
        while (startIt != s.cend() && *startIt == QChar::LineFeed)
            ++startIt;

        const auto startPos = static_cast<int>(std::distance(s.cbegin(), startIt));

        auto endIt = s.crbegin();
        while ((endIt != s.crend() - startPos) && (*endIt == QChar::LineFeed))
            ++endIt;

        const auto endPos = s.size() - startPos - static_cast<int>(std::distance(s.crbegin(), endIt));

        if (auto size = qMax(0, endPos - startPos); size < s.size())
            _str = Data::FormattedStringView(_str, startPos, size).toFormattedString();
    }

    Data::FormattedStringView getBlocksSelectedView(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        Data::FormattedStringView result;
        for (const auto& b : _blocks)
        {
            const auto text = b->selectedTextView();
            if (text.isEmpty())
                continue;

            if (b->getType() != BlockType::NewLine)
            {
                if (!result.tryToAppend(text))
                    im_assert(false);
            }
            result.tryToAppend(QChar::LineFeed);
        }
        return result;
    }

    Data::FormattedString getBlocksSelectedText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        auto result = getBlocksSelectedView(_blocks).toFormattedString();
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

    QString getBlocksTextForInstantEdit(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        QString result;
        for (const auto& b : _blocks)
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
        for (const auto& b : _blocks)
            result += b->getText();

        trimLineFeeds(result);

        return result;
    }

    Data::FormattedString getBlocksSourceText(const std::vector<BaseDrawingBlockPtr>& _blocks)
    {
        Data::FormattedString result;
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

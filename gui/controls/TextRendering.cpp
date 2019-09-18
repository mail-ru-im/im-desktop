#include "stdafx.h"
#include "TextRendering.h"
#include "../cache/emoji/EmojiDb.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../my_info.h"
#include "../gui_settings.h"

#include "private/qtextengine_p.h"
#include "private/qfontengine_p.h"
#include "qtextformat.h"

#include "../styles/ThemeParameters.h"
#include "../styles/ThemesContainer.h"

namespace
{
    static std::map<QFont, QFontMetricsF> cachedMetrics;

    const QFontMetricsF& getMetrics(const QFont& _font)
    {
        auto metrics = cachedMetrics.find(_font);
        if (metrics == cachedMetrics.end())
        {
            metrics = cachedMetrics.insert(std::make_pair(_font, QFontMetricsF(_font))).first;
        }

        return metrics->second;
    }

    int ceilToInt(double _value)
    {
        return static_cast<int>(std::ceil(_value));
    }

    int roundToInt(double _value)
    {
        return static_cast<int>(std::round(_value));
    }

    double textWidth(const QFont& _font, const QString& _text)
    {
        return getMetrics(_font).width(_text);
    }

    int textHeight(const QFont& _font)
    {
        return roundToInt(getMetrics(_font).height());
    }

    constexpr size_t maximumEmojiCount() noexcept { return 3; }

    bool isUnderlined()
    {
        return Styling::getThemesContainer().getCurrentTheme()->linksUnderlined() == Ui::TextRendering::LinksStyle::UNDERLINED;
    }

    Ui::TextRendering::EmojiScale getEmojiScale(const int _emojiCount = 0)
    {
        switch (_emojiCount)
        {
        case 1:
            return Ui::TextRendering::EmojiScale::BIG;
        case 2:
            return Ui::TextRendering::EmojiScale::MEDIUM;
        case 3:
            return Ui::TextRendering::EmojiScale::REGULAR;
        default:
            return Ui::TextRendering::EmojiScale::NORMAL;
        }
    }

    int emojiHeight(const QFont& _font, const int _emojiCount = 0)
    {
        int height = textHeight(_font) + Utils::scale_value(1);

        const Ui::TextRendering::EmojiScale _scale = getEmojiScale(_emojiCount);

        switch (_scale)
        {
        case Ui::TextRendering::EmojiScale::REGULAR:
            height = platform::is_apple() ? Utils::scale_value(37) : Utils::scale_value(32);
            break;
        case Ui::TextRendering::EmojiScale::MEDIUM:
            height = platform::is_apple() ? Utils::scale_value(45) : Utils::scale_value(40);
            break;
        case Ui::TextRendering::EmojiScale::BIG:
            height = platform::is_apple() ? Utils::scale_value(53) : Utils::scale_value(48);
            break;
        default:
            break;
        }
        return height;
    }

    int getEmojiSize(const QFont& _font, const int _emojiCount = 0)
    {
        auto result = emojiHeight(_font, _emojiCount);
        if (platform::is_apple())
            result += Utils::scale_value(2);

        return result;
    }

    double textSpaceWidth(const QFont& _font)
    {
        static std::map<QFont, double> spaces;
        auto width = spaces.find(_font);
        if (width == spaces.end())
        {
            width = spaces.insert(std::make_pair(_font, textWidth(_font, qsl(" ")))).first;
        }

        return width->second;
    }

    double textAscent(const QFont& _font)
    {
        return getMetrics(_font).ascent();
    }

    double averageCharWidth(const QFont& _font)
    {
        return getMetrics(_font).averageCharWidth();
    }

    QString elideText(const QFont& _font, const QString& _text, int _width)
    {
        const auto& metrics = getMetrics(_font);
        QString result;
        int approxWidth = 0;
        int i = 0;
        while (i < _text.size() && approxWidth < _width)
        {
            auto c = _text[i++];
            result += c;
            approxWidth += metrics.width(c);
        }

        i = result.size();
        while (roundToInt(metrics.width(result)) < _width && i < _text.size())
            result.push_back(_text[i++]);

        while (!result.isEmpty() && roundToInt(metrics.width(result)) > _width)
            result.chop(1);

        return result;
    }

    void drawLine(QPainter& _p, const QPoint& _point, const std::vector<Ui::TextRendering::TextWord>& _line, int _lineHeight, const QColor& _selectColor, const QColor& _highlightColor, const QColor& _linkColor, Ui::TextRendering::VerPosition _pos, Ui::TextRendering::HorAligment _align, int _width, int _selectionDiff, int _lineSpacing)
    {
        if (_line.empty())
            return;

        QFixed x = QFixed::fromReal(_point.x());
        QStackTextEngine engine(QString(), _line.begin()->getFont());

        if (_align != Ui::TextRendering::HorAligment::LEFT)
        {
            auto lineWidth = 0;
            for (const auto& w : _line)
            {
                lineWidth += w.cachedWidth();

                if (w.isSpaceAfter())
                    lineWidth += w.spaceWidth();
            }

            if (lineWidth < _width)
                x += (_align == Ui::TextRendering::HorAligment::CENTER) ? ((_width - lineWidth) / 2) : (_width - lineWidth);
        }

        std::unique_ptr<QTextEngine::LayoutData> releaseMem;
        auto prepareEngine = [&engine, &releaseMem](const QString& _text, const Ui::TextRendering::TextWord& _word)
        {
            if (engine.font() != _word.getFont())
                engine = QStackTextEngine(QString(), _word.getFont());

            engine.text = _text;
            engine.layoutData = nullptr;
            engine.option.setTextDirection(Qt::LeftToRight);
            engine.itemize();
            engine.fnt = _word.getFont();

            // in engine.itemize() memory for engine.layoutData is allocated (ALOT!)
            releaseMem = std::unique_ptr<QTextEngine::LayoutData>(engine.layoutData);

            QScriptLine line;
            line.length = _text.length();
            engine.shapeLine(line);

            int nItems = engine.layoutData->items.size();
            QVarLengthArray<int> visualOrder(nItems);
            QVarLengthArray<uchar> levels(nItems);
            for (int i = 0; i < nItems; ++i)
                levels[i] = engine.layoutData->items[i].analysis.bidiLevel;
            QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());
            return std::make_pair(std::move(visualOrder), std::move(nItems));
        };

        auto prepareGlyph = [&engine](const int item)
        {
            const QScriptItem &si = engine.layoutData->items.at(item);

            QFont f = engine.font(si);

            QTextItemInt gf(si, &f);
            gf.glyphs = engine.shapedGlyphs(&si);
            gf.chars = engine.layoutData->string.unicode() + si.position;
            gf.num_chars = engine.length(item);
            if (engine.forceJustification)
            {
                for (int j = 0; j < gf.glyphs.numGlyphs; ++j)
                    gf.width += gf.glyphs.effectiveAdvance(j);
            }
            else
            {
                gf.width = si.width;
            }
            gf.logClusters = engine.logClusters(&si);

            return std::make_pair(std::move(gf), std::move(f));
        };

        std::vector<QString> selectParts;
        selectParts.reserve(3);

        auto drawWord =
                [&x, &_p, _point, _lineHeight, _pos, &_selectColor, &_linkColor, &_highlightColor, _selectionDiff, _lineSpacing, &prepareEngine, &prepareGlyph, &selectParts]
                (const auto& w, const auto& _nextWord, const bool _needsSpace = true)
        {
            auto pen = QPen(w.getColor());
            const auto extraSpace = Utils::scale_value(2);
            const auto yDiff = Utils::scale_value(1);
            _p.setPen(pen);

            bool selectionHandled = false;
            if (w.isEmoji())
            {
                const bool selected = w.isFullSelected();

                const auto emojiSize = w.emojiSize();
                auto emoji = Emoji::GetEmoji(w.getCode(), emojiSize * Utils::scale_bitmap_ratio());
                Utils::check_pixel_ratio(emoji);
                const auto b = Utils::scale_bitmap_ratio();
                auto y = _point.y() + (_lineHeight / 2.0 - emoji.height() / 2.0 / b);

                if (_pos == Ui::TextRendering::VerPosition::MIDDLE) //todo: handle other cases
                    y -= _lineHeight / 2.0;
                else if (_pos == Ui::TextRendering::VerPosition::BASELINE)
                    y -= textAscent(w.getFont());
                else if (_pos == Ui::TextRendering::VerPosition::BOTTOM)
                    y -= _lineHeight;

                const auto r = QRectF(x.toReal(), y, emoji.width() / b, emoji.height() / b);
                const auto s = w.isSpaceAfter() ? w.spaceWidth() : 0;

                if (selected)
                    _p.fillRect(QRectF(x.toReal()-extraSpace/2, _point.y() - _selectionDiff + yDiff, w.cachedWidth() + (w.isSpaceSelected() ? s : 0) + extraSpace/2, _lineHeight+yDiff), _selectColor);

                if (w.underline())
                {
                    auto someSpaces = qsl(" ");
                    const auto fontMetrics = getMetrics(w.getFont());

                    while (fontMetrics.width(someSpaces) <= emoji.width() / b)
                    {
                        someSpaces += ql1c(' ');
                    }
                    if (!_needsSpace && w.isSpaceAfter())
                        someSpaces += ql1c(' ');

                    const auto [visualOrder, nItems] = prepareEngine(someSpaces, w);

                    for (int i = 0; i < nItems; ++i)
                    {
                        int item = visualOrder[i];
                        const auto [gf, f] = prepareGlyph(item);

                        y = _point.y();

                        switch (_pos)
                        {
                        case Ui::TextRendering::VerPosition::TOP:
                            y += gf.ascent.toReal();
                            y += (_lineHeight - (gf.ascent.toReal() + gf.descent.toReal())) / (2 / Utils::scale_bitmap_ratio());
                            break;

                        case Ui::TextRendering::VerPosition::BASELINE:
                            break;

                        case Ui::TextRendering::VerPosition::BOTTOM:
                            y -= gf.descent.toReal();
                            break;

                        case Ui::TextRendering::VerPosition::MIDDLE:
                            y += (gf.ascent.toInt() - (gf.ascent.toInt() + gf.descent.toInt()) / 2);
                            break;
                        }

                        y -= 1.*_lineSpacing / 2;
                        y = platform::is_apple() ? std::ceil(y) : std::floor(y);
                        _p.setPen(QPen(selected ? w.getColor() : _linkColor));
                        _p.drawTextItem(QPointF(x.toReal()-1, y), gf);
                    }
                }

                _p.drawImage(r, emoji);

                x += (roundToInt(w.cachedWidth()) + s);

                return;
            }

            const auto addSpace = w.isSpaceAfter() ? w.spaceWidth() : 0;
            auto text = w.getText();
            if (!_needsSpace && w.isSpaceAfter())
                text += QChar::Space;

            auto haveText = true;
            auto swithPen = w.isLink() && w.getShowLinks() == Ui::TextRendering::LinksVisible::SHOW_LINKS;
            selectParts.clear();
            auto selectedPart = text.mid(w.selectedFrom(), w.selectedTo() - w.selectedFrom());

            while (haveText)
            {
                auto curText = text;
                if (!w.isLink() || !w.isSelected())
                {
                    haveText = false;
                }
                else if (selectParts.empty())
                {
                    if (w.isFullSelected())
                    {
                        haveText = false;
                        swithPen = false;
                    }
                    else if (w.selectedFrom() == 0)
                    {
                        selectParts.push_back(selectedPart);
                        selectParts.push_back(text.mid(w.selectedTo()));
                    }
                    else
                    {
                        selectParts.push_back(text.left(w.selectedFrom()));
                        selectParts.push_back(selectedPart);
                        selectParts.push_back(text.mid(w.selectedTo()));
                    }
                }

                if (!selectParts.empty())
                {
                    curText = selectParts.front();
                    selectParts.erase(selectParts.begin());
                    if (curText.isEmpty() && selectParts.empty())
                        break;

                    haveText = !selectParts.empty();
                    swithPen = curText != selectedPart;
                }

                const auto [visualOrder, nItems] = prepareEngine(curText, w);

                if (w.isHighlighted() && _highlightColor.isValid())
                {
                    auto hlY = _point.y();
                    if (_pos == Ui::TextRendering::VerPosition::MIDDLE) //todo: handle other cases
                        hlY -= _lineHeight / 2.0;
                    else if (_pos == Ui::TextRendering::VerPosition::BASELINE)
                        hlY -= textAscent(w.getFont());

                    const auto addedWidth = (&w != &_nextWord && _nextWord.isHighlighted()) ? extraSpace : 0;
                    const auto hlWidth = w.cachedWidth() + (w.isSpaceHighlighted() ? w.spaceWidth() : 0) + addedWidth;
                    _p.fillRect(QRectF(QPointF(x.toReal(), hlY), QSizeF(hlWidth, _lineHeight)), _highlightColor);
                }

                for (int i = 0; i < nItems; ++i)
                {
                    int item = visualOrder[i];

                    if (swithPen)
                        _p.setPen(QPen(_linkColor));

                    const auto [gf, f] = prepareGlyph(item);

                    if (w.isSelected() && !selectionHandled)
                    {
                        const auto curWidth = nItems == 1 ? gf.width.toReal() : textWidth(f, curText);
                        const auto selectionY = _point.y() - _selectionDiff + yDiff;
                        const auto selectionH = _lineHeight + yDiff;
                        const auto addedWidth =  w.isSpaceSelected() ? addSpace + extraSpace : 0;

                        if (w.isFullSelected() || (w.isLink() && curText == selectedPart))
                        {
                            _p.fillRect(QRectF(QPointF(x.toReal(), selectionY), QSizeF(curWidth + addedWidth, selectionH)), _selectColor);
                            selectionHandled = true;
                        }
                        else if (!w.isLink())
                        {
                            const auto left = text.left(w.selectedFrom());
                            const auto leftWidth = textWidth(w.getFont(), left);
                            const QPointF selTopLeft(x.toReal() + leftWidth, selectionY);

                            if (w.selectedTo() != text.length())
                            {
                                auto mid = text.mid(w.selectedFrom(), w.selectedTo() - w.selectedFrom());
                                _p.fillRect(QRectF(selTopLeft, QSizeF(textWidth(f, mid) + addedWidth, selectionH)), _selectColor);
                            }
                            else
                            {
                                _p.fillRect(QRectF(selTopLeft, QSizeF(curWidth - leftWidth + addedWidth, selectionH)), _selectColor);
                            }
                            selectionHandled = true;
                        }
                    }

                    auto y = _point.y();
                    switch (_pos)
                    {
                    case Ui::TextRendering::VerPosition::TOP:
                        y += gf.ascent.toReal();
                        y += (_lineHeight - (gf.ascent.toReal() + gf.descent.toReal())) / (2 / Utils::scale_bitmap_ratio());
                        break;

                    case Ui::TextRendering::VerPosition::BASELINE:
                        break;

                    case Ui::TextRendering::VerPosition::BOTTOM:
                        y -= gf.descent.toReal();
                        break;

                    case Ui::TextRendering::VerPosition::MIDDLE:
                        y += (gf.ascent.toInt() - (gf.ascent.toInt() + gf.descent.toInt()) / 2);
                        break;
                    }

                    y -= _lineSpacing / 2;

                    _p.drawTextItem(QPointF(x.toReal(), y), gf);
                    _p.setPen(pen);
                    x += gf.width;
                }
            }

            if (_needsSpace && w.isSpaceAfter())
                x += addSpace;
        };

        for (auto w = _line.begin(); w != _line.end(); ++w)
        {
            const auto& subwords = w->getSubwords();
            if (subwords.empty())
            {
                const auto sw = w+1 == _line.end() ? w : w+1;
                drawWord(*w, *sw);
            }
            else
            {
                for (auto s = subwords.begin(); s != subwords.end(); ++s)
                {
                    const auto sw = s+1 == subwords.end() ? s : s+1;
                    const auto space = s+1 == subwords.end();
                    drawWord(*s, *sw, space);
                }
            }
        }
    }

    QString stringFromCode(int _code)
    {
        QString result;

        if (QChar::requiresSurrogates(_code))
        {
            result.reserve(2);
            result += (QChar)(QChar::highSurrogate(_code));
            result += (QChar)(QChar::lowSurrogate(_code));
        }
        else
        {
            result.reserve(1);
            result += (QChar)_code;
        }

        return result;
    };

    double getLineWidth(const std::vector<Ui::TextRendering::TextWord>& _line)
    {
        const auto sum = [](double _sum, const auto& _w)
        {
            auto width = _sum + _w.cachedWidth();
            if (_w.isSpaceAfter())
                width += _w.spaceWidth();
            return width;
        };

        return std::accumulate(_line.begin(), _line.end(), 1.0, sum);
    }

    size_t getEmojiCount(const std::vector<Ui::TextRendering::TextWord>& _words)
    {
        if (_words.size() > maximumEmojiCount())
            return 0;

        if (std::all_of(_words.begin(), _words.end(), [](const auto& w)
        {
            return (w.isEmoji() && !w.isSpaceAfter());
        }))
            return _words.size();
        else
            return 0;
    }

    void makeHeightCorrection(int &_lineHeight, const Ui::TextRendering::TextWord &_word, const int _emojiCount)
    {
        if constexpr (platform::is_apple())
        {
            if (_word.isResizable() && _emojiCount != 0)
                _lineHeight += Utils::scale_value(3);

            if ((_word.emojiSize() - textHeight(_word.getFont())) > 2)
                _lineHeight -= Utils::scale_value(4);
        }
        else if (_emojiCount != 0)
        {
            _lineHeight += Utils::scale_value(4);
        }
    }

    const auto S_MARK = qsl("`");
    const auto M_MARK = qsl("```");
}

namespace Ui
{
namespace TextRendering
{
    bool isEmoji(const QStringRef& _text)
    {
        int pos = 0;

        return (Emoji::getEmoji(_text, pos) != Emoji::EmojiCode() && Emoji::getEmoji(_text, pos) == Emoji::EmojiCode());
    }

    QString TextWord::getText(TextType _text_type) const
    {
        if (isEmoji())
            return Emoji::EmojiCode::toQString(code_);

        if (_text_type == TextType::SOURCE && type_ == WordType::MENTION)
            return originalMention_;

        if (!subwords_.empty())
        {
            QString result;
            for (const auto& s : subwords_)
            {
                result += s.getText(_text_type);
                if (s.isSpaceAfter())
                    result += QChar::Space;
            }

            return result;
        }

        return text_;
    }

    bool TextWord::equalTo(const QString& _sl) const
    {
        return text_ == _sl;
    }

    QString TextWord::getLink() const
    {
        if (type_ == WordType::MENTION)
            return originalMention_;
        else if (type_ == WordType::NICK)
            return text_;
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

        emojiSize_ = getEmojiSize(font_, isResizable() ? _emojiCount : 0);
        if (isResizable() && _emojiCount == 0)
            emojiSize_ += Utils::scale_value(1);

        if (platform::is_apple() && isInTooltip())
            emojiSize_ -= Utils::scale_value(1);

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

    int TextWord::getPosByX(int _x)
    {
        if (isEmoji())
        {
            if (_x >= cachedWidth_ / 2)
                return 1;
            else
                return 0;
        }

        const auto text = subwords_.empty() ? text_ : getText(TextType::VISIBLE);

        if (_x <= 0)
            return 0;
        else if (_x >= cachedWidth_)
            return text.length();

        for (int i = 1; i < text.length(); ++i)
        {
            if (textWidth(font_, text.left(i)) >= _x)
                return i;
        }

        return text.length();
    }

    void TextWord::select(int _from, int _to, SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = ((selectedTo_ == text_.length() || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        if (type_ == WordType::MENTION  && (_from != 0 || _to != 0))
            return selectAll((_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter() && _to == getText().length()) ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);

        if (_from == _to)
        {
            if (_from == 0)
                clearSelection();

            return;
        }

        selectedFrom_ = _from;
        selectedTo_ = isEmoji() ? 1 : _to;
        isSpaceSelected_ = ((selectedTo_ == text_.length() || (isEmoji() && selectedTo_ == 1)) && _type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
    }

    void TextWord::selectAll(SelectionType _type)
    {
        if (selectionFixed_)
        {
            isSpaceSelected_ = (_type == SelectionType::WITH_SPACE_AFTER && isSpaceAfter());
            return;
        }

        selectedFrom_ = 0;
        selectedTo_ = isEmoji() ? 1 : text_.length();
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
            emit Utils::InterConnector::instance().openDialogOrProfileById(trimmedText_);
        }
        else if (type_ == WordType::LINK || !originalLink_.isEmpty())
        {
            if (Utils::isMentionLink(originalLink_))
                return Utils::openUrl(originalLink_);

            Utils::UrlParser parser;
            parser.process(originalLink_.isEmpty() ? trimmedText_ : originalLink_);
            if (parser.hasUrl())
            {
                auto url = parser.getUrl();
                if (url.is_email())
                    return Utils::openUrl(ql1s("mailto:") + QString::fromStdString(url.url_));

                Utils::openUrl(QString::fromStdString(url.url_));
            }
        }
    }

    double TextWord::cachedWidth() const
    {
        return cachedWidth_;
    }

    int TextWord::spaceWidth() const
    {
        return ceilToInt(spaceWidth_);
    }

    bool TextWord::applyMention(const std::pair<QString, QString>& _mention)
    {
        if (!Utils::isMentionLink(text_))
            return false;

        const auto mention = text_.midRef(2, text_.length() - 3);
        if (mention != _mention.first)
            return false;

        if (mention == Ui::MyInfo()->aimId())
            isHighlighted_ = true;

        originalMention_ = text_;
        text_ = _mention.second;
        type_ = WordType::MENTION;
        return true;
    }

    bool TextWord::applyMention(const QString& _mention, const std::vector<TextWord>& _mentionWords)
    {
        if (!Utils::isMentionLink(text_))
            return false;

        const auto mention = text_.midRef(2, text_.length() - 3);
        if (mention != _mention)
            return false;

        if (mention == Ui::MyInfo()->aimId())
            isHighlighted_ = true;

        originalMention_ = text_;
        text_.clear();
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
        if (type_ == WordType::LINK || type_ == WordType::NICK)
        {
            type_ = WordType::TEXT;
            originalLink_.clear();
            showLinks_ = LinksVisible::DONT_SHOW_LINKS;
        }
    }

    void TextWord::setFont(const QFont& _font)
    {
        font_ = _font;
        for (auto& s : subwords_)
            s.setFont(_font);

        cachedWidth_ = 0;
    }

    void TextWord::applyFontColor(const TextWord& _other)
    {
        setFont(_other.getFont());
        setColor(_other.getColor());
        setUnderline(_other.underline());
    }

    std::vector<TextWord> TextWord::splitByWidth(int _width)
    {
        std::vector<TextWord> result;
        if (subwords_.empty())//todo implement for common words
            return result;

        auto curwidth = 0;
        std::vector<TextWord> subwordsFirst;

        auto finalize = [&result, &subwordsFirst, this](auto iter)
        {
            auto tw = *this;
            tw.setSubwords(subwordsFirst);
            tw.width(WidthMode::FORCE);
            result.push_back(tw);
            tw.setSubwords(std::vector<TextWord>(iter, subwords_.end()));
            tw.width(WidthMode::FORCE);
            result.push_back(tw);
        };

        for (std::vector<TextWord>::iterator iter = subwords_.begin(); iter != subwords_.end(); ++iter)
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

            tw = TextWord(curWordText.mid(tmpWord.length()), getSpace(), getType(), linkDisabled_ ? LinksVisible::DONT_SHOW_LINKS : getShowLinks());
            tw.applyFontColor(*this);
            tw.width();
            if (tw.isLink())
                tw.setOriginalLink(getText());

            std::vector<TextWord> subwordsSecond = { tw };
            if (++iter != subwords_.end())
                subwordsSecond.insert(subwordsSecond.end(), iter, subwords_.end());

            auto temp = *this;
            temp.setSubwords(subwordsFirst);
            temp.width(WidthMode::FORCE);
            result.push_back(temp);
            temp.setSubwords(subwordsSecond);
            temp.width(WidthMode::FORCE);
            result.push_back(temp);
            break;
        }

        if (result.empty())
        {
            auto temp = *this;
            temp.setSubwords(subwordsFirst);
            temp.width(WidthMode::FORCE);
            result.push_back(temp);
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

    void TextWord::setEmojiSizeType(EmojiSizeType _emojiSizeType)
    {
        emojiSizeType_ = _emojiSizeType;
    }

    void TextWord::checkSetNick()
    {
        if (showLinks_ == LinksVisible::SHOW_LINKS && type_ != WordType::EMOJI && trimmedText_.startsWith(ql1c('@')))
        {
            if (Utils::isNick(&trimmedText_))
                type_ = WordType::NICK;
        }
    }

    TextDrawingBlock::TextDrawingBlock(const QStringRef& _text, LinksVisible _showLinks, BlockType _blockType)
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
        : BaseDrawingBlock(BlockType::TYPE_TEXT)
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
        auto textBlock = dynamic_cast<TextDrawingBlock*>(_other);
        if (textBlock)
        {
            type_ = textBlock->type_;
            linkColor_ = textBlock->linkColor_;
            selectionColor_ = textBlock->selectionColor_;
            highlightColor_ = textBlock->highlightColor_;
            align_ = textBlock->align_;
        }

        calcDesired();
    }

    void TextDrawingBlock::init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType, const LinksStyle _linksStyle)
    {
        for (auto& w : words_)
        {
            w.setFont(_font);
            w.setColor(_color);
            w.setEmojiSizeType(_emojiSizeType);

            if (w.getShowLinks() == LinksVisible::SHOW_LINKS && w.isLink() && _linksStyle == LinksStyle::UNDERLINED)
                w.setUnderline(true);
        }

        for (auto& w : originalWords_)
        {
            w.setFont(_font);
            w.setColor(_color);
            w.setEmojiSizeType(_emojiSizeType);

            if (w.getShowLinks() == LinksVisible::SHOW_LINKS && w.isLink() && _linksStyle == LinksStyle::UNDERLINED)
                w.setUnderline(true);
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
        _point =  QPoint(_point.x() + margins_.left(), _point.y() + margins_.top());

        if (lines_.size() > 1)
            _point.ry() -= lineSpacing_ / 2;

        for (const auto& l : lines_)
        {
            drawLine(_p, _point, l, lineHeight_, selectionColor_, highlightColor_, linkColor_, _pos, align_, cachedSize_.width(), 0, lineSpacing_);
            _point.ry() += lineHeight_;
        }

        if (lines_.size() > 1)
        {
            _point.ry() += lineSpacing_ / 2;
        }
        else if (platform::is_apple() && !lines_.empty())
        {
            const auto& line = lines_.front();
            if (std::any_of(line.begin(), line.end(), [](const auto &w) { return (w.isEmoji() && w.underline()); }))
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

        drawLine(_p, _point, lines_[0], lineHeight_, selectionColor_, highlightColor_, linkColor_, VerPosition::MIDDLE, align_, cachedSize_.width(), lineHeight_ / 2, lineSpacing_);

        _point.ry() += lineHeight_ + margins_.bottom();
    }

    int TextDrawingBlock::getHeight(int width, int lineSpacing, CallType type, bool _isMultiline)
    {
        if (width == 0 || words_.empty())
            return 0;

        if (cachedSize_.width() == width && type == CallType::USUAL)
            return cachedSize_.height();

        lineSpacing_ = lineSpacing;

        lines_.clear();
        linkRects_.clear();
        links_.clear();
        std::vector<TextWord> curLine;
        width -= (margins_.left() + margins_.right());
        double curWidth = 0;
        lineHeight_ = 0;

        const size_t emojiCount = _isMultiline ? 0 : getEmojiCount(words_);

        if (emojiCount != 0)
            needsEmojiMargin_ = true;

        const auto& last = *(words_.rbegin());
        auto fillLines = [width, &curLine, &curWidth, emojiCount, last, this]()
        {
            for (auto& w : words_)
            {
                const auto limited = (maxLinesCount_ != std::numeric_limits<size_t>::max() && (lineBreak_ == LineBreakType::PREFER_WIDTH || lines_.size() == maxLinesCount_ - 1));

                lineHeight_ = std::max(lineHeight_, textHeight(w.getFont()));

                auto wordWidth = w.width(WidthMode::FORCE, emojiCount);

                if (w.isSpaceAfter())
                    wordWidth += w.spaceWidth();

                auto curWord = w;
                double cmpWidth = width;
                if (limited && !curLine.empty())
                    cmpWidth -= curWidth;

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

                    if ((splitedWords.empty() && tmpWord.isEmpty()) || (!splitedWords.empty() && !splitedWords[0].hasSubwords()) || splitedWords.size() == 1)
                    {
                        if (!curLine.empty())
                        {
                            lines_.push_back(curLine);
                            curLine.clear();
                            if (lines_.size() == maxLinesCount_)
                                return true;

                            cmpWidth = width;
                            curWidth = 0;
                            continue;
                        }
                        break;
                    }

                    auto tw = splitedWords.empty() ? TextWord(tmpWord, Space::WITHOUT_SPACE_AFTER, curWord.getType(), (curWord.isLinkDisabled() || curWord.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : curWord.getShowLinks()) : splitedWords[0];
                    if (splitedWords.empty())
                    {
                        tw.applyFontColor(w);
                        tw.setHighlighted(w.isHighlighted());
                        tw.setWidth(cmpWidth, emojiCount);
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
                        cmpWidth = width;
                    }

                    if (lines_.size() == maxLinesCount_)
                        return true;

                    curWord = splitedWords.size() <= 1 ? TextWord(curWordText.mid(tmpWord.length()), curWord.getSpace(), curWord.getType(), (curWord.isLinkDisabled() || curWord.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : curWord.getShowLinks()) : splitedWords[1];
                    if (curWord.isLink())
                        curWord.setOriginalLink(w.getText());

                    if (splitedWords.size() <= 1)
                    {
                        curWord.applyFontColor(w);
                        curWord.setHighlighted(w.isHighlighted());

                        if (wordWidth - tw.cachedWidth() < cmpWidth)
                            curWord.width(WidthMode::PLAIN, emojiCount);
                        else
                            curWord.setWidth(wordWidth - tw.cachedWidth(), emojiCount);
                    }

                    wordWidth = curWord.cachedWidth();
                }

                if (curWidth + wordWidth <= width)
                {
                    curLine.push_back(curWord);
                    if (curWord.isLink())
                    {
                        linkRects_.emplace_back(curWidth, lines_.size(), wordWidth, 0);
                        links_[linkRects_.size() - 1] = curWord.getLink();
                    }
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

                    curLine.clear();
                    curLine.push_back(curWord);
                    if (curWord.isLink())
                    {
                        linkRects_.emplace_back(0, lines_.size(), wordWidth, 0);
                        links_[linkRects_.size() - 1] = curWord.getLink();
                    }
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
                lines_[maxLinesCount_ - 1] = elideWords(lines_[maxLinesCount_ - 1], lastLineWidth_ == -1 ? width : lastLineWidth_, QWIDGETSIZE_MAX, true);
                elided_ = true;
            }
        }

        lineHeight_ = std::max(lineHeight_, words_.back().emojiSize());
        lineHeight_ += lineSpacing_;

        makeHeightCorrection(lineHeight_, words_.back(), emojiCount);

        const auto underlineOffset = getMetrics(words_[0].getFont()).underlinePos();
        for (auto& r : linkRects_)
        {
            r.setY(r.y() * lineHeight_);
            r.setHeight(lineHeight_ + underlineOffset);
        }

        auto cachedHeight = lineHeight_ * lines_.size() + margins_.top() + margins_.bottom();
        if (lines_.size() == 1 && !_isMultiline && !linkRects_.empty())
            cachedHeight += underlineOffset;

        cachedSize_ = QSize(width, cachedHeight);
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
            if (i < 0 || i >= int(lines_.size()))
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
                        w.select(w.getPosByX(x1 - diff), (x2 >= curWidth) ? w.getText().length() : w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
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
                        w.select(w.getPosByX(x1 - diff), (x2 >= curWidth) ? w.getText().length() : w.getPosByX(x2 - diff), selectSpace ? SelectionType::WITH_SPACE_AFTER : SelectionType::WITHOUT_SPACE_AFTER);
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
                {
                    if (w.isSelected())
                    {
                        w.clearSelection();
                        continue;
                    }
                }
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
        if (_p.x() < 0)
            return;

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
                    auto lineWidth = 0;
                    for (const auto& w : l)
                    {
                        lineWidth += w.cachedWidth();
                        if (w.isSpaceAfter())
                            lineWidth += w.spaceWidth();
                    }

                    if (lineWidth < cachedSize_.width())
                        x -= (align_ == Ui::TextRendering::HorAligment::CENTER) ? ((cachedSize_.width() - lineWidth) / 2) : (cachedSize_.width() - lineWidth);
                }

                if (x < 0)
                    return;

                for (auto& w : l)
                {
                    curWidth += w.cachedWidth();
                    if (w.isSpaceAfter())
                        curWidth += w.spaceWidth();

                    if (curWidth > x)
                    {
                        w.clicked();
                        return;
                    }
                }
            }
        }
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
                    auto lineWidth = 0;
                    for (const auto& w : l)
                    {
                        lineWidth += w.cachedWidth();
                        if (w.isSpaceAfter())
                            lineWidth += w.spaceWidth();
                    }

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
        int x = 0;
        if (align_ != Ui::TextRendering::HorAligment::LEFT)
        {
            int i = 0;
            for (const auto& l : lines_)
            {
                ++i;
                auto y1 = (i - 1) * lineHeight_;
                auto y2 = i * lineHeight_;

                if (_p.y() >= y1 && _p.y() < y2)
                {
                    auto lineWidth = 0;
                    for (const auto& w : l)
                    {
                        lineWidth += w.cachedWidth();
                        if (w.isSpaceAfter())
                            lineWidth += w.spaceWidth();
                    }

                    if (lineWidth < cachedSize_.width())
                        x += (align_ == Ui::TextRendering::HorAligment::CENTER) ? ((cachedSize_.width() - lineWidth) / 2) : (cachedSize_.width() - lineWidth);

                    break;
                }
            }
        }

        auto p = _p;
        p.setX(_p.x() - x);
        return std::any_of(linkRects_.begin(), linkRects_.end(), [p](auto r) { return r.contains(p); });
    }

    QString TextDrawingBlock::getLink(const QPoint& _p) const
    {
        int i = 0;
        for (auto r : linkRects_)
        {
            if (r.contains(_p))
            {
                const auto it = links_.find(i);
                return it != links_.end() ? it->second : QString();
            }

            ++i;
        }

        return QString();
    }

    void TextDrawingBlock::applyMentions(const Data::MentionMap& _mentions)
    {
        if (_mentions.empty())
            return;

        for (const auto& m : _mentions)
        {
            const QString mention = ql1s("@[") % m.first % ql1c(']');
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

                auto space = iter->getSpace();
                auto type = iter->getType();
                auto links = iter->getShowLinks();

                auto func = [space, type, text, &iter, links, this](QString _left, QString _right)
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

                const auto isLengthEqual = text.length() == mention.length();
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

                func(mention, text.mid(mention.length()));
            }
        }

        for (const auto& m : _mentions)
        {
            std::vector<TextWord> mentionWords;
            QStringRef ref(&m.second);
            parseForWords(ref, LinksVisible::SHOW_LINKS, mentionWords, WordType::MENTION);
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
        }
    }

    int TextDrawingBlock::desiredWidth() const
    {
        return ceilToInt(desiredWidth_);
    }

    void TextDrawingBlock::elide(int _width, ELideType _type)
    {
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

    void TextDrawingBlock::parseForWords(const QStringRef& _text, LinksVisible _showLinks, std::vector<TextWord>& _words, WordType _type)
    {
        const auto textSize = _text.size();

        QString tmp;
        for (int i = 0; i < textSize;)
        {
            auto isSpace = _text.at(i).isSpace();
            if (!tmp.isEmpty() && (isSpace || (tmp.endsWith(S_MARK) && _text.at(i) != S_MARK)) || (!tmp.endsWith(S_MARK) && _text.at(i) == S_MARK))
            {
                while (tmp.startsWith(QChar::Space) && !tmp.isEmpty())
                {
                    _words.emplace_back(tmp.left(1), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                    tmp.remove(0, 1);
                }
                if (!tmp.isEmpty())
                {
                    auto word = TextWord(tmp, isSpace ? Space::WITH_SPACE_AFTER : Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                    if ((word.getType() != WordType::LINK || !parseWordLink(word, _words)) && !parseWordNick(word, _words))
                        _words.emplace_back(std::move(word));
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
                        _words.emplace_back(tmp, space, _type, _showLinks);
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
                _words.emplace_back(tmp.left(1), Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                tmp.remove(0, 1);
            }
            if (!tmp.isEmpty())
            {
                auto word = TextWord(tmp, Space::WITHOUT_SPACE_AFTER, _type, _showLinks);
                if ((word.getType() != WordType::LINK || !parseWordLink(word, _words)) && !parseWordNick(word, _words))
                    _words.emplace_back(std::move(word));
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

    bool TextDrawingBlock::parseWordNick(const TextWord& _word, std::vector<TextWord>& _words, const QString& _text)
    {
        const auto text = _text.isEmpty() ? _word.getText() : _text;
        const auto showLinks = _word.getShowLinks();
        const auto wordSpace = _word.getSpace();
        if (const auto index = text.indexOf(ql1c('@')); index >= 0)
        {
            auto before = text.leftRef(index);
            auto nick = text.midRef(index, text.size() - before.size());
            while (!nick.isEmpty() && !Utils::isNick(nick))
                nick = text.midRef(index, nick.size() - 1);

            if (!nick.isEmpty())
            {
                auto after = text.midRef(index + nick.size(), text.size() - index - nick.size());
                if (after.startsWith(ql1c('@')))
                    return false;

                if (!before.isEmpty())
                    _words.emplace_back(before.toString(), Space::WITHOUT_SPACE_AFTER, WordType::TEXT, showLinks);

                _words.emplace_back(nick.toString(), after.isEmpty() ? wordSpace : Space::WITHOUT_SPACE_AFTER, WordType::LINK, showLinks);
                if (!after.isEmpty() && !parseWordNick(_word, _words, after.toString()))
                    _words.emplace_back(after.toString(), wordSpace, WordType::TEXT, showLinks);

                return true;
            }
        }
        return false;
    }

    std::vector<TextWord> TextDrawingBlock::elideWords(const std::vector<TextWord>& _original, int _width, int _desiredWidth, bool _forceElide, const ELideType& _type)
    {
        if (_original.empty())
            return _original;

        static const auto dots = qsl("...");
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

            if (_type == ELideType::ACCURATE)
            {
                while (!tmp.isEmpty() && textWidth(f, tmp) > (_width - curWidth))
                    tmp.chop(1);
            }
            else if (_type == ELideType::FAST)
            {
                auto left = 0, right = tmp.length();
                const auto chWidth = averageCharWidth(f);
                while (left != right)
                {
                    const auto mid = left + (right - left) / 2;
                    if (mid == left || mid == right)
                        break;

                    const auto t = tmp.left(mid);
                    const auto width = textWidth(f, t);
                    const auto cmp = (_width - curWidth) - width;
                    const auto cmpCh = chWidth - abs(cmp);

                    if (cmpCh >= 0 && cmpCh <= chWidth)
                    {
                        tmp = t;
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

            if (tmp.length() == w.getText().length())
            {
                result.push_back(w);
                curWidth += wordWidth;
                continue;
            }

            if (!tmp.isEmpty())
            {
                auto word = TextWord(std::move(tmp), Space::WITHOUT_SPACE_AFTER, w.getType(), (w.isLinkDisabled() || w.getType() == WordType::TEXT) ? LinksVisible::DONT_SHOW_LINKS : w.getShowLinks()); //todo: implement method makeFrom()
                word.applyFontColor(w);
                word.setHighlighted(w.isHighlighted());
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
                w.setHighlighted(result.back().isHighlighted());
            }
            else if (!words_.empty())
            {
                w.applyFontColor(words_.back());
                w.setHighlighted(words_.back().isHighlighted());
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
                if (textRef.size() > 1 && (textRef.endsWith(ql1c(',')) || textRef.endsWith(ql1c('.'))))
                    textRef.truncate(textRef.size() - 1);

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

    void TextDrawingBlock::setSelectionColor(const QColor & _color)
    {
        selectionColor_ = _color;
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

        if (!words_.empty() && _words.size() == 1 && _words.front().getText() == qsl(" "))
        {
            words_.back().setSpace(Space::WITH_SPACE_AFTER);

            desiredWidth_ += words_.back().spaceWidth();
        }
        else
        {
            if (!words_.empty() && words_.back().isHighlighted() && !_words.empty() && _words.front().isHighlighted())
                words_.back().setSpaceHighlighted(true);

            auto it = words_.insert(words_.end(), _words.begin(), _words.end());

            for (; it != words_.end(); it++)
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
                            i->setUnderline(isUnderlined());
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
                        iter->setUnderline(isUnderlined());
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
        switch (_type)
        {
        case Ui::TextRendering::MarkdownType::ALL:
        case Ui::TextRendering::MarkdownType::FROM:
        {
            auto iter = words_.begin();
            auto found = (_type == Ui::TextRendering::MarkdownType::ALL) ? true : false;
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
                        iter->setUnderline(isUnderlined());
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
                        iter->setUnderline(isUnderlined());
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
        for (auto& w : words_)
        {
            w.setHighlighted(_isHighlighted);
            w.setSpaceHighlighted(_isHighlighted);
        }

        if (!words_.empty())
            words_.back().setSpaceHighlighted(false);
    }

    void TextDrawingBlock::setUnderline(const bool _enabled)
    {
        for (auto& w : words_)
            w.setUnderline(_enabled);

        for (auto& l : lines_)
        {
            for (auto& w : l)
                w.setUnderline(_enabled);
        }
    }

    double TextDrawingBlock::getLastLineWidth() const
    {
        if (lines_.empty())
            return 0;

        return getLineWidth(lines_.back());
    }

    double TextDrawingBlock::getMaxLineWidth() const
    {
        if (cachedMaxLineWidth_ != -1)
            return cachedMaxLineWidth_;

        if (lines_.empty())
            return 0;

        std::vector<double> lineSizes;
        lineSizes.reserve(lines_.size());

        for (const auto& l : lines_)
            lineSizes.push_back(getLineWidth(l));

        cachedMaxLineWidth_ = *std::max_element(lineSizes.begin(), lineSizes.end());

        return cachedMaxLineWidth_;
    }

    DebugInfoTextDrawingBlock::DebugInfoTextDrawingBlock(qint64 _id, Subtype _subtype, LinksVisible _showLinks)
        : TextDrawingBlock(QString::number(_id).midRef(0), _showLinks, BlockType::TYPE_DEBUG_TEXT),
          subtype_(_subtype),
          messageId_(_id)
    {

    }

    DebugInfoTextDrawingBlock::Subtype DebugInfoTextDrawingBlock::getSubtype() const
    {
        return subtype_;
    }

    qint64 DebugInfoTextDrawingBlock::getMessageId() const
    {
        return messageId_;
    }

    void DebugInfoTextDrawingBlock::setMessageId(qint64 _id)
    {
        messageId_ = _id;
    }


    NewLineBlock::NewLineBlock()
        : BaseDrawingBlock(BlockType::TYPE_NEW_LINE)
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

    void NewLineBlock::elide(int, ELideType)
    {

    }

    std::vector<BaseDrawingBlockPtr> parseForBlocks(const QString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds)
    {
        std::vector<BaseDrawingBlockPtr> blocks;
        QString text;
        QStringRef textRef(&_text);
        if (_processLineFeeds == ProcessLineFeeds::REMOVE_LINE_FEEDS)
        {
            text = _text;
            text.replace(QChar::LineFeed, QChar::Space);
            textRef = QStringRef(&text);
        }

        int i = textRef.indexOf(QChar::LineFeed);
        bool skipNewLine = true;
        while (i != -1 && !textRef.isEmpty())
        {
            const auto t = textRef.left(i);
            if (!t.isEmpty())
                blocks.push_back(std::make_unique<TextDrawingBlock>(t, _showLinks));

            if (!skipNewLine)
                blocks.push_back(std::make_unique<NewLineBlock>());

            textRef = textRef.mid(i + 1, textRef.length() - i - 1);
            i = textRef.indexOf(QChar::LineFeed);
            skipNewLine = (i != 0);
        }

        if (!textRef.isEmpty())
            blocks.push_back(std::make_unique<TextDrawingBlock>(textRef, _showLinks));

        if (!_mentions.empty())
        {
            for (auto& b : blocks)
                b->applyMentions(_mentions);
        }

        return blocks;
    }

    int getBlocksHeight(const std::vector<BaseDrawingBlockPtr>& _blocks, int width, int lineSpacing, CallType _calltype)
    {
        if (width <= 0)
            return 0;

        int h = 0;
        bool isMultiline = (_blocks.size() > 1);
        for (auto& b : _blocks)
            h += b->getHeight(width, lineSpacing, _calltype, isMultiline);

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

    static void trimLineFeeds(QString& str)
    {
        if (str.isEmpty())
            return;

        QStringRef strRef(&str);
        int atStart = 0;
        int atEnd = 0;
        while (strRef.startsWith(QChar::LineFeed))
        {
            strRef = strRef.mid(1, str.size() - 1);
            ++atStart;
        }
        while (strRef.endsWith(QChar::LineFeed))
        {
            strRef.truncate(strRef.size() - 1);
            ++atEnd;
        }

        if (strRef.isEmpty())
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
            if (b->getType() == BlockType::TYPE_DEBUG_TEXT)
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

    void elideBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width, ELideType _type)
    {
        for (auto& b : _blocks)
            b->elide(_width, _type);
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
        {
            return appendWholeBlocks(_to, _from);
        }

        (*_to.rbegin())->appendWords((*_from.begin())->getWords());
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
}
}

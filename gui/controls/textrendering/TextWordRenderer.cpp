#include "stdafx.h"

#include "TextWordRenderer.h"
#include "TextRenderingUtils.h"

#include "utils/utils.h"

#include "private/qtextengine_p.h"
#include "private/qfontengine_p.h"
#include "qtextformat.h"

#include "styles/ThemeParameters.h"

namespace
{
    int getExtraSpace() noexcept { return Utils::scale_value(2); }
    int getYDiff() noexcept { return Utils::scale_value(1); }

    constexpr bool useCustomDots() noexcept { return true; }
    constexpr bool useDotsViaLine() noexcept { return false; }

    double getVerticalShift(const int _lineHeight, const Ui::TextRendering::VerPosition _pos, const QTextItemInt& _gf)
    {
        switch (_pos)
        {
        case Ui::TextRendering::VerPosition::TOP:
            return _gf.ascent.toReal() + (_lineHeight - (_gf.ascent.toReal() + _gf.descent.toReal())) / (2 / Utils::scale_bitmap_ratio());

        case Ui::TextRendering::VerPosition::BASELINE:
            break;

        case Ui::TextRendering::VerPosition::BOTTOM:
            return -_gf.descent.toReal();

        case Ui::TextRendering::VerPosition::MIDDLE:
            return _gf.ascent.toInt() - (_gf.ascent.toInt() + _gf.descent.toInt()) / 2;
        }

        return 0;
    }

    QTextCharFormat spellCheckUnderline()
    {
        QTextCharFormat format;
        const static auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);
        format.setUnderlineColor(color);
        format.setUnderlineStyle(QTextCharFormat::UnderlineStyle::DotLine);
        return format;
    }

    QTextCharFormat makeCharFormat(QTextCharFormat::UnderlineStyle _underlineStyle)
    {
        if (_underlineStyle == QTextCharFormat::UnderlineStyle::SpellCheckUnderline)
            return spellCheckUnderline();
        return {};
    }

    std::pair<QTextItemInt, QFont> prepareGlyph(const QStackTextEngine& _engine, const int _item, QTextCharFormat::UnderlineStyle _underlineStyle = QTextCharFormat::UnderlineStyle::NoUnderline)
    {
        const QScriptItem &si = _engine.layoutData->items.at(_item);

        QFont f = _engine.font(si);

        QTextItemInt gf(si, &f, makeCharFormat(_underlineStyle));
        gf.glyphs = _engine.shapedGlyphs(&si);
        gf.chars = _engine.layoutData->string.unicode() + si.position;
        gf.num_chars = _engine.length(_item);
        if (_engine.forceJustification)
        {
            for (int j = 0; j < gf.glyphs.numGlyphs; ++j)
                gf.width += gf.glyphs.effectiveAdvance(j);
        }
        else
        {
            gf.width = si.width;
        }
        gf.logClusters = _engine.logClusters(&si);

        return { std::move(gf), std::move(f) }; // gf stores pointer to f inside
    }
}

using namespace TextWordRendererPrivate;

namespace Ui
{
    namespace TextRendering
    {
        struct TextWordRenderer_p
        {
            std::unique_ptr<QStackTextEngine> engine_;
            std::unique_ptr<QTextEngine::LayoutData> releaseMem_;
        };

        TextWordRenderer::TextWordRenderer(
            QPainter* _painter,
            const QPointF& _point,
            const int _lineHeight,
            const int _lineSpacing,
            const int _selectionDiff,
            const VerPosition _pos,
            const QColor& _selectColor,
            const QColor& _highlightColor,
            const QColor& _hightlightTextColor,
            const QColor& _linkColor
            )
            : painter_(_painter)
            , point_(_point)
            , lineHeight_(_lineHeight)
            , lineSpacing_(_lineSpacing)
            , selectionDiff_(_selectionDiff)
            , pos_(_pos)
            , selectColor_(_selectColor)
            , highlightColor_(_highlightColor)
            , hightlightTextColor_(_hightlightTextColor)
            , linkColor_(_linkColor)
            , addSpace_(0.0)
            , d(std::make_unique<TextWordRenderer_p>())
        {
            im_assert(painter_);
        }

        TextWordRenderer::~TextWordRenderer() = default;

        void TextWordRenderer::draw(const TextWord& _word, bool _needsSpace, bool _isLast)
        {
            addSpace_ = _word.isSpaceAfter() ? _word.spaceWidth() : 0;

            if (_word.isEmoji())
                drawEmoji(_word, _needsSpace);
            else
                drawWord(_word, _needsSpace, _isLast);
        }

        void TextWordRenderer::drawWord(const TextWord& _word, bool _needsSpace, bool _isLast)
        {
            im_assert(!_word.hasSubwords());
            const auto text = _word.getText();
            const auto useLinkPen = _word.isLink() && _word.getShowLinks() == Ui::TextRendering::LinksVisible::SHOW_LINKS;
            const auto textColor = useLinkPen ? linkColor_ : _word.getColor();

            textParts_.clear();
            textParts_.push_back(TextPart(&text, textColor));

            if (_word.hasSpellError())
            {
                for (auto e : boost::adaptors::reverse(_word.getSyntaxWords()))
                {
                    if (e.spellError)
                        split(text, e.offset_, e.end(), textColor, {}, SpellError::Yes);
                }
            }

            if (_word.isHighlighted() && highlightColor_.isValid())
            {
                const auto highlightedTextColor = hightlightTextColor_.isValid() ? hightlightTextColor_ : textColor;
                if (highlightedTextColor == textColor && textParts_.size() == 1)
                    fill(text, _word.highlightedFrom(), _word.highlightedTo(), highlightColor_);
                else
                    split(text, _word.highlightedFrom(), _word.highlightedTo(), highlightedTextColor, highlightColor_);
            }

            if (_word.isSelected() && selectColor_.isValid())
            {
                const auto& selectedTextColor = _word.getColor();
                if (selectedTextColor == textColor && textParts_.size() == 1)
                    fill(text, _word.selectedFrom(), _word.selectedTo(), selectColor_);
                else
                    split(text, _word.selectedFrom(), _word.selectedTo(), selectedTextColor, selectColor_);
            }

            if (textParts_.size() > 1)
                textParts_.erase(std::remove_if(textParts_.begin(), textParts_.end(), [](const auto& p) { return p.ref_.isEmpty(); }), textParts_.end());

            if (!_needsSpace && _word.isSpaceAfter())
            {
                const static auto sp = spaceAsString();
                textParts_.push_back(TextPart(&sp, textColor));

                if (_word.isSpaceSelected())
                    textParts_.back().fill_.push_back({ QStringRef(), selectColor_ });
                else if (_word.isSpaceHighlighted())
                    textParts_.back().fill_.push_back({ QStringRef(), highlightColor_ });
            }

            const auto fillH = lineHeight_ + getYDiff();
            auto fillY = point_.y() - selectionDiff_ + getYDiff();
            if (pos_ == Ui::TextRendering::VerPosition::MIDDLE) //todo: handle other cases
                fillY -= lineHeight_ / 2.0;
            else if (pos_ == Ui::TextRendering::VerPosition::BASELINE)
                fillY -= textAscent(_word.getFont());

            QVarLengthArray<int> visualOrder;
            for (auto it = textParts_.cbegin(); it != textParts_.cend(); ++it)
            {
                const auto& part = *it;
                prepareEngine(part.ref_.toString(), _word.getFont(), visualOrder);

                const auto underLineStyle = part.hasSpellError_ && !useCustomDots()
                    ? QTextCharFormat::UnderlineStyle::SpellCheckUnderline
                    : QTextCharFormat::UnderlineStyle::NoUnderline;
                bool fillHanded = false;
                const auto startX = point_.x();
                auto startY = point_.y();

                for (auto item : visualOrder)
                {
                    const auto[gf, font] = prepareGlyph(*d->engine_, item, underLineStyle);

                    const auto x = point_.x();
                    const auto y = point_.y() + getVerticalShift(lineHeight_, pos_, gf) - lineSpacing_ / 2;
                    if (y > startY)
                        startY = y;

                    if (!part.fill_.isEmpty())
                    {
                        for (const auto& f : part.fill_)
                        {
                            if (f.ref_.isEmpty())
                            {
                                painter_->fillRect(QRect(x, fillY, gf.width.ceil().toInt(), fillH), f.color_);
                            }
                            else if (!fillHanded)
                            {
                                const auto leftWidth = textWidth(_word.getFont(), text.left(f.ref_.position()));
                                const auto subStr = text.mid(f.ref_.position(), f.ref_.size());
                                const auto selWidth = _isLast && _word.italic()
                                    ? textVisibleWidth(_word.getFont(), subStr)
                                    : textWidth(_word.getFont(), subStr);
                                const QRect r(roundToInt(x + leftWidth), fillY, ceilToInt(selWidth), fillH);
                                painter_->fillRect(r, f.color_);
                                fillHanded = true;
                            }
                        }
                    }
                    if (it == std::prev(textParts_.cend()) && (_word.isSpaceSelected() || _word.isSpaceHighlighted()))
                    {
                        const auto addedWidth = addSpace_ + getExtraSpace();
                        if (auto fillColor = _word.isSpaceSelected() ? selectColor_ : highlightColor_; fillColor.isValid())
                            painter_->fillRect(QRect(point_.x() + gf.width.toReal(), fillY, addedWidth, fillH), fillColor);
                    }

                    if (_word.hasShadow())
                    {
                        const auto shadow = _word.getShadow();
                        painter_->setPen(shadow.color);
                        painter_->drawTextItem(QPointF(x + shadow.offsetX, y + shadow.offsetY), gf);
                    }

                    painter_->setPen(part.textColor_);
                    painter_->drawTextItem(QPointF(x, y), gf);

                    point_.rx() += gf.width.toReal();
                }

                const auto endX = point_.x();
                if (useCustomDots() && part.hasSpellError_)
                {
                    im_assert(_word.getFont().pixelSize() > 0);
                    auto fontFactor = qreal(_word.getFont().pixelSize()) / qreal(10.);
                    fontFactor = qAbs(fontFactor);
                    startY += Utils::scale_value(3);
                    Utils::PainterSaver painterSaver(*painter_);
                    painter_->setRenderHint(QPainter::Antialiasing);
                    const static auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);

                    if constexpr (useDotsViaLine())
                    {
                        QPen pen;
                        pen.setStyle(Qt::PenStyle::DotLine);
                        pen.setColor(color);
                        pen.setCapStyle(Qt::RoundCap);
                        pen.setJoinStyle(Qt::RoundJoin);
                        pen.setWidthF(1 * fontFactor);
                        painter_->setBrush(Qt::NoBrush);
                        painter_->setPen(pen);
                        painter_->drawLine(QPointF{ startX, startY }, { endX, startY });
                    }
                    else
                    {
                        const auto r = 1 * fontFactor / 2;
                        auto p = QPointF{ startX + r, startY };
                        painter_->setPen(Qt::NoPen);
                        painter_->setBrush(color);
                        for (;;)
                        {
                            painter_->drawEllipse(p, r, r);
                            p.rx() += r * 4;
                            if (p.x() + r > endX)
                                break;
                        }
                    }
                }
            }

            if (_needsSpace && _word.isSpaceAfter())
                point_.rx() += addSpace_;
        }

        void TextWordRenderer::drawEmoji(const TextWord& _word, const bool _needsSpace)
        {
            const auto b = Utils::scale_bitmap_ratio();
            auto emoji = Emoji::GetEmoji(_word.getCode(), _word.emojiSize() * b);
            Utils::check_pixel_ratio(emoji);

            auto y = std::round(point_.y() + (lineHeight_ / 2.0 - emoji.height() / 2.0 / b) + getYDiff());
            if (pos_ == VerPosition::MIDDLE) //todo: handle other cases
                y -= lineHeight_ / 2.0;
            else if (pos_ == VerPosition::BASELINE)
                y -= textAscent(_word.getFont());
            else if (pos_ == VerPosition::BOTTOM)
                y -= lineHeight_;

            const bool selected = _word.isFullSelected() && selectColor_.isValid();
            const bool highlighted = _word.isFullHighlighted() && highlightColor_.isValid();
            if (selected || highlighted)
            {
                const QRectF rect(
                    point_.x() - getExtraSpace() / 2,
                    point_.y() - selectionDiff_ + getYDiff(),
                    _word.cachedWidth() + getExtraSpace() / 2,
                    lineHeight_ + getYDiff());

                if (highlighted)
                    painter_->fillRect(rect.adjusted(0, 0, _word.isSpaceHighlighted() ? addSpace_ : 0, 0), highlightColor_);
                if (selected)
                    painter_->fillRect(rect.adjusted(0, 0, _word.isSpaceSelected() ? addSpace_ : 0, 0), selectColor_);
            }

            if (_word.underline())
            {
                QString someSpaces;
                someSpaces += QChar::Space;
                const auto& fontMetrics = getMetrics(_word.getFont());
                while (fontMetrics.horizontalAdvance(someSpaces) <= emoji.width() / b)
                    someSpaces += QChar::Space;
                if (!_needsSpace && _word.isSpaceAfter())
                    someSpaces += QChar::Space;

                QVarLengthArray<int> visualOrder;
                prepareEngine(someSpaces, _word.getFont(), visualOrder);

                for (auto item : visualOrder)
                {
                    const auto [gf, f] = prepareGlyph(*d->engine_, item);
                    const auto underlineY = point_.y() + getVerticalShift(lineHeight_, pos_, gf) - lineSpacing_ / 2;
                    painter_->setPen((selected || highlighted) ? _word.getColor() : linkColor_);
                    painter_->drawTextItem(QPointF(point_.x() - 1., underlineY), gf);
                }
            }

            const auto imageRect = QRectF(point_.x(), y, emoji.width() / b, emoji.height() / b);
            painter_->drawImage(imageRect, emoji);

            point_.rx() += roundToInt(_word.cachedWidth()) + addSpace_;
        }

        void TextWordRenderer::split(const QString& _text, const int _from, const int _to, const QColor& _textColor, const QColor& _fillColor, SpellError _e)
        {
            const auto processFillRightSide = [](const auto _it, const auto& _oldFill)
            {
                for (auto f : _oldFill)
                {
                    if (f.ref_.isEmpty())
                    {
                        _it->fill_.push_back(std::move(f));
                    }
                    else if (f.ref_.position() + f.ref_.size() >= _it->from())
                    {
                        f.ref_ = f.ref_.mid(_it->from());
                        _it->fill_.push_back(std::move(f));
                    }
                }
            };

            const int prevSize = textParts_.size() - 1;
            for (int i = prevSize; i >= 0; --i)
            {
                auto& p = textParts_[i];

                if (_to <= p.from() || _from >= p.to()) //not included
                    continue;

                const auto r = p.to();
                if (_from <= p.from() && _to >= p.to()) //full included
                {
                    p.textColor_ = _textColor;
                    p.fillEntirely(_fillColor);
                    p.hasSpellError_ |= SpellError::Yes == _e;
                }
                else if (_from > p.from() && _to < p.to()) //inside
                {
                    auto partFill = p.fill_;
                    p.truncate(_from - p.from());
                    auto it = textParts_.insert(std::next(textParts_.begin(), i + 1), TextPart(_text.midRef(_from, _to - _from), _textColor, _fillColor));
                    it->hasSpellError_ = SpellError::Yes == _e || p.hasSpellError_;
                    it = textParts_.insert(std::next(textParts_.begin(), i + 2), TextPart(_text.midRef(_to, r - _to), p.textColor_));
                    it->hasSpellError_ = p.hasSpellError_;
                    processFillRightSide(it, partFill);
                }
                else if (_from <= p.from() && _to < p.to()) //left
                {
                    auto partFill = p.fill_;
                    p.truncate(_to - p.from());
                    p.hasSpellError_ |= SpellError::Yes == _e;

                    auto it = textParts_.insert(std::next(textParts_.begin(), i + 1), TextPart(_text.midRef(_to, r - _to), p.textColor_));
                    if (SpellError::No == _e)
                        it->hasSpellError_ = p.hasSpellError_;
                    processFillRightSide(it, partFill);

                    p.textColor_ = _textColor;
                    p.fillEntirely(_fillColor);
                }
                else //right
                {
                    p.truncate(_from - p.from());
                    p.hasSpellError_ |= SpellError::Yes == _e;
                    auto it = textParts_.insert(std::next(textParts_.begin(), i + 1), TextPart(_text.midRef(_from,  r - _from), _textColor, _fillColor));
                    it->hasSpellError_ = SpellError::Yes == _e || p.hasSpellError_;
                }
            }
        }

        void TextWordRenderer::fill(const QString& _text, const int _from, const int _to, const QColor& _fillColor, SpellError _e)
        {
            for (auto& p : textParts_)
            {
                if (_to <= p.from() || _from >= p.to()) //not included
                    continue;

                if (_from <= p.from() && _to >= p.to()) //full included
                {
                    p.fillEntirely(_fillColor);
                }
                else //partially included
                {
                    const int from = std::clamp(_from, p.from(), p.to());
                    const int to = std::clamp(_to, p.from(), p.to());
                    if (from != to)
                        p.fill_.push_back({ _text.midRef(from, to - from), _fillColor });
                }
            }
        }

        void TextWordRenderer::prepareEngine(const QString& _text, const QFont& _font, Out QVarLengthArray<int>& _visualOrder)
        {
            auto& engine = d->engine_;
            if (!engine || engine->font() != _font)
            {
                engine = std::make_unique<QStackTextEngine>(QString(), _font);
                engine->option.setTextDirection(Qt::LeftToRight);
                engine->fnt = _font;
            }

            engine->text = _text;
            engine->layoutData = nullptr;
            engine->itemize();

            // in engine.itemize() memory for engine.layoutData is allocated (ALOT!)
            d->releaseMem_ = std::unique_ptr<QTextEngine::LayoutData>(engine->layoutData);

            QScriptLine line;
            line.length = _text.size();
            engine->shapeLine(line);

            const int nItems = engine->layoutData->items.size();
            _visualOrder.resize(nItems);
            QVarLengthArray<uchar> levels(nItems);
            for (int i = 0; i < nItems; ++i)
                levels[i] = engine->layoutData->items[i].analysis.bidiLevel;
            QTextEngine::bidiReorder(nItems, levels.data(), _visualOrder.data());
        }
    }
}

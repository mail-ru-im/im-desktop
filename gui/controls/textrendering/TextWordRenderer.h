#pragma once

#include "TextRendering.h"

namespace TextWordRendererPrivate
{
    constexpr auto preallocTextPartsCount = 16;

    struct TextPart
    {
        struct Fill
        {
            QStringRef ref_;
            QColor color_;
        };

        QStringRef ref_;
        QColor textColor_;
        QVarLengthArray<Fill, preallocTextPartsCount> fill_;
        bool hasSpellError_ = false;

        TextPart() = default;
        TextPart(const QStringRef& _ref, const QColor& _textColor, const QColor& _fillColor = QColor())
            : ref_(_ref)
            , textColor_(_textColor)
        {
            fillEntirely(_fillColor);
        }

        void fillEntirely(const QColor& _fillColor)
        {
            fill_.clear();
            if (_fillColor.isValid())
                fill_.push_back({ QStringRef(), _fillColor });
        }
        int from() const { return ref_.position(); }
        int to() const { return ref_.position() + ref_.size(); }
        void truncate(const int _pos)
        {
            ref_.truncate(_pos);

            for (auto it = fill_.begin(); it != fill_.end();)
            {
                if (!it->ref_.isEmpty())
                {
                    it->ref_.truncate(from() + _pos - it->ref_.position());
                    if (it->ref_.isEmpty())
                    {
                        it = fill_.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
    };
}

namespace Ui
{
    namespace TextRendering
    {
        struct TextWordRenderer_p;

        class TextWordRenderer
        {
        public:
            TextWordRenderer(
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
                );

            ~TextWordRenderer();

            void draw(const TextWord& _word, const bool _needsSpace = true);
            void setPoint(const QPointF& _point) { point_ = _point; }

        private:
            void drawWord(const TextWord& _word, const bool _needsSpace);
            void drawEmoji(const TextWord& _word, const bool _needsSpace);

            enum class SpellError
            {
                No,
                Yes
            };

            void split(const QString& _text, const int _from, const int _to, const QColor& _textColor, const QColor& _fillColor, SpellError _e = SpellError::No);
            void fill(const QString& _text, const int _from, const int _to, const QColor& _fillColor, SpellError _e = SpellError::No);

            void prepareEngine(const QString& _text, const QFont& _font, Out QVarLengthArray<int>& _visualOrder);

        private:
            QPainter* painter_;
            QPointF point_;
            int lineHeight_;
            int lineSpacing_;
            int selectionDiff_;
            VerPosition pos_;

            QColor selectColor_;
            QColor highlightColor_;
            QColor hightlightTextColor_;
            QColor linkColor_;

            QVarLengthArray<TextWordRendererPrivate::TextPart, TextWordRendererPrivate::preallocTextPartsCount> textParts_;
            int addSpace_;

            std::unique_ptr<TextWordRenderer_p> d;
        };
    }
}
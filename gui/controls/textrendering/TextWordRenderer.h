#pragma once

#include "TextRendering.h"

namespace TextWordRendererPrivate
{
    constexpr auto fillTypesCount = 2;
    constexpr auto textPartsCount = (fillTypesCount * 2) + 1 + 1;

    struct TextPart
    {
        struct Fill
        {
            QStringRef ref_;
            QColor color_;
        };

        QStringRef ref_;
        QColor textColor_;
        QVarLengthArray<Fill, fillTypesCount> fill_;

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

            void split(const QString& _text, const int _from, const int _to, const QColor& _textColor, const QColor& _fillColor);
            void fill(const QString& _text, const int _from, const int _to, const QColor& _fillColor);

            std::pair<QVarLengthArray<int>, int> prepareEngine(const QString& _text, const QFont& _font);

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

            QVarLengthArray<TextWordRendererPrivate::TextPart, TextWordRendererPrivate::textPartsCount> textParts_;
            int addSpace_;

            std::unique_ptr<TextWordRenderer_p> d;
        };
    }
}
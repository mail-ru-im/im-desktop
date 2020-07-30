#pragma once

namespace Utils
{

    class TextHeightMetrics
    {
    public:
        TextHeightMetrics(const int32_t heightLines, const int32_t lineHeight);

        int32_t getHeightPx() const;

        int32_t getHeightLines() const;

        int32_t getLineHeight() const;

        bool isEmpty() const;

        TextHeightMetrics linesNumberChanged(const int32_t delta);

    private:
        int32_t HeightLines_;

        int32_t LineHeight_;
    };

    int applyMultilineTextFix(const int32_t textHeight, const int32_t contentHeight);

    TextHeightMetrics evaluateTextHeightMetrics(const QString &text, const int32_t textWidth, const QFontMetrics &fontMetrics);

    int32_t evaluateActualLineHeight(const QFontMetrics &_m);

    int32_t evaluateActualLineHeight(const QFont &_font);

    QString limitLinesNumber(
        const QString &text,
        const int32_t textWidth,
        const QFontMetrics &fontMetrics,
        const int32_t maxLinesNumber,
        const QString &ellipsis);

}
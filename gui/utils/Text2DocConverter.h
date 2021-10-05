#pragma once

#include "../cache/emoji/Emoji.h"
#include "../common.shared/message_processing/text_formatting.h"

namespace Ui
{
    class TextEditEx;
}

namespace Emoji
{
    class EmojiCode;
}

namespace Logic
{
    using Text2HtmlUriCallback = std::function<void(const QString &uri, const int startPos)>;
    using ResourceMap = std::map<QString, QString>;

    struct Replacement
    {
        qint64 sourcePos_ = 0;
        qint64 sourceSize_ = 0;
        qint64 replacementSize_ = 0;
    };
    using ReplacementsInfo = std::vector<Replacement>;

    enum class Text2DocHtmlMode
    {
        Invalid,
        Min,

        Escape = Min,
        Pass,

        Max = Pass
    };

    bool hasBoldStyle(const QTextCharFormat& _format);

    bool hasUnderlineStyle(const QTextCharFormat& _format);

    bool hasMonospaceStyle(const QTextCharFormat& _format);

    void setBoldStyle(QTextCharFormat& _format, bool _isOn);

    void setUnderlineStyle(QTextCharFormat& _format, bool _isOn);

    void setMonospaceStyle(QTextCharFormat& _format, bool _isOn);

    void setMonospaceFont(QTextCharFormat& _charFormat, bool _isOn);

    core::data::format_type getBlockType(const QTextBlock& _block);

    core::data::format_type getBlockType(const QTextBlockFormat& _blockFormat);

    std::pair<bool, core::data::format_data> hasStyle(const QTextCharFormat& _charFormat, core::data::format_type _type);

    class TextBlockFormat : public QTextBlockFormat
    {
    public:
        TextBlockFormat(const QTextBlockFormat& _other) : QTextBlockFormat(_other) { }

        core::data::format_type type() const;

        bool isFirst() const;

        bool isLast() const;

        void setIsFirst(bool _isFirst, bool _updateMargins = true);

        void setIsLast(bool _isLast, bool _updateMargins = true);

        void setType(core::data::format_type _type, bool _updateMargins = true);

        void set(core::data::format_type _type, bool _isFirst, bool _isLast);

        int vPadding() const;

    protected:
        void setMargins(const QMargins& _margins);;

        void updateMargins();
    };

    ReplacementsInfo Text4Edit(
        const QString& _text,
        Ui::TextEditEx& _edit,
        const Text2DocHtmlMode _htmlMode = Text2DocHtmlMode::Escape,
        const bool _convertLinks = true,
        const bool _breakDocument = false,
        const Text2HtmlUriCallback _uriCallback = nullptr,
        const Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto,
        const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBottom,
        bool _placeEachNewLineInSeparateTextBlock = true);

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, const Text2DocHtmlMode _htmlMode, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize);
    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, QTextCursor& _cursor, const Text2DocHtmlMode _htmlMode, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize);

    void Text4EditEmoji(const QString& text, Ui::TextEditEx& _edit, Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto, const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBottom);

    void Text2Doc(
        const QString &text,
        QTextCursor &cursor,
        const Text2DocHtmlMode htmlMode = Text2DocHtmlMode::Escape,
        const bool convertLinks = true,
        const Text2HtmlUriCallback uriCallback = nullptr,
        const Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto);

    void CutText(
        const QString &text,
        const QString &_term,
        const int _width,
        QFontMetrics _base_text_metrics,
        QFontMetrics _term_metrics,
        QTextCursor &cursor,
        const Text2DocHtmlMode htmlMode,
        const bool convertLinks,
        const Text2HtmlUriCallback uriCallback,
        QString& leftPart,
        QString& rightPart,
        QString& termPart);

    ResourceMap InsertEmoji(const Emoji::EmojiCode& _code, Ui::TextEditEx& _edit);
}

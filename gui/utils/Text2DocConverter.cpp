#include "stdafx.h"

#include "Text2DocConverter.h"
#include "fonts.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/emoji/EmojiDb.h"
#include "../controls/TextEditEx.h"
#include "../controls/textrendering/FormattedTextRendering.h"
#include "../utils/log/log.h"
#include "../utils/UrlParser.h"
#include "../utils/SChar.h"
#include "../url_config.h"
#include "../types/message.h"

namespace
{
    using namespace Logic;
    using ftype = core::data::format_type;

    constexpr auto textFormatLogicalBoldProperty = QTextFormat::UserProperty + 0x000001;
    constexpr auto textFormatLogicalUnderlineProperty = QTextFormat::UserProperty + 0x000002;
    constexpr auto textFormatLogicalMonospaceProperty = QTextFormat::UserProperty + 0x000003;

    constexpr auto textBlockFormatFirst = QTextFormat::UserProperty + 0x000011;
    constexpr auto textBlockFormatLast = QTextFormat::UserProperty + 0x000012;

    constexpr auto textBlockIsQuote = QTextFormat::UserProperty + 0x000020;
    constexpr auto textBlockIsCode = QTextFormat::UserProperty + 0x000021;
    constexpr auto textBlockIsUnorderedList = QTextFormat::UserProperty + 0x000022;
    constexpr auto textBlockIsOrderedList = QTextFormat::UserProperty + 0x000023;

    class Text2DocConverter
    {
    public:
        Text2DocConverter();

        void Convert(const QString& text, QTextCursor& cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment, bool _placeEachNewLineInSeparateTextBlock = false);

        void ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment);

        const ResourceMap& InsertEmoji(const Emoji::EmojiCode& _code, QTextCursor& cursor);

        void MakeUniqueResources(const bool make);

        const ResourceMap& GetResources() const;

        static bool AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled);

        int GetSymbolWidth(const QString &text, int& ind, const QFontMetrics& metrics, const Text2DocHtmlMode htmlMode
        , const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment, bool _toRight);

        void setMentions(Data::MentionMap _mentions);

        const ReplacementsInfo& replacements() const { return replacements_; }

    private:

        int ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment);

        bool ParseUrl(bool breakDocument);
        int ParseMention();

        bool ConvertNewline(bool _placeEachNewLineInSeparateTextBlock);

        void ConvertPlainCharacter(const bool isWordWrapEnabled);

        bool IsEos(const int offset = 0) const;

        bool IsHtmlEscapingEnabled() const;

        void FlushBuffers();

        Utils::SChar PeekSChar();

        Emoji::EmojiCode PeekEmojiCode();
        Emoji::EmojiCode ReadEmojiCode();

        void PushInputCursor();

        void PopInputCursor(const int steps = 1);

        Utils::SChar ReadSChar();

        QString ReadString(const int length);

        int ReplaceEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment);
        QTextImageFormat handleEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment);

        void SaveAsHtml(const QString& _text, const QString& _url, bool _isEmail, bool _isWordWrapEnabled);

        void ConvertMention(QStringView _sn, const QString& _friendly);

        QTextStream Input_;

        QTextCursor Writer_;

        qint64 WriterPosAfterFlush_ = 0;

        QString LastWord_;

        QString Buffer_;

        QString UrlAccum_;

        QString TmpBuf_;

        Text2HtmlUriCallback UriCallback_;

        std::vector<int> InputCursorStack_;

        Text2DocHtmlMode HtmlMode_;

        ResourceMap Resources_;
        Data::MentionMap mentions_;

        bool MakeUniqResources_;

        Utils::UrlParser parser_;

        ReplacementsInfo replacements_;
    };

    void ReplaceUrlSpec(const QString &url, QString &out, bool isWordWrapEnabled = false);

    const auto SPACE_ENTITY = ql1s("<span style='white-space: pre'>&#32;</span>");
}

namespace Logic
{
    bool hasBoldStyle(const QTextCharFormat& _format)
    {
        return _format.boolProperty(textFormatLogicalBoldProperty);
    }

    bool hasUnderlineStyle(const QTextCharFormat& _format)
    {
        return _format.boolProperty(textFormatLogicalUnderlineProperty);
    }

    bool hasMonospaceStyle(const QTextCharFormat& _format)
    {
        return _format.boolProperty(textFormatLogicalMonospaceProperty);
    }

    void setBoldStyle(QTextCharFormat& _format, bool _isOn)
    {
        _format.setProperty(textFormatLogicalBoldProperty, _isOn);
        _format.setFontWeight(_isOn ? QFont::Weight::Bold : QFont::Weight::Normal);
    }

    void setUnderlineStyle(QTextCharFormat& _format, bool _isOn)
    {
        _format.setProperty(textFormatLogicalUnderlineProperty, _isOn);
        _format.setFontUnderline(_isOn || hasStyle(_format, ftype::link).first);
    }

    void setMonospaceStyle(QTextCharFormat& _format, bool _isOn)
    {
        _format.setProperty(textFormatLogicalMonospaceProperty, _isOn);
        _format.setFontFamily(_isOn ? Fonts::getInputMonospaceTextFont().family() : Fonts::getInputTextFont().family());
    }

    void setMonospaceFont(QTextCharFormat& _charFormat, bool _isOn)
    {
        const auto fontUnderline = _charFormat.fontUnderline();
        auto font = _charFormat.font();
        const auto newFont = _isOn ? Fonts::getInputMonospaceTextFont() : Fonts::getInputTextFont();
        font.setFamily(newFont.family());
        font.setPixelSize(newFont.pixelSize());
        _charFormat.setFont(font);
        _charFormat.setFontUnderline(fontUnderline);
    }

    core::data::format_type getBlockType(const QTextBlock& _block)
    {
        return getBlockType(_block.blockFormat());
    }

    core::data::format_type getBlockType(const QTextBlockFormat& _blockFormat)
    {
        if (_blockFormat.boolProperty(textBlockIsQuote))
            return ftype::quote;
        else if (_blockFormat.boolProperty(textBlockIsCode))
            return ftype::pre;
        else if (_blockFormat.boolProperty(textBlockIsUnorderedList))
            return ftype::unordered_list;
        else if (_blockFormat.boolProperty(textBlockIsOrderedList))
            return ftype::ordered_list;
        else
            return ftype::none;
    }

    std::pair<bool, core::data::format_data> hasStyle(const QTextCharFormat& _charFormat, ftype _type)
    {
        im_assert(core::data::is_style(_type));

        const auto url = _charFormat.anchorHref();
        switch (_type)
        {
        case ftype::italic:
            return { _charFormat.fontItalic(),{} };
        case ftype::bold:
            return { Logic::hasBoldStyle(_charFormat),{} };
        case ftype::underline:
            return { Logic::hasUnderlineStyle(_charFormat),{} };
        case ftype::monospace:
            return { Logic::hasMonospaceStyle(_charFormat),{} };
        case ftype::strikethrough:
            return { _charFormat.fontStrikeOut(),{} };
        case ftype::mention:
            return { (!url.isEmpty() && Utils::isMentionLink(url)), url.toStdString() };
        case ftype::link:
            return { (!url.isEmpty() && !Utils::isMentionLink(url)), url.toStdString() };
        default:
            return { false,{} };
        }
    }

    ReplacementsInfo Text4Edit(
        const QString& _text,
        Ui::TextEditEx& _edit,
        const Text2DocHtmlMode _htmlMode,
        const bool _convertLinks,
        const bool _breakDocument,
        const Text2HtmlUriCallback _uriCallback,
        const Emoji::EmojiSizePx _emojiSize,
        const QTextCharFormat::VerticalAlignment _alignment,
        bool _placeEachNewLineInSeparateTextBlock)
    {
        auto result = ReplacementsInfo();
        {
            QSignalBlocker sbEdit(_edit);
            QSignalBlocker sbDoc(_edit.document());

            _edit.setUpdatesEnabled(false);

            Text2DocConverter converter;
            converter.MakeUniqueResources(true);
            converter.setMentions(_edit.getMentions());

            auto cursor = _edit.textCursor();
            cursor.beginEditBlock();
            converter.Convert(_text, cursor, _htmlMode, _convertLinks, _breakDocument, _uriCallback, _emojiSize, _alignment, _placeEachNewLineInSeparateTextBlock);
            cursor.endEditBlock();

            _edit.mergeResources(converter.GetResources());
            _edit.setUpdatesEnabled(true);
            result = converter.replacements();
        }

        Q_EMIT (_edit.document()->contentsChanged());
        return result;
    }

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, const Text2DocHtmlMode _htmlMode, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize)
    {
        auto cursor = _edit.textCursor();
        Text4Edit(_text, _edit, cursor, _htmlMode, _convertLinks, _emojiSize);
    }

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, QTextCursor& _cursor, const Text2DocHtmlMode _htmlMode, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize)
    {
        {
            QSignalBlocker sbEdit(_edit);
            QSignalBlocker sbDoc(_edit.document());

            _edit.setUpdatesEnabled(false);

            Text2DocConverter converter;
            converter.MakeUniqueResources(true);
            converter.setMentions(_edit.getMentions());

            _cursor.beginEditBlock();

            converter.Convert(_text, _cursor, _htmlMode, _convertLinks, false, nullptr, _emojiSize, QTextCharFormat::AlignBottom);

            _cursor.endEditBlock();

            _edit.mergeResources(converter.GetResources());

            _edit.setUpdatesEnabled(true);
        }

        Q_EMIT (_edit.document()->contentsChanged());
    }

    void Text4EditEmoji(const QString& text, Ui::TextEditEx& _edit, Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment)
    {
        {
            QSignalBlocker sbEdit(_edit);
            QSignalBlocker sbDocument(_edit.document());

            _edit.setUpdatesEnabled(false);

            Text2DocConverter converter;
            converter.MakeUniqueResources(true);

            auto cursor = _edit.textCursor();
            cursor.beginEditBlock();

            converter.ConvertEmoji(text, cursor, _emojiSize, _alignment);

            cursor.endEditBlock();

            _edit.mergeResources(converter.GetResources());
            _edit.setUpdatesEnabled(true);
        }
        Q_EMIT (_edit.document()->contentsChanged());
    }

    void Text2Doc(
        const QString &text,
        QTextCursor &cursor,
        const Text2DocHtmlMode htmlMode,
        const bool convertLinks,
        const Text2HtmlUriCallback uriCallback,
        const Emoji::EmojiSizePx _emojiSize)
    {
        Text2DocConverter converter;
        {
            QSignalBlocker sb(cursor.document());
            converter.Convert(text, cursor, htmlMode, convertLinks, false, uriCallback, _emojiSize, QTextCharFormat::AlignBottom);
        }
        Q_EMIT (cursor.document()->contentsChanged());
    }

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
        QString& termPart)
    {
        Text2DocConverter converter;
        auto term_pos = text.indexOf(_term, 0, Qt::CaseInsensitive);

        auto term_in_text = text.mid(term_pos, _term.size());

        // calc part of term
        auto symb_index = term_pos;
        auto term_width = 0;
        while (term_width < _width && symb_index < term_pos + term_in_text.size())
        {
            auto width = converter.GetSymbolWidth(text, symb_index, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom, true);
            term_width += width;
        }

        termPart = text.mid(term_pos, symb_index - term_pos);
        if (term_width >= _width)
        {
            return;
        }

        // left and right part
        if (symb_index == term_in_text.size() + term_pos)
        {
            auto right_width = 0;
            auto left_width = 0;
            auto rigth_pos = term_in_text.size() + term_pos;
            auto left_pos = term_pos;

            while (right_width + left_width + term_width < _width)
            {
                auto copy_rigth_pos = rigth_pos;
                if (rigth_pos < text.size())
                {
                    right_width += converter.GetSymbolWidth(text, rigth_pos, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom, true);
                    if (right_width + left_width + term_width > _width)
                    {
                        rigth_pos = copy_rigth_pos;
                        break;
                    }
                }

                auto copy_left_pos = left_pos;
                if (left_pos > 0)
                {
                    left_width += converter.GetSymbolWidth(text, left_pos, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom, false);
                    if (right_width + left_width + term_width > _width)
                    {
                        left_pos = copy_left_pos;
                        break;
                    }
                }

                if (left_pos == 0 && rigth_pos >= text.size())
                    break;
            }

            leftPart = text.mid(left_pos, term_pos - left_pos);
            rightPart = text.mid(term_in_text.size() + term_pos, rigth_pos - term_in_text.size() - term_pos);
        }
    }

    ResourceMap InsertEmoji(const Emoji::EmojiCode& _code, Ui::TextEditEx& _edit)
    {
        Text2DocConverter converter;
        converter.MakeUniqueResources(true);
        QTextCursor cursor = _edit.textCursor();
        return converter.InsertEmoji(_code, cursor);
    }

    core::data::format_type TextBlockFormat::type() const
    {
        return getBlockType(*this);
    }

    bool TextBlockFormat::isFirst() const
    {
        return boolProperty(textBlockFormatFirst);
    }

    bool TextBlockFormat::isLast() const
    {
        return boolProperty(textBlockFormatLast);
    }

    void TextBlockFormat::setIsFirst(bool _isFirst, bool _updateMargins)
    {
        setProperty(textBlockFormatFirst, _isFirst);

        if (_updateMargins)
            updateMargins();
    }

    void TextBlockFormat::setIsLast(bool _isLast, bool _updateMargins)
    {
        setProperty(textBlockFormatLast, _isLast);

        if (_updateMargins)
            updateMargins();
    }

    void TextBlockFormat::setType(core::data::format_type _type, bool _updateMargins)
    {
        *this = QTextBlockFormat();
        if (ftype::quote == _type)
            setProperty(textBlockIsQuote, true);
        else if (ftype::pre == _type)
            setProperty(textBlockIsCode, true);
        else if (ftype::unordered_list == _type)
            setProperty(textBlockIsUnorderedList, true);
        else if (ftype::ordered_list == _type)
            setProperty(textBlockIsOrderedList, true);

        if (_updateMargins)
            updateMargins();
    }

    void TextBlockFormat::set(ftype _type, bool _isFirst, bool _isLast)
    {
        setType(_type, false);
        setIsFirst(_type == ftype::none ? false : _isFirst, false);
        setIsLast(_type == ftype::none ? false : _isLast, false);
        updateMargins();
    }

    int TextBlockFormat::vPadding() const
    {
        return type() == ftype::pre
            ? Ui::TextRendering::getPreVPadding()
            : 0;
    }

    void TextBlockFormat::setMargins(const QMargins& _margins)
    {
        setLeftMargin(_margins.left());
        setRightMargin(_margins.right());
        setTopMargin(_margins.top());
        setBottomMargin(_margins.bottom());
    }

    void TextBlockFormat::updateMargins()
    {
        auto margins = QMargins();
        const auto blockType = type();

        if (ftype::none != blockType)
        {
            const auto baseVMargin = Ui::TextRendering::getParagraphVMargin();
            margins.setTop(isFirst() ? baseVMargin : 0);
            margins.setBottom(isLast() ? baseVMargin : 0);
        }

        if (blockType == ftype::quote)
        {
            margins.setLeft(Ui::TextRendering::getQuoteTextLeftMargin());
        }
        else if (blockType == ftype::pre)
        {
            const auto horizontal = Ui::TextRendering::getPreHPadding();
            margins.setLeft(horizontal);
            margins.setRight(horizontal);

            if (isFirst())
                margins.setTop(margins.top() + vPadding());

            if (isLast())
                margins.setBottom(margins.bottom() + vPadding());
        }
        else if (blockType == ftype::unordered_list)
        {
            margins.setLeft(Ui::TextRendering::getUnorderedListLeftMargin());
        }
        else if (blockType == ftype::ordered_list)
        {
            margins.setLeft(Ui::TextRendering::getOrderedListLeftMargin());
        }

        setMargins(margins);
    }
}

namespace
{
    const auto WORD_WRAP_LIMIT = 15;

    Text2DocConverter::Text2DocConverter()
        : HtmlMode_(Text2DocHtmlMode::Pass)
        , MakeUniqResources_(false)
    {
        // allow downsizing without reallocation
        const auto DEFAULT_SIZE = 1024;
        UrlAccum_.reserve(DEFAULT_SIZE);
        TmpBuf_.reserve(DEFAULT_SIZE);
        LastWord_.reserve(DEFAULT_SIZE);
        Buffer_.reserve(DEFAULT_SIZE);

        // must be sufficient
        InputCursorStack_.reserve(32);
    }

    void Text2DocConverter::Convert(const QString& text, QTextCursor& cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment, bool _placeEachNewLineInSeparateTextBlock)
    {
        im_assert(!UriCallback_);
        im_assert(htmlMode >= Text2DocHtmlMode::Min);
        im_assert(htmlMode <= Text2DocHtmlMode::Max);

        Input_.setString((QString*)&text, QIODevice::ReadOnly);
        Writer_ = cursor;
        UriCallback_ = uriCallback;
        HtmlMode_ = htmlMode;

        while (!IsEos())
        {
            const auto sourcePos = Input_.pos();
            auto cursorPos = qint64(Writer_.position()) + Buffer_.size();
            InputCursorStack_.resize(0);
            auto needReplacement = false;
            auto replacementSize = 0;

            if (ConvertNewline(_placeEachNewLineInSeparateTextBlock))
                needReplacement = false;
            else if (replacementSize = ParseMention())
                needReplacement = true;
            else if (convertLinks && ParseUrl(breakDocument))
                needReplacement = true;
            else if (replacementSize = ConvertEmoji(_emojiSize, _alignment))
                needReplacement = true;
            else
                ConvertPlainCharacter(breakDocument);

            if (needReplacement && replacementSize > 0)
            {
                cursorPos = qMax(cursorPos, WriterPosAfterFlush_);
                const auto sourceSize = Input_.pos() - sourcePos;
                im_assert(sourceSize > 0);
                im_assert(replacementSize > 0);
                replacements_.push_back({sourcePos, sourceSize, replacementSize});
            }
        }

        FlushBuffers();

        InputCursorStack_.resize(0);
        TmpBuf_.resize(0);
        LastWord_.resize(0);
        Buffer_.resize(0);
        UrlAccum_.resize(0);
        UriCallback_ = nullptr;
    }

    void Text2DocConverter::ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);
        Writer_ = cursor;

        while (!IsEos())
        {
            InputCursorStack_.resize(0);
            if (ConvertEmoji(_emojiSize, _alignment))
                continue;

            Buffer_ += ReadSChar().ToQString();
        }

        FlushBuffers();

        InputCursorStack_.resize(0);
        TmpBuf_.resize(0);
        LastWord_.resize(0);
        Buffer_.resize(0);
        UrlAccum_.resize(0);
    }

    int Text2DocConverter::GetSymbolWidth(const QString &text, int& ind, const QFontMetrics& metrics, const Text2DocHtmlMode htmlMode
        , const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment, bool _toRight)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);

        if (_toRight)
        {
            Input_.seek(ind);

            const auto ch = PeekSChar();

            if (!platform::is_apple() && ch.IsEmoji())
            {
                ind += 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
                return (int32_t)_emojiSize;
            }

            auto result = metrics.horizontalAdvance(text.mid(ind, 1));
            ind += 1;
            return result;
        }
        else
        {
            if (ind != 1)
            {
                Input_.seek(ind - 2);
                const auto ch = PeekSChar();

                if (!platform::is_apple() && ch.IsEmoji())
                {
                    ind -= 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
                    return (int32_t)_emojiSize;
                }
            }

            Input_.seek(ind - 1);
            const auto ch = PeekSChar();

            if (!platform::is_apple() && ch.IsEmoji())
            {
                ind += 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
                return (int32_t)_emojiSize;
            }

            auto result = metrics.horizontalAdvance(text.mid(ind, 1));
            ind -= 1;
            return result;
        }
    }

    void Text2DocConverter::setMentions(Data::MentionMap _mentions)
    {
        mentions_ = std::move(_mentions);
    }

    bool Text2DocConverter::AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled)
    {
        if constexpr (platform::is_apple())
        {
            if (isWordWrapEnabled && !word.isEmpty() && (word.length() % WORD_WRAP_LIMIT == 0))
            {
                output += QChar::SoftHyphen;
                return true;
            }
            return false;
        }

        const auto applyHyphen = (isWordWrapEnabled && (word.length() >= WORD_WRAP_LIMIT));
        if (applyHyphen)
            output += QChar::SoftHyphen;
        return applyHyphen;
    }

    const ResourceMap& Text2DocConverter::InsertEmoji(const Emoji::EmojiCode& _code, QTextCursor& _cursor)
    {
        Writer_ = _cursor;
        ReplaceEmoji(_code, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom);
        return Resources_;
    }

    int Text2DocConverter::ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment)
    {
        if (const auto code = PeekEmojiCode(); !code.isNull())
        {
            ReadEmojiCode();
            return ReplaceEmoji(code, _emojiSize, _alignment);
        }
        return 0;
    }

    bool Text2DocConverter::ParseUrl(bool breakDocument)
    {
        if (!IsHtmlEscapingEnabled())
            return false;

        const bool isWordWrapEnabled = breakDocument;

        PushInputCursor();

        parser_.reset();

        QString buf;

        auto onUrlFound = [isWordWrapEnabled, this]()
        {
            PopInputCursor();
            const auto url = parser_.rawUrlString();
            const auto charsProcessed = url.size();
            if (!Input_.seek(Input_.pos() + charsProcessed))
                Input_.readAll(); // end of stream
            SaveAsHtml(url, parser_.formattedUrl(), parser_.isEmail(), isWordWrapEnabled);
        };

        while (true)
        {
            if (Input_.atEnd())
            {
                if (parser_.hasUrl())
                {
                    onUrlFound();
                    return true;
                }
                else
                {
                    PopInputCursor();
                    return false;
                }
            }

            QString s;
            Input_ >> s;

            parser_(s);

            if (parser_.hasUrl())
            {
                onUrlFound();
                return true;
            }
            else
            {
                PopInputCursor();
                return false;
            }

            buf += s;
        }
    }

    int Text2DocConverter::ParseMention()
    {
        if (mentions_.empty())
            return 0;

        PushInputCursor();
        const auto c1 = ReadSChar();
        if (!c1.IsAtSign() || c1.IsNull())
        {
            PopInputCursor();
            return 0;
        }

        const auto c2 = ReadSChar();
        if (!c2.EqualTo('[') || c2.IsNull())
        {
            PopInputCursor();
            return 0;
        }

        static QString buf;
        buf.reserve(100);
        buf.clear();

        bool endFound = false;

        while (!Input_.atEnd())
        {
            const auto ch = ReadSChar();
            buf += ch.ToQString();

            if (ch.EqualTo(']'))
            {
                endFound = true;
                break;
            }
        }

        if (buf.isEmpty() || !endFound || (endFound && buf.size() == sizeof(']')))
        {
            PopInputCursor();
            return 0;
        }

        const auto view = QStringView(buf).left(buf.length() - 1);
        const auto it = mentions_.find(view);
        if (it == mentions_.end())
        {
            PopInputCursor();
            return 0;
        }

        ConvertMention(view, it->second);
        return it->second.size();
    }

    bool Text2DocConverter::ConvertNewline(bool _placeEachNewLineInSeparateTextBlock)
    {
        const auto ch = PeekSChar();

        const auto isNewline = (ch.IsNewline() || ch.IsCarriageReturn());
        if (!isNewline)
            return false;

        ReadSChar();

        auto insertNewLine = [this, _placeEachNewLineInSeparateTextBlock] ()
        {
            if (_placeEachNewLineInSeparateTextBlock)
            {
                FlushBuffers();
                auto blockFormat = TextBlockFormat(Writer_.blockFormat());

                const auto wasLast = blockFormat.isLast();
                if (wasLast)
                    blockFormat.setIsLast(false);
                Writer_.setBlockFormat(blockFormat);

                blockFormat.setIsFirst(false);
                blockFormat.setIsLast(wasLast);
                Writer_.insertBlock(blockFormat);
            }
            else if (IsHtmlEscapingEnabled())
            {
                Buffer_ += ql1s("<br/>");
            }
            else
            {
                Buffer_ += ql1c('\n');
            }
        };

        insertNewLine();
        LastWord_.resize(0);

        if (!ch.IsCarriageReturn())
            return true;

        const auto hasTrailingNewline = PeekSChar().IsNewline();
        if (hasTrailingNewline)
        {
            // eat a newline after the \r because we had been processed it already
            ReadSChar();
        }

        return true;
    }

    void Text2DocConverter::ConvertPlainCharacter(const bool isWordWrapEnabled)
    {
        const auto nextChar = ReadSChar();
        const auto nextCharStr = nextChar.ToQString();
        const auto isNextCharSpace = nextChar.IsSpace();
        const auto isEmoji = nextChar.IsEmoji();
        const auto isDelimeter = nextChar.IsDelimeter();

        if (isNextCharSpace || isDelimeter)
        {
            LastWord_.resize(0);
        }
        else
        {
            LastWord_ += IsHtmlEscapingEnabled() ? nextCharStr.toHtmlEscaped() : nextCharStr;

            if (!isEmoji && !platform::is_apple())
                AddSoftHyphenIfNeed(LastWord_, LastWord_, isWordWrapEnabled);
        }

        if (IsHtmlEscapingEnabled())
        {
            if (isNextCharSpace)
                Buffer_ += SPACE_ENTITY;
            else
                Buffer_ += nextCharStr.toHtmlEscaped();
        }
        else
        {
            Buffer_ += nextCharStr;
        }

        if (!isEmoji || platform::is_apple())
            AddSoftHyphenIfNeed(Buffer_, LastWord_, isWordWrapEnabled);
    }

    bool Text2DocConverter::IsEos(const int offset) const
    {
        im_assert(offset >= 0);
        im_assert(Input_.string());

        const auto &text = *Input_.string();
        return ((Input_.pos() + offset) >= text.length());
    }

    bool Text2DocConverter::IsHtmlEscapingEnabled() const
    {
        im_assert(HtmlMode_ >= Text2DocHtmlMode::Min);
        im_assert(HtmlMode_ <= Text2DocHtmlMode::Max);

        return (HtmlMode_ == Text2DocHtmlMode::Escape);
    }

    void Text2DocConverter::FlushBuffers()
    {
        if (Buffer_.isEmpty())
            return;

        Writer_.setCharFormat(QTextCharFormat());

        if (IsHtmlEscapingEnabled())
            Writer_.insertHtml(Buffer_);
        else
            Writer_.insertText(Buffer_);

        WriterPosAfterFlush_ = Writer_.position();

        Buffer_.resize(0);
        LastWord_.resize(0);
    }

    Utils::SChar Text2DocConverter::PeekSChar()
    {
        return Utils::PeekNextSuperChar(Input_);
    }

    // duplicate in TextRendering.cpp

    static Emoji::EmojiCode getEmoji(QTextStream &s)
    {
        size_t i = 0;
        Emoji::EmojiCode current;
        std::array<qint64, Emoji::EmojiCode::maxSize()> prevPositions = { 0 };

        do
        {
            prevPositions[i] = s.pos();
            if (const auto codePoint = Utils::ReadNextCodepoint(s))
                current = Emoji::EmojiCode::addCodePoint(current, codePoint);
            else
                break;
        } while (++i < Emoji::EmojiCode::maxSize());

        while (i > 0)
        {
            if (Emoji::isEmoji(current))
                return current;
            current = current.chopped(1);
            s.seek(prevPositions[--i]);
        }
        s.seek(prevPositions.front());
        return Emoji::EmojiCode();
    }

    Emoji::EmojiCode Text2DocConverter::PeekEmojiCode()
    {
        const auto pos = Input_.pos();
        const auto code = ReadEmojiCode();
        Input_.seek(pos);

        return code;
    }

    Emoji::EmojiCode Text2DocConverter::ReadEmojiCode()
    {
        return getEmoji(Input_);
    }

    // end of 'duplicate in TextRendering.cpp'

    void Text2DocConverter::PushInputCursor()
    {
        InputCursorStack_.push_back((int)Input_.pos());
        im_assert(InputCursorStack_.size() < InputCursorStack_.capacity());
    }

    void Text2DocConverter::PopInputCursor(const int steps)
    {
        im_assert(!InputCursorStack_.empty());
        im_assert(steps > 0);
        im_assert(steps < 100);

        const auto newSize = (InputCursorStack_.size() - (unsigned)steps);
        const auto storedPos = InputCursorStack_[newSize];
        InputCursorStack_.resize(newSize);

        const auto success = Input_.seek(storedPos);
        im_assert(success);
        (void)success;
    }

    Utils::SChar Text2DocConverter::ReadSChar()
    {
        return Utils::ReadNextSuperChar(Input_);
    }

    QString Text2DocConverter::ReadString(const int length)
    {
        return Input_.read(length);
    }

    QTextImageFormat Text2DocConverter::handleEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment)
    {
        auto img = Emoji::GetEmoji(_code, _emojiSize);
        Utils::check_pixel_ratio(img);

        static int64_t uniq_index = 0;

        QString resource_name;
        if (MakeUniqResources_)
        {
            auto str = Emoji::EmojiCode::toQString(_code);
            resource_name = str % u'_' % QString::number(++uniq_index);
            Resources_[resource_name] = std::move(str);
        }
        else
        {
            resource_name = QString::number(img.cacheKey());
        }

        Writer_.document()->addResource(QTextDocument::ImageResource, QUrl(resource_name), img);

        QTextImageFormat format;
        format.setName(resource_name);
        format.setVerticalAlignment(_alignment);
        return format;
    }

    int Text2DocConverter::ReplaceEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _alignment)
    {
        im_assert(!_code.isNull());
        FlushBuffers();
        const auto charFormat = Writer_.charFormat();
        Writer_.insertText(QString(QChar::ObjectReplacementCharacter), handleEmoji(_code, _emojiSize, _alignment));
        Writer_.setCharFormat(charFormat);
        return 1; // size of QChar
    }

    void Text2DocConverter::SaveAsHtml(const QString& _text, const QString& _url, bool _isEmail, bool _isWordWrapEnabled)
    {
        QString displayText;
        ReplaceUrlSpec(_text, displayText, _isWordWrapEnabled);

        const auto bufferPos1 = Buffer_.length();

        const QStringView prefix = _isEmail ? u"<a type=\"email\" href=\"mailto:" : u"<a href=\"";

        Buffer_ += prefix % _url % u"\">" % displayText % u"</a>";

        const auto bufferPos2 = Buffer_.length();

        if (UriCallback_)
            UriCallback_(Buffer_.mid(bufferPos1, bufferPos2 - bufferPos1), Writer_.position());
    }

    void Text2DocConverter::ConvertMention(QStringView _sn, const QString& _friendly)
    {
        Buffer_ += u"<a href=\"@[" % _sn % u"]\">" % _friendly.toHtmlEscaped() % u"</a>";
    }

    void Text2DocConverter::MakeUniqueResources(const bool _make)
    {
        MakeUniqResources_ = _make;
    }

    const ResourceMap& Text2DocConverter::GetResources() const
    {
        return Resources_;
    }
}

namespace
{
    void ReplaceUrlSpec(const QString &url, QString &out, bool isWordWrapEnabled)
    {
        im_assert(out.isEmpty());
        im_assert(!url.isEmpty());

        static const auto AMP_ENTITY = ql1s("&amp;");

        for (QChar ch : url)
        {
            switch (ch.unicode())
            {
                case '&':
                    out += AMP_ENTITY;
                    break;

                default:
                    out += ch;
                    break;
            }

            if (isWordWrapEnabled && !ch.isHighSurrogate())
                Text2DocConverter::AddSoftHyphenIfNeed(out, out, isWordWrapEnabled);
        }
    }
}

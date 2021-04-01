#include "stdafx.h"

#include "../../common.shared/url_parser/url_parser.h"

#include "Text2DocConverter.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/emoji/EmojiDb.h"
#include "../controls/TextEditEx.h"
#include "../utils/log/log.h"
#include "../utils/SChar.h"
#include "../url_config.h"

namespace
{
    using namespace Logic;

    class Text2DocConverter
    {
    public:
        Text2DocConverter();

        void Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

        void ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

        const ResourceMap& InsertEmoji(const Emoji::EmojiCode& _code, QTextCursor& cursor);

        void MakeUniqueResources(const bool make);

        const ResourceMap& GetResources() const;

        static bool AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled);

        int GetSymbolWidth(const QString &text, int& ind, const QFontMetrics& metrics, const Text2DocHtmlMode htmlMode
        , const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment, bool _to_right);

        void setMentions(Data::MentionMap _mentions);

    private:

        bool ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

        bool ParseUrl(bool breakDocument);
        bool ParseMention();

        bool ConvertNewline();

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

        void ReplaceEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

        void SaveAsHtml(const QString& _text, const common::tools::url& _url, bool isWordWrapEnabled);

        void ConvertMention(QStringView _sn, const QString& _friendly);

        QTextStream Input_;

        QTextCursor Writer_;

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

        common::tools::url_parser parser_;
    };

    void ReplaceUrlSpec(const QString &url, QString &out, bool isWordWrapEnabled = false);

    const auto SPACE_ENTITY = ql1s("<span style='white-space: pre'>&#32;</span>");
}

namespace Logic
{
    void Text4Edit(
        const QString& _text,
        Ui::TextEditEx& _edit,
        const Text2DocHtmlMode _htmlMode,
        const bool _convertLinks,
        const bool _breakDocument,
        const Text2HtmlUriCallback _uriCallback,
        const Emoji::EmojiSizePx _emojiSize,
        const QTextCharFormat::VerticalAlignment _aligment)
    {
        {
            QSignalBlocker sbEdit(_edit);
            QSignalBlocker sbDoc(_edit.document());

            _edit.setUpdatesEnabled(false);

            Text2DocConverter converter;
            converter.MakeUniqueResources(true);
            converter.setMentions(_edit.getMentions());

            auto cursor = _edit.textCursor();
            cursor.beginEditBlock();

            converter.Convert(_text, cursor, _htmlMode, _convertLinks, _breakDocument, _uriCallback, _emojiSize, _aligment);

            cursor.endEditBlock();

            _edit.mergeResources(converter.GetResources());

            _edit.setUpdatesEnabled(true);
        }

        Q_EMIT (_edit.document()->contentsChanged());
    }

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, const Text2DocHtmlMode _htmlMode, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize)
    {
        auto cur = _edit.textCursor();
        Text4Edit(_text, _edit, cur, _htmlMode, _convertLinks, _emojiSize);
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

    void Text4EditEmoji(const QString& text, Ui::TextEditEx& _edit, Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        {
            QSignalBlocker sbEdit(_edit);
            QSignalBlocker sbDocument(_edit.document());

            _edit.setUpdatesEnabled(false);

            Text2DocConverter converter;
            converter.MakeUniqueResources(true);

            auto cursor = _edit.textCursor();
            cursor.beginEditBlock();

            converter.ConvertEmoji(text, cursor, _emojiSize, _aligment);

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
}

namespace
{
    const auto WORD_WRAP_LIMIT = 15;

    Text2DocConverter::Text2DocConverter()
        : HtmlMode_(Text2DocHtmlMode::Pass)
        , MakeUniqResources_(false)
        , parser_(Ui::getUrlConfig().getUrlFilesParser().toStdString())
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

    void Text2DocConverter::Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
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
            InputCursorStack_.resize(0);

            if (ConvertNewline())
            {
                continue;
            }

            if (ParseMention())
            {
                continue;
            }

            if (convertLinks && ParseUrl(breakDocument))
            {
                continue;
            }

            if (//!platform::is_apple() &&
                ConvertEmoji(_emojiSize, _aligment))
            {
                continue;
            }

            ConvertPlainCharacter(breakDocument);
        }

        FlushBuffers();

        InputCursorStack_.resize(0);
        TmpBuf_.resize(0);
        LastWord_.resize(0);
        Buffer_.resize(0);
        UrlAccum_.resize(0);
        UriCallback_ = nullptr;
    }

    void Text2DocConverter::ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);
        Writer_ = cursor;

        while (!IsEos())
        {
            InputCursorStack_.resize(0);
            if (//!platform::is_apple() &&
                ConvertEmoji(_emojiSize, _aligment))
            {
                continue;
            }

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
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment, bool _to_right)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);

        if (_to_right)
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
        {
            output += QChar::SoftHyphen;
        }
        return applyHyphen;
    }

    const ResourceMap& Text2DocConverter::InsertEmoji(const Emoji::EmojiCode& _code, QTextCursor& _cursor)
    {
        Writer_ = _cursor;

        ReplaceEmoji(_code, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom);

        return Resources_;
    }

    bool Text2DocConverter::ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        const auto code = PeekEmojiCode();

        if (!code.isNull())
        {
            ReadEmojiCode();
            ReplaceEmoji(code, _emojiSize, _aligment);
            return true;
        }

        return false;
    }

    bool Text2DocConverter::ParseUrl(bool breakDocument)
    {
        if (!IsHtmlEscapingEnabled())
        {
            return false;
        }

        const bool isWordWrapEnabled = breakDocument;

        PushInputCursor();

        parser_.reset();

        QString buf;

        auto onUrlFound = [isWordWrapEnabled, this]()
        {
            PopInputCursor();
            const auto& url = parser_.get_url();
            const auto urlAsString = QString::fromUtf8(url.original_.c_str(), url.original_.size());
            const auto charsProcessed = urlAsString.size();
            if (!Input_.seek(Input_.pos() + charsProcessed))
            {
                Input_.readAll(); // end of stream
            }
            SaveAsHtml(urlAsString, url, isWordWrapEnabled);
        };

        auto getChar = [this]()
        {
            QString buf;

            QChar c1;
            Input_ >> c1;
            buf.append(c1);

            if (c1.isHighSurrogate())
            {
                im_assert(!Input_.atEnd());
                if (!Input_.atEnd())
                {
                    QChar c2;
                    Input_ >> c2;
                    buf.append(c2);
                }
            }

            return buf;
        };

        while (true)
        {
            if (Input_.atEnd())
            {
                parser_.finish();

                if (parser_.has_url())
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

            const auto charAsStr = getChar();
            const auto utf8 = charAsStr.toUtf8();
            for (char c : utf8)
                parser_.process(c);

            if (parser_.skipping_chars())
            {
                PopInputCursor();
                return false;
            }

            if (parser_.has_url())
            {
                onUrlFound();
                return true;
            }

            buf += charAsStr;
        }
    }

    bool Text2DocConverter::ParseMention()
    {
        if (mentions_.empty())
            return false;

        PushInputCursor();
        const auto c1 = ReadSChar();
        if (!c1.IsAtSign() || c1.IsNull())
        {
            PopInputCursor();
            return false;
        }

        const auto c2 = ReadSChar();
        if (!c2.EqualTo('[') || c2.IsNull())
        {
            PopInputCursor();
            return false;
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
            return false;
        }

        const auto view = QStringView(buf).left(buf.length() - 1);
        const auto it = mentions_.find(view);
        if (it == mentions_.end())
        {
            PopInputCursor();
            return false;
        }

        ConvertMention(view, it->second);
        return true;
    }

    bool Text2DocConverter::ConvertNewline()
    {
        const auto ch = PeekSChar();

        const auto isNewline = (ch.IsNewline() || ch.IsCarriageReturn());
        if (!isNewline)
        {
            return false;
        }

        ReadSChar();

        auto insertNewLine = [this] ()
        {
            if (IsHtmlEscapingEnabled())
            {
                static const auto HTML_NEWLINE = ql1s("<br/>");
                Buffer_ += HTML_NEWLINE;
            }
            else
            {
                static const auto TEXT_NEWLINE = ql1c('\n');
                Buffer_ += TEXT_NEWLINE;
            }
        };

        insertNewLine();
        LastWord_.resize(0);

        if (PeekSChar().IsEmoji())
            insertNewLine();

        if (!ch.IsCarriageReturn())
        {
            return true;
        }

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
            if (IsHtmlEscapingEnabled())
            {
                const auto htmlEscaped = nextCharStr.toHtmlEscaped();

                LastWord_ += htmlEscaped;
            }
            else
            {
                LastWord_ += nextCharStr;
            }

            if (!isEmoji && !platform::is_apple())
            {
                AddSoftHyphenIfNeed(LastWord_, LastWord_, isWordWrapEnabled);
            }
        }

        if (IsHtmlEscapingEnabled())
        {
            if (isNextCharSpace)
            {
                Buffer_ += SPACE_ENTITY;
            }
            else
            {
                const auto htmlEscaped = nextCharStr.toHtmlEscaped();
                Buffer_ += htmlEscaped;
            }
        }
        else
        {
            Buffer_ += nextCharStr;
        }

        if (!isEmoji || platform::is_apple())
        {
            AddSoftHyphenIfNeed(Buffer_, LastWord_, isWordWrapEnabled);
        }

        /* this crushes emojis with skin tones
        const auto applyOsxEmojiFix = (
            platform::is_apple() &&
            isEmoji &&
            !Buffer_.endsWith(QChar::SoftHyphen));
        if (applyOsxEmojiFix)
        {
            Buffer_ += QChar::SoftHyphen;
        }
        */

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

    void Text2DocConverter::ReplaceEmoji(const Emoji::EmojiCode& _code, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        im_assert(!_code.isNull());

        FlushBuffers();

        const auto charFormat = Writer_.charFormat();

        QImage img = Emoji::GetEmoji(_code, _emojiSize);
        Utils::check_pixel_ratio(img);

        static int64_t uniq_index = 0;

        QString resource_name;
        if (!MakeUniqResources_)
        {
            resource_name = QString::number(img.cacheKey());
        }
        else
        {
            QString str = Emoji::EmojiCode::toQString(_code);
            resource_name = str % u'_' % QString::number(++uniq_index);
            Resources_[resource_name] = std::move(str);
        }

        Writer_.document()->addResource(
            QTextDocument::ImageResource,
            QUrl(resource_name),
            img
        );

        QTextImageFormat format;
        format.setName(resource_name);
        format.setVerticalAlignment(_aligment);
        Writer_.insertText(QString(QChar::ObjectReplacementCharacter), format);

        Writer_.setCharFormat(charFormat);
    }

    void Text2DocConverter::SaveAsHtml(const QString& _text, const common::tools::url& _url, bool isWordWrapEnabled)
    {
        QString displayText;
        ReplaceUrlSpec(_text, displayText, isWordWrapEnabled);

        const auto bufferPos1 = Buffer_.length();

        const QStringView prefix = _url.is_email() ? u"<a type=\"email\" href=\"mailto:" : u"<a href=\"";

        Buffer_ += prefix % QString::fromUtf8(_url.url_.c_str()) % u"\">" % displayText % u"</a>";

        const auto bufferPos2 = Buffer_.length();

        if (UriCallback_)
        {
            UriCallback_(Buffer_.mid(bufferPos1, bufferPos2 - bufferPos1), Writer_.position());
        }
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
            {
                Text2DocConverter::AddSoftHyphenIfNeed(out, out, isWordWrapEnabled);
            }
        }
    }
}

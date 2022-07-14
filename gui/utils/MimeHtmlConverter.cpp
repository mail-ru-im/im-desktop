#include "stdafx.h"
#include "MimeHtmlConverter.h"
#include "utils/utils.h"

namespace
{
    static constexpr QStringView headTagFirst = u"<head>";
    static constexpr QStringView headTagLast = u"</head>";

    static constexpr QStringView boldOpen = u"<b>";
    static constexpr QStringView boldClose = u"</b>";
    static constexpr QStringView italicOpen = u"<i>";
    static constexpr QStringView italicClose = u"</i>";
    static constexpr QStringView underlineOpen = u"<u>";
    static constexpr QStringView underlineClose = u"</u>";
}

namespace Utils
{
    class TextDocumentVisitor
    {
        class BitMatrix
        {
        public:
            BitMatrix() : rows_(0), cols_(0) {}
            BitMatrix(int _rows, int _cols, bool _value = true) :
                bits_(_rows * _cols, _value),
                rows_(_rows),
                cols_(_cols)
            {}

            bool test(int _row, int _column) const
            {
                const int idx = (_row * cols_ + _column);
                im_assert(idx >= 0 && idx < bits_.size());
                return bits_.testBit(idx);
            }

            void set(int _row, int _column, bool _value = true)
            {
                const int idx = _row * cols_ + _column;
                im_assert(idx >= 0 && idx < bits_.size());
                bits_.setBit(idx, _value);
            }

            void spanRows(int _column, int _row, int _spanCount, bool _value)
            {
                im_assert(_row >= 0 && _row < rows_);
                im_assert(_column >= 0 && _column < cols_);
                im_assert((_row + _spanCount) <= rows_);

                _row = qBound(0, _row, rows_);
                _column = qBound(0, _column, cols_);
                _spanCount = qBound(0, _row + _spanCount, rows_) - _row;

                for (int s = 0; s < _spanCount; ++s)
                    bits_.setBit((_row + s) * cols_ + _column, _value);
            }

            void spanColumns(int _row, int _column, int _spanCount, bool _value)
            {
                im_assert(_row >= 0 && _row < rows_);
                im_assert(_column >= 0 && _column < cols_);
                im_assert((_column + _spanCount) <= cols_);

                _row = qBound(0, _row, rows_);
                _column = qBound(0, _column, cols_);
                _spanCount = qBound(0, _column + _spanCount, cols_) - _column;

                const int idx = _row * cols_ + _column;
                bits_.fill(_value, idx, idx + _spanCount);
            }

        private:
            QBitArray bits_;
            int rows_, cols_;
        };

        static BitMatrix createTableBits(const QTextTable* _table, Qt::Orientations spanHints = Qt::Vertical|Qt::Horizontal)
        {
            const int rows = _table->rows();
            const int cols = _table->columns();

            QTextTableCell cell;
            BitMatrix bits(rows, cols, true);
            if (spanHints & Qt::Horizontal)
            {
                // traverse row-major
                for (int i = 0; i < rows; ++i)
                {
                    int span = 1;
                    for (int j = 0; j < cols; j += span)
                    {
                        cell = _table->cellAt(i, j);
                        span = cell.columnSpan();

                        if (span > 1)
                            bits.spanColumns(i, j + 1, span - 1, false);
                    }
                }
            }

            if (spanHints & Qt::Vertical)
            {
                // traverse column-major
                for (int i = 0; i < cols; ++i)
                {
                    int span = 1;
                    for (int j = 0; j < rows; j += span)
                    {
                        cell = _table->cellAt(j, i);
                        span = cell.rowSpan();

                        if (bits.test(j, i) && span > 1)
                            bits.spanRows(i, j + 1, span - 1, false);
                    }
                }
            }

            return bits;
        }

        Qt::Orientations tableSpans(const QTextTable* _table) const
        {
            Qt::Orientations result;
            const int rows = _table->rows();
            const int cols = _table->columns();
            QTextTableCell cell;

            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    cell = _table->cellAt(i, j);
                    const int rowSpan = cell.rowSpan();
                    const int colSpan = cell.columnSpan();

                    if (rowSpan > 1)
                        result |= Qt::Vertical;
                    if (colSpan > 1)
                        result |= Qt::Horizontal;

                    if (result == (Qt::Vertical | Qt::Horizontal))
                        return result; // all span combinations found - no need to check more
                }
            }
            return result;
        }


    public:
        Data::FString processDocument(QTextDocument* _doc) const
        {
            if (const QTextFrame* frame = _doc->rootFrame())
            {
                Data::FString::Builder builder;
                visitFrame(frame, builder);
                return builder.finalize();
            }
            return {};
        }

    private:
        void insertFormattedChar(Data::FString::Builder& _builder, QChar _ch, core::data::format_type _type) const
        {
            auto line = Data::FString(1, _ch);
            if (_type != core::data::format_type::none)
                line.addFormat(_type);
            _builder %= line;
        }

        void visitFrame(const QTextFrame* _frame, Data::FString::Builder& _builder) const
        {
            if (_frame == nullptr)
                return;

            const QTextFormat format = _frame->format();
            if (format.isTableFormat())
            {
                visitTable(qobject_cast<const QTextTable*>(_frame), _builder);
                return;
            }

            for (auto it = _frame->begin(); it != _frame->end(); ++it)
            {
                if (it.currentFrame() != nullptr)
                    visitFrame(it.currentFrame(), _builder);
                else
                    visitBlock(it.currentBlock(), _builder, (!it.atEnd() ? ql1c('\n') : ql1c('\0')));
            }
        }

        void visitTable(const QTextTable* _table, Data::FString::Builder& _builder) const
        {
            using ftype = core::data::format_type;

            constexpr QStringView rowSep = u"\n";
            constexpr QStringView columnSep = u"\t";
            constexpr QStringView tableSep = u" ";
            constexpr QStringView placeholder = u" ";

            if (!_table)
                return;

            BitMatrix bits;
            // fast check if we have any spans in table
            const Qt::Orientations spanHints = tableSpans(_table);
            // if we haven't got any spans we do not need
            // to waste any memory nor CPU cycles to create
            // span bit-matrix
            if (spanHints != 0)
                bits = createTableBits(_table, spanHints);

            QTextTableCell cell;
            const int rows = _table->rows();
            const int cols = _table->columns();

            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    if (spanHints != 0 && !bits.test(i, j))
                    {
                        // if we have span at (i, j) append value
                        // placeholder and separator, then continue
                        _builder %= placeholder;
                        if (j != (cols - 1))
                            _builder %= columnSep;
                        continue;
                    }

                    cell = _table->cellAt(i, j);
                    for (auto it = cell.begin(); it != cell.end(); ++it)
                    {
                        if (it.currentFrame() != nullptr)
                            visitFrame(it.currentFrame(), _builder);
                        else
                            visitBlock(it.currentBlock(), _builder, ql1c('\t'));

                    }

                    if (j != (cols - 1))
                        _builder %= columnSep;
                }
                _builder %= rowSep;
            }
            _builder %= tableSep;
        }

        void visitBlock(const QTextBlock& _block,
                        Data::FString::Builder& _builder,
                        QChar _separator = ql1c('\0')) const
        {
            using ftype = core::data::format_type;
            int n = 0;
            const auto blockFType = convertBlockFormatType(_block);
            for (auto it = _block.begin(); it != _block.end(); ++it)
            {
                if (const auto fragment = it.fragment(); fragment.isValid())
                {
                    auto fragmentString = convertFragmentFormat(fragment);
                    if (fragmentString.isEmpty())
                        continue;

                    const auto endsWithLinefeed = Utils::isLineBreak(fragmentString.back());
                    n += fragmentString.size();
                    if (blockFType != ftype::none)
                        fragmentString.addFormat(blockFType);
                    _builder %= fragmentString;
                    if (endsWithLinefeed)
                        _builder %= ql1s("\n");
                }
            }

            if (_separator != 0 && n != 0)
                insertFormattedChar(_builder, _separator, blockFType);
        }


        static core::data::format_type convertBlockFormatType(const QTextBlock& _block)
        {
            using ftype = core::data::format_type;
            const auto blockFormat = _block.blockFormat();
            auto doc = _block.document();

            // check quote
            if (blockFormat.hasProperty(QTextFormat::BlockQuoteLevel))
                return ftype::quote;

            // check pre
            if (blockFormat.hasProperty(QTextFormat::BlockNonBreakableLines))
                return ftype::pre;

            // check lists
            const auto objectIndex = blockFormat.objectIndex();
            if (objectIndex == -1)
                return ftype::none;

            const auto obj = doc->object(objectIndex);
            if (!obj)
                return ftype::none;

            const auto format = obj->format();
            if (format.type() != QTextFormat::ObjectTypes::TableCellObject)
                return ftype::none;

            const bool isTableCellObject = format.type() == QTextFormat::ObjectTypes::TableCellObject;
            const auto objProps = format.properties();
            if (const auto propIt = objProps.find(QTextFormat::ListStyle); propIt != objProps.cend())
            {
                const auto listStyle = propIt.value().toInt();
                switch (listStyle)
                {
                case QTextListFormat::ListDisc:
                case QTextListFormat::ListCircle:
                case QTextListFormat::ListSquare:
                case QTextListFormat::ListStyleUndefined:
                    return ftype::unordered_list;
                case QTextListFormat::ListDecimal:
                case QTextListFormat::ListLowerAlpha:
                case QTextListFormat::ListUpperAlpha:
                case QTextListFormat::ListLowerRoman:
                case QTextListFormat::ListUpperRoman:
                    return ftype::ordered_list;
                }
            }

            return ftype::none;
        }

        //! Extract type from fragment created in QTextDocument::setHtml()
        static Data::FString convertFragmentFormat(const QTextFragment& _fragment)
        {
            using ftype = core::data::format_type;
            const auto charFormat = _fragment.charFormat();

            QString text;
            const QString source = _fragment.text();
            std::copy_if(source.cbegin(), source.cend(), std::back_inserter(text),
                [](const QChar _c) { return (_c.category() != QChar::Symbol_Other); });

            Data::FString result(text);

            const QString href = charFormat.anchorHref();
            if (!href.isEmpty() && !text.isEmpty())
                result.addFormat(ftype::link, href.toStdString());

            if (charFormat.fontWeight() > QFont::Weight::Normal)
                result.addFormat(ftype::bold);

            if (charFormat.fontItalic())
                result.addFormat(ftype::italic);

            if (charFormat.fontUnderline())
                result.addFormat(ftype::underline);

            if (charFormat.fontStrikeOut())
                result.addFormat(ftype::strikethrough);

            // this monospace check most probably doesn't work, but luckily might work out
            if (charFormat.font().styleHint() == QFont::StyleHint::Monospace)
                result.addFormat(ftype::monospace);

            return result;
        }
    };


    class MimeHtmlConverterPrivate : public TextDocumentVisitor
    {
    public:
        QStringView getMsWordListTag(QStringView _part)
        {
            static QString expr = qsl("(?><span style='mso-list:[a-zA-Z0-9]+'>)([^\\<\\>]+)(?><span\\s+|<\\/span>)");
            static QRegularExpression re(expr, QRegularExpression::CaseInsensitiveOption);
            const auto match = re.match(_part);
            if (match.hasMatch())
                return match.capturedView(1).front().isDigit() ? u"ol" : u"ul";
            else
                return u"ol";
        }

        QString cleanMsListItemText(QStringView _text)
        {
            static constexpr QStringView ifBegin = u"<![if !supportLists]>";
            static constexpr QStringView ifEnd = u"<![endif]>";
            QString result = _text.toString();
            int start = result.indexOf(ifBegin);
            int end = -1;
            while (start != -1)
            {
                end = result.indexOf(ifEnd, start);
                if (end == -1)
                    break;
                end += ifEnd.size();
                result = result.remove(start, end - start);
                start = result.indexOf(ifBegin, start);
            }
            return result;
        }

        QStringView searchStyleParam(QStringView _view, QStringView _paramName)
        {
            if (_view.isEmpty() || _paramName.isEmpty())
                return QStringView();

            const auto paramStart = _view.indexOf(_paramName);
            if (paramStart == -1)
                return QStringView();

            const auto paramEnd = _view.indexOf(ql1s(";"), paramStart + _paramName.size());
            if (paramEnd == -1)
                return QStringView();

            return _view.mid(paramStart + _paramName.size(), paramEnd - (paramStart + _paramName.size()));
        }

        auto parseFormats(const QString& _view)
        {
            std::unordered_map<QString, Data::FormatTypes> docFormats;
            static const QString exprStyle = qsl("\\.\\w+\\s.{[\\w\\:\\;\\s.\\-\\,]+}");
            static QRegularExpression re(exprStyle, QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
            QRegularExpressionMatch match;

            for (auto start = _view.indexOf(re, 0, &match); start != -1; start = _view.indexOf(re, start + 1, &match))
            {
                const auto styleCSS = match.capturedView(0);
                const auto styleStart = styleCSS.indexOf(ql1s("{"));
                if (styleStart == -1)
                    break;

                const auto formatName = styleCSS.mid(1, styleStart - 1);

                Data::FormatTypes types;
                if (searchStyleParam(styleCSS, u"font-weight:").toInt() > 400)
                    types.setFlag(core::data::format_type::bold);

                if (searchStyleParam(styleCSS, u"font-style:") == u"italic")
                    types.setFlag(core::data::format_type::italic);

                if (searchStyleParam(styleCSS, u"text-decoration:") == u"underline")
                    types.setFlag(core::data::format_type::underline);

                docFormats[formatName.toString().simplified()] = std::move(types);
            }
            return docFormats;
        }

        /*
        * Head of html stores .css styles for cells and parts of cell-text.
        * We exract styles and add proper tags to cells manually.
        */
        QString preprocessMsExcelFragment(const QString& _html)
        {
            const int startHead = _html.indexOf(headTagFirst);
            const int endHead = _html.indexOf(headTagLast) + headTagLast.size();
            const auto head = _html.mid(startHead, endHead - startHead);
            auto textFormats = parseFormats(head);

            if (textFormats.empty())
                return _html;

            QString result; // cleaned html suitable for Qt parsing
            if (startHead != -1)
            {
                result += _html.mid(0, startHead);
                result += _html.mid(endHead);
            }

            static QString classRegex = ql1s("class=(xl\\d+|\\\"font\\d+\\\")[\\s\\w\\=\\'\\:\\;\\.\\-]*>(.*)<");
            static QRegularExpression re(classRegex, QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
            QRegularExpressionMatch match;

            for (auto start = result.indexOf(re, 0, &match); start != -1; start = result.indexOf(re, start + 1, &match))
            {
                const QString type = match.capturedView(1).toString().replace(qsl("\""), qsl(""));
                const QString text = match.capturedView(2).toString();
                const auto& formats = textFormats[type];
                QString openingFormats;
                QString endingFormats;

                using ft = core::data::format_type;
                auto addTags = [&formats, &openingFormats, &endingFormats](ft _flag, QStringView _opening, QStringView _ending)
                {
                    if (formats.testFlag(_flag))
                    {
                        openingFormats = openingFormats % _opening;
                        endingFormats += _ending;
                    }
                };

                addTags(ft::bold, boldOpen, boldClose);
                addTags(ft::italic, italicOpen, italicClose);
                addTags(ft::underline, underlineOpen, underlineClose);

                result = result.replace(match.capturedStart(2), text.size(), openingFormats % text % endingFormats);
            }

            return result;
        }

        QString preprocessMsWordFragment(const QString& _html)
        {
            static constexpr QStringView pattern = u"<meta name=ProgId content=Word.Document>";
            static constexpr QStringView listFirst = u"MsoListParagraphCxSpFirst";
            static constexpr QStringView listMiddle = u"MsoListParagraphCxSpMiddle";
            static constexpr QStringView listLast = u"MsoListParagraphCxSpLast";

            if (!_html.contains(pattern)) // check if it's really MS Word application
                return _html;

            static QString listItemRegExp = u"<p\\s+(?>class=(" % listFirst % u'|' % listMiddle % u'|' % listLast % u"))(?>[a-zA-Z '-=:]+)>(.*)<\\/p>";
            static QRegularExpression re(listItemRegExp, QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);

            QString result; // cleaned html suitable for Qt parsing

            // erase <head>...</head> since it haven't got any interesting information
            int start = _html.indexOf(headTagFirst);
            int end = _html.indexOf(headTagLast) + headTagLast.size();
            if (start != -1)
            {
                result += _html.mid(0, start);
                result += _html.mid(end);
            }
            result.replace(QChar::LineFeed, QChar::Space); // replace lf with spaces
            result.replace(ql1s("</p>"), ql1s("</p>\n"));  // add lf after paragraphs
            QStringView listTag = u"ul";
            // replace paragraphs with list items
            QRegularExpressionMatch match;
            start = result.indexOf(re, 0, &match);
            while (start != -1)
            {
                QStringView type = match.capturedView(1);
                QString text = cleanMsListItemText(match.capturedView(2));
                if (type == listFirst) // first item
                {
                    listTag = getMsWordListTag(match.capturedView(0));
                    result = result.replace(start, match.capturedLength(0), u'<' % listTag % u"><li>" % text % u"</li>");
                }
                else if (type == listLast) // last item
                {
                    result = result.replace(start, match.capturedLength(0), u"<li>" % text % u"</li></" % listTag % u">");
                    listTag = u"ul"; // restore defaults
                }
                else // middle item
                {
                    result = result.replace(start, match.capturedLength(0), u"<li>" % text % u"</li>");
                }
                start = result.indexOf(re, start, &match);
            }
            return result;
        }

        QString preprocessMsOfficeFragment(const QString& _html)
        {
            return preprocessMsExcelFragment(preprocessMsWordFragment(_html));
        }

        QString preprocessBrTag(const QString& _html)
        {
            // <br> in html is getting lost somehow, so we replace it with <p>
            QString result = _html;
            static const QString brTag = ql1s("<br");
            static const QString endTag = ql1s(">");
            int start = result.indexOf(brTag, 0);
            while (start != -1)
            {
                int end = result.indexOf(endTag, start);
                if (end == -1)
                    break;
                result.insert(end + 1, ql1s("</p>"));
                result.replace(start, brTag.size(), ql1s("<p"));
                start = result.indexOf(brTag, start);
            }

            return result;
        }

        QString preprocessHtml(const QString& _html)
        {
            return preprocessBrTag(preprocessMsOfficeFragment(_html));
        }
    };


    MimeHtmlConverter::MimeHtmlConverter()
        : d(std::make_unique<MimeHtmlConverterPrivate>())
    {}

    MimeHtmlConverter::~MimeHtmlConverter() = default;

    Data::FString MimeHtmlConverter::fromHtml(const QString& _html) const
    {
        auto doc = std::make_unique<QTextDocument>();
        doc->setHtml(d->preprocessHtml(_html));
        return d->processDocument(doc.get());
    }
}

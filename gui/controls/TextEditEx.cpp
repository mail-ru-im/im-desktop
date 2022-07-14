#include "stdafx.h"

#include "gui_settings.h"
#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"
#include "../cache/emoji/Emoji.h"
#include "../fonts.h"
#include "../utils/log/log.h"
#include "../utils/Text2DocConverter.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/features.h"
#include "../utils/MimeHtmlConverter.h"
#include "../utils/MimeDataUtils.h"
#include "../controls/CustomButton.h"
#include "../controls/ContextMenu.h"
#include "../controls/textrendering/FormattedTextRendering.h"
#include "../main_window/history_control/MessageStyle.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/MainWindow.h"

#include "TextEditEx.h"

using ftype = core::data::format_type;

namespace
{
    constexpr bool needShowDebugInfo()
    {
        return false && build::is_debug();
    }

    constexpr QStringView orderedListAutoStartPrefix() { return u"1."; }

    Logic::TextBlockFormat getBlockFormat(const QTextCursor& _cursor)
    {
        return _cursor.blockFormat();
    }

    Logic::TextBlockFormat getBlockFormat(const QTextBlock& _block)
    {
        return _block.blockFormat();
    }

    int getContextMenuMinWidth() noexcept
    {
        return Utils::scale_value(212);
    }

    void mergeCharFormat(QTextCursor& _cursor, QTextCharFormat _format)
    {
        _format.clearProperty(QTextFormat::ImageName);
        _cursor.mergeCharFormat(_format);
    }

    void selectText(QTextCursor& _cursor, int _start, int _end)
    {
        im_assert(_start >= 0);
        im_assert(_start <= _end);
        _cursor.setPosition(_start);
        _cursor.movePosition(QTextCursor::MoveOperation::NextCharacter, QTextCursor::MoveMode::KeepAnchor, _end - _start);
    }

    std::vector<core::data::range> getSubrangesWithDifferentCharFormats(QTextCursor _cursor, core::data::range _range)
    {
        if (_range.size_ < 1)
            return {};

        const auto [offset, length, startIndex] = _range;
        const auto endPos = offset + length;
        auto result = std::vector<core::data::range>();

        selectText(_cursor, offset, offset + 1);
        auto prevFormat = _cursor.charFormat();

        auto subrangeStart = offset;
        auto pos = offset + 1;
        while (pos + 1 <= endPos)
        {
            selectText(_cursor, pos, pos + 1);

            if (auto format = _cursor.charFormat(); pos == endPos || format != prevFormat)
            {
                result.emplace_back(subrangeStart, pos - subrangeStart);
                subrangeStart = pos;
                prevFormat = std::move(format);
            }
            ++pos;
        }

        if (auto length = endPos - subrangeStart; length > 0)
            result.emplace_back(subrangeStart, length);

        return result;
    }

    core::data::range getSelectionRange(const QTextCursor& _cursor)
    {
        const auto pos = _cursor.position();
        const auto anchor = _cursor.anchor();
        return { qMin(pos, anchor), qAbs(pos - anchor) };
    }

    int getBlockEndPosition(const QTextBlock& _block)
    {
        return _block.position() + _block.text().size();
    }

    void setCharFormatForLink(QTextCharFormat& _charFormat, const QString& _url)
    {
        if (!_url.isEmpty())
            im_assert(!Utils::isMentionLink(_url));
        const auto doAddLink = !_url.isEmpty();
        _charFormat.setAnchor(doAddLink);
        _charFormat.setAnchorHref(_url);
        _charFormat.setForeground(doAddLink ? Ui::MessageStyle::getLinkColor() : Ui::MessageStyle::getTextColor());
        _charFormat.setFontUnderline(doAddLink);
    }

    void setCharFormatForMention(QTextCharFormat& _charFormat, const QString& _mention)
    {
        im_assert(_mention.isEmpty() || Utils::isMentionLink(_mention));

        const auto isOn = !_mention.isEmpty();
        _charFormat.setAnchor(isOn);
        _charFormat.setAnchorHref(_mention);
        _charFormat.setForeground(isOn ? Ui::MessageStyle::getLinkColor() : Ui::MessageStyle::getTextColor());
    }

    //! A wrapper for more convenient use
    std::pair<bool, core::data::format_data> hasStyle(QTextCharFormat&& _charFormat, ftype _type)
    {
        return Logic::hasStyle(_charFormat, _type);
    }

    void setTextStyle(QTextCharFormat& _charFormat, ftype _type, bool _isOn, const core::data::format_data _data = {})
    {
        switch (_type)
        {
        case ftype::italic:
            _charFormat.setFontItalic(_isOn);
            break;
        case ftype::bold:
            Logic::setBoldStyle(_charFormat, _isOn);
            break;
        case ftype::underline:
            Logic::setUnderlineStyle(_charFormat, _isOn);
            break;
        case ftype::monospace:
        {
            Logic::setMonospaceStyle(_charFormat, _isOn);
            break;
        }
        case ftype::strikethrough:
            _charFormat.setFontStrikeOut(_isOn);
            break;
        case ftype::mention:
            im_assert(!_isOn || _data);
            setCharFormatForMention(_charFormat, _isOn && _data ? QString::fromStdString(*_data) : QString());
            break;
        case ftype::link:
            im_assert(!_isOn || _data);
            setCharFormatForLink(_charFormat, _isOn && _data ? QString::fromStdString(*_data) : QString());
            break;
        default:
            im_assert(!"not implemented");
        }
    }

    //! Replacement for QTextCursor::setBlockFormat with a hack to fix loosing mention format at line start
    void setBlockFormat(const QTextBlock& _block, QTextCursor _cursor, const QTextBlockFormat& _blockFormat)
    {
        _cursor.setPosition(_block.position());
        _cursor.insertText(qsl("_"));
        _cursor.setBlockFormat(_blockFormat);
        _cursor.deletePreviousChar();
    };

    void clearCharFormats(QTextCharFormat& _format, Data::FormatTypes _exclusions = {})
    {
        for (auto type : core::data::get_all_non_mention_styles())
        {
            if (!_exclusions.testFlag(type) && Logic::hasStyle(_format, type).first)
                setTextStyle(_format, type, false);
        }
    }

    Data::FormatTypes getCurrentNonMentionCursorStyles(const QTextCharFormat& _charFormat)
    {
        Data::FormatTypes result;
        for (auto type : core::data::get_all_non_mention_styles())
        {
            if (Logic::hasStyle(_charFormat, type).first)
                result.setFlag(type);
        }
        return result;
    }

    void applyReplacementShifts(core::data::range& _range, const Logic::ReplacementsInfo& _replacements)
    {
        auto& [offset, length, startIndex] = _range;

        auto offsetDelta = 0;
        auto lengthDelta = 0;
        for (auto [sPos, sSize, rSize] : _replacements)
        {
            if (sPos < offset)
                offsetDelta += rSize - sSize;

            if (sPos >= offset && sPos + sSize <= offset + length)
                lengthDelta += rSize - sSize;
        }
        offset += offsetDelta;
        length += lengthDelta;
    }

    //! Keeps mention and etc formats
    template <typename T>
    void updateCharFormat(QTextCursor _cursor, core::data::range _range, T _updateFunction, ftype ft = ftype::none)
    {
        _cursor.setPosition(_range.offset_);
        _cursor.select(QTextCursor::WordUnderCursor);

        const auto subranges = getSubrangesWithDifferentCharFormats(_cursor, _range);
        for (auto [pos, subrangeSize, startIndex] : subranges)
        {
            auto endPos = pos + subrangeSize;
            int shift = 0;

            _cursor.select(QTextCursor::WordUnderCursor);
            auto currentCharFormat = _cursor.charFormat();
            QTextCharFormat charFormat;


            bool is_mention = Utils::isMentionLink(currentCharFormat.anchorHref()) || ft == ftype::mention;

            if (is_mention)
            {
                shift = 1;
                _cursor.setPosition(pos);
                _cursor.insertText(qsl("_"));
            }

            _updateFunction(charFormat);

            selectText(_cursor, pos + shift , endPos + shift);
            mergeCharFormat(_cursor, charFormat);

            if (is_mention)
            {
                _cursor.setPosition(pos);
                _cursor.deleteChar();
            }
        }
    }

    //! Add break inside current block format if any
    void addLineBreak(QTextCursor _cursor)
    {
        const auto block = _cursor.block();
        auto newBf = getBlockFormat(block);

        if (newBf.type() == ftype::none)
        {
            _cursor.insertBlock();
            return;
        }

        const auto wasLast = newBf.isLast();
        newBf.setIsLast(false);
        setBlockFormat(block, _cursor, newBf);
        newBf.setIsFirst(false);
        newBf.setIsLast(wasLast);
        _cursor.insertBlock(newBf);
    }
}


namespace Ui
{
    TextEditEx::TextEditEx(QWidget* _parent, const QFont& _font, const Styling::ThemeColorKey& _color, bool _input, bool _isFitToText)
        : QTextBrowser(_parent)
        , index_(0)
        , font_(_font)
        , color_{ _color }
        , prevPos_(-1)
        , input_(_input)
        , isFitToText_(_isFitToText)
        , heightSupplement_(0)
        , limit_(-1)
        , prevCursorPos_(0)
        , defaultDocumentMargin_(document()->documentMargin())
        , maxHeight_(Utils::scale_value(250))
    {
        setAttribute(Qt::WA_InputMethodEnabled);
        setInputMethodHints(Qt::ImhMultiLine);

        updatePalette();

        setAcceptRichText(false);
        setFont(font_);
        document()->setDefaultFont(font_);

        if (isFitToText_)
        {
            connect(document(), &QTextDocument::contentsChanged, this, &TextEditEx::editContentChanged, Qt::QueuedConnection);
            // AutoConnection using breaks rename contact dialog. FIXME: editContentChanged or rename contact dialog
        }

        connect(this, &QTextBrowser::textChanged, this, &TextEditEx::onTextChanged);

        connect(this, &QTextBrowser::anchorClicked, this, &TextEditEx::onAnchorClicked);
        connect(this, &TextEditEx::cursorPositionChanged, this, &TextEditEx::onCursorPositionChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearSelection, this, &TextEditEx::clearSelection, Qt::QueuedConnection);

        if (input_)
            connect(document(), &QTextDocument::contentsChange, this, &TextEditEx::onContentsChanged);

        if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
        {
            for (auto [type, action] : mainWindow->getFormatActions())
            {
                connect(action, &QAction::triggered, this, [this, type = type]
                {
                    onFormatUserAction(type);
                });

                // On Mac shortcuts handled in mac_support.mm
                if constexpr (!platform::is_apple())
                    addAction(action);
            }
        }

        onTextChanged();

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &TextEditEx::onThemeChange);

        Styling::setStyleSheet(verticalScrollBar(), Styling::getParameters().getScrollBarQss());
    }

    void TextEditEx::limitCharacters(const int _count)
    {
        limit_ = _count;
    }

    void TextEditEx::discardMentions(const int _pos, const int _len)
    {
        if (_pos < 0 || _len == 0)
            return;

        auto cur = textCursor();
        cur.setPosition(_pos);
        if (Utils::isMentionLink(cur.charFormat().anchorHref()))
        {
            const auto dir = _len > 0 ? QTextCursor::Right : QTextCursor::Left;

            for (auto i = 1; i <= abs(_len); ++i)
            {
                cur.movePosition(dir, QTextCursor::KeepAnchor);
                if (!cur.charFormat().isImageFormat())
                    cur.setCharFormat(QTextCharFormat());
                cur.clearSelection();
            }
        }
    }

    void TextEditEx::editContentChanged()
    {
        const auto docHeight = qBound(Utils::scale_value(20),
            qRound(this->document()->size().height()),
            maxHeight_);

        const auto oldHeight = height();
        setFixedHeight(docHeight + heightSupplement_);
        if (height() != oldHeight)
            Q_EMIT setSize(0, height() - oldHeight);

        if (textCursor().atEnd())
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    void TextEditEx::onAnchorClicked(const QUrl& _url)
    {
        Utils::openUrl(_url.toString());
    }

    void TextEditEx::onCursorPositionChanged()
    {
        auto cursor = textCursor();
        const auto curPos = cursor.position();
        const auto charFormat = cursor.charFormat();
        const auto anchor = charFormat.anchorHref();

        if (Utils::isMentionLink(anchor))
        {
            auto block = cursor.block();
            for (auto itFragment = block.begin(); itFragment != block.end(); ++itFragment)
            {
                QTextFragment cf = itFragment.fragment();

                if (cf.charFormat() == charFormat && cf.charFormat().anchorHref() == anchor)
                {
                    const auto mentionStart = cf.position();
                    const auto mentionEnd = cf.position() + cf.length();

                    auto newPos = -1;
                    if (input_)
                    {
                        const auto cursorNowIn = curPos > mentionStart && curPos < mentionEnd;

                        if (!cursorNowIn)
                            continue;

                        const auto selStart = cursor.selectionStart();
                        const auto selEnd = cursor.selectionEnd();
                        const auto isSelecting = cursorNowIn && cursor.hasSelection();
                        const auto isSelectingByMouse = (QApplication::mouseButtons() & Qt::LeftButton) && curPos > mentionStart && curPos < mentionEnd;

                        const auto movingSelLeft = (curPos < prevCursorPos_ || isSelectingByMouse) && isSelecting && prevCursorPos_ == mentionStart && selEnd >= mentionEnd;
                        const auto movingSelRight = (curPos > prevCursorPos_ || isSelectingByMouse) && isSelecting && prevCursorPos_ == mentionEnd && selStart <= mentionStart;

                        if (movingSelLeft || movingSelRight)
                            newPos = prevCursorPos_;
                        else
                            newPos = prevCursorPos_ > curPos ? mentionStart : mentionEnd;
                    }
                    else
                    {
                        newPos = curPos > 0 ? mentionEnd : mentionStart;
                    }

                    if (newPos != -1 && curPos != newPos)
                    {
                        QSignalBlocker sb(this);
                        cursor.setPosition(newPos, cursor.hasSelection() ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                        setTextCursor(cursor);
                        prevCursorPos_ = newPos;
                    }
                    return;
                }
            }
        }

        prevCursorPos_ = curPos;
    }

    void TextEditEx::onContentsChanged(int _pos, int _removed, int _added)
    {
        if (_added > 0)
            discardMentions(_pos, _added);
    }

    void TextEditEx::onFormatUserAction(core::data::format_type _type)
    {
        if (!hasFocus())
            return;

        if (const auto cursor = textCursor(); cursor.hasSelection())
        {
            formatSelectionUserAction(_type);
        }
        else if (_type == ftype::none)
        {
            blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this, &cursor]()
            {
                const auto block = cursor.block();
                auto blockRange = core::data::range{ block.position(), block.text().size() };
                blockRange = surroundWithNewLinesIfNone(blockRange, true);
                formatRangeClearBlockTypes(blockRange);
                discardAllStylesAtCursor();
                return true;
            });
        }

        if (textCursor().atEnd())
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    void TextEditEx::updatePalette()
    {
        auto p = palette();
        p.setColor(QPalette::Text, Styling::getColor(color_));
        p.setColor(QPalette::Highlight, MessageStyle::getTextSelectionColor());
        p.setColor(QPalette::HighlightedText, MessageStyle::getSelectionFontColor());
        setPalette(p);
    }

    QString TextEditEx::getPlainText(int _from, int _to) const
    {
        QString outString;
        if (_from == _to)
            return outString;

        if (_to != -1 && _to < _from)
        {
            im_assert(!"invalid data");
            return outString;
        }

        QTextStream result(&outString);

        int posStart = 0;
        int length = 0;

        bool first = true;

        for (QTextBlock it_block = document()->begin(); it_block != document()->end(); it_block = it_block.next())
        {
            if (!first)
                result << ql1c('\n');

            posStart = it_block.position();
            if (_to != -1 && posStart >= _to)
                break;

            for (QTextBlock::iterator itFragment = it_block.begin(); itFragment != it_block.end(); ++itFragment)
            {
                QTextFragment currentFragment = itFragment.fragment();

                if (currentFragment.isValid())
                {
                    posStart = currentFragment.position();
                    length = currentFragment.length();

                    if (posStart + length <= _from)
                        continue;

                    if (_to != -1 && posStart >= _to)
                        break;

                    first = false;

                    auto charFormat = currentFragment.charFormat();
                    if (charFormat.isImageFormat())
                    {
                        if (posStart < _from)
                            continue;

                        QTextImageFormat imgFmt = charFormat.toImageFormat();

                        auto iter = resourceIndex_.find(imgFmt.name());
                        if (iter != resourceIndex_.end())
                            result << iter->second;
                    }
                    else if (charFormat.isAnchor() && Utils::isMentionLink(charFormat.anchorHref()))
                    {
                        result << charFormat.anchorHref();
                    }
                    else
                    {
                        int cStart = std::max((_from - posStart), 0);
                        int count = -1;
                        if (_to != -1 && _to <= posStart + length)
                            count = _to - posStart - cStart;

                        QString txt = currentFragment.text().mid(cStart, count);
                        txt.remove(QChar::SoftHyphen);

                        QChar *uc = txt.data();
                        QChar *e = uc + txt.size();

                        for (; uc != e; ++uc)
                        {
                            switch (uc->unicode())
                            {
                            case 0xfdd0: // QTextBeginningOfFrame
                            case 0xfdd1: // QTextEndOfFrame
                            case QChar::ParagraphSeparator:
                            case QChar::LineSeparator:
                                *uc = ql1c('\n');
                                break;
                            case QChar::Nbsp:
                                *uc = ql1c(' ');
                                break;
                            default:
                                ;
                            }
                        }

                        result << txt;
                    }
                }
            }
        }

        return outString;
    }

    Data::FString TextEditEx::getText(int _from, int _to) const
    {
        if (_from == _to)
            return {};

        if (_to != -1 && _to < _from)
        {
            im_assert(!"invalid data");
            return {};
        }
        if (_to == -1)
            _to = toPlainText().size();

        auto blockFormatTypeAccumulated = ftype::none;
        Data::FString::Builder blockBuilder;

        Data::FString::Builder result;
        int posStart = 0;
        int length = 0;
        auto first = true;
        for (auto block = document()->begin(); block != document()->end(); block = block.next())
        {
            posStart = block.position();
            if (posStart > _to)
                break;

            const auto blockFirstPos = block.position();
            const auto blockLastPos = getBlockEndPosition(block);

            if (!first)
                ((blockFormatTypeAccumulated == ftype::none) ? result : blockBuilder) %= QStringView(u"\n");

            const auto bf = Logic::TextBlockFormat(block.blockFormat());
            const auto blockType = bf.type();
            if (bf.type() != ftype::none && _from < blockLastPos)
                blockFormatTypeAccumulated = bf.type();

            for (auto itFragment = block.begin(); itFragment != block.end(); ++itFragment)
            {
                auto fragment = itFragment.fragment();
                if (!fragment.isValid())
                    continue;

                posStart = fragment.position();
                length = fragment.length();

                if (posStart + length <= _from)
                    continue;

                if (posStart > _to)
                    break;

                first = false;

                int cStart = std::max((_from - posStart), 0);
                int count = -1;
                if (_to <= posStart + length)
                    count = _to - posStart - cStart;

                auto txt = fragment.text().mid(cStart, count);
                txt.remove(QChar::SoftHyphen);

                QChar *uc = txt.data();
                QChar *e = uc + txt.size();
                for (; uc != e; ++uc)
                {
                    switch (uc->unicode())
                    {
                    case 0xfdd0: // QTextBeginningOfFrame
                    case 0xfdd1: // QTextEndOfFrame
                    case QChar::ParagraphSeparator:
                    case QChar::LineSeparator:
                        *uc = ql1c('\n');
                        break;
                    case QChar::Nbsp:
                        *uc = ql1c(' ');
                        break;
                    default:;
                    }
                }

                const auto txtView = QStringView(txt);
                auto cursor = textCursor();

                auto getStringFromRange = [&txtView, this, posStart, &cursor](int _begin, int _end = -1) -> Data::FString
                {
                    _end = (_end == -1) ? txtView.size() : _end;
                    selectText(cursor, posStart + _begin, posStart + _begin + 1);
                    return getTextFromFragment(txtView.mid(_begin, _end - _begin).toString(), cursor.charFormat());
                };

                auto start = 0;
                auto end = -1;
                auto& builder = (blockFormatTypeAccumulated == ftype::none) ? result : blockBuilder;
                while ((end = txtView.indexOf(QChar::ObjectReplacementCharacter, start)) > -1)
                {
                    if (end - start > 0)
                        builder %= getStringFromRange(start, end);

                    builder %= getStringFromRange(end, end + 1);
                    start = end + 1;
                }
                if (start < txtView.size())
                    builder %= getStringFromRange(start);
            }

            if (bf.type() != ftype::none && blockFormatTypeAccumulated != ftype::none && (bf.isLast() || _to <= blockLastPos))
            {
                im_assert(blockFormatTypeAccumulated == bf.type());

                auto blockStr = blockBuilder.finalize(true);
                blockStr.addFormat(blockFormatTypeAccumulated);
                result %= std::move(blockStr);

                blockBuilder = Data::FString::Builder();
                blockFormatTypeAccumulated = ftype::none;
            }
            else if (block == document()->end())
            {
                im_assert(blockFormatTypeAccumulated == ftype::none);
            }
        }

        return result.finalize(true);
    }

    QMimeData* TextEditEx::createMimeDataFromSelection() const
    {
        auto mimeData = MimeData::toMimeData(
            getText(textCursor().selectionStart(), textCursor().selectionEnd()),
            mentions_, files_);

        if (needShowDebugInfo())
            Debug::debugFormattedText(mimeData);

        return mimeData;
    }

    void TextEditEx::setLinkToSelection(const QString& _url)
    {
        auto cursor = textCursor();
        const auto range = getSelectionRange(cursor);

        updateCharFormat(cursor, range, [](QTextCharFormat& _charFormat)
        {
            setTextStyle(_charFormat, ftype::mention, false);
        });

        formatRangeStyleType({ ftype::link, range, _url.toStdString() });
        cursor.setPosition(range.offset_ + range.size_);
        setTextCursor(cursor);
        updateCurrentCursorCharFormat();
    }

    bool TextEditEx::isFormatEnabled() const
    {
        return Features::isFormattingInInputEnabled() && isFormatEnabled_;
    }

    void TextEditEx::removeBlockFormatFromCurrentLine(QTextCursor& _cursor)
    {
        auto currentBlock = _cursor.block();

        _cursor.setPosition(currentBlock.position());
        const auto bf = getBlockFormat(_cursor);

        setBlockFormat(currentBlock, _cursor, {});

        if (bf.type() == ftype::pre)
        {
            const auto blockStart = currentBlock.position();
            const auto blockSize = currentBlock.text().size();

            if (blockSize > 0)
            {
                updateCharFormat(_cursor, {blockStart, blockSize}, [](QTextCharFormat& _charFormat)
                {
                    Logic::setMonospaceFont(_charFormat, false);
                });
            }

            setTextCursor(_cursor);
        }

        if (!bf.isLast())
        {
            if (auto nextBlock = currentBlock.next(); !bf.isLast() && nextBlock.isValid())
            {
                if (auto nextBf = getBlockFormat(nextBlock); nextBf.type() == bf.type())
                {
                    nextBf.setIsFirst(true);
                    setBlockFormat(nextBlock, _cursor, nextBf);
                }
            }
        }

        if (!bf.isFirst())
        {
            if (auto prevBlock = currentBlock.previous(); prevBlock.isValid())
            {
                if (auto prevBf = getBlockFormat(prevBlock); prevBf.type() == bf.type())
                {
                    prevBf.setIsLast(true);
                    setBlockFormat(prevBlock, _cursor, prevBf);
                }
            }
        }
    }

    core::data::range TextEditEx::surroundWithNewLinesIfNone(core::data::range _range, bool _onlyIfBlock)
    {
        auto [offset, length, startIndex] = _range;
        const auto selectionEnd = offset + length;

        auto cursor = textCursor();
        auto doc = document();

        auto block = doc->findBlock(offset + length);
        auto blockStart = block.position();
        auto blockEnd = getBlockEndPosition(block);
        if (selectionEnd > blockStart && selectionEnd < blockEnd && (!_onlyIfBlock || Logic::getBlockType(block) != ftype::none))
        {
            cursor.setPosition(offset + length);
            addLineBreak(cursor);
        }

        block = doc->findBlock(offset);
        blockStart = block.position();
        blockEnd = getBlockEndPosition(block);
        if (offset > blockStart && (!_onlyIfBlock || Logic::getBlockType(block) != ftype::none))
        {
            cursor.setPosition(offset);
            addLineBreak(cursor);
            ++offset;
        }
        return {offset, length};
    }

    void TextEditEx::removeBlockFormatFromLine(int _pos)
    {
        auto cursor = textCursor();
        cursor.setPosition(_pos);
        removeBlockFormatFromCurrentLine(cursor);
    }

    void TextEditEx::replaceSelectionBy(const Data::FString& _text)
    {
        auto doc = document();
        auto cursor = textCursor();
        const auto [offset, length, startIndex] = getSelectionRange(cursor);

        const auto firstBlock = doc->findBlock(offset);
        auto firstBF = getBlockFormat(firstBlock);

        const auto lastBlock = doc->findBlock(offset + length);
        auto lastBF = getBlockFormat(lastBlock);

        const auto blockAfterLast = lastBlock.next();

        if (length > 0)
        {
            selectText(cursor, offset, offset + length);
            cursor.removeSelectedText();

            const auto isSelectionBetweenDifferentTypedBlocks = firstBF.type() != lastBF.type();
            const auto isSelectionMultiline = firstBlock != lastBlock;

            if (lastBlock.isValid() && firstBF.isFirst() && !isSelectionBetweenDifferentTypedBlocks)
            {
                lastBF.setIsFirst(true);
                setBlockFormat(lastBlock, cursor, lastBF);
            }

            const auto newFirstBlock = doc->findBlock(offset);
            if (newFirstBlock == firstBlock && firstBlock.isValid() && (isSelectionBetweenDifferentTypedBlocks || (isSelectionMultiline && lastBF.isLast())))
            {
                firstBF.setIsLast(true);
                setBlockFormat(firstBlock, cursor, firstBF);
            }

            if (blockAfterLast.isValid() && isSelectionBetweenDifferentTypedBlocks)
            {
                auto bf = getBlockFormat(blockAfterLast);
                bf.setIsFirst(true);
                setBlockFormat(blockAfterLast, cursor, bf);
            }
        }

        if (!_text.isEmpty())
            insertFormattedText(_text);
    }

    void TextEditEx::onTextChanged()
    {
        clearCache();
        { // Hack to fix incorrect bullet y-placement in a line containing only emojis
            blockNumbersToApplyDescentHack_.clear();
            for (auto block = document()->findBlockByNumber(0); block.isValid(); block = block.next())
            {
                if (auto t = getBlockFormat(block).type(); t == ftype::ordered_list || t == ftype::unordered_list)
                {
                    const auto s = block.text();
                    const auto allAreSpecialChars = !s.isEmpty() && std::all_of(s.cbegin(), s.cend(),
                        [](auto _char) { return _char == QChar::ObjectReplacementCharacter; });

                    if (allAreSpecialChars)
                        blockNumbersToApplyDescentHack_.push_back(block.blockNumber());
                }
            }
        }

        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this]()
        {
            // Clear formats if not text left
            auto cursor = textCursor();
            const auto text = toPlainText();
            auto bf = getBlockFormat(cursor);
            auto wasAnythingChanged = false;

            QSignalBlocker sbEdit(this);
            QSignalBlocker sbDoc(document());

            if (text.isEmpty() && !bf.isEmpty() && !hasJustAutoCreatedList_)
            {
                cursor.setBlockFormat({});
                wasAnythingChanged = true;
            }

            hasJustAutoCreatedList_ = false;

            // Fix for larger margins at top and at bottom via frame as block margins
            // are not taken into account when calculating document height
            if (isFormatEnabled())
            {
                const auto defaultVMargin = defaultDocumentMargin_;

                cursor.setPosition(0);
                const auto topMargin = getBlockFormat(cursor).type() == ftype::pre
                    ? TextRendering::getParagraphVMargin() + defaultVMargin : defaultVMargin;

                cursor.setPosition(text.size());
                const auto bottomMargin = getBlockFormat(cursor).type() == ftype::pre
                    ? TextRendering::getParagraphVMargin() + defaultVMargin : defaultVMargin;

                auto rootFrame = document()->rootFrame();
                auto frameFormat = rootFrame->frameFormat();
                const auto currentTopMargin = frameFormat.topMargin();
                const auto currentBottomMargin = frameFormat.bottomMargin();
                if (currentTopMargin != topMargin || currentBottomMargin != bottomMargin)
                {
                    frameFormat.setTopMargin(topMargin);
                    frameFormat.setBottomMargin(bottomMargin);
                    rootFrame->setFrameFormat(frameFormat);

                    if (text.isEmpty())
                    {
                        // Trick to make frame margins to be updated, as otherwise they are not somehow
                        cursor.insertText(qsl(" "));
                        cursor.deletePreviousChar();
                    }
                    wasAnythingChanged = true;
                }
            }
            return wasAnythingChanged;
        });
    }

    void TextEditEx::discardFormatAtCursor(core::data::format_type _type, bool _atStart, bool _atEnd)
    {
        auto cursor = textCursor();
        auto charFormat = cursor.charFormat();
        const auto pos = cursor.position();

        const auto isAtLineStart = cursor.positionInBlock() == 0;
        const auto hasFormatAtLeft = !isAtLineStart && Logic::hasStyle(charFormat, _type).first;
        auto hasFormatAtRight = false;
        if (pos < toPlainText().size())
        {
            cursor.setPosition(pos + 1);
            if (cursor.positionInBlock() > 0)
                hasFormatAtRight = hasStyle(cursor.charFormat(), _type).first;
        }

        if (hasFormatAtRight == hasFormatAtLeft && !isAtLineStart)
            return;

        if (isAtLineStart || (hasFormatAtRight && _atStart) || (hasFormatAtLeft && _atEnd))
        {
            cursor.setPosition(pos);
            setTextStyle(charFormat, _type, false);
            cursor.setCharFormat(charFormat);
            setTextCursor(cursor);
        }
    }

    void TextEditEx::discardAllStylesAtCursor()
    {
        auto cursor = textCursor();
        auto charFormat = cursor.charFormat();

        for (auto type : core::data::get_all_non_mention_styles())
            setTextStyle(charFormat, type, false);

        cursor.setCharFormat(charFormat);
        setTextCursor(cursor);
    }

    void TextEditEx::updateCurrentCursorCharFormat()
    {
        auto cursor = textCursor();
        auto charFormat = cursor.charFormat();
        if (!isFormatEnabled() || cursor.hasSelection())
            return;

        for (auto type : core::data::get_all_non_mention_styles())
            discardFormatAtCursor(type, true, type == ftype::link);

        if (Logic::TextBlockFormat(cursor.blockFormat()).type() == ftype::pre)
        {
            auto charFormat = cursor.charFormat();
            Logic::setMonospaceFont(charFormat, true);
            cursor.setCharFormat(charFormat);
            setTextCursor(cursor);
        }
    }

    void TextEditEx::insertFromMimeData(const QMimeData* _source)
    {
        if (_source->hasImage() && !Utils::haveText(_source))
        {
            Q_EMIT insertFiles();
            return;
        }

        if (_source->hasUrls())
        {
            const auto urls = _source->urls();
            for (const auto& url : urls)
            {
                if (url.isLocalFile())
                {
                    Q_EMIT insertFiles();
                    return;
                }
            }
        }

        if (!_source->hasText())
            return;

        // avoid double parsing
        if (QString text = _source->text(); Utils::isBase64EncodedImage(text))
        {
            Q_EMIT insertFiles();
            return;
        }

        if (limit_ != -1 && document()->characterCount() >= limit_)
            return;

        QString text;
        core::data::format format;
        bool withMentions = false;

        const auto formatEnabled = isFormatEnabled() && !(QApplication::keyboardModifiers() & Qt::ShiftModifier);

        if (formatEnabled && _source->hasFormat(MimeData::getTextFormatMimeType()))
        {
            if (const auto rawText = _source->data(MimeData::getTextFormatMimeType()); !rawText.isEmpty())
            {
                rapidjson::Document doc;
                doc.Parse(rawText.data());
                im_assert(doc.IsObject());
                if (doc.HasMember("format"))
                    format = core::data::format(doc["format"]);
            }
        }

        if (formatEnabled && _source->hasHtml())
        {
            const auto formats = _source->formats();
            const auto textPriority = formats.indexOf(MimeData::getMimeTextFormat());
            const auto htmlPriority = formats.indexOf(MimeData::getMimeHtmlFormat());
            if (textPriority < 0 || textPriority > htmlPriority)
            {
                Utils::MimeHtmlConverter converter;
                insertFormattedText(converter.fromHtml(_source->html()));
                return;
            }
        }
        if (_source->hasFormat(MimeData::getRawMimeType()))
        {
            if (const auto rawText = _source->data(MimeData::getRawMimeType()); !rawText.isEmpty())
            {
                withMentions = true;
                text = QString::fromUtf8(rawText);

                if (_source->hasFormat(MimeData::getMentionMimeType()))
                {
                    if (const auto mentionsArray = _source->data(MimeData::getMentionMimeType()); !mentionsArray.isEmpty())
                    {
                        auto m = MimeData::convertArrayToMap(mentionsArray);
                        mentions_.insert(std::make_move_iterator(m.begin()), std::make_move_iterator(m.end()));
                    }
                }

                if (_source->hasFormat(MimeData::getFileMimeType()))
                {
                    if (const auto filesArray = _source->data(MimeData::getFileMimeType()); !filesArray.isEmpty())
                    {
                        auto f = MimeData::convertArrayToMap(filesArray);
                        files_.insert(std::make_move_iterator(f.begin()), std::make_move_iterator(f.end()));
                    }
                }
            }
        }
        else
        {
            text = _source->text();
        }

        if (limit_ != -1 && text.length() + document()->characterCount() > limit_)
        {
            const auto maxLen = limit_ - document()->characterCount();
            auto lenToResize = maxLen;

            if (withMentions)
            {
                static constexpr auto mentionMarker = QStringView(u"@[");
                auto ndxStart = text.indexOf(mentionMarker);
                auto ndxEnd = -1;

                if (ndxStart != -1 && ndxStart < maxLen)
                {
                    while (ndxStart != -1)
                    {
                        ndxEnd = text.indexOf(u']', ndxStart + 2);
                        if (ndxEnd != -1)
                        {
                            if (ndxEnd < maxLen)
                            {
                                ndxStart = text.indexOf(mentionMarker, ndxEnd + 1);
                            }
                            else
                            {
                                lenToResize = ndxStart;
                                break;
                            }
                        }
                    }
                }
            }

            text.resize(lenToResize);
        }

        const auto pos = textCursor().hasSelection() ? textCursor().selectionStart() : textCursor().position();

        if (!format.empty())
            insertFormattedText(Data::FString(text, format), QTextCharFormat::AlignBottom, true);
        else if (!text.isEmpty())
            Logic::Text4Edit(text, *this, Logic::Text2DocHtmlMode::Escape, false, false, nullptr, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBottom);

        auto nickPos = -1;
        auto nickRef = QStringRef(&text).trimmed();
        if (nickRef.startsWith(ql1c('@')) && Utils::isNick(nickRef.toString()))
            nickPos = pos + nickRef.position();

        if (nickPos >= 0)
            Q_EMIT nickInserted(nickPos, nickRef.mid(1).toString(), QPrivateSignal());
    }

    void TextEditEx::focusInEvent(QFocusEvent* _event)
    {
        Q_EMIT focusIn();
        QTextBrowser::focusInEvent(_event);
    }

    void TextEditEx::focusOutEvent(QFocusEvent* _event)
    {
        if (_event->reason() != Qt::ActiveWindowFocusReason)
            Q_EMIT focusOut(_event->reason());

        QTextBrowser::focusOutEvent(_event);
    }

    void TextEditEx::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            if (_event->button() == Qt::LeftButton && !Utils::InterConnector::instance().isMultiselect())
                Q_EMIT Utils::InterConnector::instance().clearSelection();

            Q_EMIT clicked();
            QTextBrowser::mousePressEvent(_event);
            if (!input_)
                _event->ignore();
        }
    }

    void TextEditEx::mouseReleaseEvent(QMouseEvent* _event)
    {
        printDebugInfo();

        QTextBrowser::mouseReleaseEvent(_event);
        if (!input_)
            _event->ignore();
    }

    void TextEditEx::mouseMoveEvent(QMouseEvent * _event)
    {
        if (_event->buttons() & Qt::LeftButton && !input_)
            _event->ignore();
        else
            QTextBrowser::mouseMoveEvent(_event);
    }

    void TextEditEx::keyPressEvent(QKeyEvent* _event)
    {
        const auto key = _event->key();
        const auto hasCtrl = _event->modifiers() & Qt::ControlModifier;
        const auto hasShift = _event->modifiers() & Qt::ShiftModifier;
        const auto isEnter = key == Qt::Key_Return || key == Qt::Key_Enter;
        const auto eventText = _event->text();

        if (!eventText.isEmpty())
            updateCurrentCursorCharFormat();

        Q_EMIT keyPressed(key);

        auto doc = document();
        if (!doc)
            return;

        auto cursor = textCursor();
        const auto isInputEmpty = toPlainText().isEmpty();
        const auto block = cursor.block();
        const auto bf = Logic::TextBlockFormat(block.blockFormat());
        const auto blockText = block.text();
        const auto hasSelection = cursor.hasSelection();
        const auto isAtLineStart = cursor.atBlockStart();
        const auto isAtEmptyLine = blockText.isEmpty();

        if (hasSelection)
        {
            if (key == Qt::Key_Delete || key == Qt::Key_Backspace)
            {
                blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this]()
                {
                    replaceSelectionBy({});
                    return true;
                });
                return;
            }
        }
        else if (bf.type() != ftype::none)
        {
            if (key == Qt::Key_Delete && blockText.isEmpty())
            {
                cursor.movePosition(QTextCursor::MoveOperation::NextCharacter, QTextCursor::MoveMode::KeepAnchor);
                setTextCursor(cursor);
                blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this]()
                {
                    replaceSelectionBy({});
                    return true;
                });
                return;
            }

            if (key == Qt::Key_Backspace && isAtLineStart)
            {
                if (bf.isFirst())
                {
                    blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this, &cursor]()
                    {
                        removeBlockFormatFromLine(cursor.position());
                        return true;
                    });
                    return;
                }
                else if (auto prevBlock = block.previous(); prevBlock.isValid())
                {
                    if (bf.isLast())
                    {
                        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([&prevBlock, &cursor]()
                        {
                            const auto wasPrevFirst = getBlockFormat(prevBlock).isFirst();
                            cursor.deletePreviousChar();
                            auto newBf = getBlockFormat(cursor);
                            newBf.setIsLast(true);
                            newBf.setIsFirst(wasPrevFirst);
                            cursor.setBlockFormat(newBf);
                            return true;
                        });
                        return;
                    }
                    if (auto prevBf = getBlockFormat(block.previous()); prevBf.isFirst())
                    {
                        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([&bf, &cursor]()
                        {
                            auto newBf = bf;
                            newBf.setIsFirst(true);
                            cursor.setBlockFormat(newBf);
                            cursor.deletePreviousChar();
                            return true;
                        });
                        return;
                    }
                }
            }
        }

        if (key == Qt::Key_Backspace || key == Qt::Key_Delete)
        {
            const auto canDeleteMention =
                !hasSelection
                && ((!textCursor().atStart() && key == Qt::Key_Backspace)
                    || (!textCursor().atEnd() && key == Qt::Key_Delete));

            if (isInputEmpty)
            {
                Q_EMIT emptyTextBackspace();
            }
            else if (canDeleteMention)
            {
                if (key == Qt::Key_Delete)
                    cursor.movePosition(QTextCursor::Right);

                const auto charFormat = cursor.charFormat();
                if (const auto anchor = charFormat.anchorHref(); Utils::isMentionLink(anchor))
                {
                    auto block = cursor.block();
                    for (auto itFragment = block.begin(); itFragment != block.end(); ++itFragment)
                    {
                        auto frag = itFragment.fragment();
                        const auto fragStart = frag.position();
                        const auto fragLen = frag.length();

                        if (const auto curPos = cursor.position(); curPos >= fragStart && curPos <= fragStart + fragLen)
                        {
                            if (const auto cf = frag.charFormat(); cf == charFormat && cf.anchorHref() == anchor)
                            {
                                cursor.setPosition(fragStart);
                                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, fragLen);
                                cursor.removeSelectedText();

                                return;
                            }
                        }
                    }
                }
            }

            if (platform::is_apple() && hasCtrl)
            {
                auto cursor = textCursor();
                if (!cursor.selectedText().isEmpty())
                    cursor.setPosition(cursor.selectionEnd());

                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                return;
            }
        }

        if (key == Qt::Key_Escape)
        {
            Q_EMIT escapePressed();
        }
        else if (key == Qt::Key_Up)
        {
            Q_EMIT upArrow();
        }
        else if (key == Qt::Key_Down)
        {
            Q_EMIT downArrow();
        }
        else if (isEnter)
        {
            if (const auto modifiers = _event->modifiers(); catchEnter(modifiers))
            {
                Q_EMIT enter();
                _event->ignore();
                return;
            }
            else if (catchNewLine(modifiers))
            {
                blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this, isAtLineStart, isAtEmptyLine, &bf, &cursor]()
                {
                    for (auto type : core::data::get_all_styles())
                        discardFormatAtCursor(type, true, true);

                    if (bf.type() == ftype::none)
                    {
                        addLineBreak(cursor);
                    }
                    else
                    {
                        if (isAtLineStart && isAtEmptyLine && bf.isLast())
                        {
                            removeBlockFormatFromLine(cursor.position());
                        }
                        else
                        {
                            addLineBreak(cursor);
                            discardAllStylesAtCursor();
                        }
                    }
                    return true;
                });

                ensureCursorVisible();
            }
            return;
        }
        else if (_event->matches(QKeySequence::DeleteStartOfWord) && hasSelection)
        {
            _event->setModifiers(Qt::NoModifier);
            QTextBrowser::keyPressEvent(_event);
            _event->ignore();

            return;
        }
        else if (platform::is_apple() && input_ && key == Qt::Key_Backspace && hasCtrl)
        {
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            QTextBrowser::keyPressEvent(_event);
            _event->ignore();

            return;
        }
        else if (isFormatEnabled() && key == Qt::Key_Space)
        {
            const auto block = cursor.block();
            const auto blockText = block.text();
            const auto bf = Logic::TextBlockFormat(block.blockFormat());
            if (blockText == orderedListAutoStartPrefix() && bf.type() == ftype::none)
            {
                const auto cursorPosition = cursor.position();
                const auto initialString = getText();
                Data::FStringView initialView(initialString);

                Data::FString::Builder builder;
                builder %= initialView.left(cursorPosition).toFString();
                builder %= qsl(" ");
                builder %= initialView.mid(cursorPosition);
                Q_EMIT intermediateTextChange(builder.finalize(), cursorPosition + 1);

                const auto prefixRange = core::data::range{block.position(), blockText.size()};
                formatRangeBlockType({ ftype::ordered_list, prefixRange });
                cursor.deletePreviousChar();
                hasJustAutoCreatedList_ = true;
                cursor.deletePreviousChar();

                _event->ignore();
                return;
            }
        }

        const auto length = doc->characterCount();
        const auto deny = limit_ != -1 && length >= limit_ && !eventText.isEmpty();

        printDebugInfo();

        const auto oldPos = textCursor().position();
        if (_event->key() == Qt::Key_V && (_event->modifiers() & Qt::ControlModifier))
            _event->setModifiers(_event->modifiers() &= ~Qt::ShiftModifier);//makes QTextBrowser recognize ctrl+shift+v as Paste action
        QTextBrowser::keyPressEvent(_event);
        const auto newPos = textCursor().position();

        if (deny && doc->characterCount() > length)
            textCursor().deletePreviousChar();

        if (hasShift && key == Qt::Key_Up && oldPos == newPos)
        {
            auto cur = textCursor();
            cur.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            setTextCursor(cur);
        }

        _event->ignore();
    }

    void TextEditEx::inputMethodEvent(QInputMethodEvent* e)
    {
        QTextBrowser::setPlaceholderText(e->preeditString().isEmpty() ? placeholderText_ : QString());

#ifdef __APPLE__
        const auto& s = e->commitString();
        qsizetype i = 0;
        auto emoji = Emoji::getEmoji(s, i);
        if (!emoji.isNull())
            insertEmoji(emoji);
        else
#endif //__APPLE__
            QTextBrowser::inputMethodEvent(e);
    }

    void TextEditEx::drawDebugLines(QPainter& _painter) const
    {
        auto doc = document();
        if (!doc)
            return;

        auto layout = doc->documentLayout();
        const auto scrollVShift = verticalScrollBar() ? verticalScrollBar()->value() : 0;

        const auto painterSaver = Utils::PainterSaver(_painter);
        QPen pen(Qt::lightGray);
        pen.setStyle(Qt::PenStyle::DotLine);
        _painter.setPen(pen);
        for (auto block = doc->begin(); block != doc->end(); block = block.next())
        {
            auto rect = layout->blockBoundingRect(block);
            rect.moveTo({ rect.x(), rect.y() - scrollVShift });
            _painter.drawRect(rect);
        }
    }

    void TextEditEx::printDebugInfo() const
    {
        if (!(needShowDebugInfo() && Features::isBlockFormattingInInputEnabled()))
            return;

        auto debugBlockInfo = [](const auto& _block, int _indent = 0)
        {
            const auto bf = Logic::TextBlockFormat(_block.blockFormat());
            const auto typeName = core::data::to_string(bf.type());

            qDebug().noquote() << qsl("    ").repeated(_indent) << qsl("block #%1: ").arg(_block.blockNumber())
                << typeName.data() << qsl("first,last=%1,%2").arg(bf.isFirst()).arg(bf.isLast())
                << qsl("[%1;%2]").arg(_block.position()).arg(_block.position() + _block.length())
                << _block.text();
        };

        std::function<void(QAbstractTextDocumentLayout*, QTextFrame*, int)> logFrame
            = [&debugBlockInfo, &logFrame](QAbstractTextDocumentLayout* layout, QTextFrame* frame, int depth) -> void
        {
            qDebug() << qsl("%1frame:").arg(qsl("    ").repeated(depth))
                << qsl("[%1;%2]").arg(frame->firstPosition()).arg(frame->lastPosition())
                << frame;
            depth += 1;
            for (auto it = frame->begin(); ; ++it)
            {
                if (auto f = it.currentFrame())
                    logFrame(layout, f, depth);
                else if (auto b = it.currentBlock(); b.isValid())
                    debugBlockInfo(b, depth);

                if (it.atEnd())
                    break;
            }
        };

        if (auto doc = document())
        {
            auto rf = doc->rootFrame();
            const auto cursor = textCursor();
            qDebug().noquote() << "\ncursor at (anch, pos)" << cursor.anchor() << "till" << cursor.position()
                               << "with" << getCurrentNonMentionFormats()
                               << "and" << cursor.charFormat().font().family();
            qDebug() << "plaintext:" << toPlainText();
            qDebug() << getText();
            logFrame(doc->documentLayout(), rf, 0);

            qDebug() << "rootFrame top-bottom margins" << rf->frameFormat().topMargin() << rf->frameFormat().bottomMargin();
        }
    }

    void TextEditEx::drawBlockFormatsStuff(QPainter& _painter) const
    {
        auto doc = document();
        if (!doc)
            return;

        Utils::PainterSaver ps(_painter);
        _painter.setRenderHint(QPainter::RenderHint::Antialiasing);

        auto layout = doc->documentLayout();
        const auto scrollVShift = static_cast<qreal>(verticalScrollBar() ? verticalScrollBar()->value() : 0);

        auto drawQuoteBar = [scrollVShift, &_painter](QRectF _rect)
        {
            _rect.translate({ 0.0, -scrollVShift });
            TextRendering::drawQuoteBar(_painter, _rect.topLeft(), _rect.height());
        };

        auto drawCodeRect = [scrollVShift, &_painter](QRectF _rect)
        {
            _rect.translate({ 0.0, -scrollVShift });
            TextRendering::drawPreBlockSurroundings(_painter, _rect);
        };

        const auto fm = QFontMetrics(font_);
        const auto fontLineHeight = fm.height();
        const auto fontLineDescent = fm.descent();
        const auto textColor = palette().color(QPalette::Text);
        auto drawListItemBullet =
            [scrollVShift, &_painter, this, fontLineHeight, fontLineDescent, textColor]
            (ftype _type, QPoint _baselineLeft, int _textIndent, int _bulletNum = 0)
        {
            _baselineLeft.setY(_baselineLeft.y() - scrollVShift);

            if (_type == ftype::unordered_list)
                TextRendering::drawUnorderedListBullet(_painter, _baselineLeft, _textIndent, fontLineHeight, textColor);
            else if (_type == ftype::ordered_list)
                TextRendering::drawOrderedListBullet(_painter, _baselineLeft + QPoint{0, fontLineDescent}, _textIndent, font_, _bulletNum, textColor);
        };

        auto block = doc->rootFrame()->begin().currentBlock();
        auto bf = Logic::TextBlockFormat(block.blockFormat());
        while (block.isValid())
        {
            if (bf.type() != ftype::none && bf.isFirst())
            {
                const auto formatType = bf.type();
                const auto topLeft = layout->blockBoundingRect(block).topLeft();
                const auto firstBlockNum = block.blockNumber();
                const auto topMargin = bf.topMargin();

                while (block.isValid() && !bf.isLast())
                {
                    block = block.next();
                    bf = Logic::TextBlockFormat(block.blockFormat());
                }
                const auto bottomMargin = bf.bottomMargin();

                const auto lastBlockNum = block.blockNumber();
                if (bf.type() == ftype::quote)
                {
                    drawQuoteBar(QRectF(topLeft, layout->blockBoundingRect(block).bottomRight()));
                }
                else if (bf.type() == ftype::pre)
                {
                    const auto r = QRectF(topLeft, layout->blockBoundingRect(block).bottomRight());
                    const auto topAdjust = topMargin - bf.vPadding();
                    const auto bottomAdjust = bottomMargin - bf.vPadding();
                    const auto rightAdjust = bf.leftMargin() + bf.rightMargin();
                    drawCodeRect(r.adjusted(0, -topAdjust, rightAdjust, bottomAdjust));
                }
                else if (Data::FormatTypes({ ftype::unordered_list, ftype::ordered_list }).testFlag(bf.type()))
                {
                    auto bulletNum = 1;
                    for (auto i = firstBlockNum; i <= lastBlockNum; ++i)
                    {
                        const auto listBlock = doc->findBlockByNumber(i);
                        const auto line = listBlock.layout()->lineAt(0);

                        auto descent = line.descent();
                        if (doesBlockNeedADescentHack(listBlock.blockNumber()))
                            descent /= 2.0;

                        auto rect = layout->blockBoundingRect(listBlock);
                        rect.setY(rect.y() - descent);
                        rect.setHeight(line.height());

                        drawListItemBullet(bf.type(), rect.bottomLeft().toPoint(), listBlock.blockFormat().leftMargin(), bulletNum);

                        ++bulletNum;
                    }
                }
            }

            block = block.next();
            bf = Logic::TextBlockFormat(block.blockFormat());
        }
    }

    void TextEditEx::paintEvent(QPaintEvent* _event)
    {
        if (Features::isBlockFormattingInInputEnabled())
        {
            QPainter p(viewport());

            drawBlockFormatsStuff(p);

            if constexpr (needShowDebugInfo())
                drawDebugLines(p);
        }

        QTextBrowser::paintEvent(_event);
    }

    QMenu* TextEditEx::createContextMenu()
    {
        // object names of actions created by createStandardContextMenu are: edit-undo, edit-redo, edit-cut, edit-copy, edit-paste, select-all
        auto menu = createStandardContextMenu();
        if (!menu)
            return nullptr;

        Testing::setAccessibleName(menu, qsl("AS ChatInput contextMenu"));
        ContextMenu::applyStyle(menu, false, Utils::scale_value(15), Utils::scale_value(36));

        if (isFormatEnabled())
        {
            auto menuFormat = new ContextMenu(menu);
            menuFormat->setTitle(QT_TRANSLATE_NOOP("context_menu", "Format"));
            Testing::setAccessibleName(menuFormat, qsl("AS ChatInput contextMenuFormat"));

            menuFormat->setEnabled(isAnyFormatActionAllowed());
            if (menuFormat->isEnabled())
            {
                if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
                {
                    const auto addFormatAction = [menuFormat, mainWindow, this](auto _type, auto _enabled)
                    {
                        auto source = mainWindow->getFormatAction(_type);
                        auto action = new QAction(this);
                        action->setText(source->text());
                        action->setShortcut(source->shortcut());
                        action->setIcon(source->icon());
                        action->setShortcutVisibleInContextMenu(true);
                        action->setData(source->data());
                        action->setEnabled(_enabled);

                        connect(action, &QAction::triggered, this, [this, _type]
                        {
                            onFormatUserAction(_type);
                        });

                        menuFormat->addAction(action);
                    };

                    try
                    {
                        constexpr auto formatsInOrder = std::array{
                            ftype::bold,
                            ftype::italic,
                            ftype::underline,
                            ftype::strikethrough,
                            ftype::monospace,
                            ftype::link,
                            ftype::unordered_list,
                            ftype::ordered_list,
                            ftype::quote,
                            ftype::pre,
                            ftype::none,
                        };
                        for (auto type : formatsInOrder)
                            addFormatAction(type, isFormatActionAllowed(type));
                    }
                    catch (const std::out_of_range&)
                    {
                        im_assert(!"lost action initialization");
                    }
                }
            }

            menu->addSeparator();
            const auto action = menu->addMenu(menuFormat);
            connect(menuFormat, &QMenu::aboutToShow, this, [menu, action, menuFormat](){
                const auto actionGeometry = menu->actionGeometry(action);
                const auto menuPosition = menu->mapToGlobal(actionGeometry.topLeft());
                menuFormat->selectBestPositionAroundRectIfNeeded({menuPosition, actionGeometry.size()});
            });
        }

        menu->setMinimumWidth(getContextMenuMinWidth());

        return menu;
    }

    bool TextEditEx::isEmptyOrHasOnlyWhitespaces() const
    {
        return QStringView(QTextEdit::toPlainText()).trimmed().isEmpty();
    }

    QString TextEditEx::getPlainText() const
    {
        return getPlainText(0);
    }

    Data::FString TextEditEx::getText() const
    {
        if(cache_.isEmpty())
        {
            cache_ = getText(0);
            Utils::convertFilesPlaceholders(cache_, files_);
        }
        return cache_;
    }

    Data::FormatTypes TextEditEx::getCurrentNonMentionFormats() const
    {
        const auto cursor = textCursor();
        auto result = getCurrentNonMentionCursorStyles(cursor.charFormat());
        if (auto type = getBlockFormat(cursor).type(); type != ftype::none)
            result.setFlag(type);
        return result;
    }

    void TextEditEx::insertPlainText(const QString& _text, bool _convertLinks, const QTextCharFormat::VerticalAlignment _alignment)
    {
        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([&_text, _alignment, _convertLinks, this]()
        {
            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Escape, _convertLinks, false, nullptr, Emoji::EmojiSizePx::Auto, _alignment);
            return true;
        });
    }

    void TextEditEx::setPlainText(const QString& _text, bool _convertLinks, const QTextCharFormat::VerticalAlignment _alignment)
    {
        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::ClearText>([&_text, _convertLinks, _alignment, this]()
        {
            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Escape, _convertLinks, false, nullptr, Emoji::EmojiSizePx::Auto, _alignment);
            return true;
        });
    }

    void TextEditEx::insertFormattedText(Data::FStringView _view, const QTextCharFormat::VerticalAlignment _alignment, const bool _isInsertFromMimeData)
    {
        if (!isFormatEnabled() || !_view.hasFormatting())
        {
            insertPlainText(_view.string().toString(), false, _alignment);
            return;
        }

        const auto wasMonospaced = isCursorInCode();
        auto cursor = textCursor();
        auto text = _view.toFString();
        im_assert(text.isFormatValid());
        Utils::convertMentions(text, mentions_);

        const auto bf = getBlockFormat(cursor);
        if (bf.type() == ftype::none)
        {
            constexpr auto blockTypes = Data::FormatTypes({ftype::quote, ftype::ordered_list, ftype::unordered_list, ftype::pre});
            if (cursor.positionInBlock() > 0 && _view.left(1).isAnyOf(blockTypes))
                cursor.insertBlock();
        }
        else if (bf.type() == ftype::pre)
        {
            text = text.string();
        }
        else
        {
            text.removeAllBlockFormats();
        }

        const auto startPos = getSelectionRange(cursor).offset_;
        auto replacements = Logic::Text4Edit(text.string(), *this, Logic::Text2DocHtmlMode::Escape,
            false, false, nullptr, Emoji::EmojiSizePx::Auto, _alignment);
        for (auto& r : replacements)
            r.sourcePos_ += startPos;

        for (auto ft : text.formatting().formats())
        {
            im_assert(ft.type_ != ftype::none);

            const QString textFromBegin = text.string().mid(0, ft.range_.offset_ + ft.range_.size_);
            const int carriageReturnCount = textFromBegin.count(QChar::CarriageReturn % QChar::LineFeed);
            if (textFromBegin.at(textFromBegin.size() - 1) == QChar::CarriageReturn)
                --ft.range_.size_;

            ft.range_.offset_ -= carriageReturnCount;


            if (ft.type_ == ftype::mention && !ft.data_)
                ft.data_ = text.string().mid(ft.range_.offset_, ft.range_.size_).toStdString();
            ft.range_.offset_ += startPos;
            applyReplacementShifts(ft.range_, replacements);
            formatRange(ft, _isInsertFromMimeData);
        }

        //! Keep font monospaced if needed
        if (wasMonospaced)
        {
            auto fullRange = core::data::range{ startPos, text.size() };
            applyReplacementShifts(fullRange, replacements);
            updateCharFormat(cursor, fullRange, [](QTextCharFormat& _charFormat)
            {
                Logic::setMonospaceFont(_charFormat, true);
            });
        }

        onTextChanged();
    }

    void TextEditEx::setFormattedText(Data::FStringView _view, const QTextCharFormat::VerticalAlignment _alignment)
    {
        blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::ClearText>([_view, _alignment, this]()
        {
            insertFormattedText(_view, _alignment);
            return !_view.isEmpty();
        });
    }

    void TextEditEx::clear()
    {
        QTextEdit::clear();
        if (auto doc = document())
            doc->clear();
        textCursor().setBlockFormat({});
        textCursor().setCharFormat({});
        resourceIndex_.clear();
    }

    void TextEditEx::setMentions(Data::MentionMap _mentions)
    {
        mentions_ = std::move(_mentions);
    }

    const Data::MentionMap& TextEditEx::getMentions() const
    {
        return mentions_;
    }

    void TextEditEx::setFiles(Data::FilesPlaceholderMap _files)
    {
        files_ = std::move(_files);
    }

    const Data::FilesPlaceholderMap & TextEditEx::getFiles() const
    {
        return files_;
    }

    void TextEditEx::insertMention(const QString& _aimId, const QString& _friendly, bool _addSpace)
    {
        if (getCurrentNonMentionFormats().testFlag(core::data::format_type::pre))
        {
            insertFormattedText(Data::FString(_friendly));
            return;
        }

        mentions_.emplace(_aimId, _friendly);
        const auto url = QString(u"@[" % _aimId % u"]");
        auto text = url;
        if (_addSpace)
            text += u' ';

        const auto charFormat = textCursor().charFormat();
        auto formats = std::vector<core::data::range_format>();
        formats.emplace_back(ftype::mention, core::data::range{0, url.size()});

        for (auto type : core::data::get_all_non_mention_styles())
        {
            if (Logic::hasStyle(charFormat, type).first)
                formats.emplace_back(type, core::data::range{0, url.size()});
        }

        insertFormattedText(Data::FString(text, std::move(formats)));
    }

    void TextEditEx::insertEmoji(const Emoji::EmojiCode& _code)
    {
        mergeResources(Logic::InsertEmoji(_code, *this));
#ifdef __APPLE__
        auto cursor = textCursor();
        if (getBlockFormat(cursor).type() == ftype::pre)
        {
            blockIntermediateTextChangeSignalsWrapper<WrapperPolicy::KeepText>([this, &cursor]()
            {
                const auto block = cursor.block();
                auto blockRange = core::data::range{ block.position(), block.text().size() };
                blockRange = surroundWithNewLinesIfNone(blockRange, true);
                formatRangeClearBlockTypes(blockRange);
                discardAllStylesAtCursor();
                blockRange = formatRange({ftype::pre, blockRange});
                return blockRange.size_ > 0;
            });

            setTextCursor(cursor);
        }
#endif
    }

    void TextEditEx::selectWholeText()
    {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }

    void TextEditEx::selectFromBeginning(const QPoint& _p)
    {
        const auto cursorTo = cursorForPosition(mapFromGlobal(_p));
        const auto posTo = cursorTo.position();

        prevPos_ = posTo;

        auto cursor = textCursor();

        auto isCursorChanged = false;

        const auto isCursorAtTheBeginning = (cursor.position() == 0);
        if (!isCursorAtTheBeginning)
        {
            isCursorChanged = cursor.movePosition(QTextCursor::Start);
            im_assert(isCursorChanged);
        }

        const auto isCursorAtThePos = (cursor.position() == posTo);
        if (!isCursorAtThePos)
        {
            cursor.setPosition(posTo, QTextCursor::KeepAnchor);
            isCursorChanged = true;
        }

        if (isCursorChanged)
            setTextCursor(cursor);
    }

    void TextEditEx::selectTillEnd(const QPoint& _p)
    {
        const auto cursorTo = cursorForPosition(mapFromGlobal(_p));
        const auto posTo = cursorTo.position();

        prevPos_ = posTo;

        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.setPosition(posTo, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }

    void TextEditEx::selectByPos(const QPoint& _p)
    {
        const auto cursorP = cursorForPosition(mapFromGlobal(_p));
        const auto posP = cursorP.position();

        auto cursor = textCursor();

        if (!cursor.hasSelection() && prevPos_ == -1)
        {
            cursor.setPosition(posP);
            cursor.setPosition(posP > prevPos_ ? posP : posP - 1, QTextCursor::KeepAnchor);
        }
        else
        {
            cursor.setPosition(posP, QTextCursor::KeepAnchor);
        }

        prevPos_ = posP;

        setTextCursor(cursor);
    }

    void TextEditEx::selectByPos(const QPoint& _from, const QPoint& _to)
    {
        auto cursorFrom = cursorForPosition(mapFromGlobal(_from));

        const auto cursorTo = cursorForPosition(mapFromGlobal(_to));
        const auto posTo = cursorTo.position();

        cursorFrom.setPosition(posTo, QTextCursor::KeepAnchor);

        prevPos_ = posTo;

        setTextCursor(cursorFrom);
    }

    void TextEditEx::clearSelection()
    {
        QTextCursor cur = textCursor();
        const auto scrollPos = verticalScrollBar()->value();
        cur.clearSelection();
        setTextCursor(cur);
        verticalScrollBar()->setValue(scrollPos);
        prevPos_ = -1;
    }

    bool TextEditEx::isAllSelected()
    {
        QTextCursor cur = textCursor();
        if (!cur.hasSelection())
            return false;

        const int start = cur.selectionStart();
        const int end = cur.selectionEnd();
        cur.setPosition(start);
        if (cur.atStart())
        {
            cur.setPosition(end);
            return cur.atEnd();
        }
        else if (cur.atEnd())
        {
            cur.setPosition(start);
            return cur.atStart();
        }

        return false;
    }

    QString TextEditEx::selection()
    {
        return getPlainText(textCursor().selectionStart(), textCursor().selectionEnd());
    }

    Data::FString TextEditEx::getTextFromFragment(QString _fragmentText, const QTextCharFormat& _charFormat) const
    {
        Data::FString::Builder result;

        if (_charFormat.isImageFormat() && _fragmentText == QChar::ObjectReplacementCharacter)
        {
            const auto imgFmt = _charFormat.toImageFormat();
            if (auto iter = resourceIndex_.find(imgFmt.name()); iter != resourceIndex_.end())
            {
                _fragmentText = iter->second;

                if (auto [has, data] = Logic::hasStyle(_charFormat, ftype::mention); has && data)
                    result %= Data::FString(QString::fromStdString(*data), ftype::mention, data);
                else if (auto [has, data] = Logic::hasStyle(_charFormat, ftype::link); has && data)
                    result %= Data::FString(QString(_fragmentText), ftype::link, data);
                else
                    result %= _fragmentText;
            }
        }
        else
        {
            if (auto [doesHave, formatData] = Logic::hasStyle(_charFormat, ftype::mention); doesHave && formatData)
                _fragmentText = QString::fromStdString(*formatData);

            const core::data::range range{ 0, _fragmentText.size() };
            core::data::format::builder builder;
            for (auto type : core::data::get_all_styles())
            {
                if (auto [doesHave, formatData] = Logic::hasStyle(_charFormat, type); doesHave)
                    builder %= { type, range, formatData };
            }
            result %= Data::FString(_fragmentText, builder.finalize());
        }

        return result.finalize();
    }

    void TextEditEx::formatRangeStyleType(core::data::range_format _format)
    {
        auto& [format_type, range, format_data] = _format;
        auto& [offset, length, startIndex] = range;
        im_assert(core::data::is_style_or_none(format_type));
        im_assert(format_type != ftype::mention || format_data);

        auto cursor = textCursor();

        auto needToAdd = false;
        if (format_type == ftype::none || format_type == ftype::link)
        {
            needToAdd = true;
        }
        else
        {
            for (auto pos = offset; pos < offset + length; ++pos)
            {
                cursor.setPosition(pos + 1);
                if (!hasStyle(cursor.charFormat(), format_type).first)
                {
                    needToAdd = true;
                    break;
                }
            }
        }

        cursor.setPosition(offset + qMin(1, length));
        if (format_type == ftype::none)
        {
            if (auto [has, url] = hasStyle(cursor.charFormat(), ftype::link); has && url)
                clearAdjacentLinkFormats({ offset, length }, *url);
        }

        const auto rangeEnd = offset + length;
        auto block = document()->findBlock(offset);
        auto blockEnd = getBlockEndPosition(block);
        while (block.isValid() && block.position() < rangeEnd)
        {
            if (getBlockFormat(block).type() != ftype::pre)
            {
                const auto subRangeOffset = qMax(block.position(), offset);
                const auto subRangeSize = qMin(blockEnd, rangeEnd) - subRangeOffset;
                updateCharFormat(cursor, core::data::range{subRangeOffset, subRangeSize},
                    [type = format_type, needToAdd, data = format_data](QTextCharFormat& _charFormat)
                    {
                        if (type == ftype::none)
                            clearCharFormats(_charFormat);
                        else
                            setTextStyle(_charFormat, type, needToAdd, data);
                    }, format_type);
            }

            block = block.next();
            blockEnd = getBlockEndPosition(block);
        }
    }

    void TextEditEx::formatRangeCreateLink(core::data::range _range)
    {
        const auto [offset, length, startIndex] = _range;
        auto name = getText(offset, offset + length);
        Utils::convertMentions(name, mentions_);
        const auto nameTrimmed = Data::FStringView(name).trimmed();
        if (nameTrimmed.isEmpty())
            return;

        auto url = QString();
        for (const auto& ft : name.formatting().formats())
        {
            if (ft.type_ == ftype::link && ft.data_)
            {
                url = QString::fromStdString(*ft.data_);
                break;
            }
        }
        const auto isConfirmed = Utils::UrlEditor(this, nameTrimmed.string(), url);
        if (isConfirmed && !url.isEmpty())
            setLinkToSelection(url);
    }

    bool TextEditEx::clearBlockFormatOnDemand(core::data::range_format _format)
    {
        auto doc = document();
        auto [offset, length, startIndex] = _format.range_;
        auto cursor = textCursor();

        const auto firstBlockNum = doc->findBlock(offset).blockNumber();
        const auto lastBlockNum = doc->findBlock(offset + length - 1).blockNumber();
        cursor.setPosition(offset);

        bool res = false;

        for (auto i = firstBlockNum; i <= lastBlockNum; ++i)
        {
            auto block = doc->findBlockByNumber(i);
            cursor.setPosition(block.position());
            auto bf = getBlockFormat(cursor);

            if (bf.type() == _format.type_)
            {
                removeBlockFormatFromLine(block.position() + 1);
                res = true;
            }
        }

        return res;
    }

    core::data::range TextEditEx::formatRangeBlockType(core::data::range_format _format, const bool _isInsertFromMimeData)
    {
        im_assert(!core::data::is_style_or_none(_format.type_));
        auto doc = document();
        if (!doc)
            return _format.range_;

        auto [offset, length, startIndex] = _format.range_;
        auto cursor = textCursor();

        if (_format.type_ == ftype::pre)
        {
            clearSelectionFormats();

            updateCharFormat(cursor, {offset, length}, [](QTextCharFormat& _charFormat)
            {
                clearCharFormats(_charFormat);
                setTextStyle(_charFormat, ftype::mention, false);
                Logic::setMonospaceFont(_charFormat, true);
            });
        }
        cursor.setPosition(offset);

        if (clearBlockFormatOnDemand(_format))
            return {offset, length};

        if (!core::data::is_style(_format.type_) && !_isInsertFromMimeData)
            offset = surroundWithNewLinesIfNone(_format.range_).offset_;

        const auto firstBlockNum = doc->findBlock(offset).blockNumber();
        const auto lastBlockNum = doc->findBlock(offset + length - 1).blockNumber();
        cursor.setPosition(offset);

        for (auto i = firstBlockNum; i <= lastBlockNum; ++i)
        {
            auto block = doc->findBlockByNumber(i);
            auto bf = getBlockFormat(cursor);
            bf.set(_format.type_, firstBlockNum == i, lastBlockNum == i);
            setBlockFormat(block, cursor, bf);
        }
        return {offset, length};
    }

    void TextEditEx::formatRangeClearBlockTypes(core::data::range _range)
    {
        auto cursor = textCursor();
        auto block = document()->findBlock(_range.offset_);
        while (block.isValid() && block.position() < _range.end())
        {
            if (getBlockFormat(block).type() != ftype::none)
            {
                cursor.setPosition(block.position());
                removeBlockFormatFromCurrentLine(cursor);
            }
            block = block.next();
        }
    }

    void TextEditEx::clearAdjacentLinkFormats(core::data::range _range, std::string_view _url)
    {
        if (_range.size_ < 1 || _url.empty())
            return;

        auto cursor = textCursor();
        auto clearFormatAt = [&cursor, &_url](auto _pos) -> bool
        {
            selectText(cursor, _pos, _pos + 1);
            auto charFormat = cursor.charFormat();
            if (auto [has, url] = Logic::hasStyle(charFormat, ftype::link); has && url && *url == _url)
            {
                setTextStyle(charFormat, ftype::link, false);
                cursor.setCharFormat(charFormat);
                return true;
            }
            return false;
        };

        for (auto pos = _range.offset_; pos >= 0; --pos)
        {
            if (!clearFormatAt(pos))
                break;
        }

        const auto maxPos = toPlainText().size();
        for (auto pos = _range.offset_ + _range.size_; pos < maxPos; ++pos)
        {
            if (!clearFormatAt(pos))
                break;
        }
    }

    bool TextEditEx::isAnyFormatActionAllowed() const
    {
        return isFormatActionAllowed(ftype::none, true);
    }

    bool TextEditEx::isFormatActionAllowed(core::data::format_type _type, bool _justCheckIfAnyAllowed) const
    {
        if (!isFormatEnabled())
            return false;

        const auto cursor = textCursor();
        const auto [offset, size, startIndex] = getSelectionRange(cursor);
        const auto selectedText = getText(offset, offset + size);
        const auto& formats = selectedText.formatting().formats();
        const auto hasSelection = size > 0;

        auto allowClear = std::any_of(formats.cbegin(), formats.cend(),
            [](auto _ft) { return _ft.type_ != ftype::mention; });

        if (!allowClear && selectedText.isEmpty())
        {
            allowClear = getCurrentNonMentionCursorStyles(cursor.charFormat()) != Data::FormatTypes()
                      || ftype::none != Logic::TextBlockFormat(cursor.block().blockFormat()).type();
        }

        if (_justCheckIfAnyAllowed)
            return hasSelection || allowClear;

        const auto isSelectionEntirelyWithinCode = hasSelection && Data::FStringView(selectedText).isAnyOf(ftype::pre);
        const auto plainSelected = toPlainText().mid(offset, size);
        const auto hasNonEmojiChars = std::any_of(plainSelected.cbegin(), plainSelected.cend(),
            [](auto _ch) { return _ch != QChar::ObjectReplacementCharacter; });

        const auto hasNonWhitespaceSelection = !QStringView(plainSelected).trimmed().isEmpty();
        const auto selectionHasMention = selectedText.containsFormat(ftype::mention);
        const auto allowGeneralStyle = hasNonEmojiChars && hasSelection && !isSelectionEntirelyWithinCode;
        const auto allowGeneralBlockFormat = hasNonWhitespaceSelection && !isSelectionEntirelyWithinCode;

        switch (_type)
        {
        case ftype::bold:
        case ftype::italic:
        case ftype::underline:
        case ftype::monospace:
        case ftype::strikethrough:
            return allowGeneralStyle;
        case ftype::link:
            return hasNonWhitespaceSelection && !isSelectionEntirelyWithinCode;
        case ftype::none:
            return allowClear;
        case ftype::quote:
        case ftype::unordered_list:
        case ftype::ordered_list:
        case ftype::pre:
            return allowGeneralBlockFormat;
        default:
            return false;
        }
    }

    bool TextEditEx::isCursorInCode() const
    {
        return getCurrentNonMentionFormats().testFlag(ftype::pre);
    }

    core::data::range TextEditEx::formatRange(const core::data::range_format& _format, const bool _isInsertFromMimeData)
    {
        if (ftype::none == _format.type_)
        {
            auto range = _format.range_;
            formatRangeStyleType({ ftype::none, range });
            range = surroundWithNewLinesIfNone(range, true);
            formatRangeClearBlockTypes(range);
            return range;
        }
        else if (core::data::is_style(_format.type_))
        {
            formatRangeStyleType(_format);
            return _format.range_;
        }
        else
        {
            auto range = _format.range_;
            if (!_isInsertFromMimeData)
                range = surroundWithNewLinesIfNone(range);
            formatRangeClearBlockTypes(range);
            return formatRangeBlockType({_format.type_, range, _format.data_});
        }
    }

    void TextEditEx::formatSelectionUserAction(core::data::format_type _type)
    {
        if (!isFormatActionAllowed(_type))
            return;

        auto cursor = textCursor();
        auto range = getSelectionRange(cursor);

        selectText(cursor, 0, 1);
        auto currentFormat = cursor.charFormat();
        bool is_mention = Utils::isMentionLink(currentFormat.anchorHref());

        int shift = 0;
        if (is_mention)
        {
            shift = 1;
            cursor.setPosition(0);
            cursor.insertText(qsl("_"));
        }

        selectText(cursor, range.offset_ + shift, range.offset_ + range.size_ + shift);
        currentFormat = cursor.charFormat();

        QTextCharFormat fmt;
        switch (_type)
        {
            case ftype::bold:
                Logic::setBoldStyle(fmt, currentFormat.fontWeight() == QFont::Weight::Normal ? true : false);
                break;
            case ftype::italic:
                fmt.setFontItalic(!currentFormat.fontItalic());
                break;
            case ftype::monospace:
                Logic::setMonospaceStyle(fmt, !(currentFormat.fontFamily() == Fonts::getInputMonospaceTextFont().family()));
                break;
            case ftype::underline:
                Logic::setUnderlineStyle(fmt, !currentFormat.fontUnderline());
                break;
            case ftype::strikethrough:
                fmt.setFontStrikeOut(!currentFormat.fontStrikeOut());
                break;
            case ftype::ordered_list:
                formatRangeBlockType({ ftype::ordered_list, range });
                break;
            case ftype::unordered_list:
                formatRangeBlockType({ ftype::unordered_list, range });
                break;
            case ftype::pre:
                formatRangeBlockType({ ftype::pre, range });
                break;
            case ftype::quote:
                formatRangeBlockType({ ftype::quote, range });
                break;
            case ftype::link:
                formatRangeCreateLink(range);
                break;
            default:
                break;
        }

        if (_type == ftype::none)
            clearSelectionFormats();
        else
            cursor.mergeCharFormat(fmt);

        if (is_mention)
        {
            cursor.setPosition(0);
            cursor.deleteChar();
        }

        cursor.setPosition(range.offset_ + range.size_);
        setTextCursor(cursor);
    }

    void TextEditEx::clearSelectionFormats()
    {
        auto cursor = textCursor();
        auto range = getSelectionRange(cursor);
        auto block = document()->findBlock(range.offset_);

        while (block.isValid() && block.position() < range.end())
        {
            for (auto itFragment = block.begin(); itFragment != block.end(); ++itFragment)
            {
                auto fragment = itFragment.fragment();

                cursor.setPosition(fragment.position());
                selectText(cursor, fragment.position(), fragment.position() + fragment.length());

                auto cf = fragment.charFormat();
                clearCharFormats(cf);
                mergeCharFormat(cursor, cf);
            }

            block = block.next();
        }
        formatRangeClearBlockTypes(range);
    }

    QSize TextEditEx::getTextSize() const
    {
        return document()->documentLayout()->documentSize().toSize();
    }

    int32_t TextEditEx::getTextHeight() const
    {
        return getTextSize().height();
    }

    int32_t TextEditEx::getTextWidth() const
    {
        return getTextSize().width();
    }

    void TextEditEx::mergeResources(const ResourceMap& _resources)
    {
        for (const auto& [name, code] : _resources)
            resourceIndex_[name] = code;
    }

    QSize TextEditEx::sizeHint() const
    {
        QSize sizeRect(document()->idealWidth(), document()->size().height());

        return sizeRect;
    }

    void TextEditEx::setPlaceholderText(const QString& _text)
    {
        QTextBrowser::setPlaceholderText(_text);
        placeholderText_ = _text;
    }

    void TextEditEx::setEnterKeyPolicy(TextEditEx::EnterKeyPolicy _policy)
    {
        enterPolicy_ = _policy;
    }

    bool TextEditEx::catchEnter(const int _modifiers)
    {
        switch (enterPolicy_)
        {
            case EnterKeyPolicy::CatchEnter:
            case EnterKeyPolicy::CatchEnterAndNewLine:
                return true;
            case EnterKeyPolicy::FollowSettingsRules:
                return catchEnterWithSettingsPolicy(_modifiers);
            default:
                break;
        }

        return false;
    }

    bool TextEditEx::catchNewLine(const int _modifiers)
    {
        switch (enterPolicy_)
        {
            case EnterKeyPolicy::CatchNewLine:
            case EnterKeyPolicy::CatchEnterAndNewLine:
                return true;
            case EnterKeyPolicy::FollowSettingsRules:
                return catchNewLineWithSettingsPolicy(_modifiers);
            default:
                break;
        }

        return false;
    }

    int TextEditEx::adjustHeight(int _width)
    {
        if (_width)
        {
            setFixedWidth(_width);
            document()->setTextWidth(_width);
        }

        const auto height = getTextSize().height();
        setFixedHeight(height + heightSupplement_);

        return height;
    }

    void TextEditEx::setMaxHeight(int _height)
    {
        maxHeight_ = _height;
        editContentChanged();
    }

    void TextEditEx::setViewportMargins(int left, int top, int right, int bottom)
    {
        if (viewportMargins() != QMargins(left, top, right, bottom))
            QAbstractScrollArea::setViewportMargins(left, top, right, bottom);
    }

    void TextEditEx::setDocumentMargin(int _margin)
    {
        defaultDocumentMargin_ = _margin;
        document()->setDocumentMargin(_margin);
        document()->adjustSize();
    }

    void TextEditEx::contextMenuEvent(QContextMenuEvent* _e)
    {
        execContextMenu(_e->globalPos());
    }

    bool Ui::TextEditEx::isCursorInBlockFormat() const
    {
        return getBlockFormat(textCursor()).type() != ftype::none;
    }

    void TextEditEx::execContextMenu(QPoint _globalPos)
    {
        if (auto menu = createContextMenu())
        {
            menu->exec(_globalPos);
            menu->deleteLater();
        }
    }

    void TextEditEx::clearCache()
    {
        cache_.clear();
    }

    bool TextEditEx::catchEnterWithSettingsPolicy(const int _modifiers) const
    {
        auto key1ToSend = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1ToSend == KeyToSendMessage::Enter_Old)
            key1ToSend = KeyToSendMessage::Enter;

        switch (key1ToSend)
        {
        case Ui::KeyToSendMessage::Enter:
            return _modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier;
        case Ui::KeyToSendMessage::Shift_Enter:
            return _modifiers & Qt::ShiftModifier;
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return _modifiers & Qt::ControlModifier;
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return _modifiers & Qt::MetaModifier;
        default:
            break;
        }
        return false;
    }

    bool TextEditEx::catchNewLineWithSettingsPolicy(const int _modifiers) const
    {
        auto key1ToSend = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1ToSend == KeyToSendMessage::Enter_Old)
            key1ToSend = KeyToSendMessage::Enter;

        const auto ctrl = _modifiers & Qt::ControlModifier;
        const auto shift = _modifiers & Qt::ShiftModifier;
        const auto meta = _modifiers & Qt::MetaModifier;
        const auto enter = (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);

        switch (key1ToSend)
        {
        case Ui::KeyToSendMessage::Enter:
            return shift || ctrl || meta;
        case Ui::KeyToSendMessage::Shift_Enter:
            return  ctrl || meta || enter;
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return shift || meta || enter;
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return shift || ctrl || enter;
        default:
            break;
        }
        return false;
    }

    bool TextEditEx::doesBlockNeedADescentHack(int _blockNumber) const
    {
        const auto& numbers = blockNumbersToApplyDescentHack_;
        return std::any_of(numbers.cbegin(), numbers.cend(),
                           [_blockNumber](auto _value) { return _value == _blockNumber; });
    }

    void Ui::TextEditEx::onThemeChange()
    {
        const auto text = getText();
        updatePalette();
        setFormattedText(text);
    }


    class TextEditBoxPrivate
    {
    public:
        QWidget* textWidget_ = nullptr;
        QWidget* emojiWidget_ = nullptr;
        QHBoxLayout* layout_ = nullptr;
        QFont font_;
        Styling::ThemeColorKey color_;
        QSize iconSize_;
        bool input_;
        bool fitToText_;
    };


    TextEditBox::TextEditBox(QWidget* _parent, const QFont& _font, const Styling::ThemeColorKey& _color,
                             bool _input, bool _isFitToText, const QSize& _iconSize)
        : QFrame(_parent), d(std::make_unique<TextEditBoxPrivate>())
    {
        d->font_ = _font;
        d->color_ = _color;
        d->input_ = _input;
        d->fitToText_ = _isFitToText;
        d->iconSize_ = _iconSize;
        d->layout_ = Utils::emptyHLayout(this);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    TextEditBox::~TextEditBox() {}

    void TextEditBox::initUi()
    {
        if (!d->textWidget_)
        {
            d->textWidget_ = createTextEdit(this);
            d->textWidget_->installEventFilter(this);
            d->layout_->addWidget(d->textWidget_, 0, Qt::AlignBottom);
        }

        if (!d->emojiWidget_)
        {
            d->emojiWidget_ = createEmojiButton(this);
            d->layout_->addWidget(d->emojiWidget_, 0, Qt::AlignBottom);
        }
    }

    QWidget* TextEditBox::textWidget() const { return d->textWidget_; }

    QWidget* TextEditBox::emojiWidget() const { return d->emojiWidget_; }

    void Ui::TextEditBox::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        QStyleOptionFrame panel;
        initStyleOption(&panel);

        if (d->textWidget_ && d->textWidget_->hasFocus())
            panel.state |= QStyle::State_HasFocus;

        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
    }

    bool Ui::TextEditBox::eventFilter(QObject* _object, QEvent* _event)
    {
        if (_object == d->textWidget_ && (_event->type() == QEvent::FocusIn || _event->type() == QEvent::FocusOut))
            update();

        return QFrame::eventFilter(_object, _event);
    }

    QWidget* TextEditBox::createTextEdit(QWidget* _parent) const
    {
        return new TextEditEx(_parent, d->font_, d->color_, d->input_, d->fitToText_);
    }

    QWidget* TextEditBox::createEmojiButton(QWidget* _parent) const
    {
        return new CustomButton(_parent);
    }
}


#include "stdafx.h"

#include "gui_settings.h"
#include "../cache/emoji/Emoji.h"
#include "../fonts.h"
#include "../utils/log/log.h"
#include "../utils/Text2DocConverter.h"
#include "../utils/utils.h"
#include "../controls/ContextMenu.h"
#include "../main_window/history_control/MessageStyle.h"

#include "TextEditEx.h"

namespace Ui
{
    TextEditEx::TextEditEx(QWidget* _parent, const QFont& _font, const QPalette& _palette, bool _input, bool _isFitToText)
        :TextEditEx(_parent, _font, QColor(), _input, _isFitToText)
    {
        setPalette(_palette);
    }

    TextEditEx::TextEditEx(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText)
        : QTextBrowser(_parent)
        , index_(0)
        , font_(_font)
        , prevPos_(-1)
        , input_(_input)
        , isFitToText_(_isFitToText)
        , add_(0)
        , limit_(-1)
        , prevCursorPos_(0)
        , maxHeight_(Utils::scale_value(250))
    {
        setAttribute(Qt::WA_InputMethodEnabled);
        setInputMethodHints(Qt::ImhMultiLine);

        setAcceptRichText(false);
        setFont(font_);
        document()->setDefaultFont(font_);

        QPalette p = palette();
        p.setColor(QPalette::Text, _color);
        p.setColor(QPalette::Highlight, MessageStyle::getTextSelectionColor());
        p.setColor(QPalette::HighlightedText, MessageStyle::getSelectionFontColor());
        setPalette(p);

        if (isFitToText_)
        {
            connect(document(), &QTextDocument::contentsChanged, this, &TextEditEx::editContentChanged, Qt::QueuedConnection);
            // AutoConnection using breaks rename contact dialog. FIXME: editContentChanged or rename contact dialog
        }

        connect(this, &QTextBrowser::anchorClicked, this, &TextEditEx::onAnchorClicked);
        connect(this, &TextEditEx::cursorPositionChanged, this, &TextEditEx::onCursorPositionChanged);

        if (input_)
            connect(document(), &QTextDocument::contentsChange, this, &TextEditEx::onContentsChanged);
    }

    void TextEditEx::limitCharacters(const int _count)
    {
        limit_ = _count;
    }

    void TextEditEx::resetFormat(const int _pos, const int _len)
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
                {
                    cur.setCharFormat(QTextCharFormat());
                }
                cur.clearSelection();
            }
        }
    }

    void TextEditEx::editContentChanged()
    {
        int docHeight = this->document()->size().height();

        const auto minHeight = Utils::scale_value(20);
        if (docHeight < minHeight)
            docHeight = minHeight;

        if (docHeight > maxHeight_)
            docHeight = maxHeight_;

        auto oldHeight = height();

        setFixedHeight(docHeight + add_);

        if (height() != oldHeight)
        {
            emit setSize(0, height() - oldHeight);
        }
    }

    void TextEditEx::onAnchorClicked(const QUrl &_url)
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
                        const auto cursorWasOut = prevCursorPos_ <= mentionStart || prevCursorPos_ >= mentionEnd;
                        const auto cursorNowIn = curPos > mentionStart && curPos < mentionEnd;

                        if (!cursorWasOut || !cursorNowIn)
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
            resetFormat(_pos, _added);
    }

    QString TextEditEx::getPlainText(int _from, int _to) const
    {
        QString outString;
        if (_from == _to)
            return outString;

        if (_to != -1 && _to < _from)
        {
            assert(!"invalid data");
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

    QMimeData* TextEditEx::createMimeDataFromSelection() const
    {
        auto dt = new QMimeData();

        auto rawText = getPlainText(textCursor().selectionStart(), textCursor().selectionEnd());
        if (!mentions_.empty() || !files_.empty())
        {
            auto text = !mentions_.empty() ? Utils::convertMentions(rawText, mentions_) : rawText;
            text = !files_.empty() ? Utils::convertFilesPlaceholders(text, files_) : text;

            dt->setText(text);
            dt->setData(MimeData::getRawMimeType(), std::move(rawText).toUtf8());

            if (!mentions_.empty())
                dt->setData(MimeData::getMentionMimeType(), MimeData::convertMapToArray(mentions_));
            if (!files_.empty())
                dt->setData(MimeData::getFileMimeType(), MimeData::convertMapToArray(files_));
        }
        else
        {
            dt->setText(rawText);
        }

        return dt;
    }

    void TextEditEx::insertFromMimeData(const QMimeData* _source)
    {
        if (_source->hasImage() && !Utils::haveText(_source))
        {
            emit insertFiles();
            return;
        }

        if (_source->hasUrls())
        {
            const auto urls = _source->urls();
            for (const auto& url : urls)
            {
                if (url.isLocalFile())
                {
                    emit insertFiles();
                    return;
                }
            }
        }

        if (!_source->hasText())
            return;

        if (limit_ != -1 && document()->characterCount() >= limit_)
            return;

        QString text;
        bool withMentions = false;

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
            text = _source->text();

        if (limit_ != -1 && text.length() + document()->characterCount() > limit_)
        {
            const auto maxLen = limit_ - document()->characterCount();
            auto lenToResize = maxLen;

            if (withMentions)
            {
                static const auto mentionMarker = ql1s("@[");
                auto ndxStart = text.indexOf(mentionMarker);
                auto ndxEnd = -1;

                if (ndxStart != -1 && ndxStart < maxLen)
                {
                    while (ndxStart != -1)
                    {
                        ndxEnd = text.indexOf(ql1c(']'), ndxStart + 2);
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

        if (!text.isEmpty())
            Logic::Text4Edit(text, *this, Logic::Text2DocHtmlMode::Escape, false);

        auto nickPos = -1;
        auto nickRef = QStringRef(&text).trimmed();
        if (nickRef.startsWith(ql1c('@')) && Utils::isNick(nickRef))
            nickPos = pos + nickRef.position();

        if (nickPos >= 0)
            emit nickInserted(nickPos, nickRef.mid(1).toString(), QPrivateSignal());
    }

    void TextEditEx::insertPlainText_(const QString& _text)
    {
        if (!_text.isEmpty())
            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Pass, false);
    }

    void TextEditEx::focusInEvent(QFocusEvent* _event)
    {
        emit focusIn();
        QTextBrowser::focusInEvent(_event);
    }

    void TextEditEx::focusOutEvent(QFocusEvent* _event)
    {
        if (_event->reason() != Qt::ActiveWindowFocusReason)
            emit focusOut();

        QTextBrowser::focusOutEvent(_event);
    }

    void TextEditEx::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            emit clicked();
            QTextBrowser::mousePressEvent(_event);
            if (!input_)
                _event->ignore();
        }
    }

    void TextEditEx::mouseReleaseEvent(QMouseEvent* _event)
    {
        QTextBrowser::mouseReleaseEvent(_event);
        if (!input_)
            _event->ignore();
    }

    void TextEditEx::mouseMoveEvent(QMouseEvent * _event)
    {
        if (_event->buttons() & Qt::LeftButton && !input_)
        {
            _event->ignore();
        }
        else
        {
            QTextBrowser::mouseMoveEvent(_event);
        }
    }

    void TextEditEx::keyPressEvent(QKeyEvent* _event)
    {
        emit keyPressed(_event->key());

        if (_event->key() == Qt::Key_Backspace || _event->key() == Qt::Key_Delete)
        {
            const auto canDeleteMention =
                textCursor().selectedText().isEmpty()
                && ((!textCursor().atStart() && _event->key() == Qt::Key_Backspace)
                    || (!textCursor().atEnd() && _event->key() == Qt::Key_Delete));

            if (toPlainText().isEmpty())
            {
                emit emptyTextBackspace();
            }
            else if (canDeleteMention)
            {
                auto cursor = textCursor();

                if (_event->key() == Qt::Key_Delete)
                    cursor.movePosition(QTextCursor::Right);

                const auto charFormat = cursor.charFormat();
                if (const auto anchor = charFormat.anchorHref(); Utils::isMentionLink(anchor))
                {
                    auto block = cursor.block();
                    for (QTextBlock::iterator itFragment = block.begin(); itFragment != block.end(); ++itFragment)
                    {
                        QTextFragment frag = itFragment.fragment();
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

            if (platform::is_apple() && (_event->modifiers() & Qt::ControlModifier))
            {
                auto cursor = textCursor();
                if (!cursor.selectedText().isEmpty())
                    cursor.setPosition(cursor.selectionEnd());

                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                return;
            }
        }

        if (_event->key() == Qt::Key_Escape)
            emit escapePressed();

        if (_event->key() == Qt::Key_Up)
            emit upArrow();

        if (_event->key() == Qt::Key_Down)
            emit downArrow();

        if (_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _event->modifiers();

            if (catchEnter(modifiers))
            {
                emit enter();
                return;
            }
            else
            {
                if (catchNewLine(modifiers))
                {
                    textCursor().insertText(qsl("\n"));
                    ensureCursorVisible();
                }
                return;
            }
        }

        if (_event->matches(QKeySequence::DeleteStartOfWord) && !selection().isEmpty())
        {
            _event->setModifiers(Qt::NoModifier);
            QTextBrowser::keyPressEvent(_event);
            _event->ignore();

            return;
        }

        if (platform::is_apple() && input_ && _event->key() == Qt::Key_Backspace && _event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        {
            auto cursor = textCursor();
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            //setTextCursor(cursor);

            QTextBrowser::keyPressEvent(_event);
            _event->ignore();

            return;
        }

        const auto length = document()->characterCount();
        const auto deny = limit_ != -1 && length >= limit_ && !_event->text().isEmpty();

        auto oldPos = textCursor().position();
        QTextBrowser::keyPressEvent(_event);
        auto newPos = textCursor().position();

        if (deny && document()->characterCount() > length)
            textCursor().deletePreviousChar();

        if (_event->modifiers() & Qt::ShiftModifier && _event->key() == Qt::Key_Up && oldPos == newPos)
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
        int i = 0;
        auto emoji = Emoji::getEmoji(QStringRef(&s), i);
        if (!emoji.isNull())
            insertEmoji(emoji);
        else
#endif //__APPLE__
            QTextBrowser::inputMethodEvent(e);
    }

    QMenu* TextEditEx::getContextMenu()
    {
        if (auto menu = createStandardContextMenu(); menu)
        {
            ContextMenu::applyStyle(menu, false, Utils::scale_value(15), Utils::scale_value(36));

            // The value (212) was taken from UI design.
            menu->setMinimumWidth(Utils::scale_value(212));
            return menu;
        }
        return nullptr;
    }

    QString TextEditEx::getPlainText() const
    {
        return getPlainText(0);
    }

    void TextEditEx::setPlainText(const QString& _text, bool _convertLinks, const QTextCharFormat::VerticalAlignment _aligment)
    {
        const auto prevSize = document()->size();
        {
            QSignalBlocker sb(document()->documentLayout());

            {
                QSignalBlocker sbEdit(this);
                QSignalBlocker sbDoc(document());
                clear();
            }

            resourceIndex_.clear();

            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Escape, _convertLinks, false, nullptr, Emoji::EmojiSizePx::Auto, _aligment);
        }

        if (const auto newSize = document()->size(); newSize != prevSize)
            emit document()->documentLayout()->documentSizeChanged(newSize);
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

    void TextEditEx::insertMention(const QString& _aimId, const QString& _friendly)
    {
        mentions_.emplace(_aimId, _friendly);

        const QString add = ql1s("@[") % _aimId % ql1s("] ");
        Logic::Text4Edit(add, *this, Logic::Text2DocHtmlMode::Escape, false, Emoji::EmojiSizePx::Auto);
    }

    void TextEditEx::insertEmoji(const Emoji::EmojiCode& _code)
    {
        mergeResources(Logic::InsertEmoji(_code, *this));
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
            assert(isCursorChanged);
        }

        const auto isCursorAtThePos = (cursor.position() == posTo);
        if (!isCursorAtThePos)
        {
            cursor.setPosition(posTo, QTextCursor::KeepAnchor);
            isCursorChanged = true;
        }

        if (isCursorChanged)
        {
            setTextCursor(cursor);
        }
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
        cur.clearSelection();
        setTextCursor(cur);
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

        int height = getTextSize().height();
        setFixedHeight(height + add_);

        return height;
    }

    void TextEditEx::setMaxHeight(int _height)
    {
        maxHeight_ = _height;
        editContentChanged();
    }

    void TextEditEx::setViewportMargins(int left, int top, int right, int bottom)
    {
        QAbstractScrollArea::setViewportMargins(left, top, right, bottom);
    }

    QFont TextEditEx::getFont() const
    {
        return font_;
    }

    void TextEditEx::contextMenuEvent(QContextMenuEvent *e)
    {
        if (auto menu = getContextMenu(); menu)
        {
            menu->exec(e->globalPos());
            menu->deleteLater();
        }
    }

    bool TextEditEx::catchEnterWithSettingsPolicy(const int _modifiers) const
    {
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        switch (key1_to_send)
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
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        const auto ctrl = _modifiers & Qt::ControlModifier;
        const auto shift = _modifiers & Qt::ShiftModifier;
        const auto meta = _modifiers & Qt::MetaModifier;
        const auto enter = (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);

        switch (key1_to_send)
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
}


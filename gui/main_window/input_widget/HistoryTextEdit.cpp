#include "stdafx.h"

#include "HistoryTextEdit.h"
#include "Text2Symbol.h"
#include "HistoryUndoStack.h"
#include "AttachFilePopup.h"
#include "InputWidgetUtils.h"

#include "fonts.h"
#include "main_window/MainWindow.h"
#include "main_window/MainPage.h"
#include "main_window/ContactDialog.h"
#include "main_window/smiles_menu/SuggestsWidget.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "main_window/history_control/MentionCompleter.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/MentionModel.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"
#include "../../gui_settings.h"
#include "../../controls/ContextMenu.h"

namespace
{
    constexpr auto symbolAt() noexcept { return u'@'; }

    auto getInputTextColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    QString textEditStyle()
    {
        return qsl("Ui--HistoryTextEdit {background: transparent; border: none; margin: 0; padding: 0; line-height: 16dip;}");
    }

    int getPlaceholderAnimEndX()
    {
        return Utils::scale_value(80);
    }

    bool canPasteImage()
    {
        const auto md = QGuiApplication::clipboard()->mimeData();
        return md && (md->hasImage() || md->hasUrls());
    }

    constexpr std::chrono::milliseconds getDurationAppear() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds getDurationDisappear() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds keyPressTimeout() noexcept { return std::chrono::milliseconds(100); }
}

namespace Ui
{
    HistoryTextEdit::HistoryTextEdit(QWidget* _parent)
        : TextEditEx(_parent, Fonts::getInputTextFont(), getInputTextColorKey(), true, false)
        , placeholderAnim_(new QVariantAnimation(this))
        , placeholderOpacityAnim_(new QVariantAnimation(this))
        , isEmpty_(true)
        , isPreediting_(false)
        , phAnimEnabled_(true)
        , stackRedo_(std::make_unique<HistoryUndoStack>())
        , emojiReplacer_(std::make_unique<Emoji::TextSymbolReplacer>())
        , keyPressTimer_(new QTimer(this))
        , marginScaleCorrection_(1.0)
    {
        setPlaceholderTextEx(QT_TRANSLATE_NOOP("input_widget", "Message"));
        setAcceptDrops(true);
        setAutoFillBackground(false);
        setTabChangesFocus(true);
        setEnterKeyPolicy(EnterKeyPolicy::FollowSettingsRules);
        setFormatEnabled(true);

        setDocumentMargin(0);
        document()->documentLayout()->setPaintDevice(nullptr);//avoid unnecessary scaling performed by its own paintDevice

        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);

        setUndoRedoEnabled(true);
        updateLinkColor();

        Utils::ApplyStyle(this, textEditStyle());

        setCursor(Qt::IBeamCursor);
        viewport()->setCursor(Qt::IBeamCursor);

        connect(document(), &QTextDocument::contentsChanged, this, &HistoryTextEdit::onEditContentChanged);

        keyPressTimer_->setSingleShot(true);

        placeholderAnim_->setStartValue(0.0);
        placeholderAnim_->setEndValue(1.0);
        placeholderAnim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(placeholderAnim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));

        placeholderOpacityAnim_->setStartValue(0.0);
        placeholderOpacityAnim_->setEndValue(1.0);
        connect(placeholderOpacityAnim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
        connect(this, &TextEditEx::intermediateTextChange, this, &HistoryTextEdit::onIntermediateTextChange);
    }

    void HistoryTextEdit::setPlaceholderTextEx(const QString &_text)
    {
        customPlaceholderText_ = _text;
    }

    void HistoryTextEdit::setPlaceholderAnimationEnabled(const bool _enabled)
    {
        phAnimEnabled_ = _enabled;
    }

    void HistoryTextEdit::updateLinkColor()
    {
        document()->setDefaultStyleSheet(u"a { color:" % Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_PRIMARY) % u"; text-decoration:none;}");
    }

    Emoji::TextSymbolReplacer& HistoryTextEdit::getReplacer()
    {
        return *emojiReplacer_;
    }

    HistoryUndoStack& HistoryTextEdit::getUndoRedoStack()
    {
        return *stackRedo_;
    }

    bool HistoryTextEdit::tabWillFocusNext()
    {
        return focusNextPrevChild(true);
    }

    void HistoryTextEdit::setMarginScaleCorrection(qreal _koef)
    {
        marginScaleCorrection_ = _koef;
    }

    qreal HistoryTextEdit::getMarginScaleCorrection() const
    {
        return marginScaleCorrection_;
    }

    bool HistoryTextEdit::isEmpty() const
    {
        return isEmpty_;
    }

    QWidget* HistoryTextEdit::setContentWidget(QWidget* _widget)
    {
        return std::exchange(contentWidget_, _widget);
    }

    void HistoryTextEdit::registerTextChange()
    {
        const auto currentText = getText();
        const auto stackTopText = getUndoRedoStack().top().Text();

        const auto currentTextLenght = currentText.size();
        const auto stackTextLenght = stackTopText.size();

        auto cursor = textCursor();
        auto startPosition = cursor.position();

        if (currentText == stackTopText)
        {
            getUndoRedoStack().top().Cursor() = startPosition;
            getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
            return;
        }

        if (currentTextLenght == stackTextLenght)
        {
            getUndoRedoStack().push(currentText);
            getUndoRedoStack().top().Cursor() = startPosition;
        }
        else if (currentTextLenght - stackTextLenght > 0)
        {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            auto delim = *cursor.selectedText().cbegin();

            if (delim.isSpace())
            {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor);
                qsizetype endPosition = cursor.position();

                if (isEmoji(currentText.string(), endPosition) && (endPosition == currentTextLenght))
                {
                    if (currentText.startsWith(stackTopText.string()))
                    {
                        getUndoRedoStack().top().Text() += delim;
                    }
                    else
                    {
                        getUndoRedoStack().push(currentText);
                        getUndoRedoStack().top().Cursor() = startPosition;
                    }
                }
                else
                {
                    getUndoRedoStack().push(currentText);
                    getUndoRedoStack().top().Cursor() = startPosition;
                    if (replaceEmoji())
                    {
                        startPosition = textCursor().position();
                        cursor.setPosition(startPosition);
                        getUndoRedoStack().top().Cursor() = startPosition;
                        return;
                    }
                }
            }
            else
            {
                getUndoRedoStack().push(currentText);
                getUndoRedoStack().top().Cursor() = startPosition;
                if (currentTextLenght - stackTextLenght == 1)
                    getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
            }
        }
        else
        {
            getUndoRedoStack().push(currentText);
            getUndoRedoStack().top().Cursor() = startPosition;
            cursor.setPosition(startPosition);
            if (currentText.isEmpty())
                getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
        }

        if (startPosition <= getPlainText().length())
            cursor.setPosition(startPosition);
        else
            cursor.movePosition(QTextCursor::NextCharacter);

        setTextCursor(cursor);
    }

    std::pair<QString, int> HistoryTextEdit::getLastWord()
    {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
        const auto endPosition = cursor.position();

        QChar currentChar;

        while (!currentChar.isSpace())
        {
            if (cursor.atStart())
                break;

            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            currentChar = *cursor.selectedText().begin();
            cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
        }

        if (!cursor.atStart())
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

        const auto startPositon = cursor.position();
        cursor.setPosition(startPositon, QTextCursor::MoveAnchor);
        cursor.setPosition(endPosition, QTextCursor::KeepAnchor);

        return { cursor.selectedText(), startPositon };
    }

    bool HistoryTextEdit::replaceEmoji()
    {
        if (!Ui::get_gui_settings()->get_value<bool>(settings_autoreplace_emoji, settings_autoreplace_emoji_default()))
            return false;

        const auto [lastWord, position] = getLastWord();
        const auto text = getPlainText();
        auto cursor = textCursor();
        cursor.setPosition(position);

        if (cursor.charFormat().isAnchor())
            return false;

        const auto& emojiReplacer = getReplacer();
        if (emojiReplacer.isAutoreplaceAvailable())
        {
            const auto& replacement = emojiReplacer.getReplacement(lastWord);
            if (!replacement.isNull())
            {
                const auto asCode = replacement.value<Emoji::EmojiCode>();
                const auto asString = replacement.value<QString>();
                if (!asCode.isNull() || !asString.isEmpty())
                {
                    QChar currentChar;

                    while (currentChar != QChar::Space && currentChar != QChar::LineFeed && currentChar != QChar::ParagraphSeparator)
                    {
                        if (cursor.atEnd())
                            break;

                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                        currentChar = *cursor.selectedText().crbegin();
                        cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
                    }

                    cursor.removeSelectedText();

                    if (currentChar == QChar::ParagraphSeparator)
                        currentChar = ql1c('\n');

                    if (!asString.isEmpty())
                    {
                        QTextBrowser::insertPlainText(asString % currentChar);
                    }
                    else if (!asCode.isNull())
                    {
                        insertEmoji(asCode);
                        QTextBrowser::insertPlainText(currentChar);
                        ensureCursorVisible();
                    }
                    getUndoRedoStack().push(getPlainText());

                    return true;
                }
            }
        }
        return false;
    }

    void HistoryTextEdit::keyPressEvent(QKeyEvent* _event)
    {
        keyPressTimer_->start(keyPressTimeout());

        if (passKeyEventToPopup(_event))
            return;

        if (passKeyEventToContentWidget(_event))
            return;

        if (processUndoRedo(_event))
            return;

        if (_event->matches(QKeySequence::Paste) && !isVisible())
        {
            _event->accept();
            return;
        }

        auto cursor = textCursor();
        const auto block = cursor.block();
        const auto bf = Logic::TextBlockFormat(block.blockFormat());
        const auto key = _event->key();
        const auto isEnter = catchNewLine(_event->modifiers());
        const auto blockText = block.text();

        if (core::data::format_type::ordered_list == bf.type() && isEnter && blockText.isEmpty())
        {
            // Remove state with empty ordered paragraph when delete style
            getUndoRedoStack().pop();
        }

        TextEditEx::keyPressEvent(_event);

        if (cursor == textCursor())
        {
            const auto moveMode = _event->modifiers().testFlag(Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
            bool set = false;
            if (key == Qt::Key_Up || key == Qt::Key_PageUp)
                set = cursor.movePosition(QTextCursor::Start, moveMode);
            else if (key == Qt::Key_Down || key == Qt::Key_PageDown)
                set = cursor.movePosition(QTextCursor::End, moveMode);

            if (set && cursor != textCursor())
                setTextCursor(cursor);
        }
    }

    void HistoryTextEdit::paintEvent(QPaintEvent* _event)
    {
        QPainter p(viewport());

        if (useCursorPaintHack_ && hasFocus() && !keyPressTimer_->isActive())
        {
            // draw cursor one more time to avoid some gui artifacts; maybe it'll go away in further Qt version
            auto block = document()->findBlock(textCursor().position());
            if (block.isValid())
                block.layout()->drawCursor(&p, { 0, static_cast<qreal>(-verticalScrollBar()->value()) }, textCursor().positionInBlock());
        }

        const auto animRunning = placeholderAnim_->state() == QAbstractAnimation::State::Running;
        if (!customPlaceholderText_.isEmpty() && (animRunning || (isEmpty_ && !isPreediting_ && !isCursorInBlockFormat())))
        {
            p.setFont(getFont());

            const auto colorVar = hasFocus() ? Styling::StyleVariable::BASE_PRIMARY : Styling::StyleVariable::BASE_PRIMARY_ACTIVE;

            auto color = Styling::getParameters().getColor(colorVar);
            auto x = 0;
            if (animRunning)
            {
                const auto val = placeholderAnim_->currentValue().toDouble();
                if (phAnimType_ == AnimType::Appear)
                {
                    color.setAlphaF(val);
                }
                else
                {
                    const auto alphaVal = placeholderOpacityAnim_->currentValue().toDouble();
                    color.setAlphaF(1.0 - alphaVal);
                    x = getPlaceholderAnimEndX() * val, 0;
                }
            }

            p.setPen(color);
            const auto fmt = document()->rootFrame()->frameFormat();
            p.drawText(viewport()->rect().translated(fmt.leftMargin() * marginScaleCorrection_ + x, fmt.topMargin() * marginScaleCorrection_), Qt::AlignTop | Qt::TextWordWrap, customPlaceholderText_);
        }

        TextEditEx::paintEvent(_event);
    }

    bool HistoryTextEdit::focusNextPrevChild(bool _next)
    {
        if (!getInputPopups().empty())
            return false;

        return TextEditEx::focusNextPrevChild(_next);
    }

    void HistoryTextEdit::contextMenuEvent(QContextMenuEvent* e)
    {
        if (auto menu = createContextMenu(); menu)
        {
            const auto actions = menu->actions();
            const auto it = std::find_if(actions.begin(), actions.end(), [](auto a) { return a->objectName() == u"edit-paste"; });
            if (it != actions.end() && !(*it)->isEnabled())
            {
                if (canPasteImage())
                    (*it)->setEnabled(true);
            }
            menu->setAttribute(Qt::WA_DeleteOnClose);
            menu->popup(e->globalPos());
            ContextMenu::updatePosition(menu, e->globalPos());
        }
    }

    void HistoryTextEdit::checkMentionNeeded()
    {
        auto cursor = textCursor();
        const auto pos = cursor.position();
        if (getPlainText().contains(symbolAt()))
        {
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);

            QChar currentChar;
            while (currentChar != symbolAt())
            {
                if (cursor.atStart())
                    break;

                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                currentChar = *cursor.selectedText().begin();

                if (currentChar == QChar::Space)
                {
                    cursor.setPosition(pos);
                    setTextCursor(cursor);
                    return;
                }

                cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
            }
            setTextCursor(cursor);
            Q_EMIT needsMentionComplete(cursor.position(), QPrivateSignal());
            cursor.setPosition(pos);
            setTextCursor(cursor);
        }
    }

    void HistoryTextEdit::onEditContentChanged()
    {
        const auto animate = [this](const auto _type, const std::chrono::milliseconds _duration)
        {
            phAnimType_ = _type;

            placeholderAnim_->stop();
            placeholderAnim_->setDuration(_duration.count());
            placeholderAnim_->start();

            placeholderOpacityAnim_->stop();
            if (phAnimType_ == AnimType::Disappear)
            {
                placeholderOpacityAnim_->setDuration(_duration.count() / 2);
                placeholderOpacityAnim_->start();
            }
        };

        const bool wasEmpty = isEmpty_;
        const bool wasPreediting = isPreediting_;
        isEmpty_ = document()->isEmpty();
        isPreediting_ = isPreediting();

        if (!phAnimEnabled_)
            return;

        if ((wasPreediting && !isPreediting_ && !isEmpty_) || isCursorInBlockFormat())
            return;

        if ((!wasEmpty && isEmpty_) || (wasPreediting && !isPreediting_))
            animate(AnimType::Appear, getDurationAppear());
        else if ((wasEmpty && !isEmpty_) || (!wasPreediting && isPreediting_))
            animate(AnimType::Disappear, getDurationDisappear());
    }

    void HistoryTextEdit::onIntermediateTextChange(const Data::FString& _string, int _cursor)
    {
        auto& stack = getUndoRedoStack();
        stack.push(_string);
        stack.top().Cursor() = _cursor;
    }

    bool HistoryTextEdit::isPreediting() const
    {
        const auto l = textCursor().block().layout();
        return l && !l->preeditAreaText().isEmpty();
    }

    bool HistoryTextEdit::passKeyEventToContentWidget(QKeyEvent* _e)
    {
        if (contentWidget_)
        {
            const auto atTextStart = textCursor().atStart();
            const auto atTextEnd = textCursor().atEnd();

            const auto needSendStart = [_e]()
            {
                return _e->key() == Qt::Key_Up || (_e->key() == Qt::Key_PageUp && _e->modifiers() == Qt::NoModifier) || isApplePageUp(_e);
            };
            const auto needSendEnd = [_e, emptyInput = atTextStart == atTextEnd]()
            {
                const auto ctrlEnd = emptyInput && isCtrlEnd(_e);
                return _e->key() == Qt::Key_Down || (_e->key() == Qt::Key_PageDown && _e->modifiers() == Qt::NoModifier) || isApplePageDown(_e) || ctrlEnd;
            };

            if ((atTextStart && needSendStart()) || (atTextEnd && needSendEnd()))
            {
                Q_EMIT navigationKeyPressed(_e->key(), _e->modifiers(), QPrivateSignal());
                _e->accept();
                return true;
            }
        }

        return false;
    }

    bool HistoryTextEdit::passKeyEventToPopup(QKeyEvent* _e)
    {
        static constexpr Qt::Key keys[] = {
                Qt::Key_Up,
                Qt::Key_Down,
                Qt::Key_Left,
                Qt::Key_Right,
                Qt::Key_Enter,
                Qt::Key_Return,
                Qt::Key_Tab,
                Qt::Key_Backtab,
        };
        const auto acceptableKey =
            !(_e->modifiers() & Qt::ShiftModifier) &&
            std::any_of(std::begin(keys), std::end(keys), [key = _e->key()](const auto x) { return x == key; });

        if (!acceptableKey)
            return false;

        if (auto popups = getInputPopups(); !popups.empty())
        {
            QApplication::sendEvent(popups.front(), _e);
            return _e->isAccepted();
        }
        return false;
    }

    bool HistoryTextEdit::processUndoRedo(QKeyEvent* _e)
    {
        const auto isUndo = _e->matches(QKeySequence::Undo);
        const auto isRedo = _e->matches(QKeySequence::Redo);

        if (!isUndo && !isRedo)
            return false;

        const auto canDo = isUndo ? getUndoRedoStack().canUndo() : getUndoRedoStack().canRedo();
        if (canDo)
        {
            emojiReplacer_->setAutoreplaceAvailable(isUndo ? Emoji::ReplaceAvailable::Unavailable : Emoji::ReplaceAvailable::Available);

            if (isUndo)
                getUndoRedoStack().pop();
            else
                getUndoRedoStack().redo();

            auto cursor = textCursor();
            auto currentTop = getUndoRedoStack().top().Text();
            const auto prevPosition = getUndoRedoStack().top().Cursor();

            if (currentTop.hasFormatting())
                setFormattedText(currentTop);
            else
                setPlainText(currentTop.string(), false);

            checkMentionNeeded();
            if (prevPosition > -1 && prevPosition < getPlainText().length())
                cursor.setPosition(prevPosition);
            else
                cursor.movePosition(QTextCursor::End);

            setTextCursor(cursor);
        }

        _e->accept();
        return true;
    }

    std::vector<QWidget*> HistoryTextEdit::getInputPopups() const
    {
        auto popups = std::vector<QWidget*>
        {
            getWidgetIf<MentionCompleter>(contentWidget_, &MentionCompleter::completerVisible),
            getWidgetIf<Stickers::StickersSuggest>(contentWidget_, &Stickers::StickersSuggest::isTooltipVisible),
            getWidgetIfNot<Smiles::SmilesMenu>(contentWidget_, &Smiles::SmilesMenu::isHidden),
            getWidgetIf<AttachFilePopup>(window(), &AttachFilePopup::isVisible),
        };
        popups.erase(std::remove_if(popups.begin(), popups.end(), [](auto p) { return p == nullptr; }), popups.end());

        return popups;
    }

}

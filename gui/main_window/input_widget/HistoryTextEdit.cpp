#include "stdafx.h"

#include "HistoryTextEdit.h"
#include "Text2Symbol.h"
#include "HistoryUndoStack.h"
#include "AttachFilePopup.h"


#include "fonts.h"
#include "main_window/MainWindow.h"
#include "main_window/MainPage.h"
#include "main_window/ContactDialog.h"
#include "main_window/smiles_menu/suggests_widget.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/history_control/MentionCompleter.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/MentionModel.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto symbolAt() noexcept { return u'@'; }

    QFont getInputTextFont()
    {
        auto font = Fonts::appFontScaled(15);

        if constexpr (platform::is_apple())
            font.setLetterSpacing(QFont::AbsoluteSpacing, -0.25);

        return font;
    }

    QColor getInputTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
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
}

namespace Ui
{
    HistoryTextEdit::HistoryTextEdit(QWidget* _parent)
        : TextEditEx(_parent, getInputTextFont(), getInputTextColor(), true, false)
        , placeholderAnim_(new QVariantAnimation(this))
        , placeholderOpacityAnim_(new QVariantAnimation(this))
        , isEmpty_(true)
        , isPreediting_(false)
        , phAnimEnabled_(true)
        , stackRedo_(std::make_unique<HistoryUndoStack>())
        , emojiReplacer_(std::make_unique<Emoji::TextSymbolReplacer>())
    {
        setPlaceholderTextEx(QT_TRANSLATE_NOOP("input_widget", "Message"));
        setAcceptDrops(false);
        setAutoFillBackground(false);
        setTabChangesFocus(true);
        setEnterKeyPolicy(EnterKeyPolicy::FollowSettingsRules);

        document()->setDocumentMargin(0);

        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);

        setUndoRedoEnabled(true);
        updateLinkColor();

        Utils::ApplyStyle(this, textEditStyle());

        setCursor(Qt::IBeamCursor);
        viewport()->setCursor(Qt::IBeamCursor);

        connect(document(), &QTextDocument::contentsChanged, this, &HistoryTextEdit::onEditContentChanged);
        connect(placeholderAnim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
        connect(placeholderOpacityAnim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
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

    void HistoryTextEdit::keyPressEvent(QKeyEvent* _e)
    {
        if (auto page = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact()))
        {
            const auto forwardKeyToWidget = [_e](const auto _widget, const auto& _keys)
            {
                if (_widget && !(_e->modifiers() & Qt::ShiftModifier) && std::any_of(std::begin(_keys), std::end(_keys), [key = _e->key()](const auto x) { return x == key; }))
                {
                    QApplication::sendEvent(_widget, _e);
                    return _e->isAccepted();
                }
                return false;
            };

            if (const auto mentionCompleter = page->getMentionCompleter(); mentionCompleter && mentionCompleter->completerVisible())
            {
                static constexpr Qt::Key keys[] = {
                    Qt::Key_Up,
                    Qt::Key_Down,
                    Qt::Key_Enter,
                    Qt::Key_Return,
                    Qt::Key_Tab,
                };

                if (forwardKeyToWidget(mentionCompleter, keys))
                    return;
            }

            if (auto mainPage = Utils::InterConnector::instance().getMainWindow()->getMainPage())
            {
                if (const auto suggest = mainPage->getStickersSuggest(); suggest && suggest->isTooltipVisible())
                {
                    static constexpr Qt::Key keys[] = {
                        Qt::Key_Up,
                        Qt::Key_Down,
                        Qt::Key_Enter,
                        Qt::Key_Return,
                        Qt::Key_Left,
                        Qt::Key_Right,
                    };

                    if (forwardKeyToWidget(suggest, keys))
                        return;
                }

                if (auto cd = mainPage->getContactDialog(); cd && cd->isShowingSmileMenu())
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
                        Qt::Key_Escape,
                    };

                    if (forwardKeyToWidget(cd->getSmilesMenu(), keys))
                        return;
                }
            }

            if (AttachFilePopup::isOpen())
            {
                static constexpr Qt::Key keys[] = {
                    Qt::Key_Up,
                    Qt::Key_Down,
                    Qt::Key_Enter,
                    Qt::Key_Return,
                    Qt::Key_Tab,
                    Qt::Key_Backtab,
                };

                if (forwardKeyToWidget(&AttachFilePopup::instance(), keys))
                    return;
            }

            const auto atTextStart = textCursor().atStart();
            const auto atTextEnd = textCursor().atEnd();

            if (atTextStart)
            {
                const auto applePageUp = (platform::is_apple() && _e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && _e->key() == Qt::Key_Up);
                if (_e->key() == Qt::Key_Up || _e->key() == Qt::Key_PageUp || applePageUp)
                {
                    QApplication::sendEvent(page, _e);
                    _e->accept();
                    return;
                }
            }

            if (atTextEnd)
            {
                const auto applePageDown = (platform::is_apple() && _e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && _e->key() == Qt::Key_Down);
                const auto applePageEnd = (platform::is_apple() && ((_e->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && _e->key() == Qt::Key_Right) || _e->key() == Qt::Key_End));
                const auto ctrlEnd = atTextStart == atTextEnd && ((_e->modifiers() == Qt::CTRL && _e->key() == Qt::Key_End) || applePageEnd);

                if (_e->key() == Qt::Key_Down || _e->key() == Qt::Key_PageDown || applePageDown || ctrlEnd)
                {
                    QApplication::sendEvent(page, _e);
                    _e->accept();
                    return;
                }
            }
        }

        if (_e->matches(QKeySequence::Undo))
        {
            if (getUndoRedoStack().canUndo())
            {
                auto cursor = textCursor();
                getUndoRedoStack().pop();
                emojiReplacer_->setAutoreplaceAvailable(Emoji::ReplaceAvailable::Unavailable);
                const auto currentTop = getUndoRedoStack().top().Text();
                const auto prevPosition = getUndoRedoStack().top().Cursor();

                const auto delim = currentTop.endsWith(QChar::LineFeed) ? QChar::LineFeed : QChar::Null;
                setPlainText(currentTop % delim, false);

                checkMentionNeeded();
                if (prevPosition > -1 && prevPosition < getPlainText().length())
                    cursor.setPosition(prevPosition);
                else
                    cursor.movePosition(QTextCursor::End);

                setTextCursor(cursor);
            }
            _e->accept();
            return;
        }
        else if (_e->matches(QKeySequence::Redo))
        {
            if (getUndoRedoStack().canRedo())
            {
                auto cursor = textCursor();
                emojiReplacer_->setAutoreplaceAvailable();
                getUndoRedoStack().redo();

                const auto currentTop = getUndoRedoStack().top().Text();
                const auto prevPosition = getUndoRedoStack().top().Cursor();
                const auto delim = currentTop.endsWith(QChar::LineFeed) ? QChar::LineFeed : QChar::Null;
                setPlainText(currentTop % delim, false);
                checkMentionNeeded();

                if (prevPosition > -1 && prevPosition < getPlainText().length())
                    cursor.setPosition(prevPosition);
                else
                    cursor.movePosition(QTextCursor::End);

                setTextCursor(cursor);
            }
            _e->accept();
            return;
        }

        if (!isVisible())
        {
            if (_e->matches(QKeySequence::Paste))
            {
                _e->accept();
                return;
            }
        }

        auto cursor = textCursor();
        TextEditEx::keyPressEvent(_e);

        if (cursor == textCursor())
        {
            const auto moveMode = _e->modifiers().testFlag(Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
            bool set = false;
            if (_e->key() == Qt::Key_Up || _e->key() == Qt::Key_PageUp)
                set = cursor.movePosition(QTextCursor::Start, moveMode);
            else if (_e->key() == Qt::Key_Down || _e->key() == Qt::Key_PageDown)
                set = cursor.movePosition(QTextCursor::End, moveMode);

            if (set && cursor != textCursor())
                setTextCursor(cursor);
        }
    }

    void HistoryTextEdit::paintEvent(QPaintEvent* _event)
    {
        QPainter p(viewport());

        if (hasFocus())
        {
            //draw cursor one more time to avoid some gui artifacts; maybe it'll go away in further Qt version
            auto block = document()->findBlock(textCursor().position());
            if (block.isValid())
                block.layout()->drawCursor(&p, { 0, static_cast<qreal>(-verticalScrollBar()->value()) }, textCursor().positionInBlock());
        }

        const auto animRunning = placeholderAnim_->state() == QAbstractAnimation::State::Running;
        if (!customPlaceholderText_.isEmpty() && (animRunning || (isEmpty_ && !isPreediting_)))
        {
            p.setFont(getFont());

            const auto colorVar = hasFocus() ? Styling::StyleVariable::BASE_PRIMARY : Styling::StyleVariable::BASE_PRIMARY_ACTIVE;

            auto color = Styling::getParameters().getColor(colorVar);
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
                    p.translate(getPlaceholderAnimEndX() * val, 0);
                }
            }

            p.setPen(color);
            const auto fmt = document()->rootFrame()->frameFormat();
            p.drawText(viewport()->rect().translated(fmt.leftMargin(), fmt.topMargin()), Qt::AlignTop | Qt::TextWordWrap, customPlaceholderText_);
        }

        TextEditEx::paintEvent(_event);
    }

    bool HistoryTextEdit::focusNextPrevChild(bool _next)
    {
        if (const auto mainPage = Utils::InterConnector::instance().getMainWindow()->getMainPage())
        {
            if (const auto cd = mainPage->getContactDialog())
            {
                if (AttachFilePopup::isOpen() || cd->isShowingSmileMenu())
                {
                    return false;
                }
                else if (const auto page = cd->getHistoryPage(Logic::getContactListModel()->selectedContact()))
                {
                    if (const auto mc = page->getMentionCompleter(); mc && mc->completerVisible())
                        return false;
                }
            }
        }

        return TextEditEx::focusNextPrevChild(_next);
    }

    void HistoryTextEdit::contextMenuEvent(QContextMenuEvent* e)
    {
        if (auto menu = getContextMenu(); menu)
        {
            const auto actions = menu->actions();
            const auto it = std::find_if(actions.begin(), actions.end(), [](auto a) { return a->objectName() == u"edit-paste"; });
            if (it != actions.end() && !(*it)->isEnabled())
            {
                if (canPasteImage())
                    (*it)->setEnabled(true);
            }
            menu->exec(e->globalPos());
            menu->deleteLater();
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
            placeholderAnim_->setStartValue(0.0);
            placeholderAnim_->setEndValue(1.0);
            placeholderAnim_->setDuration(_duration.count());
            placeholderAnim_->setEasingCurve(QEasingCurve::InOutSine);
            placeholderAnim_->start();

            placeholderOpacityAnim_->stop();
            if (phAnimType_ == AnimType::Disappear)
            {
                placeholderOpacityAnim_->setStartValue(0.0);
                placeholderOpacityAnim_->setEndValue(1.0);
                placeholderOpacityAnim_->setDuration(_duration.count() / 2);
                placeholderOpacityAnim_->setEasingCurve(QEasingCurve::Linear);
                placeholderOpacityAnim_->start();
            }
        };

        const bool wasEmpty = isEmpty_;
        const bool wasPreediting = isPreediting_;
        isEmpty_ = document()->isEmpty();
        isPreediting_ = isPreediting();

        if (!phAnimEnabled_)
            return;

        if (wasPreediting && !isPreediting_ && !isEmpty_)
            return;

        if ((!wasEmpty && isEmpty_) || (wasPreediting && !isPreediting_))
            animate(AnimType::Appear, getDurationAppear());
        else if ((wasEmpty && !isEmpty_) || (!wasPreediting && isPreediting_))
            animate(AnimType::Disappear, getDurationDisappear());
    }

    bool HistoryTextEdit::isPreediting() const
    {
        const auto l = textCursor().block().layout();
        return l && !l->preeditAreaText().isEmpty();
    }
}

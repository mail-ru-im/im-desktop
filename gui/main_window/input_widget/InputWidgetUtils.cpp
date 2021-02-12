#include "stdafx.h"

#include "InputWidgetUtils.h"
#include "HistoryTextEdit.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "controls/CustomButton.h"
#include "controls/textrendering/TextRendering.h"
#include "fonts.h"

#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/contact_list/ContactListModel.h"

#include "core_dispatcher.h"
#include "gui_settings.h"

namespace Ui
{
    std::string getStatsChatType()
    {
        const auto& contact = Logic::getContactListModel()->selectedContact();
        return Utils::isChat(contact) ? (Logic::getContactListModel()->isChannel(contact) ? "channel" : "group") : "chat";
    }

    int getInputTextLeftMargin()
    {
        return Utils::scale_value(16);
    }

    int getDefaultInputHeight()
    {
        return Utils::scale_value(52);
    }

    int getHorMargin()
    {
        return Utils::scale_value(52);
    }

    int getVerMargin()
    {
        return Utils::scale_value(8);
    }

    int getHorSpacer()
    {
        return Utils::scale_value(12);
    }

    int getEditHorMarginLeft()
    {
        return Utils::scale_value(20);
    }

    int getQuoteRowHeight()
    {
        return Utils::scale_value(16);
    }

    int getBubbleCornerRadius()
    {
        return Utils::scale_value(18);
    }

    QSize getCancelBtnIconSize()
    {
        return QSize(16, 16);
    }

    QSize getCancelButtonSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    void drawInputBubble(QPainter& _p, const QRect& _widgetRect, const QColor& _color, const int _topMargin, const int _botMargin)
    {
        if (_widgetRect.isEmpty() || !_color.isValid())
            return;

        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);

        const auto bubbleRect = _widgetRect.adjusted(0, _topMargin, 0, -_botMargin);
        const auto radius = getBubbleCornerRadius();

        QPainterPath path;
        path.addRoundedRect(bubbleRect, radius, radius);

        Utils::drawBubbleShadow(_p, path, radius);

        _p.setPen(Qt::NoPen);
        _p.setCompositionMode(QPainter::CompositionMode_Source);
        _p.fillPath(path, _color);
    }

    bool isMarkdown(const QStringRef& _text, const int _position)
    {
        auto position = _position;
        if (position != 0)
        {
            const auto rightPart = _text.mid(_position);
            position = rightPart.indexOf(QChar::Space);

            if (position == -1)
                position = rightPart.indexOf(ql1c('\n'));
        }

        if (position == -1)
            position = _position;
        else
            position += _position;

        const auto leftPart = _text.mid(0, (position == 0) ? 0 : position + 1);

        bool isMD = false;

        const auto M_MARK = Ui::TextRendering::tripleBackTick().toString();
        const auto mmCount = leftPart.count(M_MARK);

        if (mmCount % 2 != 0)
        {
            isMD = true;
        }
        else
        {
            if (mmCount % 2 == 0)
            {
                auto lastLF = -1;
                auto textToCheck = _text;

                if (const auto lastMM = _text.lastIndexOf(M_MARK); lastMM != -1)
                    textToCheck = _text.mid(lastMM + M_MARK.size());

                lastLF = textToCheck.lastIndexOf(ql1c('\n'));

                lastLF = (lastLF == -1) ? 0 : lastLF;
                const auto partAfterLF = textToCheck.mid(lastLF);
                const auto S_MARK = Ui::TextRendering::singleBackTick();
                const auto smCount = partAfterLF.count(S_MARK);
                isMD = (smCount > 0 && smCount % 2 != 0);
            }
        }

        return isMD;
    }

    MentionCompleter* getMentionCompleter()
    {
        const auto currentContact = Logic::getContactListModel()->selectedContact();
        if (!currentContact.isEmpty())
        {
            auto page = Utils::InterConnector::instance().getHistoryPage(currentContact);
            if (page)
                return page->getMentionCompleter();
        }

        return nullptr;

    }

    bool showMentionCompleter(const QString& _initialPattern, const QPoint& _pos)
    {
        auto page = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
        if (page)
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_popup_show);
            page->showMentionCompleter(_initialPattern, _pos);
            return true;
        }

        return false;
    }

    bool isEmoji(QStringView _text, qsizetype& _pos)
    {
        const auto emojiMaxSize = Emoji::EmojiCode::maxSize();
        for (size_t i = 0; i < emojiMaxSize; ++i)
        {
            qsizetype position = _pos - i;
            if (Emoji::getEmoji(_text, position) != Emoji::EmojiCode())
            {
                _pos = position;
                return true;
            }
        }
        return false;
    }

    std::pair<QString, int> getLastWord(HistoryTextEdit* _textEdit)
    {
        auto cursor = _textEdit->textCursor();
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

    bool replaceEmoji(HistoryTextEdit* _textEdit)
    {
        if (!Ui::get_gui_settings()->get_value<bool>(settings_autoreplace_emoji, settings_autoreplace_emoji_default()))
            return false;

        const auto[lastWord, position] = getLastWord(_textEdit);
        const auto text = _textEdit->getPlainText();
        auto cursor = _textEdit->textCursor();
        cursor.setPosition(position);

        if (cursor.charFormat().isAnchor())
            return false;

        if (isMarkdown(QStringRef(&text), position))
        {
            cursor.movePosition(QTextCursor::End);
            return false;
        }

        const auto &emojiReplacer = _textEdit->getReplacer();
        if (emojiReplacer.isAutoreplaceAvailable())
        {
            const auto &replacement = emojiReplacer.getReplacement(lastWord);
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
                        _textEdit->insertPlainText(asString % currentChar);
                    }
                    else if (!asCode.isNull())
                    {
                        _textEdit->insertEmoji(asCode);
                        _textEdit->insertPlainText(currentChar);
                        _textEdit->ensureCursorVisible();
                    }
                    _textEdit->getUndoRedoStack().push(_textEdit->getPlainText());

                    return true;
                }
            }
        }
        return false;
    }


    void RegisterTextChange(HistoryTextEdit* _textEdit)
    {
        const auto currentText = _textEdit->getPlainText();
        const auto stackTopText = _textEdit->getUndoRedoStack().top().Text();

        const auto currentTextLenght = currentText.length();
        const auto stackTextLenght = stackTopText.length();

        auto cursor = _textEdit->textCursor();
        auto startPosition = cursor.position();

        if (currentText == stackTopText)
        {
            _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
            _textEdit->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
            return;
        }

        if (currentTextLenght == stackTextLenght)
        {
            _textEdit->getUndoRedoStack().push(currentText);
            _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
        }
        else if (currentTextLenght - stackTextLenght > 0)
        {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            auto delim = *cursor.selectedText().cbegin();

            if (delim.isSpace())
            {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor);
                qsizetype endPosition = cursor.position();

                if (isEmoji(currentText, endPosition) && (endPosition == currentTextLenght))
                {
                    if (currentText.startsWith(stackTopText))
                        _textEdit->getUndoRedoStack().top().Text() += delim;
                    else
                    {
                        _textEdit->getUndoRedoStack().push(currentText);
                        _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
                    }
                }
                else
                {
                    _textEdit->getUndoRedoStack().push(currentText);
                    _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
                    if (replaceEmoji(_textEdit))
                    {
                        startPosition = _textEdit->textCursor().position();
                        cursor.setPosition(startPosition);
                        _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
                        return;
                    }
                }
            }
            else
            {
                _textEdit->getUndoRedoStack().push(currentText);
                _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
                if (currentTextLenght - stackTextLenght == 1)
                    _textEdit->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
            }
        }
        else
        {
            _textEdit->getUndoRedoStack().push(currentText);
            _textEdit->getUndoRedoStack().top().Cursor() = startPosition;
            cursor.setPosition(startPosition);
            if (currentText.isEmpty())
                _textEdit->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Available);
        }

        if (startPosition <= _textEdit->getPlainText().length())
            cursor.setPosition(startPosition);
        else
            cursor.movePosition(QTextCursor::NextCharacter);

        _textEdit->setTextCursor(cursor);
    }

    void updateButtonColors(CustomButton* _button, const InputStyleMode _mode)
    {
        if (_mode == InputStyleMode::Default)
            Styling::InputButtons::Default::setColors(_button);
        else
            Styling::InputButtons::Alternate::setColors(_button);
    }

    QColor getPopupHoveredColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.05);
    }

    QColor getPopupPressedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.08);
    }

    QColor focusColorPrimary()
    {
        return Styling::getParameters().getPrimaryTabFocusColor();
    }

    QColor focusColorAttention()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION, 0.22);
    }

    void sendStat(const core::stats::stats_event_names _event, const std::string_view _from)
    {
        Ui::GetDispatcher()->post_stats_to_core(_event, { { "from", std::string(_from) }, {"chat_type", Ui::getStatsChatType() } });
    }

    void sendShareStat(bool _sent)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharecontactscr_contact_action,
        {
            {"chat_type", Ui::getStatsChatType() },
            { "type", std::string("internal") },
            { "status", std::string( _sent ? "sent" : "not_sent") }
        });
    }

    BubbleWidget::BubbleWidget(QWidget* _parent, const int _topMargin, const int _botMargin, const Styling::StyleVariable _bgColor)
        : QWidget(_parent)
        , topMargin_(_topMargin)
        , botMargin_(_botMargin)
        , bgColor_(_bgColor)
    {}

    void BubbleWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        Ui::drawInputBubble(p, rect(), Styling::getParameters().getColor(bgColor_), topMargin_, botMargin_);
    }
}

namespace Styling::InputButtons::Default
{
    void setColors(Ui::CustomButton* _button)
    {
        assert(_button);
        if (_button)
        {
            _button->setDefaultColor(defaultColor());
            _button->setHoverColor(hoverColor());
            _button->setPressedColor(pressedColor());
            _button->setActiveColor(activeColor());
        }
    }

    QColor defaultColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor hoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY_HOVER);
    }

    QColor pressedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY_ACTIVE);
    }

    QColor activeColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }
}

namespace Styling::InputButtons::Alternate
{
    void setColors(Ui::CustomButton* _button)
    {
        assert(_button);
        if (_button)
        {
            _button->setDefaultColor(defaultColor());
            _button->setHoverColor(hoverColor());
            _button->setPressedColor(pressedColor());
            _button->setActiveColor(activeColor());
        }
    }

    QColor defaultColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE);
    }

    QColor hoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE_HOVER);
    }

    QColor pressedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE);
    }

    QColor activeColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE);
    }
}

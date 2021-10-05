#pragma once

#include "../namespaces.h"

#include "../utils/utils.h"

#include  "../types/message.h"

#include "../utils/Text2DocConverter.h"

FONTS_NS_BEGIN

enum class FontFamily;
enum class FontWeight;

FONTS_NS_END

namespace Emoji
{
    class EmojiCode;
}

namespace Ui
{
    using ResourceMap = std::map<QString, QString>;

    class TextEditEx : public QTextBrowser
    {
        Q_OBJECT

    Q_SIGNALS:

        void focusIn();
        void focusOut();
        void clicked();
        void emptyTextBackspace();
        void escapePressed();
        void upArrow();
        void downArrow();
        void enter();
        void setSize(int, int);
        void keyPressed(int);
        void nickInserted(const int _atSignPos, const QString& _nick, QPrivateSignal);
        void insertFiles();
        void intermediateTextChange(const Data::FString& _string, int _cursor);

    private Q_SLOTS:
        void editContentChanged();
        void onAnchorClicked(const QUrl& _url);
        void onCursorPositionChanged();
        void onContentsChanged(int _pos, int _removed, int _added);
        void onTextChanged();
        void onFormatUserAction(core::data::format_type _type);

    public:
        TextEditEx(QWidget* _parent, const QFont& _font, const QPalette& _palette, bool _input, bool _isFitToText);
        TextEditEx(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText);

        void setFormatEnabled(bool _isEnabled) { isFormatEnabled_ = _isEnabled; }
        void limitCharacters(const int _count);

        virtual QSize sizeHint() const override;
        void setPlaceholderText(const QString& _text);

        bool isEmptyOrHasOnlyWhitespaces() const;
        QString getPlainText() const;
        Data::FString getText() const;
        void setPlainText(const QString& _text, bool _convertLinks = true, const QTextCharFormat::VerticalAlignment _alignment = QTextCharFormat::AlignBottom);
        void insertFormattedText(Data::FStringView _text, const QTextCharFormat::VerticalAlignment _alignment = QTextCharFormat::AlignBottom);
        void setFormattedText(Data::FStringView _text, const QTextCharFormat::VerticalAlignment _alignment = QTextCharFormat::AlignBottom);
        void clear();

        void setMentions(Data::MentionMap _mentions);
        const Data::MentionMap& getMentions() const;

        void setFiles(Data::FilesPlaceholderMap _files);
        const Data::FilesPlaceholderMap& getFiles() const;

        void mergeResources(const ResourceMap& _resources);
        void insertEmoji(const Emoji::EmojiCode& _code);
        void insertPlainText(const QString& _text, bool _convertLinks = false, const QTextCharFormat::VerticalAlignment _alignment = QTextCharFormat::AlignBottom);
        void insertMention(const QString& _aimId, const QString& _friendly, bool _addSpace = true);

        void selectWholeText();
        void selectFromBeginning(const QPoint& _p);
        void selectTillEnd(const QPoint& _p);
        void selectByPos(const QPoint& _p);
        void selectByPos(const QPoint& _from, const QPoint& _to);
        void clearSelection();
        bool isAllSelected();
        QString selection();
        Data::FormatTypes getCurrentNonMentionFormats() const;

        //! Open url edit dialog for user interaction
        void formatRangeCreateLink(core::data::range _range);

        core::data::range formatRange(const core::data::range_format& _format);

        void formatRangeStyleType(core::data::range_format _format);

        void replaceSelectionBy(const Data::FString& _text);

        //! ... where _pos is any position in the line
        void removeBlockFormatFromLine(int _pos);

        void removeBlockFormatFromCurrentLine(QTextCursor& _cursor);

        core::data::range surroundWithNewLinesIfNone(core::data::range _range, bool _onlyIfBlock = false);

        //! Returns new range as it could be changed
        core::data::range formatRangeBlockType(core::data::range_format _format);

        void formatRangeClearBlockTypes(core::data::range _range);

        //! Might perform auxiliary user interaction, e. g. show url editor dialog
        void formatSelectionUserAction(core::data::format_type _type);

        bool isAnyFormatActionAllowed() const;
        bool isFormatActionAllowed(core::data::format_type _type, bool _justCheckIfAnyAllowed = false) const;

        //! As number of actions are to be forbidden in code block format
        bool isCursorInCode() const;

        QSize getTextSize() const;
        int32_t getTextHeight() const;
        int32_t getTextWidth() const;

        enum class EnterKeyPolicy
        {
            None,
            CatchNewLine,
            CatchEnter,
            CatchEnterAndNewLine,
            FollowSettingsRules,
        };

        void setEnterKeyPolicy(EnterKeyPolicy _policy);

        bool catchEnter(const int _modifiers);
        bool catchNewLine(const int _modifiers);

        int adjustHeight(int _width);

        void addSpace(int _space) { add_ = _space; }
        void setMaxHeight(int _height);

        void setViewportMargins(int left, int top, int right, int bottom);

        const QFont& getFont() const noexcept { return font_; }

        void setLinkToSelection(const QString& _url);
        void clearAdjacentLinkFormats(core::data::range _range, std::string_view _url);

        void execContextMenu(QPoint _globalPos);

    protected:
        void focusInEvent(QFocusEvent*) override;
        void focusOutEvent(QFocusEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void keyPressEvent(QKeyEvent*) override;
        void inputMethodEvent(QInputMethodEvent*) override;
        void paintEvent(QPaintEvent* _event) override;

        virtual QMenu* createContextMenu();

        QMimeData* createMimeDataFromSelection() const override;
        void insertFromMimeData(const QMimeData* _source) override;
        void contextMenuEvent(QContextMenuEvent *e) override;
        bool isCursorInBlockFormat() const;

    private:
        QString getPlainText(int _from, int _to = -1) const;
        Data::FString getText(int _from, int _to = -1) const;
        void discardMentions(const int _pos, const int _len);
        Data::FString getTextFromFragment(QString _fragmentText, const QTextCharFormat& _charFormat) const;
        bool isFormatEnabled() const;
        void discardFormatAtCursor(core::data::format_type _type, bool _atStart, bool _atEnd);
        void discardAllStylesAtCursor();
        void updateCurrentCursorCharFormat();

        void drawBlockFormatsStuff(QPainter& _painter) const;

        void drawDebugLines(QPainter& _painter) const;
        void printDebugInfo() const;

        enum class WrapperPolicy
        {
            ClearText,
            KeepText,
        };
        template <WrapperPolicy Policy, typename T>
        void blockIntermediateTextChangeSignalsWrapper(T _changeTextFunction)
        {
            const auto prevSize = document()->size();
            auto wasAnythingChanged = false;

            {
                QSignalBlocker sb(document()->documentLayout());

                if constexpr (Policy == WrapperPolicy::ClearText)
                {
                    QSignalBlocker sbEdit(this);
                    QSignalBlocker sbDoc(document());
                    clear();
                    wasAnythingChanged = true;
                }

                wasAnythingChanged |= _changeTextFunction();
            }

            if (wasAnythingChanged)
            {
                if (const auto newSize = document()->size(); newSize != prevSize)
                    Q_EMIT document()->documentLayout()->documentSizeChanged(newSize);

                update();
            }
        }

        bool catchEnterWithSettingsPolicy(const int _modifiers) const;
        bool catchNewLineWithSettingsPolicy(const int _modifiers) const;

        bool doesBlockNeedADescentHack(int _blockNumber) const;

        int index_;

        ResourceMap resourceIndex_;
        Data::MentionMap mentions_;
        Data::FilesPlaceholderMap files_;
        QFont font_;
        int prevPos_;
        bool input_;
        bool isFitToText_;
        int add_;
        int limit_;
        int prevCursorPos_;
        bool isFormatEnabled_ = false;

        QString placeholderText_;

        int maxHeight_;
        EnterKeyPolicy enterPolicy_ = EnterKeyPolicy::None;

        bool hasJustAutoCreatedList_ = false;

        std::vector<int> blockNumbersToApplyDescentHack_;
    };
}

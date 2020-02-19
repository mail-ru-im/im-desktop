#pragma once

#include "../namespaces.h"

#include "../utils/utils.h"

#include  "../types/message.h"

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

    private Q_SLOTS:
        void editContentChanged();
        void onAnchorClicked(const QUrl &_url);
        void onCursorPositionChanged();
        void onContentsChanged(int _pos, int _removed, int _added);

    public:

        TextEditEx(QWidget* _parent, const QFont& _font, const QPalette& _palette, bool _input, bool _isFitToText);
        TextEditEx(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText);

        void limitCharacters(const int _count);

        virtual QSize sizeHint() const override;
        void setPlaceholderText(const QString& _text);

        QString getPlainText() const;
        void setPlainText(const QString& _text, bool _convertLinks = true, const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBaseline);

        void setMentions(Data::MentionMap _mentions);
        const Data::MentionMap& getMentions() const;

        void setFiles(Data::FilesPlaceholderMap _files);
        const Data::FilesPlaceholderMap& getFiles() const;

        void mergeResources(const ResourceMap& _resources);
        void insertEmoji(const Emoji::EmojiCode& _code);
        void insertPlainText_(const QString& _text);
        void insertMention(const QString& _aimId, const QString& _friendly);

        void selectWholeText();
        void selectFromBeginning(const QPoint& _p);
        void selectTillEnd(const QPoint& _p);
        void selectByPos(const QPoint& _p);
        void selectByPos(const QPoint& _from, const QPoint& _to);
        void clearSelection();
        bool isAllSelected();
        QString selection();

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

        QFont getFont() const;

    protected:
        void focusInEvent(QFocusEvent*) override;
        void focusOutEvent(QFocusEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent *) override;
        void keyPressEvent(QKeyEvent*) override;
        void inputMethodEvent(QInputMethodEvent*) override;

        virtual QMenu* getContextMenu();

        QMimeData* createMimeDataFromSelection() const override;
        void insertFromMimeData(const QMimeData* _source) override;
        void contextMenuEvent(QContextMenuEvent *e) override;

    private:
        QString getPlainText(int _from, int _to = -1) const;
        void resetFormat(const int _pos, const int _len);

        bool catchEnterWithSettingsPolicy(const int _modifiers) const;
        bool catchNewLineWithSettingsPolicy(const int _modifiers) const;

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

        QString placeholderText_;

        int maxHeight_;
        EnterKeyPolicy enterPolicy_ = EnterKeyPolicy::None;
    };
}

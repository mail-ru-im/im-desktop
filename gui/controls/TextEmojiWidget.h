#pragma once

#include "../fonts.h"
#include "../utils/utils.h"
#include "../controls/TextUnit.h"
#include "LabelEx.h"

namespace Ui
{
    class TewLex;
    struct paint_info;
    class LabelEx;

    typedef std::list<std::shared_ptr<TewLex>> TewTexText;

    class CompiledText
    {
        TewTexText lexs_;

        int width_;
        int height_;
        int kerning_;

    public:
        CompiledText();

        void push_back(std::shared_ptr<TewLex> _lex);
        int draw(QPainter& _painter, int _x, int _y, int _w);
        int draw(QPainter& _painter, int _x, int _y, int _w, int _h);
        int width(const QFontMetrics& _painter);
        int height(const QFontMetrics& _painter);
        void setKerning(int _kerning) { kerning_ = _kerning; }

        static bool compileText(const QString& _text, CompiledText& _result, bool _multiline, bool _ellipsis);
    };

    enum TextEmojiAlign
    {
        allign_left,
        allign_right,
        allign_center
    };

    class TextEmojiWidgetEvents;
    class TextEmojiWidget : public QWidget
    {
        Q_OBJECT

    protected:
        QFont font_;
        QColor color_;
        QColor colorHover_;
        QColor colorActive_;
        QString text_;
        TextEmojiAlign align_;
        std::unique_ptr<CompiledText> compiledText_;
        QString sourceText_;
        int sizeToBaseline_;
        int descent_;
        bool fading_;
        bool ellipsis_;
        bool multiline_;
        bool selectable_;
        bool hovered_;
        bool active_;

        QPoint selection_;
        // If this flag true, we disable to use fixed size for preferred size polity.
        bool disableFixedPreferred_;

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent* _e) override;

        virtual void enterEvent(QEvent *_e) override;
        virtual void leaveEvent(QEvent *_e) override;
        virtual void mousePressEvent(QMouseEvent *_e) override;
        virtual void mouseReleaseEvent(QMouseEvent *_e) override;
        virtual void mouseDoubleClickEvent(QMouseEvent *_e) override;
        virtual void keyPressEvent(QKeyEvent *_e) override;
        virtual void focusOutEvent(QFocusEvent *_e) override;

        bool event(QEvent* _e) override;

    Q_SIGNALS:
        void clicked();
        void rightClicked();
        void setSize(int, int);

    public:
        TextEmojiWidget(QWidget* _parent, const QFont& _font, const QColor& _color, int _sizeToBaseline = -1);
        virtual ~TextEmojiWidget();

        int ascent();
        int descent();

        void setText(const QString& _text, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        void setText(const QString& _text, const QColor& _color, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        inline QString text() const { return text_; }

        void setColor(const QColor& _color);
        void setHoverColor(const QColor& _color);
        void setActiveColor(const QColor& _color);

        void setFading(bool _v);
        void setEllipsis(bool _v);
        void setMultiline(bool _v);
        void setSelectable(bool _v);

        void setSizeToBaseline(int v);

        void setSizePolicy(QSizePolicy _policy);

        void setSizePolicy(QSizePolicy::Policy _hor, QSizePolicy::Policy _ver);

        void setSourceText(const QString& source);

        int getCompiledWidth() const;

        QSize sizeHint() const override;

        void disableFixedPreferred();

    private:
        static TextEmojiWidgetEvents& events();
    };

    class TextEmojiWidgetEvents : public QObject
    {
        Q_OBJECT

    Q_SIGNALS :
        void selected(TextEmojiWidget*);

    private:
        TextEmojiWidgetEvents() { ; }
        TextEmojiWidgetEvents(const TextEmojiWidgetEvents&);
        TextEmojiWidgetEvents(TextEmojiWidgetEvents&&);

        friend class TextEmojiWidget;
    };

    //
    class TextUnitLabel : public QLabel
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();

    private:
        Ui::TextRendering::TextUnitPtr textUnit_;
        Ui::TextRendering::VerPosition pos_;
        int maxAvalibleWidth_;
        QPoint posClick_;
        bool fitWidth_;

    public:
        TextUnitLabel(QWidget* _parent, Ui::TextRendering::TextUnitPtr _textUnit, Ui::TextRendering::VerPosition _pos, int _maxAvalibleWidth, bool _fitWidth = false);
        ~TextUnitLabel();

        void setText(const QString &_text, const QColor& _color = QColor());
        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;

    private:
        void fitWidth();
        int correctFontHeight(const QFont& _font) const;
    };
}

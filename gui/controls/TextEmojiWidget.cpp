#include "stdafx.h"
#include "TextEmojiWidget.h"
#include "../utils/SChar.h"
#include "../cache/emoji/Emoji.h"
#include "../main_window/history_control/MessageStyle.h"

namespace Ui
{
    class TewLex
    {
    protected:
        int width_;
        int height_;

    public:
        TewLex() : width_(-1), height_(-1) {}
        virtual ~TewLex() {}

        virtual int draw(QPainter& _painter, int _x, int _y) = 0;
        virtual int width(const QFontMetrics& _fontMetrics) = 0;
        virtual int height(const QFontMetrics& _fontMetrics) = 0;

        virtual bool isSpace() const = 0;
        virtual bool isEol() const = 0;
    };

    class TewLexText : public TewLex
    {
        QString text_;

        virtual int draw(QPainter& _painter, int _x, int _y) override
        {
            auto fm = _painter.fontMetrics();
            _painter.fillRect(_x, _y - height(fm) + 1, width(fm) + 1, height(fm) + 1, _painter.brush());
            _painter.drawText(_x, _y, text_);
            return width(fm);
        }

        virtual int width(const QFontMetrics& _fontMetrics) override
        {
            if (width_ == -1)
                width_ = _fontMetrics.width(text_);
            return width_;
        }

        virtual int height(const QFontMetrics& _fontMetrics) override
        {
            if (height_ == -1)
                height_ = _fontMetrics.height();
            return height_;
        }

        virtual bool isSpace() const override
        {
            return text_.size() == 1 && text_.at(0) == ql1c(' ');
        }

        virtual bool isEol() const override
        {
            return text_.size() == 1 && text_.at(0) == ql1c('\n');
        }

    public:

        TewLexText()
        {
            text_.reserve(50);
        }

        void append(const QString& _text)
        {
            text_.append(_text);
        }
    };

    class TewLexEmoji : public TewLex
    {
        int mainCode_;
        int extCode_;

        QImage image_;

        const QImage& getImage(const QFontMetrics& _m)
        {
            if (image_.isNull())
            {
                image_ = Emoji::GetEmoji(Emoji::EmojiCode(mainCode_, extCode_), _m.ascent() - _m.descent());
            }
            return image_;
        }

        virtual int draw(QPainter& _painter, int _x, int _y) override
        {
            int imageHeight = width(_painter.fontMetrics());

            QRect drawRect(_x, _y - imageHeight, imageHeight, imageHeight);

            QFontMetrics m(_painter.font());

            _painter.fillRect(drawRect, _painter.brush());
            _painter.drawImage(drawRect, getImage(m));

            return imageHeight;
        }

        virtual int width(const QFontMetrics& _fontMetrics) override
        {
            if (width_ == -1)
            {
                int imageMaxHeight = _fontMetrics.ascent();
                int imageHeight = getImage(_fontMetrics).height();
                if (imageHeight > imageMaxHeight)
                    imageHeight = imageMaxHeight;
                width_ = imageHeight;
            }
            return width_;
        }

        virtual int height(const QFontMetrics& _fontMetrics) override
        {
            if (height_ == -1)
            {
                int imageMaxHeight = _fontMetrics.ascent();
                int imageHeight = getImage(_fontMetrics).height();
                if (imageHeight > imageMaxHeight)
                    imageHeight = imageMaxHeight;
                height_ = imageHeight;
            }
            return height_;
        }

        virtual bool isSpace() const override
        {
            return false;
        }

        virtual bool isEol() const override
        {
            return false;
        }

    public:

        TewLexEmoji(int _mainCode, int _extCode)
            : mainCode_(_mainCode),
            extCode_(_extCode)
        {
        }
    };

    CompiledText::CompiledText()
        : width_(-1), height_(-1), kerning_(0)
    {
    }

    int CompiledText::width(const QFontMetrics& _painter)
    {
        if (width_ == -1)
        {
            for (auto lex : lexs_)
            {
                width_ += lex->width(_painter);
            }
            width_ += lexs_.empty() ? 0 : (lexs_.size() - 1) * kerning_;
        }
        return width_ + 1;
    }

    int CompiledText::height(const QFontMetrics& _painter)
    {
        if (height_ == -1)
            for (auto lex : lexs_)
                height_ = std::max(height_, lex->height(_painter));
        return height_ + 1;
    }

    void CompiledText::push_back(std::shared_ptr<TewLex> _lex)
    {
        lexs_.push_back(_lex);
    }

    int CompiledText::draw(QPainter& _painter, int _x, int _y, int _w)
    {
        const static QString ellipsis = qsl("...");
        auto xmax = (_w + _x);

        if (_w > 0)
        {
            auto currentTextWidth = _x + width(_painter.fontMetrics());

            // Reserve space for "..." only if text does not fit.
            if (currentTextWidth > xmax)
            {
                xmax -= ((_painter.fontMetrics().averageCharWidth() + kerning_) * ellipsis.length());
            }
        }
        int i = 0;
        for (auto lex : lexs_)
        {
            _x += lex->draw(_painter, _x, _y) + kerning_;
            if (xmax > 0 && _x >= xmax && i < ((int)lexs_.size() - 1))
            {
                _painter.drawText(_x, _y, ellipsis);
                break;
            }
            ++i;
        }
        return _x;
    }

    int CompiledText::draw(QPainter& _painter, int _x, int _y, int _w, int _dh)
    {
        auto h = _y + _dh;
        for (auto lex : lexs_)
        {
            auto lexw = lex->width(_painter.fontMetrics());
            if ((_x + lexw) >= _w || lex->isEol())
            {
                _x = 0;
                _y += height(_painter.fontMetrics());
            }
            h = _y + _dh;
            if (!(lex->isSpace() && _x == 0))
                _x += lex->draw(_painter, _x, _y) + kerning_;
        }
        return h;
    }

    bool CompiledText::compileText(const QString& _text, CompiledText& _result, bool _multiline, bool _ellipsis)
    {
        QTextStream inputStream(const_cast<QString*>(&_text), QIODevice::ReadOnly);

        std::shared_ptr<TewLexText> lastTextLex;

        auto is_eos = [&](int _offset)
        {
            assert(_offset >= 0);
            assert(inputStream.string());

            const auto &text = *inputStream.string();
            return ((inputStream.pos() + _offset) >= text.length());
        };

        auto peekSchar = [&]
        {
            return Utils::PeekNextSuperChar(inputStream);
        };

        auto readSchar = [&]
        {
            return Utils::ReadNextSuperChar(inputStream);
        };

        auto convertPlainCharacter = [&]
        {
            if (!lastTextLex)
            {
                lastTextLex = std::make_shared<TewLexText>();
                _result.push_back(lastTextLex);
            }
            lastTextLex->append(readSchar().ToQString());
            if (_ellipsis)
            {
                lastTextLex.reset();
            }
        };

        auto convertEolSpace = [&]
        {
            const auto ch = peekSchar();
            if (ch.IsNull())
            {
                readSchar();
                lastTextLex.reset();
                return true;
            }
            if (ch.IsNewline() || ch.IsSpace())
            {
                readSchar();
                lastTextLex.reset();
                lastTextLex = std::make_shared<TewLexText>();
                _result.push_back(lastTextLex);
                lastTextLex->append(ch.ToQString());
                lastTextLex.reset();
                return true;
            }
            return false;
        };

        auto convertEmoji = [&]()
        {
            const auto ch = peekSchar();
            if (ch.IsEmoji())
            {
#ifdef __APPLE__
                lastTextLex.reset();
#else
                readSchar();
                _result.push_back(std::make_shared<TewLexEmoji>(ch.Main(), ch.Ext()));
                lastTextLex.reset();
                return true;
#endif
            }
            return false;
        };

        while (!is_eos(0))
        {
            if (_multiline && convertEolSpace())
                continue;

            if (convertEmoji())
                continue;

            convertPlainCharacter();
        }

        return true;
    }

    TextEmojiWidgetEvents& TextEmojiWidget::events()
    {
        static TextEmojiWidgetEvents e;
        return e;
    }

    TextEmojiWidget::TextEmojiWidget(QWidget* _parent, const QFont& _font, const QColor& _color, int _sizeToBaseline)
        : QWidget(_parent),
        font_(_font),
        color_(_color),
        colorHover_(_color),
        colorActive_(_color),
        align_(TextEmojiAlign::allign_left),
        sizeToBaseline_(_sizeToBaseline),
        fading_(false),
        ellipsis_(false),
        multiline_(false),
        selectable_(false),
        hovered_(false),
        active_(false),
        disableFixedPreferred_(false)
    {
        setFont(font_);

        QFontMetrics metrics = QFontMetrics(font_);

        int ascent = metrics.ascent();
        descent_ = metrics.descent();

        int height = metrics.height();

        if (_sizeToBaseline != -1)
        {
            if (_sizeToBaseline > ascent)
            {
                height = sizeToBaseline_ + descent_;
            }
        }
        else
        {
            sizeToBaseline_ = ascent;
        }

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(height);

        setFocusPolicy(Qt::StrongFocus);
    }

    int TextEmojiWidget::ascent()
    {
        return sizeToBaseline_;
    }

    int TextEmojiWidget::descent()
    {
        return descent_;
    }

    void TextEmojiWidget::setSizeToBaseline(int _v)
    {
        sizeToBaseline_ = _v;
        auto metrics = QFontMetrics(font_);
        auto ascent = metrics.ascent();
        auto height = metrics.height();
        if (_v > ascent)
        {
            height = sizeToBaseline_ + descent_;
        }
        setFixedHeight(height);
    }

    TextEmojiWidget::~TextEmojiWidget(void)
    {
    }

    void TextEmojiWidget::setText(const QString& _text, TextEmojiAlign _align)
    {
        align_ = _align;
        text_ = _text;
        compiledText_ = std::make_unique<CompiledText>();
        if (!CompiledText::compileText(text_, *compiledText_, multiline_, ellipsis_))
            assert(!"TextEmojiWidget: couldn't compile text.");
        QWidget::update();
        updateGeometry();
    }

    void TextEmojiWidget::setText(const QString& _text, const QColor& _color, TextEmojiAlign _align)
    {
        color_ = _color;
        colorHover_ = _color;
        colorActive_ = _color;
        setText(_text, _align);
    }

    void TextEmojiWidget::setColor(const QColor &_color)
    {
        color_ = _color;
        setText(text_);
    }

    void TextEmojiWidget::setHoverColor(const QColor &_color)
    {
        colorHover_ = _color;
    }

    void TextEmojiWidget::setActiveColor(const QColor &_color)
    {
        colorActive_ = _color;
    }

    void TextEmojiWidget::setFading(bool _v)
    {
        if (fading_ != _v)
        {
            if (_v)
            {
                ellipsis_ = false;
            }
            fading_ = _v;
            setText(text_);
        }
    }

    void TextEmojiWidget::setEllipsis(bool _v)
    {
        if (ellipsis_ != _v)
        {
            if (_v)
            {
                fading_ = false;
            }
            ellipsis_ = _v;
            setText(text_);
        }
    }

    void TextEmojiWidget::setMultiline(bool _v)
    {
        if (multiline_ != _v)
        {
            if (_v)
            {
                fading_ = false;
                ellipsis_ = false;
            }
            multiline_ = _v;
            setText(text_);
        }
    }

    void TextEmojiWidget::setSelectable(bool _v)
    {
        if (selectable_ != _v)
        {
            static QMetaObject::Connection con;
            if (_v)
            {
                con = connect(&events(), &TextEmojiWidgetEvents::selected, this, [this](TextEmojiWidget* _obj)
                {
                    if ((TextEmojiWidget*)this != _obj)
                    {
                        selection_ = QPoint();
                        QWidget::update();
                    }
                });
            }
            else if (con)
            {
                disconnect(con);
            }
            selectable_ = _v;
            selection_ = QPoint();
            QWidget::update();
        }
    }

    void TextEmojiWidget::setSizePolicy(QSizePolicy _policy)
    {
        return QWidget::setSizePolicy(_policy);
    }

    void TextEmojiWidget::setSizePolicy(QSizePolicy::Policy _hor, QSizePolicy::Policy _ver)
    {
        if (!disableFixedPreferred_ && _hor == QSizePolicy::Preferred)
        {
            setFixedWidth(geometry().width());
            QWidget::update();
        }

        return QWidget::setSizePolicy(_hor, _ver);
    }

    void TextEmojiWidget::setSourceText(const QString& source)
    {
        sourceText_ = source;
    }

    int TextEmojiWidget::getCompiledWidth() const
    {
        return compiledText_.get() ? compiledText_->width(QFontMetrics(font_)) : width();
    }

    void TextEmojiWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter painter(this);
        painter.setFont(font_);
        painter.setPen(hovered_ ? colorHover_ : ( active_ ? colorActive_ : color_));
        if (selection_.y() > 0)
            painter.setBrush(MessageStyle::getSelectionColor());

        QStyleOption opt;

        opt.init(this);

        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        if (compiledText_)
        {
            int offsetX = 0;

            int width = compiledText_->width(painter.fontMetrics());

            QRect rc = geometry();

            switch (align_)
            {
            case Ui::allign_left:
                offsetX = 0;
                break;
            case Ui::allign_right:
                offsetX = rc.width() - width;
                break;
            case Ui::allign_center:
                offsetX = (rc.width() - width) / 2;
                break;
            default:
                assert(false);
                break;
            }

            int offsetY = sizeToBaseline_;

            if (fading_ || ellipsis_ || !multiline_)
            {
                int w = 0;
                if (fading_)
                {
                    const auto contRect = contentsRect();
                    QLinearGradient gradient(contRect.topLeft(), contRect.topRight());
                    gradient.setColorAt(0.7, color_);
                    gradient.setColorAt(1.0, palette().color(QPalette::Window));
                    QPen pen;
                    pen.setBrush(QBrush(gradient));
                    painter.setPen(pen);

                    w = compiledText_->draw(painter, offsetX, offsetY, geometry().width());
                }
                else if (ellipsis_)
                {
                    w = compiledText_->draw(painter, offsetX, offsetY, geometry().width());
                }
                else
                {
                    w = compiledText_->draw(painter, offsetX, offsetY, -1);
                }
                if (!disableFixedPreferred_ && sizePolicy().horizontalPolicy() == QSizePolicy::Preferred && w != geometry().width())
                    setFixedWidth(w);
            }
            else
            {
                auto h = compiledText_->draw(painter, offsetX, offsetY, geometry().width(), descent());
                if (h != geometry().height())
                {
                    QFontMetrics metrics = QFontMetrics(font_);
                    setFixedHeight(std::max(metrics.height(), h));
                }
            }
        }
    }

    void TextEmojiWidget::resizeEvent(QResizeEvent *_e)
    {
        QWidget::resizeEvent(_e);
        if (_e->oldSize().width() != -1 && _e->oldSize().height() != -1)
            emit setSize(_e->size().width() - _e->oldSize().width(), _e->size().height() - _e->oldSize().height());
    }

    void TextEmojiWidget::enterEvent(QEvent *_e)
    {
        hovered_ = true;
        update();
    }

    void TextEmojiWidget::leaveEvent(QEvent *_e)
    {
        hovered_ = false;
        update();
    }

    void TextEmojiWidget::mousePressEvent(QMouseEvent *_e)
    {
        if (_e->button() == Qt::RightButton)
        {
            emit rightClicked();
            return QWidget::mousePressEvent(_e);
        }

        if (selectable_)
        {
            selection_ = QPoint();
            QWidget::update();
            return;
        }

        active_ = true;
        emit clicked();
        return QWidget::mousePressEvent(_e);
    }

    void TextEmojiWidget::mouseReleaseEvent(QMouseEvent *_e)
    {
        active_ = false;
        return QWidget::mouseReleaseEvent(_e);
    }

    void TextEmojiWidget::mouseDoubleClickEvent(QMouseEvent *_e)
    {
        if (selectable_)
        {
            selection_ = QPoint(0, text_.count());
            QWidget::update();
            emit events().selected(this);
            return;
        }
        return QWidget::mouseDoubleClickEvent(_e);
    }

    void TextEmojiWidget::keyPressEvent(QKeyEvent *_e)
    {
        if (selectable_)
        {
            if (_e->matches(QKeySequence::Copy))
            {
                qApp->clipboard()->setText(sourceText_.isEmpty() ? text() : sourceText_);
                selection_ = QPoint();
                QWidget::update();
                return;
            }
        }

        return QWidget::keyPressEvent(_e);
    }

    void TextEmojiWidget::focusOutEvent(QFocusEvent *_e)
    {
        if (selectable_)
        {
            selection_ = QPoint();
            QWidget::update();
        }
        return QWidget::focusOutEvent(_e);
    }

    bool TextEmojiWidget::event(QEvent* _e)
    {
        if (_e->type() == QEvent::ShortcutOverride)
        {
            auto ke = static_cast<QKeyEvent*>(_e);
            if (ke == QKeySequence::Copy)
            {
                ke->accept();
                return true;
            }
        }

        return QWidget::event(_e);
    }

    QSize TextEmojiWidget::sizeHint() const
    {
        return QSize((!multiline_ ? getCompiledWidth() : -1), -1);
    }

    void TextEmojiWidget::disableFixedPreferred()
    {
        disableFixedPreferred_ = true;
    }

    ////
    TextUnitLabel::TextUnitLabel(QWidget* _parent, Ui::TextRendering::TextUnitPtr _textUnit, Ui::TextRendering::VerPosition _pos, int _maxAvalibleWidth, bool _fitWidth)
        : QLabel(_parent)
        , pos_(_pos)
        , maxAvalibleWidth_(_maxAvalibleWidth)
        , fitWidth_(_fitWidth)
    {
        textUnit_ = std::move(_textUnit);
        setMaximumWidth(maxAvalibleWidth_);
        fitWidth();
        QLabel::setText(textUnit_->getText());
    }

    TextUnitLabel::~TextUnitLabel() {}

    QSize TextUnitLabel::sizeHint() const
    {
        if (textUnit_)
        {
            const auto width = qMin(maxAvalibleWidth_, QLabel::width());
            const auto height = textUnit_->getText().isEmpty() ? correctFontHeight(textUnit_->getFont()) : textUnit_->getHeight(width);
            return QSize(width, height);
        }

        return QSize(0, 0);
    }

    void TextUnitLabel::paintEvent(QPaintEvent* _e)
    {
        QPainter painter(this);
        textUnit_->draw(painter, pos_);

#ifdef _DEBUG_WIDGETS_RECT
        painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
#endif
    }

    void TextUnitLabel::resizeEvent(QResizeEvent* _e)
    {
        adjustSize();
        updateGeometry();
        update();
        QWidget::resizeEvent(_e);
    }

    void TextUnitLabel::setText(const QString &_text, const QColor& _color)
    {
        textUnit_->setText(_text, _color);
        QLabel::setText(_text);
        fitWidth();
        updateGeometry();
    }

    void TextUnitLabel::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (rect().contains(_e->pos()) && rect().contains(posClick_))
            emit clicked();
        QLabel::mouseReleaseEvent(_e);
    }

    void TextUnitLabel::mousePressEvent(QMouseEvent* _e)
    {
        posClick_ = _e->pos();
        QLabel::mousePressEvent(_e);
    }

    void TextUnitLabel::fitWidth()
    {
        if (fitWidth_)
            setFixedWidth(qMin(maxAvalibleWidth_, textUnit_->desiredWidth()));
    }

    int TextUnitLabel::correctFontHeight(const QFont& _font) const
    {
        auto fontHeight = QFontMetrics(_font).height();
        // todo: after emoji correction TextUnit height is different from font height
        if (platform::is_apple())
            fontHeight -= Utils::scale_value(1);
        else
            fontHeight += Utils::scale_value(1);
        /////////

        return fontHeight;
    }
}


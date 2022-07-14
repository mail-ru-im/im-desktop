#include "stdafx.h"
#include "TextWidget.h"

namespace Ui
{
    void TextWidget::init(const TextRendering::TextUnit::InitializeParameters& _params)
    {
        text_->init(_params);
        desiredWidth_ = text_->desiredWidth();
        text_->getHeight(maxWidth_ ? maxWidth_ : desiredWidth_);
        setFixedSize(text_->cachedSize());
        update();
    }

    void TextWidget::setVerticalPosition(TextRendering::VerPosition _position)
    {
        if (TextRendering::VerPosition::BASELINE == _position)
        {
            im_assert(!"Dont support TextRendering::VerPosition::BASELINE");
            return;
        }

        verticalPosition_ = _position;
        int verticalOffset = 0;
        const int textHeight = text_->cachedSize().height();
        switch (_position)
        {
        case TextRendering::VerPosition::MIDDLE:
            verticalOffset = textHeight / 2;
            break;
        case TextRendering::VerPosition::BOTTOM:
            verticalOffset = textHeight;
            break;
        default:
            break;
        }
        text_->setOffsets(QPoint { 0, verticalOffset });
    }

    void TextWidget::applyLinks(const std::map<QString, QString>& _links) { text_->applyLinks(_links); }

    void TextWidget::setLineSpacing(int _v) { text_->setLineSpacing(_v); }

    void TextWidget::setMaxWidth(int _width)
    {
        if (_width > 0)
        {
            maxWidth_ = _width;
            text_->getHeight(maxWidth_);
            update();
        }
    }

    void TextWidget::setMaxWidthAndResize(int _width)
    {
        if (_width > 0)
        {
            setMaxWidth(_width);
            setFixedSize(text_->cachedSize());
        }
    }

    void TextWidget::setText(const Data::FString& _text, const Styling::ThemeColorKey& _color)
    {
        text_->setText(_text, _color);
        desiredWidth_ = text_->desiredWidth();
        text_->getHeight(maxWidth_ ? maxWidth_ : desiredWidth_);
        setFixedSize(text_->cachedSize());
        update();
    }

    void TextWidget::setOpacity(qreal _opacity)
    {
        opacity_ = _opacity;
        update();
    }

    void TextWidget::setColor(const Styling::ThemeColorKey& _color) { text_->setColor(_color); }

    void TextWidget::setAlignment(const TextRendering::HorAligment _align) { text_->setAlign(_align); }

    QString TextWidget::getText() const { return text_->getText(); }

    Data::FString TextWidget::getSelectedText(TextRendering::TextType _type) const { return text_->getSelectedText(_type); }

    void TextWidget::elide(int _width) { text_->elide(_width); }

    bool TextWidget::isElided() const
    {
        return text_->isElided();
    }

    bool TextWidget::isEmpty() const { return text_->isEmpty(); }

    bool TextWidget::isSelected() const { return text_->isSelected(); }

    bool TextWidget::isAllSelected() const { return text_->isAllSelected(); }

    void TextWidget::select(const QPoint& _from, const QPoint& _to)
    {
        text_->select(_from, _to);
        update();
    }

    void TextWidget::selectAll() { text_->selectAll(); }

    void TextWidget::clearSelection()
    {
        text_->clearSelection();
        update();
    }

    void TextWidget::releaseSelection()
    {
        text_->releaseSelection();
        update();
    }

    void TextWidget::clicked(const QPoint& _p) { text_->clicked(_p); }

    void TextWidget::doubleClicked(const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback)
    {
        text_->doubleClicked(_p, _fixSelection, std::move(_callback));
    }

    void TextWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setOpacity(opacity_);
        text_->draw(p, verticalPosition_);
    }

    void TextWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        if (text_->linksVisibility() == TextRendering::LinksVisible::SHOW_LINKS)
        {
            if (text_->isOverLink(_e->pos()))
                setCursor(Qt::PointingHandCursor);
            else
                setCursor(Qt::ArrowCursor);
        }
        QWidget::mouseMoveEvent(_e);
    }

    void TextWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (const auto pos = _e->pos(); text_->isOverLink(pos))
        {
            _e->accept();
            Q_EMIT linkActivated(text_->getLink(pos).url_, QPrivateSignal());
        }
        else
        {
            _e->ignore();
        }
    }
} // namespace Ui

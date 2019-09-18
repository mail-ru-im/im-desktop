#include "stdafx.h"

#include "FontSizeSelectorWidget.h"

#include "fonts.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"

namespace
{
    int getWidgetHeight()
    {
        return Utils::scale_value(44);
    }

    int captionLeftOffset()
    {
        return Utils::scale_value(20);
    }

    int rightMargin()
    {
        return Utils::scale_value(16);
    }

    QSize fontVariantSize()
    {
        return Utils::scale_value(QSize(32, 32));
    }
}

namespace Ui
{
    FontSizeVariant::FontSizeVariant(QWidget* _parent, const int _size)
        : SimpleListItem(_parent)
        , isSelected_(false)
        , font_(Fonts::appFontScaled(_size, Fonts::FontWeight::SemiBold))
    {
        setFixedSize(fontVariantSize());
    }

    void FontSizeVariant::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;
            update();
        }
    }

    bool FontSizeVariant::isSelected() const
    {
        return isSelected_;
    }

    void FontSizeVariant::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (isHovered() || isSelected())
        {
            p.setPen(Qt::NoPen);

            const auto bgColor = isSelected() ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_BRIGHT_INVERSE;
            const auto radius = Utils::scale_value(4);
            p.setBrush(Styling::getParameters().getColor(bgColor));
            p.drawRoundedRect(rect(), radius, radius);
        }

        const auto textColor = isSelected() ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_SOLID;

        p.setPen(Styling::getParameters().getColor(textColor));
        p.setFont(font_);
        p.drawText(rect(), QT_TRANSLATE_NOOP("wallpaper_select", "A"), QTextOption(Qt::AlignCenter));
    }


    FontSizeSelectorWidget::FontSizeSelectorWidget(QWidget* _parent, const QString& _caption, const SelectableVariants& _fontSizes)
        : QWidget(_parent)
        , variants_(_fontSizes)
    {
        setFixedHeight(getWidgetHeight());

        caption_ = TextRendering::MakeTextUnit(_caption);
        caption_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        caption_->evaluateDesiredSize();

        variantList_ = new SimpleListWidget(Qt::Horizontal);
        variantList_->setSpacing(Utils::scale_value(8));
        for (const auto [size, _] : _fontSizes)
            variantList_->addItem(new FontSizeVariant(this, size));

        auto layout = Utils::emptyHLayout(this);
        layout->setContentsMargins(0, 0, rightMargin(), 0);
        layout->setAlignment(Qt::AlignRight);
        layout->addWidget(variantList_);

        connect(variantList_, &SimpleListWidget::clicked, this, &FontSizeSelectorWidget::onVariantClicked);
    }

    void FontSizeSelectorWidget::setSelectedSize(const Fonts::FontSize _size)
    {
        const auto it = std::find_if(variants_.begin(), variants_.end(), [_size](const auto& _p) { return _p.second == _size; });
        if (it != variants_.end())
            variantList_->setCurrentIndex(std::distance(variants_.begin(), it));
    }

    void FontSizeSelectorWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        caption_->setOffsets(captionLeftOffset(), height() / 2);
        caption_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void FontSizeSelectorWidget::onVariantClicked(const int _idx)
    {
        if (_idx < 0 && _idx >= int(variants_.size()))
            return;

        variantList_->setCurrentIndex(_idx);

        emit sizeSelected(variants_[_idx].second);
    }
}

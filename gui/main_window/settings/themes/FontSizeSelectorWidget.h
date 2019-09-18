#pragma once

#include "controls/TextUnit.h"
#include "controls/SimpleListWidget.h"

namespace Fonts
{
    enum class FontSize;
}

namespace Ui
{
    class SimpleListWidget;

    class FontSizeVariant : public SimpleListItem
    {
        Q_OBJECT

    public:
        FontSizeVariant(QWidget* _parent, const int _size);

        void setSelected(const bool _value) override;
        bool isSelected() const override;

    Q_SIGNALS:
        void clicked(const QString&, QPrivateSignal) const;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        bool isSelected_;
        QFont font_;
    };

    class FontSizeSelectorWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void sizeSelected(const Fonts::FontSize _size);

    public:
        using SelectableVariants = std::vector<std::pair<int, Fonts::FontSize>>;

        FontSizeSelectorWidget(QWidget* _parent, const QString& _caption, const SelectableVariants& _fontSizes);
        void setSelectedSize(const Fonts::FontSize _size);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        void onVariantClicked(const int _idx);

        TextRendering::TextUnitPtr caption_;
        SimpleListWidget* variantList_;
        SelectableVariants variants_;
    };
}
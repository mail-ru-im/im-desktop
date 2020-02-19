#pragma once

#include "Common.h"
#include "../../types/contact.h"
#include "../gui/cache/countries.h"
#include "CountryListModel.h"

namespace Fonts
{
    enum class FontWeight;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class CountryItem
    {
    public:
        CountryItem(const Data::Country& _country);
        void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0);
        ~CountryItem() = default;

    private:
        QImage icon_;
        std::unique_ptr<TextRendering::TextUnit> name_;
        std::unique_ptr<TextRendering::TextUnit> code_;
    };
}

namespace Logic
{
    class CustomAbstractListModel;

    class CountryListItemDelegate : public AbstractItemDelegateWithRegim
    {
        Q_OBJECT
    public:
        CountryListItemDelegate(QObject* _parent, CountryListModel* _countriesModel = nullptr);

        virtual ~CountryListItemDelegate();

        void setRegim(int) override {};

        void setFixedWidth(int _width) override { viewParams_.fixedWidth_ = _width; };

        void blockState(bool value) override {};

        void setDragIndex(const QModelIndex& index) override {};

        void paint(QPainter *_painter, const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        void clearCache();

    private:
        QModelIndex DragIndex_;
        Ui::ViewParams viewParams_;
        CountryListModel* countriesModel_;

        mutable std::unordered_map<QString, std::unique_ptr<Ui::CountryItem>, Utils::QStringHasher> items;
    };
}

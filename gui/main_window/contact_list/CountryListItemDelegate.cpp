#include "stdafx.h"
#include "CountryListItemDelegate.h"
#include "CountryListModel.h"
#include "../../cache/emoji/EmojiDb.h"
#include "../../styles/ThemeParameters.h"
#include "../../types/search_result.h"

using namespace Ui;
using namespace Logic;

namespace
{
    int getItemHeight()
    {
        return Utils::scale_value(44);
    }

    int getItemWidth()
    {
        return Utils::scale_value(360);
    }

    int getIconSize()
    {
        return Utils::scale_value(32);
    }

    int getHorMargin()
    {
        return Utils::scale_value(16);
    }

    int getCodeWidth()
    {
        return Utils::scale_value(50);
    }
}

CountryItem::CountryItem(const Data::Country & _country)
{
    name_ = TextRendering::MakeTextUnit(_country.name_);
    name_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
        QColor(), QColor(), QColor(),
        Ui::TextRendering::HorAligment::LEFT, 1);
    const auto width = getItemWidth() - getIconSize() - getCodeWidth();
    name_->setLastLineWidth(width);
    name_->getHeight(width);

    code_ = TextRendering::MakeTextUnit(_country.phone_code_);
    code_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
        QColor(), QColor(), QColor(),
        Ui::TextRendering::HorAligment::LEFT, 1);
    code_->getHeight(getCodeWidth());

    auto emojicode = Emoji::GetEmojiInfoByCountryName(Utils::getCountryCodeByName(_country.name_));
    if (!emojicode.isNull())
        icon_ = Emoji::GetEmoji(emojicode, Emoji::getEmojiSize());
}

void CountryItem::paint(QPainter & _painter, const bool _isHovered, const bool _isActive, const bool _isPressed, const int _curWidth)
{
    Utils::PainterSaver p(_painter);
    _painter.setRenderHint(QPainter::Antialiasing);
    const auto width = _curWidth == 0 ? getItemWidth() : _curWidth;

    RenderMouseState(_painter, _isHovered, _isActive, QRect(0, 0, width, getItemHeight()));

    _painter.drawImage(QRect(getHorMargin(), (getItemHeight() - getIconSize())/2, getIconSize(), getIconSize()), icon_);

    const auto yOffset = getItemHeight() / 2;

    name_->setOffsets(getIconSize() + 2 * getHorMargin(), yOffset);
    name_->elide(width - getIconSize() - 3 * getHorMargin() - getCodeWidth());
    name_->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);

    code_->setOffsets((width - (getIconSize() + 2 * getHorMargin())), yOffset);
    code_->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);
}

CountryListItemDelegate::CountryListItemDelegate(QObject* _parent, CountryListModel* _countriesModel)
    : AbstractItemDelegateWithRegim(_parent)
    , countriesModel_(_countriesModel)
{
}

CountryListItemDelegate::~CountryListItemDelegate()
{
    clearCache();
}

void CountryListItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    QString name;

    if (auto searchRes = _index.data(Qt::DisplayRole).value<Data::CountrySearchResultSptr>())
    {
        const auto& countryRes = std::static_pointer_cast<Data::SearchResultCountry>(searchRes);
        name = countryRes->getCountryName();
    }
    else if (qobject_cast<const Logic::CountryListModel*>(_index.model()))
    {
        auto country = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        name = country->getAimId();
    }

    const auto country = Logic::getCountryModel()->getCountry(name);
    if (country.phone_code_.isEmpty())
        return;

    Utils::PainterSaver ps(*_painter);
    _painter->setRenderHint(QPainter::Antialiasing);
    _painter->translate(_option.rect.topLeft());

    const bool isHovered = (_option.state & QStyle::State_Selected);
    const bool isMouseOver = (_option.state & QStyle::State_MouseOver);
    const bool isPressed = isHovered && isMouseOver && (QApplication::mouseButtons() & Qt::MouseButton::LeftButton);

    auto& item = items[name];
    if (!item)
        item = std::make_unique<CountryItem>(country);

    item->paint(*_painter, isHovered, false, isPressed);

}

QSize CountryListItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    return QSize(_option.rect.width(), getItemHeight());
}

void CountryListItemDelegate::clearCache()
{
    items.clear();
}

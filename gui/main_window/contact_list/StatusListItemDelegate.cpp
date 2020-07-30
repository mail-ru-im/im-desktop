#include "stdafx.h"
#include "StatusListItemDelegate.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

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

    int getIconRectSize()
    {
        return Utils::scale_value(44);
    }

    int getIconSize()
    {
        return Utils::scale_value(34);
    }

    int getHorMargin()
    {
        return Utils::scale_value(8);
    }

    int getVerMargin()
    {
        return Utils::scale_value(4);
    }

    int getMoreButtonSize()
    {
        return Utils::scale_value(20);
    }

    QPixmap getMoreButton(const bool _isHovered, const bool _isActive)
    {
        auto makeBtn = [](const auto _var){ return Utils::renderSvgScaled(qsl(":/controls/more_vertical"), QSize(getMoreButtonSize(), getMoreButtonSize()), Styling::getParameters().getColor(_var)); };
        return makeBtn(_isActive ? Styling::StyleVariable::BASE_SECONDARY_ACTIVE : (_isHovered ? Styling::StyleVariable::BASE_SECONDARY_HOVER : Styling::StyleVariable::BASE_SECONDARY));
    }
}

using namespace Ui;
using namespace Logic;

StatusItem::StatusItem(const Statuses::Status& _status)
    : QWidget(nullptr)
    , curWidth_(getItemWidth())
    , isEmpty_(false)
{
    updateParams(_status);
    setMouseTracking(true);
}

void StatusItem::paint(QPainter& _painter, bool _isHovered, bool _isActive, bool _isMoreHovered, int _curWidth)
{
    Utils::PainterSaver p(_painter);
    _painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    curWidth_ = _curWidth == 0 ? getItemWidth() : _curWidth;

    RenderMouseState(_painter, _isHovered, false, QRect(0, 0, curWidth_, getItemHeight()));

    const auto xOff = getHorMargin() + (getIconRectSize() - getIconSize())/2;
    const auto yOff = (getItemHeight() - getIconSize()) / 2;

    _painter.drawImage(xOff, yOff, status_);

    const auto hasTime = !time_->getText().isEmpty();
    auto yOffset = hasTime ? getVerMargin() : getItemHeight() / 2;

    title_->setOffsets(getIconRectSize() + 2 * getHorMargin(), yOffset);
    title_->elide(curWidth_ - getIconRectSize() - 3 * getHorMargin() - getMoreButtonSize());
    title_->draw(_painter, hasTime ? TextRendering::VerPosition::TOP : TextRendering::VerPosition::MIDDLE);

    if (hasTime)
    {
        yOffset = getItemHeight() - getVerMargin();
        time_->setOffsets(getIconRectSize() + 2 * getHorMargin(), yOffset);
        time_->draw(_painter, TextRendering::VerPosition::BOTTOM);
    }

    if (_isHovered && !isEmpty_)
    {
        const QRect btnRect(curWidth_ - getHorMargin() - getMoreButtonSize(), (getItemHeight() - getMoreButtonSize())/2, getMoreButtonSize(), getMoreButtonSize());
        _painter.drawPixmap(btnRect, getMoreButton(_isMoreHovered, _isMoreHovered && _isActive));
    }
}

void StatusItem::updateParams(const Statuses::Status &_status)
{
    title_ = TextRendering::MakeTextUnit(_status.getDescription());
    title_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                 QColor(), QColor(), QColor(),
                 Ui::TextRendering::HorAligment::LEFT, 1);
    title_->setLastLineWidth(getItemWidth() - getIconSize());
    title_->evaluateDesiredSize();

    time_ = TextRendering::MakeTextUnit(_status.getTimeString());
    time_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(_status.isSelected() ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_PRIMARY),
                QColor(), QColor(), QColor(),
                Ui::TextRendering::HorAligment::LEFT, 1);
    time_->evaluateDesiredSize();

    status_ = _status.getImage(getIconSize());
    isEmpty_ = _status.isEmpty();
}

StatusListItemDelegate::StatusListItemDelegate(QObject* _parent, StatusListModel* _statusModel)
    : AbstractItemDelegateWithRegim(_parent)
    , statusModel_(_statusModel)
{
}


void StatusListItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    auto stat = _index.data(Qt::DisplayRole).value<Statuses::Status>();

    Utils::PainterSaver ps(*_painter);
    _painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    _painter->translate(_option.rect.topLeft());

    const bool isHovered = (_option.state & QStyle::State_Selected);
    const bool isMouseOver = (_option.state & QStyle::State_MouseOver);
    const bool isPressed = isHovered && isMouseOver && (QApplication::mouseButtons() & Qt::MouseButton::LeftButton);

    auto& item = items_[stat.toString()];
    if (!item)
        item = std::make_unique<StatusItem>(stat.toString());

    item->updateParams(stat);
    item->paint(*_painter, isHovered, isPressed, hoveredMore_ == _index);
}

QSize StatusListItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    return QSize(getItemWidth(), getItemHeight());
}

void StatusListItemDelegate::clearCache()
{
    items_.clear();
}

void StatusListItemDelegate::setHoveredModeIndex(const QModelIndex& _hoveredMore)
{
    hoveredMore_ = _hoveredMore;
}

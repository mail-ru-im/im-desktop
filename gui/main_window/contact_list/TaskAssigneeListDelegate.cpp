#include "stdafx.h"

#include "TaskAssigneeListDelegate.h"
#include "SearchModel.h"
#include "ContactListModel.h"
#include "../../styles/StyleVariable.h"
#include "../../styles/ThemeParameters.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../app_config.h"
#include "../../utils/utils.h"
#include "TaskAssigneeModel.h"

namespace
{
    QFont nameFont()
    {
        return Fonts::appFontScaled(14, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
    }

    auto listVerticalPadding() noexcept
    {
        return Utils::scale_value(6);
    }

    auto avatarTopOffset() noexcept
    {
        return Utils::scale_value(6);
    }

    auto horizontalPadding() noexcept
    {
        return Utils::scale_value(16);
    }

    auto avatarRightOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    auto avatarSize() noexcept
    {
        return Utils::scale_value(20);
    }

    auto noAssigneeIconSize()
    {
        return Utils::scale_value(16);
    }

    QPixmap renderNoAssigneeAvatar()
    {
        const auto size = avatarSize();
        QPixmap result(Utils::scale_bitmap(QSize(size, size)));
        Utils::check_pixel_ratio(result);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
        painter.setPen(Qt::NoPen);
        const auto radius = size / 2;
        painter.drawRoundedRect(0, 0, size, size, radius, radius);
        const auto iconOffset = Utils::scale_value(2);
        const auto iconSize = noAssigneeIconSize();
        QPixmap icon = Utils::renderSvg(qsl(":/task/no_assignee_icon"), { iconSize, iconSize }, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        painter.drawPixmap(iconOffset, iconOffset, icon);
        return result;
    }

    const QPixmap& noAssigneeAvatar()
    {
        static const auto avatar = renderNoAssigneeAvatar();
        return avatar;
    }
}

namespace Logic
{
    TaskAssigneeListDelegate::TaskAssigneeListDelegate(QObject* _parent)
        : AbstractItemDelegateWithRegim(_parent)
    {
        name_ = Ui::TextRendering::MakeTextUnit(QString(), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        name_->init(nameFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    }

    void TaskAssigneeListDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto item = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>();
        const QString aimId = item->getAimId();
        const auto noAssignee = TaskAssigneeModel::noAssigneeAimId() == aimId;
        QString friendly = item->getFriendlyName();

        const auto topAdjust = isFirstItem(_index) ? listVerticalPadding() : 0;
        const auto bottomAdjust = isLastItem(_index) ? -listVerticalPadding() : 0;
        const auto itemRect = _option.rect.adjusted(0, topAdjust, 0, bottomAdjust);

        Utils::PainterSaver saver(*_painter);
        _painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        Ui::RenderMouseState(*_painter, (_option.state & QStyle::State_Selected), false, itemRect);
        _painter->translate(itemRect.topLeft());

        const auto ratio = Utils::scale_bitmap_ratio();
        auto isDefault = false;
        const auto avatar = noAssignee ? noAssigneeAvatar() : Logic::GetAvatarStorage()->GetRounded(aimId, friendly, Utils::scale_bitmap(avatarSize()), isDefault, false, false);

        const auto x = horizontalPadding();
        const auto y = (itemRect.height() - avatar.height() / ratio) / 2;
        _painter->drawPixmap(QPoint( x, y ), avatar);

        const auto nameHorizontalOffset = horizontalPadding() + avatarRightOffset() + avatarSize();
        const auto elidedWidth = _option.rect.width() - horizontalPadding() - nameHorizontalOffset;

        name_->setText(friendly);
        name_->elide(elidedWidth);
        name_->setOffsets(nameHorizontalOffset, itemHeight() / 2);
        name_->draw(*_painter, Ui::TextRendering::VerPosition::MIDDLE);

        elidedItems_[_index.row()] = name_->isElided();

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            _painter->setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            _painter->setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));
            const auto idX = itemRect.width();
            const auto idY = itemRect.height();
            Utils::drawText(*_painter, QPointF(idX, idY), Qt::AlignRight | Qt::AlignBottom, aimId);
        }
    }

    int TaskAssigneeListDelegate::itemHeight() noexcept
    {
        return Utils::scale_value(32);
    }

    QSize TaskAssigneeListDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto height = itemHeight();
        if (isFirstItem(_index))
            height += listVerticalPadding();
        if (isLastItem(_index))
            height += listVerticalPadding();
        return QSize(_option.rect.width(), height);
    }

    bool TaskAssigneeListDelegate::needsTooltip(const QString&, const QModelIndex& _index, QPoint) const
    {
        const auto it = elidedItems_.find(_index.row());
        return it != elidedItems_.end() && it->second;
    }

    bool TaskAssigneeListDelegate::isFirstItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == 0;
    }

    bool TaskAssigneeListDelegate::isLastItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == _index.model()->rowCount() - 1;
    }
}

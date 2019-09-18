#include "stdafx.h"
#include "LiveChatMembersControl.h"
#include "../../types/chat.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../friendly/FriendlyContainer.h"
#include "../../styles/ThemeParameters.h"

namespace Ui
{
    LiveChatMembersControl::LiveChatMembersControl(QWidget* _parent, const std::shared_ptr<Data::ChatInfo>& _info, const int _maxCount)
        :   QWidget(_parent),
            maxCount_(_maxCount),
            realCount_(_maxCount),
            color_(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID))
    {
        setFixedHeight(Utils::scale_value(controlHeight));

        int i = 0;
        for (const auto& chatMember : _info->Members_)
        {
            members_.push_back(std::make_pair(chatMember.AimId_, Logic::GetFriendlyContainer()->getFriendly(chatMember.AimId_)));
            if ((++i) >= _maxCount)
                break;
        }

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &LiveChatMembersControl::onAvatarLoaded);
    }

    LiveChatMembersControl::~LiveChatMembersControl()
    {
    }

    const int LiveChatMembersControl::sv(const int _v)
    {
        return Utils::scale_value(_v);
    }

    void LiveChatMembersControl::onAvatarLoaded(QString _aimid)
    {
        for (const auto& chat : members_)
        {
            if (chat.first == _aimid)
            {
                update();
                break;
            }
        }
    }

    void LiveChatMembersControl::adjustWidth()
    {
        int width = (sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth)) * members_.size() + (sv(avatarIntersection) + sv(frameWidth));

        setFixedWidth(width);

        realCount_ = (geometry().width() - sv(avatarWithFrameHeight)) / (sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth)) + 1;
    }

    void LiveChatMembersControl::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform|QPainter::TextAntialiasing);

        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        painter.setPen(Qt::NoPen);

        int xOffset = 0;

        int count = realCount_;
        for (const auto& chat : members_)
        {
            if (count <= 0)
                break;

            painter.drawEllipse(xOffset, 0, sv(avatarWithFrameHeight), sv(avatarWithFrameHeight));

            bool isDefault = false;
            const auto &avatar = Logic::GetAvatarStorage()->GetRounded(chat.first, chat.second, Utils::scale_bitmap(sv(avatarHeight)), QString(), isDefault, false, false);

            if (!avatar->isNull())
                painter.drawPixmap(xOffset + sv(frameWidth), sv(frameWidth), sv(avatarHeight), sv(avatarHeight), *avatar);

            xOffset += sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth);

            --count;
        }


        return QWidget::paintEvent(_e);
    }

    void LiveChatMembersControl::resizeEvent(QResizeEvent* _e)
    {
        QWidget::resizeEvent(_e);
        realCount_ = (geometry().width() - sv(avatarWithFrameHeight)) / (sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth)) + 1;
    }

    int LiveChatMembersControl::getRealCount() const
    {
        return realCount_;
    }

    void LiveChatMembersControl::updateInfo(std::shared_ptr<Data::ChatInfo> _info)
    {
        members_.clear();

        int i = 0;
        for (const auto& chatMember : std::as_const(_info->Members_))
        {
            members_.push_back(std::make_pair(chatMember.AimId_, Logic::GetFriendlyContainer()->getFriendly(chatMember.AimId_)));
            if ((++i) >= maxCount_)
                break;
        }

        update();
    }
}

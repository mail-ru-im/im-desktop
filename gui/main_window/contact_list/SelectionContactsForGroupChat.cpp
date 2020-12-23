#include "stdafx.h"
#include "SelectionContactsForGroupChat.h"

#include "AbstractSearchModel.h"
#include "CustomAbstractListModel.h"
#include "RecentsTab.h"
#include "ContactListWidget.h"
#include "ContactListModel.h"
#include "IgnoreMembersModel.h"
#include "CountryListModel.h"
#include "SearchWidget.h"
#include "../GroupChatOperations.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../history_control/MessageStyle.h"
#include "../containers/FriendlyContainer.h"
#include "../proxy/FriendlyContaInerProxy.h"
#include "../../core_dispatcher.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/DialogButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../previewer/toast.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../proxy/AvatarStorageProxy.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/CheckboxList.h"
#include "../../gui_settings.h"
#include "../../styles/ThemeParameters.h"
#include "../../controls/FlatMenu.h"
#include "../../statuses/StatusUtils.h"
#ifndef STRIP_VOIP
#include "voip/SelectionContactsForConference.h"
#endif

namespace
{
    const auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }
}


namespace TestingLocal
{
    QString getContactWidgetAccessibleName(int regim_)
    {
        switch (regim_)
        {
        case Logic::MembersWidgetRegim::COUNTRY_LIST: return qsl("AS SearchPopup countrySelectPopup");
        default: return qsl("AS SearchPopup contactsWidget");
        }
    }

    QString getSearchWidgetAccessibleName(int regim_)
    {
        switch (regim_)
        {
        case Logic::MembersWidgetRegim::COUNTRY_LIST: return qsl("AS SearchPopup countrySearch");
        default: return qsl("AS SearchPopup searchWidget");
        }
    }

    QString getContactListWidgetAccessibleName(int regim_)
    {
        switch (regim_)
        {
        case Logic::MembersWidgetRegim::COUNTRY_LIST: return qsl("AS SearchPopup countryList");
        default: return qsl("AS SearchPopup contactList");
        }
    }

}

namespace Ui
{
    constexpr const double heightPartOfMainWindowForFullView = 0.6;
    constexpr const int DIALOG_WIDTH = 360;
    constexpr const int NAME_WIDTH = 256;
    constexpr const int search_padding_ver = 8;
    constexpr const int search_padding_hor = 4;
    constexpr const int avatars_area_top_padding = 12;
    constexpr const int avatars_area_bottom_padding = 4;
    constexpr const int avatars_area_hor_padding = 16;
    constexpr const int avatars_area_spacing = 6;
    constexpr const int avatar_size = 40;
    constexpr const int avatars_scroll_step = 10;

    AvatarsArea::AvatarsArea(QWidget* _parent, int _regim, Logic::CustomAbstractListModel* _membersModel, Logic::AbstractSearchModel* _searchModel)
        : QWidget(_parent)
        , diff_(0)
        , avatarOffset_(0)
        , heightAnimation_(new QVariantAnimation(this))
        , avatarAnimation_(new QVariantAnimation(this))
        , replaceFavorites_(false)
        , membersModel_(_membersModel)
        , searchModel_(_searchModel)
        , regim_(_regim)
    {
        setMouseTracking(true);

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &AvatarsArea::onAvatarChanged);

        avatarAnimation_->setEndValue(0);
        avatarAnimation_->setDuration(300);
        connect(avatarAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            avatarOffset_ = value.toInt();
            update();
        });

        heightAnimation_->setDuration(200);
        connect(heightAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            const auto cur = value.toInt();
            setFixedHeight(cur);
            const auto endValue = heightAnimation_->endValue();
            if (endValue != 0 && endValue == cur)
                Q_EMIT showed();
        });
    }

    void AvatarsArea::onAvatarChanged(const QString& _aimId)
    {
        auto it = std::find_if(avatars_.begin(), avatars_.end(), [&_aimId](const auto& _avatar) { return _avatar.aimId_ == _aimId; });
        if (it != avatars_.end())
        {
            it->avatar_ = getAvatar(_aimId);
            Utils::check_pixel_ratio(it->avatar_);
        }

        update();
    }

    void AvatarsArea::add(const QString& _aimId, QPixmap _avatar)
    {
        AvatarData d;
        d.aimId_ = _aimId;
        d.avatar_ = _avatar;

        if (std::any_of(avatars_.begin(), avatars_.end(), [&_aimId](const auto& av) { return av.aimId_ == _aimId; }))
            return;

        Utils::check_pixel_ratio(d.avatar_);
        avatars_.insert(avatars_.begin(), d);

        if (avatars_.size() == 1)
        {
            const auto h = Utils::scale_value(avatars_area_top_padding + avatars_area_bottom_padding + avatar_size);
            heightAnimation_->stop();
            heightAnimation_->setStartValue(0);
            heightAnimation_->setEndValue(h);
            heightAnimation_->start();
        }
        else
        {
            if (avatarAnimation_->state() != QAbstractAnimation::State::Running)
            {
                avatarAnimation_->stop();
                const auto scaledAvatarSize = Utils::scale_value(avatar_size);
                avatarOffset_ = scaledAvatarSize;
                avatarAnimation_->setStartValue(scaledAvatarSize);
                avatarAnimation_->start();
            }
        }

        update();
    }

    void AvatarsArea::add(const QString& _aimId)
    {
        add(_aimId, getAvatar(_aimId));
    }

    void AvatarsArea::remove(const QString& _aimId)
    {
        if (avatars_.size() == 1 && avatars_[0].aimId_ == _aimId)
        {
            heightAnimation_->stop();
            heightAnimation_->setStartValue(height());
            heightAnimation_->setEndValue(0);
            heightAnimation_->setDuration(200);
            heightAnimation_->start();
        }

        avatars_.erase(std::remove_if(avatars_.begin(), avatars_.end(), [&_aimId](const AvatarData& _value) { return _value.aimId_ == _aimId; }), avatars_.end());
        Q_EMIT removed();
        update();
    }

    void AvatarsArea::setReplaceFavorites(bool _enable)
    {
        replaceFavorites_ = _enable;
    }

    void AvatarsArea::paintEvent(QPaintEvent* e)
    {
        if (avatars_.empty())
            return;

        const auto b = Utils::scale_bitmap_ratio();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        Utils::drawBackgroundWithBorders(p, rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT), Qt::AlignTop);

        auto horOffset = Utils::scale_value(avatars_area_hor_padding) + diff_;
        auto verOffset = Utils::scale_value(avatars_area_top_padding);

        horOffset -= avatarOffset_;
        for (const auto& a : avatars_)
        {
            p.drawPixmap(horOffset, verOffset, a.avatar_.width() / b, a.avatar_.height() / b, a.avatar_);
            if (a.hovered_)
            {
                Utils::PainterSaver ps(p);

                auto color = QColor(u"#000000");
                color.setAlphaF(0.48);
                p.setBrush(color);
                p.setPen(QPen(Qt::transparent, 0));
                const auto radius = Utils::scale_bitmap_with_value(avatar_size / 2);
                p.drawEllipse(QPoint(horOffset + radius / b, verOffset + radius / b), radius / b, radius / b);

                static const QPixmap hover(Utils::renderSvgScaled(qsl(":/controls/close_icon"), QSize(12, 12), QColor(u"#ffffff")));
                p.drawPixmap(horOffset + radius / b - hover.width() / 2 / b, verOffset + radius / b - hover.height() / 2 / b, hover.width() / b, hover.height() / b, hover);
            }

            horOffset += a.avatar_.width() / b;
            horOffset += Utils::scale_value(avatars_area_spacing);
        }

        const QSize gradientSize(Utils::scale_value(avatar_size) / 2, Utils::scale_value(avatar_size));

        const auto drawGradient = [this, &gradientSize, verOffset](const int _left, const QColor& _start, const QColor& _end)
        {
            QRect grRect(QPoint(_left, verOffset), gradientSize);

            QLinearGradient gradient(grRect.topLeft(), grRect.topRight());
            gradient.setColorAt(0, _start);
            gradient.setColorAt(1, _end);

            QPainter p(this);
            p.fillRect(grRect, gradient);
        };
        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

        if (diff_ != 0)
            drawGradient(0, bg, Qt::transparent);

        if (horOffset > width())
            drawGradient(width() - gradientSize.width(), Qt::transparent, bg);
    }

    void AvatarsArea::mousePressEvent(QMouseEvent* e)
    {
        pressPoint_ = e->pos();
        return QWidget::mousePressEvent(e);
    }

    void AvatarsArea::mouseReleaseEvent(QMouseEvent* e)
    {
        if (Utils::clicked(pressPoint_, e->pos()))
        {
            for (const auto& a : avatars_)
            {
                if (a.hovered_)
                {
                    if (Logic::is_select_chat_members_regim(regim_) && membersModel_)
                        membersModel_->setCheckedItem(a.aimId_, !membersModel_->isCheckedItem(a.aimId_));
#ifndef STRIP_VOIP
                    else if (auto conferenceModel = qobject_cast<ConferenceSearchModel*>(searchModel_))
                        conferenceModel->setCheckedItem(a.aimId_, !conferenceModel->isCheckedItem(a.aimId_));
#endif
                    else
                        Logic::getContactListModel()->setCheckedItem(a.aimId_, !Logic::getContactListModel()->isCheckedItem(a.aimId_));

                    Tooltip::hide();
                    Tooltip::forceShow(false);
                    remove(a.aimId_);
                    break;
                }
            }
        }

        updateHover(e->pos());
        return QWidget::mouseReleaseEvent(e);
    }

    void AvatarsArea::mouseMoveEvent(QMouseEvent* e)
    {
        updateHover(e->pos());
        return QWidget::mouseMoveEvent(e);
    }

    void AvatarsArea::leaveEvent(QEvent* e)
    {
        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        Tooltip::hide();
        Tooltip::forceShow(false);
        update();
        return QWidget::leaveEvent(e);
    }

    void AvatarsArea::wheelEvent(QWheelEvent* e)
    {
        int value = 0;
#ifdef __APPLE__
        auto numPixels = e->pixelDelta();
        auto numDegrees = e->angleDelta() / 8;
        if (!numPixels.isNull())
        {
            value = numPixels.x();
        }
        else if (!numDegrees.isNull())
        {
            auto numSteps = numDegrees / 15;
            value = numSteps.x();
        }
#else
        const auto numDegrees = e->delta() / 8;
        const auto numSteps = numDegrees / 15;
        value = numSteps * Utils::scale_value(avatars_scroll_step);
#endif //__APPLE__

        value /= Utils::scale_bitmap(1);

        auto avatarsWidth = Utils::scale_value(avatars_area_hor_padding);
        for (const auto& a : avatars_)
        {
            avatarsWidth += Utils::scale_value(avatars_area_spacing);
            avatarsWidth += a.avatar_.width() / Utils::scale_bitmap(1);
        }

        if (avatarsWidth < width())
            return;

        diff_ += value;
        diff_ = std::max(diff_, width() - avatarsWidth);
        diff_ = std::min(diff_, 0);

        update();
    }

    void AvatarsArea::resizeEvent(QResizeEvent* e)
    {
        QWidget::resizeEvent(e);
        Q_EMIT resized(QPrivateSignal());
    }

    void AvatarsArea::focusInEvent(QFocusEvent* _event)
    {
        updateFocus(UpdateFocusOrder::Current);
        return QWidget::focusInEvent(_event);
    }

    void AvatarsArea::focusOutEvent(QFocusEvent* _event)
    {
        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        Tooltip::hide();
        Tooltip::forceShow(false);
        update();
        return QWidget::focusOutEvent(_event);
    }

    void AvatarsArea::keyPressEvent(QKeyEvent* _event)
    {
        switch (_event->key())
        {
        case Qt::Key_Right:
            updateFocus(UpdateFocusOrder::Next);
            break;
        case Qt::Key_Left:
            updateFocus(UpdateFocusOrder::Previous);
            break;
        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            const auto hoveredAvatar = std::find_if(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });
            if (hoveredAvatar == avatars_.end())
                break;

            if (Logic::is_select_chat_members_regim(regim_) && membersModel_)
                membersModel_->setCheckedItem(hoveredAvatar->aimId_, !membersModel_->isCheckedItem(hoveredAvatar->aimId_));
            else
                Logic::getContactListModel()->setCheckedItem(hoveredAvatar->aimId_, !Logic::getContactListModel()->isCheckedItem(hoveredAvatar->aimId_));

            if (avatars_.size() > 1)
            {
                if (hoveredAvatar == std::prev(avatars_.end()))
                    std::prev(hoveredAvatar)->hovered_ = true;
                else
                    std::next(hoveredAvatar)->hovered_ = true;
            }

            Tooltip::hide();
            Tooltip::forceShow(false);
            remove(hoveredAvatar->aimId_);

            updateFocus(UpdateFocusOrder::Current);
        }
        break;
        default:
            break;
        }

        QWidget::keyPressEvent(_event);
    }

    void AvatarsArea::updateHover(const QPoint& _pos)
    {
        bool prevHovered = std::any_of(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });

        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        auto x = _pos.x();
        x -= diff_;
        x -= Utils::scale_value(avatars_area_hor_padding);
        auto avatarsWidth = Utils::scale_value(avatars_area_hor_padding);
        avatarsWidth += diff_;
        auto space = 0;
        for (auto& a : avatars_)
        {
            x -= a.avatar_.width() / Utils::scale_bitmap(1);
            x -= Utils::scale_value(avatars_area_spacing);
            avatarsWidth += Utils::scale_value(avatars_area_spacing);
            avatarsWidth += a.avatar_.width() / Utils::scale_bitmap(1);
            if (x <= 0 && avatarsWidth <= width())
            {
                a.hovered_ = true;
                setCursor(Qt::PointingHandCursor);
                Tooltip::show(Logic::getFriendlyContainerProxy(replaceFavorites_ ? Logic::FriendlyContainerProxy::ReplaceFavorites : 0).getFriendly(a.aimId_),
                              QRect(mapToGlobal(QPoint(Utils::scale_value(avatars_area_hor_padding) + space + diff_, Utils::scale_value(avatars_area_top_padding))), Utils::scale_value(QSize(avatar_size, avatar_size))));
                update();
                return;
            }

            space += Utils::scale_value(avatar_size + avatars_area_spacing);
        }

        if (prevHovered)
        {
            Tooltip::hide();
            Tooltip::forceShow(false);
            prevHovered = false;
        }

        setCursor(Qt::ArrowCursor);
        update();
    }

    QPixmap AvatarsArea::getAvatar(const QString& _aimId)
    {
        bool isDefault = false;
        const int32_t avatarProxyFlags = Logic::is_share_regims(regim_) ? Logic::AvatarStorageProxy::ReplaceFavorites : 0;
        auto avatar = Logic::getAvatarStorageProxy(avatarProxyFlags).GetRounded(_aimId,
                                                                                Logic::GetFriendlyContainer()->getFriendly(_aimId),
                                                                                Utils::scale_bitmap(Utils::scale_value(avatar_size)),
                                                                                isDefault, false, false);
        return avatar;
    }

    void AvatarsArea::updateFocus(const UpdateFocusOrder& _order)
    {
        if (avatars_.size() == 0)
            return;

        auto hoveredAvatar = std::find_if(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });
        if (hoveredAvatar != avatars_.end())
        {
            switch (_order)
            {
            case UpdateFocusOrder::Current:
                break;
            case UpdateFocusOrder::Next:
                hoveredAvatar->hovered_ = false;
                if (++hoveredAvatar == avatars_.end())
                    hoveredAvatar = avatars_.begin();
                break;
            case UpdateFocusOrder::Previous:
                hoveredAvatar->hovered_ = false;
                if (hoveredAvatar == avatars_.begin())
                    hoveredAvatar = std::prev(avatars_.end());
                else
                    --hoveredAvatar;
                break;
            }
        }
        else
        {
            hoveredAvatar = avatars_.begin();
        }

        hoveredAvatar->hovered_ = true;

        const auto aWidth = hoveredAvatar->avatar_.width() / Utils::scale_bitmap_ratio();
        const auto aAreaWidth = aWidth + Utils::scale_value(avatars_area_spacing);
        const auto horOffset = Utils::scale_value(avatars_area_hor_padding) + diff_ + std::distance(avatars_.begin(), hoveredAvatar) * aAreaWidth;
        const auto xL = rect().x() + Utils::scale_value(avatars_area_hor_padding);
        const auto xR = rect().width() - aAreaWidth;
        auto correctTooltipX = 0;

        if (horOffset < xL)
        {
            auto val = xL - horOffset;
            diff_ += val;
            correctTooltipX = val;
        }
        else if (horOffset > xR)
        {
            auto val = horOffset - xR;
            diff_ -= val;
            correctTooltipX = -val;
        }

        Tooltip::show(Logic::GetFriendlyContainer()->getFriendly(hoveredAvatar->aimId_),
                      QRect(mapToGlobal(QPoint(horOffset + correctTooltipX, Utils::scale_value(avatars_area_top_padding))), QSize(aWidth, aWidth)));
        Tooltip::forceShow(true);

        update();
    }

    void SelectContactsWidget::itemClicked(const QString& _aimid)
    {
        searchWidget_->setFocus();

        if (regim_ == Logic::MembersWidgetRegim::COUNTRY_LIST)
        {
            Logic::getCountryModel()->setSelected(_aimid);
            mainDialog_->accept();
        }


#ifndef STRIP_VOIP
        auto conferenceModel = qobject_cast<ConferenceSearchModel*>(searchModel_);
#endif

        const auto isSelectChatMembersRegim = Logic::is_select_chat_members_regim(regim_);
        // Restrict max selected elements.
        if (maximumSelectedCount_ >= 0)
        {
#ifndef STRIP_VOIP
            if (Logic::is_video_conference_regim(regim_) && conferenceModel)
            {
                bool isForCheck = !conferenceModel->isCheckedItem(_aimid);
                int selectedItemCount = conferenceModel->getCheckedItemsCount();
                if (isForCheck && (selectedItemCount >= maximumSelectedCount_))
                {
                    // Disable selection.
                    return;
                }
            }
            else
#endif
            if (isSelectChatMembersRegim)
            {
                if (!chatMembersModel_->isCheckedItem(_aimid)
                    && chatMembersModel_->getCheckedItemsCount() >= maximumSelectedCount_)
                {
                    if (auto mainPage = Utils::InterConnector::instance().getMainPage(); mainPage && mainPage->isVideoWindowOn())
                        Utils::showToastOverVideoWindow(QT_TRANSLATE_NOOP("popup_window", "Maximum number of members selected"));
                    else
                        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("popup_window", "Maximum number of members selected"), getToastVerOffset());
                    return;
                }
            }
        }

        if (Logic::is_members_regim(regim_))
        {
            if (regim_ == Logic::MembersWidgetRegim::IGNORE_LIST || regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
            {
                deleteMemberDialog(chatMembersModel_, chatAimId_, _aimid, regim_, this);
            }
            else
            {
                const auto globalCursorPos = mainDialog_->mapFromGlobal(QCursor::pos());
                const auto removeFrame = ::Ui::GetContactListParams().removeContactFrame();

                if (removeFrame.left() <= globalCursorPos.x() && globalCursorPos.x() <= removeFrame.right())
                    deleteMemberDialog(chatMembersModel_, chatAimId_, _aimid, regim_, this);

                Q_EMIT ::Utils::InterConnector::instance().profileSettingsShow(_aimid);
                if constexpr (platform::is_apple())
                    mainDialog_->close();
                else
                    show();
            }
            return;
        }

        const auto disableSelfProfile = !isSelectChatMembersRegim && Logic::is_select_members_regim(regim_) && _aimid == MyInfo()->aimId();

        const auto disableCurrentProfile = chatMembersModel_ && chatMembersModel_->contains(_aimid) && regim_ != Logic::CONTACT_LIST_POPUP && !isSelectChatMembersRegim;
        if (disableCurrentProfile || disableSelfProfile)
            return;

        if (isSelectChatMembersRegim)
        {
            chatMembersModel_->setCheckedItem(_aimid, !chatMembersModel_->isCheckedItem(_aimid));
            mainDialog_->setButtonActive(chatMembersModel_->getCheckedItemsCount() > 0);

            if (chatMembersModel_->getCheckedItemsCount() == maximumSelectedCount_)
            {
                contactList_->enableCLDelegateOpacity(true);
                contactList_->getView()->update();
            }
            else if (contactList_->isCLDelegateOpacityEnabled())
            {
                contactList_->enableCLDelegateOpacity(false);
                contactList_->getView()->update();
            }
        }
#ifndef STRIP_VOIP
        else if (Logic::is_video_conference_regim(regim_) && conferenceModel)
        {
            conferenceModel->addTemporarySearchData(_aimid);

            conferenceModel->setCheckedItem(_aimid, !conferenceModel->isCheckedItem(_aimid));
            mainDialog_->setButtonActive(conferenceModel->getCheckedItemsCount() > 0);
        }
#endif
        else
        {
            if (regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT)
                Logic::getContactListModel()->clearCheckedItems();

            if (Features::isGlobalContactSearchAllowed() && Logic::isRegimWithGlobalContactSearch(regim_))
            {
                if (!searchWidget_->isEmpty() && !Logic::getContactListModel()->contains(_aimid))
                {
                    auto searchModel = contactList_->getSearchModel();
                    searchModel->addTemporarySearchData(_aimid);
                    Logic::getContactListModel()->add(_aimid, Logic::GetFriendlyContainer()->getFriendly(_aimid), true);
                }
            }

            Logic::getContactListModel()->setCheckedItem(_aimid, !Logic::getContactListModel()->isCheckedItem(_aimid));
            contactList_->update();

            if (regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT)
                mainDialog_->accept();
            else if (!isCreateGroupMode())
                mainDialog_->setButtonActive(!Logic::getContactListModel()->getCheckedContacts().empty());
        }

        if (avatarsArea_)
        {
            if ((isSelectChatMembersRegim && chatMembersModel_->isCheckedItem(_aimid))
                || Logic::getContactListModel()->isCheckedItem(_aimid)
#ifndef STRIP_VOIP
                || conferenceModel && conferenceModel->isCheckedItem(_aimid)
#endif
                )
            {
                addAvatarToArea(_aimid);
            }
            else
            {
                removeAvatarFromArea(_aimid);
            }
        }
    }

    class AuthorWidget_p
    {
    public:

        int getCheckBoxSize() const
        {
            return Utils::scale_value(20);
        }
        QRect getCheckBoxRect() const
        {
            return {Utils::scale_value(320), Utils::scale_value(14), getCheckBoxSize(), getCheckBoxSize()};
        }
        QSize getSize() const
        {
            return {Utils::scale_value(DIALOG_WIDTH), Utils::scale_value(48)};
        }
        int getTextLeftMargin() const
        {
            return Utils::scale_value(16);
        }
        int getAuthorTopMargin() const
        {
            return Utils::scale_value(4);
        }
        int getChatNameTopMargin() const
        {
            return Utils::scale_value(24);
        }
        int getCheckBoxTextBaselineTopMargin() const
        {
            return Utils::scale_value(28);
        }
        int getCheckBoxTextRightMargin() const
        {
            return Utils::scale_value(10);
        }
        QRect getCheckBoxClickRect() const
        {
            const int checkBoxTextWidth = checkBoxTextUnit_->desiredWidth();
            const int checkBoxTextLeftOffset = getCheckBoxRect().left() - getCheckBoxTextRightMargin() - checkBoxTextWidth;

            return {checkBoxTextLeftOffset,
                    Utils::scale_value(9),
                    checkBoxTextWidth + getCheckBoxTextRightMargin() + getCheckBoxSize() + Utils::scale_value(4),
                    Utils::scale_value(28)};
        }

        TextRendering::TextUnitPtr authorUnit_;
        TextRendering::TextUnitPtr chatNameUnit_;
        TextRendering::TextUnitPtr checkBoxTextUnit_;
        AuthorWidget::Mode         mode_;
        QString                    chatName_;
        QString                    authors_;
        bool                       checked_;
        QPoint                     pos_;
        bool                       hovered_;
    };

    AuthorWidget::AuthorWidget(QWidget *_parent)
        : QWidget(_parent)
        , d(std::make_unique<AuthorWidget_p>())
    {
        setFixedSize(d->getSize());
        setMouseTracking(true);

        const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        const auto nameFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));

        d->checkBoxTextUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("popup_window", "show author"));
        d->checkBoxTextUnit_->init(nameFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        d->checked_ = true;
        d->hovered_ = false;
    }

    AuthorWidget::~AuthorWidget() = default;

    void AuthorWidget::setAuthors(const QSet<QString>& _authors)
    {
        d->authors_.clear();

        const auto delim = ql1s(", ");
        for (const auto& a : _authors)
        {
            d->authors_ += a;
            d->authors_ += delim;
        }
        if (!d->authors_.isEmpty())
            d->authors_.chop(delim.size());

        update();
    }

    void AuthorWidget::setChatName(const QString &_chatName)
    {
        d->chatName_ = _chatName;
        update();
    }

    void AuthorWidget::setMode(AuthorWidget::Mode _mode)
    {
        d->mode_ = _mode;
        update();
    }

    bool AuthorWidget::isAuthorsEnabled() const
    {
        return d->checked_;
    }

    void AuthorWidget::paintEvent(QPaintEvent *_e)
    {
        QPainter p(this);

        const auto textColor = Styling::getParameters().getColor(d->checked_ ? Styling::StyleVariable::TEXT_SOLID : Styling::StyleVariable::BASE_PRIMARY);

        if (hasFocus() || d->hovered_)
        {
            QColor color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
            p.fillRect(rect(), color);
        }

        if (!d->chatNameUnit_ && d->mode_ == Mode::Group)
        {
            static auto nameFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));
            d->chatNameUnit_ = TextRendering::MakeTextUnit(d->chatName_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            d->chatNameUnit_->init(nameFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }
        if (!d->authorUnit_)
        {
            static auto authorFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
            d->authorUnit_ = TextRendering::MakeTextUnit(d->mode_ == Mode::Channel ? d->chatName_ : d->authors_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            d->authorUnit_->init(authorFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }

        d->checkBoxTextUnit_->getHeight(width());
        const int checkBoxTextLeftOffset = d->getCheckBoxRect().left() - d->getCheckBoxTextRightMargin() - d->checkBoxTextUnit_->desiredWidth();

        d->checkBoxTextUnit_->setOffsets(checkBoxTextLeftOffset, d->getCheckBoxTextBaselineTopMargin());
        d->checkBoxTextUnit_->setColor(textColor);
        d->checkBoxTextUnit_->draw(p, TextRendering::VerPosition::BASELINE);

        const int maxTextWidth = checkBoxTextLeftOffset - d->getTextLeftMargin() - Utils::scale_value(12);

        if (d->mode_ == Mode::Group)
        {
            d->authorUnit_->setOffsets(d->getTextLeftMargin(), d->getAuthorTopMargin());
            d->authorUnit_->getHeight(maxTextWidth);
            d->authorUnit_->setColor(textColor);
            d->authorUnit_->draw(p);

            d->chatNameUnit_->setOffsets(d->getTextLeftMargin(), d->getChatNameTopMargin());
            d->chatNameUnit_->getHeight(maxTextWidth);
            d->chatNameUnit_->setColor(textColor);
            d->chatNameUnit_->draw(p);
        }
        else if (d->mode_ == Mode::Channel || d->mode_ == Mode::Contact)
        {
            d->authorUnit_->setOffsets(d->getTextLeftMargin(), d->getCheckBoxTextBaselineTopMargin());
            d->authorUnit_->getHeight(maxTextWidth);
            d->authorUnit_->setColor(textColor);
            d->authorUnit_->draw(p, TextRendering::VerPosition::BASELINE);
        }

        const auto state = d->checked_ ? CheckBox::Activity::ACTIVE : CheckBox::Activity::NORMAL;
        const auto& img = Ui::CheckBox::getCheckBoxIcon(state);

        p.drawPixmap(d->getCheckBoxRect(), img);
    }

    void AuthorWidget::mousePressEvent(QMouseEvent *_e)
    {
        d->pos_ = _e->pos();
        QWidget::mousePressEvent(_e);
    }

    void AuthorWidget::mouseReleaseEvent(QMouseEvent *_e)
    {
        if (rect().contains(_e->pos()) && rect().contains(d->pos_))
        {
            d->checked_ = !d->checked_;
            update();
        }
        QWidget::mouseReleaseEvent(_e);
    }

    void AuthorWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        if (rect().contains(_e->pos()))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        updateHover(_e->pos());

        QWidget::mouseMoveEvent(_e);
    }

    void AuthorWidget::leaveEvent(QEvent* _e)
    {
        updateHover(QPoint(-1, -1));

        QWidget::leaveEvent(_e);
    }

    void AuthorWidget::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Space || _e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            d->checked_ = !d->checked_;
            update();
        }

        QWidget::keyPressEvent(_e);
    }

    void AuthorWidget::updateHover(const QPoint& _pos)
    {
        d->hovered_ = false;

        if (rect().contains(_pos))
            d->hovered_ = true;

        update();
    }


    SelectContactsWidget::SelectContactsWidget(const QString& _labelText, QWidget* _parent)
        : SelectContactsWidget(nullptr, Logic::MembersWidgetRegim::SHARE, _labelText, QString(), _parent)
    {
    }

    SelectContactsWidget::SelectContactsWidget(
        Logic::CustomAbstractListModel* _chatMembersModel,
        int _regim,
        const QString& _labelText,
        const QString& _buttonText,
        QWidget* _parent,
        SelectContactsWidgetOptions _options)
        : QDialog(_parent)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , mainWidget_(new QWidget(this))
        , chatCreation_(_options.chatCreation_)
    {
        globalLayout_ = Utils::emptyVLayout(mainWidget_);
        mainWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        mainWidget_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
        mainWidget_->installEventFilter(this);

        if (_options.enableAuthorWidget_)
        {
            authorWidget_ = new AuthorWidget(this);
            globalLayout_->addSpacing(Utils::scale_value(10));
            Testing::setAccessibleName(authorWidget_, qsl("AS Search author"));
            globalLayout_->addWidget(authorWidget_);
            focusWidget_[authorWidget_] = FocusPosition::Author;
        }

        if (isCreateGroupMode())
        {
            auto chatCreationWidget = new QWidget(this);
            chatCreationWidget->setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
            chatCreationWidget->setFixedHeight(Utils::scale_value(64));

            auto chatCreationLayout = Utils::emptyHLayout(chatCreationWidget);
            chatCreationLayout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(8), Utils::scale_value(16), 0);

            photo_ = new ContactAvatarWidget(chatCreationWidget, QString(), QString(), Utils::scale_value(56), true);
            photo_->SetMode(ContactAvatarWidget::Mode::CreateChat);
            photo_->SetVisibleShadow(false);
            photo_->SetVisibleSpinner(false);
            photo_->UpdateParams(QString(), QString());
            Testing::setAccessibleName(photo_, qsl("AS AS GroupAndChannelCreation avatar"));
            chatCreationLayout->addWidget(photo_);
            chatCreationLayout->addSpacing(Utils::scale_value(16));

            QWidget::connect(photo_, &ContactAvatarWidget::avatarDidEdit, this, [=]()
            {
                lastCroppedImage_ = photo_->croppedImage();
            }, Qt::QueuedConnection);

            chatName_ = new TextEditEx(chatCreationWidget, Fonts::appFontScaled(18), MessageStyle::getTextColor(), true, true);
            Utils::ApplyStyle(chatName_, Styling::getParameters().getTextEditCommonQss(true));
            chatName_->setWordWrapMode(QTextOption::NoWrap);
            chatName_->setPlaceholderText(_options.isChannel_ ? QT_TRANSLATE_NOOP("groupchats", "Channel name") : QT_TRANSLATE_NOOP("groupchats", "Group name"));
            chatName_->setPlainText(QString());
            chatName_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            chatName_->setAutoFillBackground(false);
            chatName_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            chatName_->document()->setDocumentMargin(0);
            chatName_->addSpace(Utils::scale_value(4));
            chatName_->adjustHeight(Utils::scale_value(NAME_WIDTH));
            Testing::setAccessibleName(chatName_, qsl("AS  AS GroupAndChannelCreation nameInput"));
            chatCreationLayout->addWidget(chatName_);

            focusWidget_[chatName_] = FocusPosition::Name;

            QObject::connect(chatName_, &TextEditEx::textChanged, this, &Ui::SelectContactsWidget::nameChanged);

            globalLayout_->addWidget(chatCreationWidget);
        }

        searchWidget_ = new SearchWidget(this, Utils::scale_value(search_padding_hor), Utils::scale_value(search_padding_ver));
        searchWidget_->setEditEventFilter(this);
        Testing::setAccessibleName(searchWidget_, TestingLocal::getSearchWidgetAccessibleName(regim_));
        globalLayout_->addWidget(searchWidget_);
        focusWidget_[searchWidget_] = FocusPosition::Search;

        contactList_ = new ContactListWidget(this, (Logic::MembersWidgetRegim)regim_, chatMembersModel_, _options.searchModel_);
        Testing::setAccessibleName(contactList_, TestingLocal::getContactListWidgetAccessibleName(regim_));
        contactList_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        contactList_->setWidthForDelegate(Utils::scale_value(DIALOG_WIDTH));
        searchModel_ = contactList_->getSearchModel();

        clHost_ = new QWidget(this);
        auto clVLayout = Utils::emptyVLayout(clHost_);
        auto clHLayout = Utils::emptyHLayout();

        clBottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        clSpacer_ = new QSpacerItem(0, calcListHeight(), QSizePolicy::Fixed, QSizePolicy::Expanding);
        clHLayout->addSpacerItem(clSpacer_);
        clHLayout->addWidget(contactList_);
        clVLayout->addLayout(clHLayout);
        clVLayout->addSpacerItem(clBottomSpacer_);
        globalLayout_->addWidget(clHost_);

        // TODO : use SetView here
        auto dialogParent = isVideoModes() ? parentWidget() : Utils::InterConnector::instance().getMainWindow();
        mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_, dialogParent, _options.handleKeyPressEvents_, true, true, _options.withSemiwindow_);
        mainDialog_->installEventFilter(this);
        mainDialog_->addLabel(_labelText, Qt::AlignTop | Qt::AlignLeft);

        if (isShareModes() || isCreateGroupMode() || Logic::is_select_chat_members_regim(regim_) || Logic::isAddMembersRegim(regim_))
        {
            avatarsArea_ = new AvatarsArea(clHost_, regim_, chatMembersModel_, searchModel_);
            avatarsArea_->setReplaceFavorites(isShareModes());
            avatarsArea_->hide();
            connect(avatarsArea_, &AvatarsArea::removed, this, [this]()
            {
                if (Logic::is_select_chat_members_regim(regim_)
                    && chatMembersModel_->getCheckedItemsCount() < maximumSelectedCount_
                    && contactList_->isCLDelegateOpacityEnabled())
                    contactList_->enableCLDelegateOpacity(false);

                contactList_->getView()->update();

                if (avatarsArea_->avatars() < 1)
                {
                    avatarsArea_->hide();
                    updateSpacer();

                    const auto curFocus = currentFocusPosition();

                    if (!isCreateGroupMode())
                        mainDialog_->setButtonActive(false);

                    if (curFocus == FocusPosition::Avatars || curFocus == FocusPosition::Accept)
                        searchWidget_->setFocus();
                }
            });
            connect(avatarsArea_, &AvatarsArea::showed, this, [this]()
            {
                auto selectedIndexes = contactList_->getView()->selectionModel()->selectedIndexes();
                if (!selectedIndexes.isEmpty())
                    contactList_->getView()->scrollTo(selectedIndexes.first());

                updateSpacer();
            });
            connect(avatarsArea_, &AvatarsArea::resized, this, &SelectContactsWidget::recalcAvatarArea);

            focusWidget_[avatarsArea_] = FocusPosition::Avatars;
        }

        if (isPopupWithCloseBtn())
        {
            cancelButton_ = mainDialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "Close"), true);
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }
        else if (isPopupWithCancelBtn())
        {
            cancelButton_ = mainDialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "Cancel"), true);
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }
        else if ((!Logic::is_members_regim(regim_) && !Logic::is_video_conference_regim(regim_)))
        {
            auto buttonPair = mainDialog_->addButtonsPair(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                _buttonText,
                regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS,
                true,
                true);
            acceptButton_ = buttonPair.first;
            cancelButton_ = buttonPair.second;
            focusWidget_[acceptButton_] = FocusPosition::Accept;
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }

        Testing::setAccessibleName(mainDialog_.get(), TestingLocal::getContactWidgetAccessibleName(regim_));

        connect(contactList_, &ContactListWidget::searchEnd, searchWidget_, &SearchWidget::searchCompleted);
        connect(contactList_, &ContactListWidget::itemSelected, this, &SelectContactsWidget::itemClicked);

        if (chatMembersModel_)
            connect(chatMembersModel_, &Logic::CustomAbstractListModel::containsPreCheckedItems, this, &SelectContactsWidget::containsPreCheckedMembers);

        connect(searchWidget_, &SearchWidget::escapePressed, this, &SelectContactsWidget::escapePressed);
        connect(searchWidget_, &SearchWidget::enterPressed, this, &SelectContactsWidget::enterPressed);
        contactList_->connectSearchWidget(searchWidget_);

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
        {
            connect(contactList_, &ContactListWidget::itemSelected, this, &QDialog::reject, Qt::QueuedConnection);
            connect(contactList_, &ContactListWidget::itemSelected, GetDispatcher(), []()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat, { { "From", "CL_Popup" } });
            });
        }

        connect(contactList_, &ContactListWidget::moreClicked, this, [this](const QString& _aimId)
        {
            Q_EMIT moreClicked(_aimId, QPrivateSignal());
        });

        for (auto&[w, _] : focusWidget_)
            w->installEventFilter(this);

        if (regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showAddToDisallowedInvitersDialog, mainDialog_.get(), &GeneralDialog::accept);
    }

    SelectContactsWidget::~SelectContactsWidget() = default;

    bool SelectContactsWidget::isCheckboxesVisible() const
    {
        return !(Logic::is_members_regim(regim_) || isShareModes());
    }

    bool SelectContactsWidget::isShareModes() const
    {
        return regim_ == Logic::MembersWidgetRegim::SHARE || regim_ == Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE;
    }

    bool SelectContactsWidget::isCreateGroupMode() const
    {
        return chatCreation_;
    }

    bool SelectContactsWidget::isVideoModes() const
    {
        return regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE || regim_ == Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE;
    }

    bool SelectContactsWidget::isPopupWithCloseBtn() const
    {
        return
            regim_ == Logic::MembersWidgetRegim::IGNORE_LIST ||
            regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP ||
            regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS;
    }

    bool SelectContactsWidget::isPopupWithCancelBtn() const
    {
        return regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT || regim_ == Logic::MembersWidgetRegim::COUNTRY_LIST;
    }

    void SelectContactsWidget::updateFocus(bool _order)
    {
        auto reason = Qt::FocusReason::TabFocusReason;
        const auto begin = static_cast<int>(FocusPosition::begin);
        const auto end = static_cast<int>(FocusPosition::end);
        auto pos = static_cast<int>(currentFocusPosition());

        while (1)
        {
            if (pos == begin)
            {
                ++pos;
            }
            else
            {
                if (_order)
                    pos = (pos == end - 1) ? begin + 1 : pos + 1;
                else
                    pos = (pos == begin + 1) ? end - 1 : pos - 1;
            }

            switch (static_cast<FocusPosition>(pos))
            {
            case  FocusPosition::Name:
                if (chatName_)
                {
                    chatName_->setFocus(reason);
                    return;
                }
                break;
            case FocusPosition::Search:
                searchWidget_->setFocus(reason);
                return;
            case FocusPosition::Accept:
                if (acceptButton_ && acceptButton_->isEnabled())
                {
                    acceptButton_->setFocus(reason);
                    return;
                }
                break;
            case FocusPosition::Cancel:
                cancelButton_->setFocus(reason);
                return;
            case FocusPosition::Avatars:
                if (avatarsArea_ && avatarsArea_->avatars() > 0)
                {
                    avatarsArea_->setFocus(reason);
                    return;
                }
                break;
            case FocusPosition::Author:
                if (authorWidget_)
                {
                    authorWidget_->setFocus(reason);
                    return;
                }
                break;
            default:
                return;
            }
        }
    }

    void SelectContactsWidget::updateSpacer()
    {
        clBottomSpacer_->changeSize(0, bottomSpacerHeight(), QSizePolicy::Fixed, QSizePolicy::Fixed);
        clSpacer_->changeSize(0, calcListHeight(), QSizePolicy::Fixed, QSizePolicy::Expanding);
        clHost_->layout()->invalidate();
    }

    void SelectContactsWidget::addAvatarToArea(const QString& _aimId)
    {
        if (!avatarsArea_)
            return;

        avatarsArea_->show();
        avatarsArea_->raise();
        avatarsArea_->add(_aimId);
    }

    void SelectContactsWidget::removeAvatarFromArea(const QString& _aimId)
    {
        if (avatarsArea_)
            avatarsArea_->remove(_aimId);
    }

    void SelectContactsWidget::addAvatarsToArea(const std::vector<QString>& _aimIds)
    {
        if (avatarsArea_ && !_aimIds.empty())
        {
            auto it = _aimIds.cbegin();
            const size_t limit = std::min(_aimIds.size(), static_cast<size_t>(maximumSelectedCount_));
            for (size_t i = 0; i < limit; ++i, ++it)
                addAvatarToArea(*it);
        }
    }

    SelectContactsWidget::FocusPosition SelectContactsWidget::currentFocusPosition() const
    {
        auto fw = qApp->focusWidget();
        while (fw)
        {
            auto curFocus = focusWidget_.find(fw);
            if (curFocus != focusWidget_.end())
                return curFocus->second;

            fw = fw->parentWidget();
        }

        return FocusPosition::begin;
    }

    int SelectContactsWidget::bottomSpacerHeight() const
    {
        return avatarsArea_ && avatarsArea_->isVisible() ? avatarsArea_->height() : 0;
    }

    void SelectContactsWidget::searchEnd()
    {
        Q_EMIT contactList_->searchEnd();
        // contactList_->setSearchMode(false);
    }

    void SelectContactsWidget::escapePressed()
    {
        mainDialog_->close();
    }

    void SelectContactsWidget::enterPressed()
    {
        if (isShareModes() || Logic::is_select_chat_members_regim(regim_))
            return;

        if (QStringView(searchModel_->getSearchPattern()).trimmed().isEmpty() &&
            regim_ != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP &&
            regim_ != Logic::MembersWidgetRegim::SELECT_MEMBERS &&
            regim_ != Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
            mainDialog_->accept();
    }

    void SelectContactsWidget::recalcAvatarArea()
    {
        if (avatarsArea_)
        {
            avatarsArea_->move(0, clHost_->height() - avatarsArea_->height());
            avatarsArea_->setFixedWidth(clHost_->width());
        }
    }

    void SelectContactsWidget::nameChanged()
    {
        if (!photo_ || !chatName_)
            return;

        auto text = chatName_->getPlainText();
        photo_->UpdateParams(QString(), text);
        mainDialog_->setButtonActive(!QStringView(text).trimmed().isEmpty());
    }

    void SelectContactsWidget::containsPreCheckedMembers(const std::vector<QString>& _names)
    {
        if (Logic::is_select_chat_members_regim(regim_))
        {
            if (_names.empty())
                return;

            addAvatarsToArea(_names);

            // more members checked - uncheck them
            if (_names.size() > static_cast<size_t>(maximumSelectedCount_))
                std::for_each(std::next(_names.cbegin(), maximumSelectedCount_), _names.cend(), [this](auto& _aimId) { chatMembersModel_->setCheckedItem(_aimId, false); });

            contactList_->enableCLDelegateOpacity(false);
            if (chatMembersModel_->getCheckedItemsCount() == maximumSelectedCount_)
                contactList_->enableCLDelegateOpacity(true);

            contactList_->getView()->update();

            mainDialog_->setButtonActive(true);
        }
    }

    int SelectContactsWidget::calcListHeight() const
    {
        const auto parentWdg = isVideoModes() ? parentWidget() : Utils::InterConnector::instance().getMainWindow();
        const auto newHeight = ::Ui::ItemLength(false, heightPartOfMainWindowForFullView, 0, parentWdg);

        auto extraHeight = searchWidget_->sizeHint().height();

        const auto itemHeight = ::Ui::GetContactListParams().itemHeight();
        const int count = (newHeight - extraHeight) / itemHeight;

        int clHeight = (count + 0.5) * itemHeight;
        clHeight -= bottomSpacerHeight();

        return clHeight;
    }

    bool SelectContactsWidget::show()
    {
        const auto isSelectChatMembersRegim = Logic::is_select_chat_members_regim(regim_);

        Logic::getContactListModel()->setIsWithCheckedBox(!isShareModes() && !isSelectChatMembersRegim);

        if (regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
            mainDialog_->setButtonActive(true);
        else if (!isPopupWithCloseBtn() && !isPopupWithCancelBtn())
            mainDialog_->setButtonActive(!Logic::getContactListModel()->getCheckedContacts().empty());
        else if (isCreateGroupMode() || isSelectChatMembersRegim)
            mainDialog_->setButtonActive(false);

        if (isCreateGroupMode())
            chatName_->setFocus();
        else
            searchWidget_->setFocus();

        if (isShareModes() || isSelectChatMembersRegim)
            Logic::getContactListModel()->clearCheckedItems();
        addAvatarsToArea(Logic::getContactListModel()->getCheckedContacts());

        auto result = true;
        const auto guard = QPointer(this);
        const auto atExit = Utils::ScopeExitT([&result]()
        {
            if (!result)
                Logic::getContactListModel()->clearCheckedItems();
            Logic::getContactListModel()->setIsWithCheckedBox(false);
        });

        result = mainDialog_->showInCenter();
        if (!guard)
            return result;

        if (result && !isCreateGroupMode() && regim_ != Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
        {
            if (regim_ == Logic::MembersWidgetRegim::COUNTRY_LIST)
            {
                if (!Logic::getCountryModel()->isCountrySelected())
                    result = false;
            }
            else
            {
                if (!Logic::is_video_conference_regim(regim_) && !isSelectChatMembersRegim && Logic::getContactListModel()->getCheckedContacts().empty())
                    result = false;
            }
        }

        if (Features::isGlobalContactSearchAllowed() && Logic::isRegimWithGlobalContactSearch(regim_))
        {
            if (auto searchModel = contactList_->getSearchModel())
                searchModel->removeAllTemporarySearchData();
        }

        searchWidget_->clearInput();
        return result;
    }

    QString SelectContactsWidget::getName() const
    {
        if (!isCreateGroupMode())
            return QString();

        return chatName_->getPlainText().trimmed();
    }

    void SelectContactsWidget::setAuthors(const QSet<QString> &_authors)
    {
        if (authorWidget_)
            authorWidget_->setAuthors(_authors);
    }

    void SelectContactsWidget::setChatName(const QString &_chatName)
    {
        if (authorWidget_)
            authorWidget_->setChatName(_chatName);
    }

    void SelectContactsWidget::setAuthorWidgetMode(AuthorWidget::Mode _mode)
    {
        if (authorWidget_)
            authorWidget_->setMode(_mode);
    }

    bool SelectContactsWidget::isAuthorsEnabled() const
    {
        return authorWidget_ && authorWidget_->isAuthorsEnabled();
    }

    void SelectContactsWidget::setEmptyLabelVisible(bool _isEmptyIgnoreList)
    {
        contactList_->setEmptyLabelVisible(_isEmptyIgnoreList);
        updateSpacer();
    }

    void SelectContactsWidget::UpdateMembers()
    {
        mainDialog_->update();
    }

    void SelectContactsWidget::setMaximumSelectedCount(int number)
    {
        maximumSelectedCount_ = number;
    }

    void SelectContactsWidget::setChatAimId(const QString& _chatId)
    {
        chatAimId_ = _chatId;
    }

    void Ui::SelectContactsWidget::rewindToTop()
    {
        contactList_->rewindToTop();
    }

    bool SelectContactsWidget::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::ShortcutOverride)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                keyEvent->accept();
                return true;
            }
        }
        else if (_event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                keyEvent->accept();
                updateFocus(keyEvent->key() == Qt::Key_Tab ? true : false);
                return true;
            }
        }
        else if (_obj == mainWidget_ && _event->type() == QEvent::Resize)
        {
            recalcAvatarArea();
        }

        return false;
    }

    void SelectContactsWidget::UpdateContactList()
    {
        contactList_->update();
    }

    void SelectContactsWidget::reject()
    {
        if (mainDialog_)
            mainDialog_->reject();
    }

    void SelectContactsWidget::updateSize()
    {
        if (parentWidget())
            mainDialog_->updateSize();
    }
}

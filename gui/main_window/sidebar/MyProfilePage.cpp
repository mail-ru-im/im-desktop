#include "stdafx.h"
#include "MyProfilePage.h"
#include "SidebarUtils.h"
#include "EditNameWidget.h"
#include "EditDescriptionWidget.h"
#include "EditNicknameWidget.h"

#include "../MainPage.h"
#include "../MainWindow.h"
#include "../PhoneWidget.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/ContactList.h"
#include "../friendly/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"
#include "../../gui_settings.h"
#include "../../core_dispatcher.h"

#include "../../previewer/toast.h"

namespace
{
    auto getDefaultProfileWidth() noexcept
    {
        return Utils::scale_value(428);
    }

    auto getTopMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getBottomMargin() noexcept
    {
        return Utils::scale_value(52);
    }

    auto getLeftMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getInfoPlateTopMargin() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(10);
        else
            return Utils::scale_value(8);
    }

    auto getInfoPlateBottomMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getInfoPlateIconTopMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getInfoPlateIconBottomMargin() noexcept
    {
        return Utils::scale_value(10);
    }

    auto getInfoPlateIconRightMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getAvatarSize() noexcept
    {
        return Utils::scale_value(80);
    }

    auto getInfoPlateTextMaximumWidth() noexcept
    {
        return Utils::scale_value(540);
    }

    auto getInfoPlateIconSpacing() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getInfoPlateTextSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(6);
        else
            return Utils::scale_value(2);
    }

    auto getInfoPlateSpacing() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getSignoutSpacing() noexcept
    {
        return Utils::scale_value(32);
    }

    auto getInfoPlateIconSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    auto getHeaderTextFont()
    {
        return Fonts::appFontScaled(15);
    }

    auto getInfoTextFont()
    {
        return Fonts::appFontScaled(16);
    }

    auto getLabelTextFont()
    {
        return Fonts::appFontScaled(16);
    }

    auto getButtonsTopMargin() noexcept
    {
        return Utils::scale_value(21);
    }

    auto getButtonsBottomMargin() noexcept
    {
        return Utils::scale_value(19);
    }

    auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }
}

namespace Ui
{
    QSize TextResizedWidget::sizeHint() const
    {
        auto width = qMin(maxAvalibleWidth_ ? maxAvalibleWidth_ : text_->desiredWidth(), QWidget::width());
        auto height = text_->getHeight(width);
        return QSize(width, height);
    }

    QSize TextResizedWidget::minimumSizeHint() const
    {
        return sizeHint();
    }

    void TextResizedWidget::setMaximumWidth(int _width)
    {
        maxAvalibleWidth_ = _width;
        QWidget::setMaximumWidth(maxAvalibleWidth_);
        updateGeometry();
        update();
    }

    void TextResizedWidget::setText(const QString& _text, const QColor& _color)
    {
        text_->setText(_text, _color);
        updateGeometry();
        update();
    }

    void TextResizedWidget::setOpacity(qreal _opacity)
    {
        opacity_ = _opacity;
        update();
    }

    QString TextResizedWidget::getText() const
    {
        return text_->getText();
    }

    void TextResizedWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setOpacity(opacity_);
        text_->draw(p);
    }

    void TextResizedWidget::resizeEvent(QResizeEvent* _event)
    {
        updateGeometry();
        update();
        QWidget::resizeEvent(_event);
    }


    MyInfoPlate::MyInfoPlate(QWidget* _parent, const QString& _iconPath, int _leftMargin, const QString& _infoEmptyStr, Qt::Alignment _align, int _maxInfoLinesCount, int _correctTopMargin, int _correctBottomMargin)
        : QWidget(_parent)
        , leftMargin_(_leftMargin)
        , infoEmptyStr_(_infoEmptyStr)
        , hovered_(false)
        , pressed_(false)
    {
        QSizePolicy sp(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        setSizePolicy(sp);
        setMouseTracking(true);

        auto rootVLayout = Utils::emptyVLayout(this);
        rootVLayout->setContentsMargins(0, getInfoPlateTopMargin() - _correctTopMargin, 0, getInfoPlateBottomMargin() - _correctBottomMargin);
        rootVLayout->setAlignment(_align);
        auto rootHLayout = Utils::emptyHLayout();
        {
            auto textLayout = Utils::emptyVLayout();
            textLayout->setContentsMargins(leftMargin_, 0, 0, 0);
            header_ = new Ui::TextResizedWidget(this, QString(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
            header_->init(getHeaderTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            header_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
            Testing::setAccessibleName(header_, qsl("AS myinfo header_"));
            textLayout->addWidget(header_);

            textLayout->addSpacing(getInfoPlateTextSpacing());

            auto infoLayout = Utils::emptyHLayout();
            info_ = new Ui::TextResizedWidget(this, QString(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
            info_->init(getInfoTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(),
                        TextRendering::HorAligment::LEFT, _maxInfoLinesCount, Ui::TextRendering::LineBreakType::PREFER_SPACES);
            sp.setHorizontalStretch(1);
            info_->setSizePolicy(sp);
            info_->setMaximumWidth(getInfoPlateTextMaximumWidth());
            Testing::setAccessibleName(info_, qsl("AS myinfo info_"));
            infoLayout->addWidget(info_);
            infoLayout->addStretch();
            textLayout->addLayout(infoLayout);

            rootHLayout->addLayout(textLayout);

            rootHLayout->addSpacing(getInfoPlateIconSpacing());

            auto iconLayout = Utils::emptyVLayout();
            iconLayout->setContentsMargins(0, getInfoPlateIconTopMargin(), getInfoPlateIconRightMargin(), getInfoPlateIconBottomMargin());
            auto iconWidget = new QWidget(this);
            iconWidget->setFixedSize(getInfoPlateIconSize());
            auto iconWLayout = Utils::emptyVLayout(iconWidget);
            iconArea_ = new QLabel(this);
            iconHovered_ = Utils::renderSvg(_iconPath, getInfoPlateIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
            iconPressed_ = Utils::renderSvg(_iconPath, getInfoPlateIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
            iconArea_->setPixmap(iconHovered_);
            iconArea_->hide();
            iconWLayout->addWidget(iconArea_);
            iconLayout->addWidget(iconWidget);

            rootHLayout->addLayout(iconLayout);
            Utils::grabTouchWidget(this);
        }
        rootVLayout->addLayout(rootHLayout);
    }

    void MyInfoPlate::setHeader(const QString& _header)
    {
        header_->setText(_header);
        Testing::setAccessibleName(header_, qsl("AS myinfo head =") + _header);

        update();
    }

    void MyInfoPlate::setInfo(const QString& _info, const QString& _prefix)
    {
        infoStr_ = _info;

        if (infoStr_.isEmpty() && !infoEmptyStr_.isEmpty())
        {
            info_->setText(infoEmptyStr_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        }
        else
        {
            if (!infoStr_.startsWith(_prefix))
                infoStr_ = _prefix + infoStr_;

            info_->setText(infoStr_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        }
        Testing::setAccessibleName(info_, qsl("AS myinfo body =") + infoStr_);

        update();
    }

    QString MyInfoPlate::getInfoText() const
    {
        return info_ ? info_->getText() : QString();
    }

    QString MyInfoPlate::getInfoStr() const
    {
        return infoStr_;
    }

    void MyInfoPlate::paintEvent(QPaintEvent* _event)
    {
        if (hasFocus() || hovered_ || pressed_)
        {
            QPainter p(this);
            p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            if (iconArea_->isHidden())
            {
                iconArea_->setPixmap(iconHovered_);
                iconArea_->show();
            }
        }
        else
        {
            iconArea_->hide();
        }
    }

    void MyInfoPlate::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            pressed_ = true;
            pos_ = _event->pos();
            iconArea_->setPixmap(iconPressed_);
        }

        QWidget::mousePressEvent(_event);
    }

    void MyInfoPlate::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            pressed_ = false;

            if (isEnabled())
            {
                if (rect().contains(_event->pos()) && rect().contains(pos_))
                    emit clicked();

                if (hovered_)
                    iconArea_->setPixmap(iconHovered_);
            }
        }

        QWidget::mouseReleaseEvent(_event);
    }

    void MyInfoPlate::enterEvent(QEvent* _event)
    {
        if (isEnabled())
        {
            hovered_ = true;
            setCursor(Qt::PointingHandCursor);
            update();
        }

        QWidget::enterEvent(_event);
    }

    void MyInfoPlate::leaveEvent(QEvent* _event)
    {
        if (!pressed_ && isEnabled())
        {
            hovered_ = false;
            setCursor(Qt::ArrowCursor);
            update();
        }

        QWidget::leaveEvent(_event);
    }


    MyProfilePage::MyProfilePage(QWidget* _parent)
        : SidebarPage(_parent)
        , frameCountMode_(FrameCountMode::_1)
        , isActive_(false)
        , isSendCommonStat_(false)
        , isDefaultAvatar_(false)
    {
        init();
    }

    void MyProfilePage::signOutClicked()
    {
        const QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Sign out"),
            nullptr);

        if (confirm)
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_signout_action, { {"do", "sign_out"} });

            get_gui_settings()->set_value(settings_feedback_email, QString());
            GetDispatcher()->post_message_to_core("logout", nullptr);
            emit Utils::InterConnector::instance().logout();
        }
        else
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_signout_action, { {"do", "cancel"} });
        }
    }

    void MyProfilePage::changePhoneNumber()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_changenum_action);

        if (phone_->getInfoStr().isEmpty())
        {
            emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachPhone);
        }
        else
        {
            auto phoneWidget = new PhoneWidget(0);
            auto generalDialog = std::make_unique<GeneralDialog>(phoneWidget, Utils::InterConnector::instance().getMainWindow());
            generalDialog->showInCenter();
        }
    }

    void MyProfilePage::initFor(const QString& _aimId)
    {
        UNUSED_ARG(_aimId);

        isActive_ = true;
        isSendCommonStat_ = true;

        updateInfo();
    }

    void MyProfilePage::setFrameCountMode(FrameCountMode _mode)
    {
        frameCountMode_ = _mode;
    }

    void MyProfilePage::close()
    {

    }

    void MyProfilePage::init()
    {
        auto area = CreateScrollAreaAndSetTrScrollBarV(this);
        area->setContentsMargins(0, 0, 0, 0);
        area->setWidgetResizable(true);
        area->setFrameStyle(QFrame::NoFrame);
        area->setStyleSheet(qsl("background: transparent;"));
        area->horizontalScrollBar()->setDisabled(true);
        Testing::setAccessibleName(area, qsl("AS profile area"));

        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(area);

        mainWidget_ = new QWidget(area);
        mainWidget_->setStyleSheet(qsl("background: transparent;"));
        area->setWidget(mainWidget_);

        Utils::grabTouchWidget(area->viewport(), true);
        Utils::grabTouchWidget(mainWidget_);

        const auto biz_phone_disabled = build::is_biz() && !Features::bizPhoneAllowed();

        auto rootLayout = Utils::emptyVLayout(mainWidget_);
        rootLayout->setContentsMargins(0, getTopMargin(), 0, getBottomMargin());
        rootLayout->setAlignment(Qt::AlignTop);
        rootLayout->setSpacing(getInfoPlateSpacing());
        {
            auto avatarLayout = Utils::emptyHLayout();
            avatarLayout->setContentsMargins(getLeftMargin(), 0, 0, 0);
            {
                avatar_ = new ContactAvatarWidget(mainWidget_, QString(), QString(), getAvatarSize(), true, true);
                avatar_->SetMode(ContactAvatarWidget::Mode::MyProfile);

                connect(avatar_, &ContactAvatarWidget::leftClicked, this, &MyProfilePage::editAvatarClicked);
                connect(avatar_, &ContactAvatarWidget::rightClicked, this, &MyProfilePage::showAvatarMenu);
                connect(avatar_, &ContactAvatarWidget::mouseEntered, this, [this]()
                {
                    auto handCursor = !Logic::getContactListModel()->isChat(currentAimId_);
                    avatar_->setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
                });

                Utils::grabTouchWidget(avatar_);
                Testing::setAccessibleName(avatar_, qsl("AS profile avatar_"));
                avatarLayout->addWidget(avatar_);

                avatarLayout->addSpacing(getInfoPlateSpacing());

                name_ = new MyInfoPlate(mainWidget_, qsl(":/context_menu/edit"), getInfoPlateSpacing(), QT_TRANSLATE_NOOP("sidebar", "Add name"), Qt::AlignVCenter, 2, 0, Utils::scale_value(4));
                name_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Name"));
                name_->setFixedHeight(getAvatarSize());
                Testing::setAccessibleName(name_, qsl("AS profile name_"));
                avatarLayout->addWidget(name_);
            }

            rootLayout->addLayout(avatarLayout);

            aboutMe_ = new MyInfoPlate(mainWidget_, qsl(":/context_menu/edit"), getLeftMargin(), QT_TRANSLATE_NOOP("sidebar", "Add description"));
            aboutMe_->setHeader(QT_TRANSLATE_NOOP("sidebar", "About me"));
            Testing::setAccessibleName(aboutMe_, qsl("AS profile aboutMe_"));
            rootLayout->addWidget(aboutMe_);

            phone_ = new MyInfoPlate(mainWidget_, qsl(":/context_menu/edit"), getLeftMargin(), QT_TRANSLATE_NOOP("sidebar", "Attach phone"));
            phone_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Phone number"));
            Testing::setAccessibleName(phone_, qsl("AS profile phone_"));
            rootLayout->addWidget(phone_);
            phone_->setVisible(!build::is_dit() && !biz_phone_disabled);

            nickName_ = new MyInfoPlate(mainWidget_, qsl(":/context_menu/edit"), getLeftMargin(), QT_TRANSLATE_NOOP("sidebar", "Add nickname"));
            nickName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Nickname"));
            Testing::setAccessibleName(nickName_, qsl("AS profile nickName_"));
            rootLayout->addWidget(nickName_);

            email_ = new MyInfoPlate(mainWidget_, QString(), getLeftMargin());
            email_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Email"));
            email_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            email_->setEnabled(false);
            Testing::setAccessibleName(email_, qsl("AS profile email_"));
            rootLayout->addWidget(email_);

            rootLayout->addSpacing(getInfoPlateSpacing());

            {
                auto labelLayout = Utils::emptyVLayout();
                labelLayout->setContentsMargins(getLeftMargin(), 0, 0, 0);
                {
                    auto hLayout = Utils::emptyHLayout();
                    permissionsInfo_ = new LabelEx(mainWidget_);
                    permissionsInfo_->setFont(getLabelTextFont());
                    permissionsInfo_->setColors(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY),
                                                Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER),
                                                Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
                    permissionsInfo_->setText(QT_TRANSLATE_NOOP("sidebar", "Who can see my data"));
                    permissionsInfo_->setCursor(Qt::PointingHandCursor);
                    permissionsInfo_->setVisible(!build::is_dit() && !build::is_biz());
                    Utils::grabTouchWidget(permissionsInfo_);
                    Testing::setAccessibleName(permissionsInfo_, qsl("AS profile permissionsInfo_"));
                    hLayout->addWidget(permissionsInfo_);
                    hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                    labelLayout->addLayout(hLayout);
                }

                if (!build::is_dit() && !build::is_biz())
                    labelLayout->addSpacing(getSignoutSpacing());

                {
                    auto hLayout = Utils::emptyHLayout();
                    signOut_ = new LabelEx(mainWidget_);
                    signOut_->setFont(getLabelTextFont());
                    signOut_->setColors(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION),
                                        Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION_HOVER),
                                        Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE));
                    signOut_->setText(QT_TRANSLATE_NOOP("sidebar", "Sign out"));
                    signOut_->setCursor(Qt::PointingHandCursor);
                    Utils::grabTouchWidget(signOut_);
                    Testing::setAccessibleName(signOut_, qsl("AS profile quitLabel_"));
                    hLayout->addWidget(signOut_);
                    hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                    labelLayout->addLayout(hLayout);
                }
                rootLayout->addLayout(labelLayout);
            }

            rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
        }

        connect(MyInfo(), &Ui::my_info::received, this, &MyProfilePage::changed, Qt::UniqueConnection);
        connect(GetDispatcher(), &Ui::core_dispatcher::userInfo, this, &MyProfilePage::onUserInfo);

        connect(avatar_, &Ui::ContactAvatarWidget::afterAvatarChanged, this, &Ui::MyProfilePage::avatarChanged);
        connect(avatar_, &Ui::ContactAvatarWidget::cancelSelectFileForAvatar, this, &Ui::MyProfilePage::avatarCancelSelect);
        connect(name_, &Ui::MyInfoPlate::clicked, this, &Ui::MyProfilePage::editNameClicked);
        connect(aboutMe_, &Ui::MyInfoPlate::clicked, this, &Ui::MyProfilePage::editAboutMeClicked);
        connect(nickName_, &Ui::MyInfoPlate::clicked, this, &Ui::MyProfilePage::editNicknameClicked);
        connect(phone_, &Ui::MyInfoPlate::clicked, this, &Ui::MyProfilePage::changePhoneNumber);
        phone_->setEnabled(!build::is_dit() && !biz_phone_disabled);

        connect(permissionsInfo_, &LabelEx::clicked, this, &MyProfilePage::permissionsInfoClicked);
        connect(signOut_, &LabelEx::clicked, this, &MyProfilePage::signOutClicked);

        connect(GetDispatcher(), &Ui::core_dispatcher::updateProfile, this, &MyProfilePage::onUpdateProfile);
    }

    void MyProfilePage::updateInfo()
    {
        currentAimId_ = MyInfo()->aimId();

        GetDispatcher()->getUserInfo(currentAimId_);

        Logic::GetAvatarStorage()->ForceRequest(currentAimId_, getAvatarSize());
        avatar_->UpdateParams(currentAimId_, Logic::GetFriendlyContainer()->getFriendly(currentAimId_));
        avatar_->update();

        if (MyInfo()->haveConnectedEmail())
        {
            email_->show();
            nickName_->hide();
        }
        else
        {
            email_->hide();
            nickName_->setVisible(Features::isNicksEnabled() && Utils::isUin(Ui::MyInfo()->aimId()));
        }

        const auto biz_phone_disabled = build::is_biz() && !Features::bizPhoneAllowed();
        phone_->setVisible(!build::is_dit() && !biz_phone_disabled);
        phone_->setEnabled(!build::is_dit() && !biz_phone_disabled);

    }

    void Ui::MyProfilePage::sendCommonStat()
    {
        core::stats::event_props_type props = {
            { "FirstName", firstName_.isEmpty() ? "0" : "1" },
            { "LastName", lastName_.isEmpty() ? "0" : "1" },
            { "AboutMe", aboutMe_->getInfoStr().isEmpty() ? "0" : "1" },
            { "Number", phone_->getInfoStr().isEmpty() ? "0" : "1" },
            // for agent-account use "NickName" as email and it's always "1"
            { "NickName", MyInfo()->haveConnectedEmail() ? "1" : nickName_->getInfoStr().isEmpty() ? "0" : "1" }
        };

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_view, props);
    }

    void MyProfilePage::changed()
    {
        if (isActive_)
            updateInfo();
    }

    void MyProfilePage::resizeEvent(QResizeEvent* _event)
    {
        mainWidget_->setFixedWidth(_event->size().width());
        QWidget::resizeEvent(_event);
    }

    void Ui::MyProfilePage::hideEvent(QHideEvent* _event)
    {
        UNUSED_ARG(_event);
        isActive_ = false;
    }

    void MyProfilePage::onUserInfo(const int64_t _seq, const QString& _aimid, const Data::UserInfo& _info)
    {
        UNUSED_ARG(_seq);

        if (currentAimId_ != _aimid)
            return;

        firstName_ = _info.firstName_;
        lastName_ = _info.lastName_;
        name_->setInfo(_info.friendly_);

        aboutMe_->setInfo(_info.about_);

        const auto& number = _info.phoneNormalized_;
        phone_->setInfo(number);

        const auto& nick = _info.nick_;
        nickName_->setInfo(nick, nick.isEmpty() ? QString() : qsl("@"));

        email_->setInfo(currentAimId_);

        if (isSendCommonStat_)
        {
            isSendCommonStat_ = false;
            sendCommonStat();
        }
    }

    void MyProfilePage::onUpdateProfile(int _error)
    {
        auto text = _error == 0 ? QT_TRANSLATE_NOOP("sidebar", "Profile updated") : QT_TRANSLATE_NOOP("sidebar", "Error updating profile");
        auto toast = new Ui::Toast(text, this);
        Ui::ToastManager::instance()->showToast(toast, QPoint(width() / 2, height() - toast->height() - getToastVerOffset()));
    }

    void MyProfilePage::editAvatarClicked()
    {
        isDefaultAvatar_ = avatar_->isDefault();
        avatar_->selectFileForAvatar();
    }

    void MyProfilePage::viewAvatarClicked()
    {
        if (!avatar_->isDefault())
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_avatar_action, { {"do", "look"} });
            Utils::InterConnector::instance().getMainWindow()->openAvatar(currentAimId_);
        }
    }

    void MyProfilePage::avatarChanged()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_avatar_action, { {"do", "edit"}, {"type", isDefaultAvatar_ ? "new" : "change"} });
    }

    void Ui::MyProfilePage::avatarCancelSelect()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_avatar_action, { {"do", "cancel"}, {"type", isDefaultAvatar_ ? "new" : "change"} });
    }

    void MyProfilePage::showAvatarMenu()
    {
        auto menu = new ContextMenu(this);
        menu->addActionWithIcon(qsl(":/cam_icon"), QT_TRANSLATE_NOOP("sidebar", "Change avatar"), Logic::makeData(qsl("edit")));
        if (!avatar_->isDefault())
            menu->addActionWithIcon(qsl(":/eye_icon"), QT_TRANSLATE_NOOP("sidebar", "View avatar"), Logic::makeData(qsl("view")));

        connect(menu, &ContextMenu::triggered, this, &MyProfilePage::triggeredAvatarMenu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        menu->popup(QCursor::pos());
    }

    void MyProfilePage::triggeredAvatarMenu(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const auto command = params[qsl("command")].toString();

        if (command == ql1s("edit"))
            editAvatarClicked();
        else if (command == ql1s("view"))
            viewAvatarClicked();
    }

    void MyProfilePage::editNameClicked()
    {
        auto form = new EditNameWidget(this, {firstName_, lastName_});

        Ui::GeneralDialog::Options options;
        options.preferredSize_ = QSize(form->width(), -1);
        options.ignoreRejectDlgPairs_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter, Utils::CloseWindowInfo::Reason::MacEventFilter);

        auto gd = std::make_unique<Ui::GeneralDialog>(form, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, options);
        gd->setIgnoredKeys({Qt::Key_Return, Qt::Key_Enter});

        connect(form, &EditNameWidget::changed, this, [this, form]()
        {
            auto data = form->getFormData();
            firstName_ = data.firstName_;
            lastName_ = data.lastName_;
            auto nameSpacer = !lastName_.isEmpty() ? qsl(" ") : QString();
            QString friendly = firstName_ + nameSpacer + lastName_;
            name_->setInfo(friendly);
            MyInfo()->setFriendly(friendly);
            update();
        });

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_name_action);

        auto buttonsPair = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "APPLY"), true, false, false);
        form->setButtonsPair(buttonsPair);
        gd->setButtonsAreaMargins(QMargins(0, getButtonsTopMargin(), 0, getButtonsBottomMargin()));
        gd->showInCenter();
    }

    void MyProfilePage::editAboutMeClicked()
    {
        auto form = new EditDescriptionWidget(this, { aboutMe_->getInfoStr() });

        Ui::GeneralDialog::Options options;
        options.preferredSize_ = QSize(form->width(), -1);
        options.ignoreRejectDlgPairs_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter, Utils::CloseWindowInfo::Reason::MacEventFilter);

        auto gd = std::make_unique<Ui::GeneralDialog>(form, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, options);
        gd->setIgnoredKeys({ Qt::Key_Return, Qt::Key_Enter });

        connect(form, &EditDescriptionWidget::changed, this, [this, form]()
        {
            aboutMe_->setInfo(form->getFormData().description_);
            update();
        });

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_aboutme_action);

        auto buttonsPair = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "APPLY"), true, false, false);
        form->setButtonsPair(buttonsPair);
        gd->setButtonsAreaMargins(QMargins(0, getButtonsTopMargin(), 0, getButtonsBottomMargin()));
        gd->showInCenter();
    }

    void MyProfilePage::editNicknameClicked()
    {
        auto form = new EditNicknameWidget(this, { nickName_->getInfoStr().mid(1) });
        form->setStatFrom("profile_edit_scr");

        connect(form, &EditNicknameWidget::changed, this, [this, form]()
        {
            nickName_->setInfo(form->getFormData().nickName_, qsl("@"));
            update();
            onUpdateProfile(0);
        });

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_nickedit_action);

        showEditNicknameDialog(form);
    }

    void MyProfilePage::permissionsInfoClicked() const
    {
        QString url = Features::dataVisibilityLink() % Utils::GetTranslator()->getLang();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profileeditscr_access_action);
        QDesktopServices::openUrl(url);
    }
}

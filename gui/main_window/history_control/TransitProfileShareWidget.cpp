#include "stdafx.h"
#include "TransitProfileShareWidget.h"

#include "main_window/friendly/FriendlyContainer.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/stat_utils.h"
#include "utils/InterConnector.h"
#include "utils/PhoneFormatter.h"
#include "../MainWindow.h"
#include "main_window/GroupChatOperations.h"
#include "../sidebar/UserProfile.h"
#include "../sidebar/SidebarUtils.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/DialogButton.h"
#include "../contact_list/AddContactDialogs.h"
#include "previewer/toast.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

namespace
{
    int maxWidth()
    {
        return Utils::scale_value(360);
    }

    int padding()
    {
        return Utils::scale_value(16);
    }

    int avatarPadding()
    {
        return Utils::scale_value(24);
    }

    int avatarSize()
    {
        return Utils::scale_value(52);
    }

    int textHorOffset()
    {
        return Utils::scale_value(12);
    }

    int verOffset()
    {
        return Utils::scale_value(20);
    }

    QFont primaryTextFont(Fonts::FontWeight _weight = Fonts::FontWeight::Normal)
    {
        return Fonts::appFontScaled(platform::is_apple() ? 15 : 16, _weight);
    }

    QFont secondaryTextFont()
    {
        return Fonts::appFontScaled(platform::is_apple() ? 13 : 14);
    }

    QFont errorTextFont()
    {
        return Fonts::appFontScaled(platform::is_apple() ? 12 : 13);
    }

    QColor primaryTextColor()
    {
        const auto static c = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        return c;
    }

    QColor secondaryTextColor()
    {
        const auto static c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        return c;
    }

    QString getCaption(TransitState _errors)
    {
        if (_errors == TransitState::EMPTY_PROFILE)
            return QT_TRANSLATE_NOOP("profilesharing", "No phone number or nickname");
        if (_errors == TransitState::UNCHECKED)
            return QT_TRANSLATE_NOOP("profilesharing", "No phone number and nickname");

        return QT_TRANSLATE_NOOP("profilesharing", "Share contact");
    }

    QString getErrorText(TransitState _errors)
    {
        if (_errors == TransitState::UNCHECKED)
            return QT_TRANSLATE_NOOP("profilesharing", "To share contact add phone number or ask contact to add nickname.");

        return QT_TRANSLATE_NOOP("profilesharing", "To share contact ask contact to add nickname or add new contact with phone number.");
    }

    QString leftButtonText()
    {
        return QT_TRANSLATE_NOOP("profilesharing", "BACK");
    }

    QString rightButtonText(TransitState _errors)
    {
        return _errors != TransitState::ON_PROFILE ? QT_TRANSLATE_NOOP("profilesharing", "OPEN CHAT") : QT_TRANSLATE_NOOP("profilesharing", "SEND");
    }

    int buttonSize()
    {
        return Utils::scale_value(24);
    }

    struct ButtonColors
    {
        Styling::StyleVariable normal_;
        Styling::StyleVariable hover_;
        Styling::StyleVariable pressed_;
    };

    QColor getButtonColor(bool _isPhoneSelected, bool _isHovered, bool _isPressed)
    {
        static const ButtonColors normalColors =
        {
            Styling::StyleVariable::PRIMARY,
            Styling::StyleVariable::PRIMARY_HOVER,
            Styling::StyleVariable::PRIMARY_ACTIVE
        };
        static const ButtonColors disabledColors =
        {
            Styling::StyleVariable::BASE_SECONDARY,
            Styling::StyleVariable::BASE_SECONDARY_HOVER,
            Styling::StyleVariable::BASE_SECONDARY_ACTIVE
        };

        if (!_isPhoneSelected)
            return Styling::getParameters().getColor((_isPressed ? disabledColors.pressed_ : ( _isHovered ? disabledColors.hover_ : disabledColors.normal_)));
        else
            return Styling::getParameters().getColor((_isPressed ? normalColors.pressed_ : (_isHovered ? normalColors.hover_ : normalColors.normal_)));

    }
}

TransitProfileSharing::TransitProfileSharing(QWidget* _parent, const QString& _aimId)
    : QWidget(_parent)
    , userProfile_(nullptr)
    , aimId_(_aimId)
    , declinable_(true)
{
    transitProfile_ = new TransitProfileSharingWidget(parentWidget(), _aimId);
    userProfile_ = new GeneralDialog(transitProfile_, Utils::InterConnector::instance().getMainWindow());
    userProfile_->addLabel(getCaption(TransitState::ON_PROFILE));
    btnPair_ = userProfile_->addButtonsPair(leftButtonText(), rightButtonText(TransitState::ON_PROFILE), false);
    userProfile_->installEventFilter(this);
    
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::userInfo, this, &TransitProfileSharing::onUserInfo);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, [this]()
    {
        declinable_ = false;
    });

    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void TransitProfileSharing::showProfile()
{
    Ui::GetDispatcher()->getUserInfo(aimId_);
    setState(TransitState::ON_PROFILE);
}

void Ui::TransitProfileSharing::keyPressEvent(QKeyEvent * _e)
{
    if (_e->key() == Qt::Key_Escape)
    {
        emit declined();
        close();
    }
}

bool Ui::TransitProfileSharing::eventFilter(QObject * _obj, QEvent * _event)
{
    if (_event->type() == QEvent::ShortcutOverride)
    {
        auto keyEvent = static_cast<QKeyEvent*>(_event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            keyEvent->accept();

            if (userProfile_ && userProfile_->isVisible())
                userProfile_->setFocus();
            else if (errorUnchecked_ && errorUnchecked_->isVisible())
                errorUnchecked_->setFocus();

            return true;
        }
    }
    else if (_event->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent*>(_event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            keyEvent->accept();

            if (userProfile_ && userProfile_->isVisible())
                userProfile_->setFocus();
            else if (errorUnchecked_ && errorUnchecked_->isVisible())
                errorUnchecked_->setFocus();

            return true;
        }
    }

    return false;
}

void TransitProfileSharing::setState(const TransitState _state)
{
    if (_state == TransitState::ON_PROFILE)
    {
        userProfile_->setFocus();

        if (userProfile_->showInCenter())
        {
            if (state_ == TransitState::EMPTY_PROFILE)
            {
                userProfile_->hide();
                emit openChat();
                close();
            }
            else
            {
                const auto sharePhone = transitProfile_->sharePhone();
                if (!nick_.isEmpty() || sharePhone)
                {
                    userProfile_->close();
                    emit accepted(nick_, sharePhone);
                    close();
                }
                else
                {
                    userProfile_->hide();
                    setState(TransitState::UNCHECKED);
                }
            }
        }
        else
        {
            userProfile_->hide();
            if (declinable_)
                emit declined();
            close();
        }
    }
    else if (_state == TransitState::UNCHECKED)
    {
        errorUnchecked_ = new GeneralDialog(new TransitProfileSharingWidget(parentWidget(), TransitState::UNCHECKED),
            Utils::InterConnector::instance().getMainWindow());
        errorUnchecked_->addLabel(getCaption(TransitState::UNCHECKED));
        errorUnchecked_->addButtonsPair(leftButtonText(), rightButtonText(TransitState::UNCHECKED), true);
        errorUnchecked_->installEventFilter(this);
        
        errorUnchecked_->setFocus();
        if (errorUnchecked_->showInCenter())
        {
            errorUnchecked_->hide();
            emit openChat();
            close();
        }
        else
        {
            errorUnchecked_->hide();
            if (!userProfile_)
            {
                userProfile_ = new GeneralDialog(transitProfile_, Utils::InterConnector::instance().getMainWindow());
                userProfile_->addLabel(getCaption(TransitState::ON_PROFILE));
                btnPair_ = userProfile_->addButtonsPair(leftButtonText(), rightButtonText(TransitState::ON_PROFILE), true);
                userProfile_->installEventFilter(this);
            }
            setState(TransitState::ON_PROFILE);
        }
    }
}
void TransitProfileSharing::onUserInfo(const int64_t, const QString& _aimId, const Data::UserInfo& _info)
{
    state_ = TransitState::ON_PROFILE;
    nick_ = _info.nick_;
    phone_ = _info.phone_;

    Utils::UrlParser p;
    p.process(_aimId);
    if (nick_.isEmpty() && p.hasUrl() && p.getUrl().is_email())
        nick_ = _aimId;

    if (nick_.isEmpty() && phone_.isEmpty())
    {
        state_ = TransitState::EMPTY_PROFILE;
    }

    if (btnPair_.first)
    {
        btnPair_.first->setText(rightButtonText(state_));
        btnPair_.first->setEnabled(true);
    }
    transitProfile_->onUserInfo(_info);
}

TransitProfileSharingWidget::TransitProfileSharingWidget(QWidget* _parent, TransitState _error)
    : QWidget(_parent)
    , error_(_error)
{
    mainLayout_ = Utils::emptyVLayout(this);

    resizingHost_ = new QWidget(this);
    resizingLayout_ = Utils::emptyHLayout(resizingHost_);
    resizingSpacer_ = new QSpacerItem(0, avatarSize(), QSizePolicy::Fixed, QSizePolicy::Expanding);
    resizingLayout_->addSpacerItem(resizingSpacer_);
    mainLayout_->addWidget(resizingHost_);

    initError();
}

TransitProfileSharingWidget::TransitProfileSharingWidget(QWidget* _parent, const QString& _aimId)
    : QWidget(_parent)
    , aimId_(_aimId)
    , avatar_(new ContactAvatarWidget(this, _aimId, Logic::GetFriendlyContainer()->getFriendly(_aimId), avatarSize(), true))
    , nicknameVisible_(false)
    , phoneExists_(false)
    , error_(TransitState::ON_PROFILE)
    , isPhoneSelected_(false)
    , btnHovered_(false)
    , btnPressed_(false)
{
    mainLayout_ = Utils::emptyVLayout(this);

    resizingHost_ = new QWidget(this);
    resizingLayout_ = Utils::emptyHLayout(resizingHost_);
    mainLayout_->addWidget(resizingHost_);
    resizingSpacer_ = new QSpacerItem(0, avatarSize(), QSizePolicy::Fixed, QSizePolicy::Expanding);
    resizingLayout_->addSpacerItem(resizingSpacer_);

    initShort();
}

bool TransitProfileSharingWidget::sharePhone() const
{
    return isPhoneSelected_;
}

void Ui::TransitProfileSharingWidget::onUserInfo(const Data::UserInfo & _info)
{
    info_ = _info;
    init();
}

void TransitProfileSharingWidget::paintEvent(QPaintEvent * _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (error_ == TransitState::UNCHECKED)
    {
        errorTextUnit_->getHeight(width() - 2 * textHorOffset());
        errorTextUnit_->setOffsets(padding(), padding());
        errorTextUnit_->draw(p, TextRendering::VerPosition::TOP);
    }
    else
    {
        const auto textX = padding() + avatar_->width() + textHorOffset();
        const auto maxTextWidth = width() - textX - textHorOffset();
        auto currentVOffset = avatarPadding();

        const auto friendlyHeight = friendlyTextUnit_->getHeight(maxTextWidth);
        if (nicknameVisible_)
        {
            currentVOffset += Utils::scale_value(6);
            friendlyTextUnit_->setOffsets(textX, currentVOffset);
            friendlyTextUnit_->draw(p, TextRendering::VerPosition::TOP);
            currentVOffset += friendlyHeight + Utils::scale_value(2);

            const auto nickHeight = nickTextUnit_->getHeight(maxTextWidth);
            nickTextUnit_->setOffsets(textX, currentVOffset);
            nickTextUnit_->draw(p, TextRendering::VerPosition::TOP);
        }
        else
        {
            friendlyTextUnit_->setOffsets(textX, currentVOffset + (avatarSize() - friendlyHeight) / 2);
            friendlyTextUnit_->draw(p, TextRendering::VerPosition::TOP);
        }
        currentVOffset = avatarPadding() + avatarSize() + verOffset();

        if (phoneExists_)
        {
            const auto phoneTextWidth = width() - textHorOffset() - 2 * padding() - buttonSize();
            auto captionHeight = phoneCaption_->getHeight(phoneTextWidth);
            phoneCaption_->setOffsets(padding(), currentVOffset);
            phoneCaption_->draw(p, TextRendering::VerPosition::TOP);
            currentVOffset += captionHeight + Utils::scale_value(4);

            phone_->getHeight(phoneTextWidth);
            phone_->setOffsets(padding(), currentVOffset);
            phone_->draw(p, TextRendering::VerPosition::TOP);

            btnRect_ = QRect(width() - buttonSize() - padding(), currentVOffset - textHorOffset(), buttonSize(), buttonSize());
            btnSharePhone_ = getButtonPixmap();
            p.drawPixmap(btnRect_, btnSharePhone_);
        }

        if (!nicknameVisible_ && !phoneExists_ && errorTextUnit_)
        {
            errorTextUnit_->getHeight(width() - avatarPadding() - avatarSize() - 2 * textHorOffset());
            errorTextUnit_->setOffsets(padding(), currentVOffset);
            errorTextUnit_->draw(p, TextRendering::VerPosition::TOP);
        }

        const QRect borderHor(rect().left(), rect().bottom(), rect().width(), Utils::scale_value(1));
        p.fillRect(borderHor, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
    }
}

void TransitProfileSharingWidget::mouseMoveEvent(QMouseEvent* _event)
{
    btnHovered_ = btnRect_.contains(_event->pos());
    setCursor( btnHovered_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
    QWidget::mouseMoveEvent(_event);
}

void TransitProfileSharingWidget::mousePressEvent(QMouseEvent* _event)
{
    if (btnRect_.contains(_event->pos()) && _event->button() == Qt::LeftButton)
        btnPressed_ = true;

    update();
    QWidget::mousePressEvent(_event);
}
void TransitProfileSharingWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (btnRect_.contains(_event->pos()))
    {
        btnPressed_ = false;
        isPhoneSelected_ = !isPhoneSelected_;
    }

    update();
    QWidget::mouseReleaseEvent(_event);
}

void TransitProfileSharingWidget::connectAvatarSlots()
{
    connect(avatar_, &ContactAvatarWidget::leftClicked, this, [this]()
    {
        if (!avatar_->isDefault())
            Utils::InterConnector::instance().getMainWindow()->openAvatar(aimId_);
    });

    connect(avatar_, &ContactAvatarWidget::rightClicked, this, [this]()
    {
        if (!avatar_->isDefault())
            Utils::InterConnector::instance().getMainWindow()->openAvatar(aimId_);
    });

    connect(avatar_, &ContactAvatarWidget::mouseEntered, this, [this]()
    {
        if (!avatar_->isDefault())
            setCursor(Qt::PointingHandCursor);
    });

    connect(avatar_, &ContactAvatarWidget::mouseLeft, this, [this]()
    {
        if (!avatar_->isDefault())
            setCursor(Qt::ArrowCursor);
    });

    connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::galeryClosed, this, [this]()
    {
        setFocus();
    });
}

void TransitProfileSharingWidget::init()
{
    auto nick = info_.nick_;
    Utils::UrlParser p;
    p.process(aimId_);
    if (nick.isEmpty() && p.hasUrl() && p.getUrl().is_email())
        nick = aimId_;

    if (nick.isEmpty() && info_.phone_.isEmpty())
        error_ = TransitState::EMPTY_PROFILE;

    if (error_ != TransitState::EMPTY_PROFILE)
    {
        if (!nick.isEmpty())
        {
            nicknameVisible_ = true;
            if (!nick.contains(ql1c('@')))
                nick.prepend(ql1c('@'));
            nickTextUnit_ = TextRendering::MakeTextUnit(nick, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            nickTextUnit_->init(secondaryTextFont(), secondaryTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }

        if (!info_.phone_.isEmpty())
        {
            phoneExists_ = true;
            isPhoneSelected_ = true;

            phoneCaption_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profilesharing", "Mobile phone"), {},
                TextRendering::LinksVisible::DONT_SHOW_LINKS,
                TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            phoneCaption_->init(secondaryTextFont(), secondaryTextColor(), QColor(), QColor(), QColor(),
                TextRendering::HorAligment::LEFT, 1);

            phone_ = TextRendering::MakeTextUnit(PhoneFormatter::formatted(info_.phone_), {},
                TextRendering::LinksVisible::DONT_SHOW_LINKS,
                TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            phone_->init(primaryTextFont(), primaryTextColor(), QColor(), QColor(), QColor(),
                TextRendering::HorAligment::LEFT, 1);
        }

        updateSpacer(height() + (phoneExists_ ? Utils::scale_value(164) - (avatarSize() + 2 * avatarPadding()) : 0));
    }
    else
    {
        const auto errorText = getErrorText(error_);

        errorTextUnit_ = TextRendering::MakeTextUnit(errorText, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        errorTextUnit_->init(errorTextFont(), primaryTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, -1, TextRendering::LineBreakType::PREFER_SPACES);
        if constexpr (platform::is_apple())
            errorTextUnit_->setLineSpacing(Utils::scale_value(7));
        const auto errorHeight = errorTextUnit_->getHeight(maxWidth() - 2 * textHorOffset());
        updateSpacer(height() + errorHeight + 2 * textHorOffset() + padding());
    }

    setCursor(Qt::ArrowCursor);
    update();
}

void Ui::TransitProfileSharingWidget::initShort()
{
    avatar_->setFixedSize(avatarSize(), avatarSize());
    avatar_->move(padding(), avatarPadding());

    friendlyTextUnit_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(aimId_), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    friendlyTextUnit_->init(primaryTextFont(Fonts::FontWeight::SemiBold), primaryTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

    updateSpacer(avatarSize() + 2 * avatarPadding());

    setCursor(Qt::ArrowCursor);
    connectAvatarSlots();
    setMouseTracking(true);
}

void TransitProfileSharingWidget::initError()
{
    const auto errorText = getErrorText(error_);

    errorTextUnit_ = TextRendering::MakeTextUnit(errorText, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    errorTextUnit_->init(errorTextFont(), primaryTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, -1, TextRendering::LineBreakType::PREFER_SPACES);
    if constexpr (platform::is_apple())
        errorTextUnit_->setLineSpacing(Utils::scale_value(7));
    const auto errorHeight = errorTextUnit_->getHeight(maxWidth() - 2 * textHorOffset());
    updateSpacer(errorHeight + 2 * textHorOffset());
    setMouseTracking(true);
}

void TransitProfileSharingWidget::updateSpacer(const int _height)
{
    resizingSpacer_->changeSize(0, _height, QSizePolicy::Fixed, QSizePolicy::Expanding);
    resizingHost_->layout()->invalidate();
}

QPixmap TransitProfileSharingWidget::getButtonPixmap()
{
    const QSize iconSize(buttonSize(), buttonSize());
    QPixmap res(Utils::scale_bitmap(iconSize));
    res.fill(Qt::transparent);
    Utils::check_pixel_ratio(res);

    QPainter p(&res);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    {
        const auto color = getButtonColor(isPhoneSelected_, btnHovered_, btnPressed_);
        const auto shift = isPhoneSelected_ ? 0 : Utils::scale_value(1);
        const auto radius = buttonSize() - 2 * shift;

        if (isPhoneSelected_)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
            const auto r = Utils::scale_bitmap_ratio();
            const auto whiteWidth = res.width() / std::sqrt(2.5) / r;
            p.drawEllipse((res.width() / r - whiteWidth) / 2, (res.height() / r - whiteWidth) / 2, whiteWidth, whiteWidth);
            p.drawPixmap(0, 0, Utils::renderSvg(qsl(":/apply_edit"), iconSize, color));
        }
        else
        {
            p.setPen(color);
            p.setBrush(Qt::transparent);
            p.drawEllipse(shift, shift, radius, radius);
        }
    }
    return res;
}

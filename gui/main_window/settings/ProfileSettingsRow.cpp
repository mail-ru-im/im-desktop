#include "stdafx.h"

#include "ProfileSettingsRow.h"

#include "../../controls/TextUnit.h"
#include "../../controls/SimpleListWidget.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/ContextMenu.h"

#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/features.h"

#include "../../previewer/toast.h"
#include "../../styles/ThemeParameters.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/sidebar/EditNicknameWidget.h"

#include "../../fonts.h"
#include "../../my_info.h"
#include "../../core_dispatcher.h"

#include "../contact_list/Common.h"
#include "../GroupChatOperations.h"


namespace
{
    int normalModeHeight()
    {
        return Utils::scale_value(84);
    }

    int compactModeHeight()
    {
        return Utils::scale_value(92);
    }

    int avatarHeight()
    {
        return Utils::scale_value(52);
    }

    int normalModeAvatarOffset()
    {
        return Utils::scale_value(12);
    }

    int textOffset()
    {
        return Utils::scale_value(12);
    }

    QSize arrowSize()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    QColor normalArrowColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    QColor activeArrowColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QPixmap createArrow(const QColor& _color)
    {
        return Utils::mirrorPixmapHor(Utils::renderSvg(qsl(":/controls/back_icon"), arrowSize(), _color));
    }

    int arrowRightOffset()
    {
        return Utils::scale_value(16);
    }

    QColor activeBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
    }

    QColor hoveredBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    QColor normalFriendlyTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor activeFriendlyTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor normalNickTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    QColor normalHoverNickTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER);
    }

    QColor normalPressedNickTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE);
    }

    QColor activeNickTextColor()
    {
        return activeFriendlyTextColor();
    }

    QColor activeHoverNickTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);
    }

    QColor activePressedNickTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
    }

    QFont friendlyTextFont()
    {
        const auto static f = Fonts::appFontScaled(16);
        return f;
    }

    QFont statusTextFont()
    {
        const auto static f = Fonts::appFontScaled(14);
        return f;
    }

    QString getProfileDomain()
    {
        return qsl("https://") % Features::getProfileDomain() % ql1c('/');
    }

    QString getProfileDomainAgent()
    {
        return qsl("https://") % Features::getProfileDomainAgent() % ql1c('/');
    }

    const auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }
}

namespace Ui
{
    ProfileSettingsRow::ProfileSettingsRow(QWidget* _parent)
        : SimpleListItem(_parent)
        , isSelected_(false)
        , isCompactMode_(false)
        , nickHovered_(false)
        , nickPressed_(false)
        , avatar_(new ContactAvatarWidget(this, MyInfo()->aimId(), MyInfo()->friendly(), avatarHeight(), true))
    {
        avatar_->setAttribute(Qt::WA_TransparentForMouseEvents);

        friendlyTextUnit_ = TextRendering::MakeTextUnit(MyInfo()->friendly(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        friendlyTextUnit_->init(friendlyTextFont(), normalFriendlyTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        nickTextUnit_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        nickTextUnit_->init(statusTextFont(), normalNickTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        if (isEmailProfile())
            nickname_ = MyInfo()->aimId();

        connect(MyInfo(), &my_info::received, this, &ProfileSettingsRow::onMyInfo);
        connect(GetDispatcher(), &core_dispatcher::userInfo, this, &ProfileSettingsRow::onUserInfo);

        updateArrowIcon();

        updateHeight();
        updateAvatarPos();

        updateNickVisibility();
    }

    ProfileSettingsRow::~ProfileSettingsRow()
    {
    }

    void ProfileSettingsRow::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;

            updateArrowIcon();
            updateTextColor();

            update();
        }
    }

    bool ProfileSettingsRow::isSelected() const
    {
        return isSelected_;
    }

    void ProfileSettingsRow::setCompactMode(bool _value)
    {
        if (isCompactMode_ != _value)
        {
            isCompactMode_ = _value;

            updateHeight();
            updateAvatarPos();

            update();
        }
    }

    bool ProfileSettingsRow::isCompactMode() const
    {
        return isCompactMode_;
    }

    void ProfileSettingsRow::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (isSelected())
            p.fillRect(rect(), activeBackgroundColor());
        else if (isHovered() && !nickHovered_)
            p.fillRect(rect(), hoveredBackgroundColor());

        if (!isCompactMode())
        {
            const auto arrowX = width() - arrowRightOffset() - Utils::unscale_bitmap(arrow_.width());
            const auto arrowY = (height() - Utils::unscale_bitmap(arrow_.height())) / 2;
            p.drawPixmap(arrowX, arrowY, arrow_);

            const auto textX = normalModeAvatarOffset() + avatar_->width() + textOffset();
            const auto maxTextWidth = arrowX - textX - textOffset();

            friendlyTextUnit_->getHeight(maxTextWidth);
            friendlyTextUnit_->setOffsets(textX, height() / 2);
            const auto friendlyVerPos = nicknameVisible_ ? TextRendering::VerPosition::BOTTOM : TextRendering::VerPosition::MIDDLE;
            friendlyTextUnit_->draw(p, friendlyVerPos);

            if (nicknameVisible_)
            {
                nickTextUnit_->getHeight(maxTextWidth);
                nickTextUnit_->setOffsets(textX, Utils::scale_value(44));
                nickTextUnit_->draw(p, TextRendering::VerPosition::TOP);
            }
        }
    }

    void ProfileSettingsRow::showEvent(QShowEvent* _event)
    {
        updateNickname();
        SimpleListItem::showEvent(_event);
    }

    void ProfileSettingsRow::mouseMoveEvent(QMouseEvent* _e)
    {
        if (const auto overNick = isOverNickname(_e->pos()); overNick != nickHovered_)
        {
            nickHovered_ = overNick;

            updateNicknameColor();
        }
    }

    void ProfileSettingsRow::mousePressEvent(QMouseEvent* _e)
    {
        _e->ignore();

        if (const auto overNick = isOverNickname(_e->pos()))
        {
            nickPressed_ = true;
            updateNicknameColor();

            _e->accept();
        }
    }

    void ProfileSettingsRow::mouseReleaseEvent(QMouseEvent* _e)
    {
        _e->ignore();

        if (nickPressed_)
        {
            nickPressed_ = false;
            updateNicknameColor();

            if (const auto overNick = isOverNickname(_e->pos()))
            {
                if (_e->button() == Qt::LeftButton)
                    onNicknameClicked();
                else if (_e->button() == Qt::RightButton && hasNickname())
                    onNicknameContextMenu(_e->globalPos());
            }

            _e->accept();
        }
    }

    void ProfileSettingsRow::onMyInfo()
    {
        avatar_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());

        friendlyTextUnit_->setText(MyInfo()->friendly());

        updateNickVisibility();

        if (isEmailProfile())
            nickname_ = MyInfo()->aimId();
        else
            nickname_ = MyInfo()->nick();

        updateNickname();
    }

    void ProfileSettingsRow::onUserInfo(const int64_t _seq, const QString& _aimid, Data::UserInfo _info)
    {
        if (_aimid != MyInfo()->aimId())
            return;

        avatar_->UpdateParams(_aimid, _info.friendly_);
        friendlyTextUnit_->setText(_info.friendly_);

        if (!isEmailProfile())
            nickname_ = _info.nick_;

        updateNickVisibility();
        updateNickname();
    }

    void ProfileSettingsRow::onNicknameClicked()
    {
        if (hasNickname())
        {
            copyNickToClipboard();
        }
        else
        {
            auto form = new EditNicknameWidget(this);
            form->setStatFrom("settings_scr");

            connect(form, &EditNicknameWidget::changed, this, [this, form]()
            {
                nickname_ = form->getFormData().nickName_;
                update();
            });

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_nickedit_action);

            showEditNicknameDialog(form);
        }
    }

    void ProfileSettingsRow::onNicknameContextMenu(const QPoint& _pos)
    {
        auto menu = new ContextMenu(this);

        if (isEmailProfile())
            menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy email"), qsl("copy_nick"));
        else
            menu->addActionWithIcon(qsl(":/context_menu/mention"), QT_TRANSLATE_NOOP("context_menu", "Copy nickname"), qsl("copy_nick"));

        menu->addActionWithIcon(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy the link to this profile"), qsl("copy_link"));
        menu->addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Share profile link"), qsl("share_link"));

        connect(menu, &ContextMenu::triggered, this, &ProfileSettingsRow::onContextMenuItemClicked, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        menu->popup(_pos);
    }

    void ProfileSettingsRow::onContextMenuItemClicked(QAction* _action)
    {
        assert(hasNickname());
        const auto command = _action->data().toString();

        const QString link = (isEmailProfile() ? getProfileDomainAgent() : getProfileDomain()) % getNickname();

        if (command == qsl("copy_link"))
        {
            QApplication::clipboard()->setText(link);
            Utils::showToastOverMainWindow(link % QChar::LineFeed % QT_TRANSLATE_NOOP("toast", "Link copied"), getToastVerOffset());
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_nick_action, { {"do", "copy_url"} });
        }
        else if (command == qsl("share_link"))
        {
            forwardMessage(link, QString(), QString(), false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_nick_action, { {"do", "in_share"} });
        }
        else if (command == qsl("copy_nick"))
        {
            copyNickToClipboard();
        }
    }

    void ProfileSettingsRow::updateArrowIcon()
    {
        arrow_ = createArrow(isSelected() ? activeArrowColor() : normalArrowColor());
    }

    void ProfileSettingsRow::updateTextColor()
    {
        friendlyTextUnit_->setColor(isSelected() ? activeFriendlyTextColor() : normalFriendlyTextColor());
        updateNicknameColor();
    }

    void ProfileSettingsRow::updateHeight()
    {
        setFixedHeight(isCompactMode() ? compactModeHeight() : normalModeHeight());
    }

    void ProfileSettingsRow::updateNickname()
    {
        if (!nicknameVisible_)
            return;

        if (isEmailProfile())
            nickTextUnit_->setText(getNickname());
        else
            nickTextUnit_->setText(hasNickname() ? Utils::makeNick(getNickname()) : QT_TRANSLATE_NOOP("settings", "Add nickname"));

        update();
    }

    void ProfileSettingsRow::updateNicknameColor()
    {
        QColor color;
        if (nickPressed_)
            color = isSelected() ? activePressedNickTextColor() : normalPressedNickTextColor();
        else if (nickHovered_)
            color = isSelected() ? activeHoverNickTextColor() : normalHoverNickTextColor();
        else
            color = isSelected() ? activeNickTextColor() : normalNickTextColor();

        nickTextUnit_->setColor(color);
        update();
    }

    void ProfileSettingsRow::updateAvatarPos()
    {
        const auto x = isCompactMode() ? (width() - avatarHeight()) / 2 : normalModeAvatarOffset();
        const auto y = (height() - avatarHeight()) / 2;

        avatar_->move(x, y);
    }

    void ProfileSettingsRow::copyNickToClipboard()
    {
        QString target;
        QString copied;
        if (isEmailProfile())
        {
            target = getNickname();
            copied = QT_TRANSLATE_NOOP("toast", "Email copied");
        }
        else
        {
            target = Utils::makeNick(getNickname());
            copied = QT_TRANSLATE_NOOP("toast", "Nickname copied");
        }

        QApplication::clipboard()->setText(target);
        Utils::showToastOverMainWindow(target % QChar::LineFeed % copied, getToastVerOffset());

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_nick_action, { {"do", "copy_nick"} });
    }

    bool ProfileSettingsRow::isOverNickname(const QPoint& _pos) const
    {
        if (!nicknameVisible_)
            return false;

        const QRect nickRect(
            nickTextUnit_->horOffset(),
            nickTextUnit_->verOffset(),
            nickTextUnit_->getLastLineWidth(),
            nickTextUnit_->cachedSize().height()
        );

        return nickRect.contains(_pos);
    }

    bool ProfileSettingsRow::hasNickname() const
    {
        return !getNickname().isEmpty();
    }

    QString ProfileSettingsRow::getNickname() const
    {
        return nickname_;
    }

    bool ProfileSettingsRow::isEmailProfile() const
    {
        return MyInfo()->aimId().contains(ql1c('@'));
    }

    void ProfileSettingsRow::updateNickVisibility()
    {
        nicknameVisible_ = isEmailProfile() || (Features::isNicksEnabled() && Utils::isUin(Ui::MyInfo()->aimId()));
        update();
    }
}

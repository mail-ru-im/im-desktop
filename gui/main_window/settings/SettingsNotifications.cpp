#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "core_dispatcher.h"
#include "gui_settings.h"
#include "controls/GeneralCreator.h"
#include "controls/TextEmojiWidget.h"
#include "controls/TransparentScrollBar.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "my_info.h"
#include "styles/ThemeParameters.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "utils/InterConnector.h"
#include "previewer/toast.h"

using namespace Ui;

namespace
{
    void showSecurityToast()
    {
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("settings", "This setting can't be changed due to security reasons"), Utils::scale_value(20));
    };

    int getItemSpacing()
    {
        return Utils::scale_value(12);
    }

    constexpr double getVolumeStep() noexcept { return 0.25; }
}

static GeneralCreator::ProgresserDescriptor createVolumeSlider(QVBoxLayout* _mainLayout, ScrollAreaWithTrScrollBar* _scrollArea)
{
    auto on_change_volume = [](const bool _finish, TextEmojiWidget* _w, TextEmojiWidget* _aw, int _i)
    {
        double volume = _i * getVolumeStep();
        if (fabs(get_gui_settings()->get_value<double>(settings_notification_volume, settings_notification_volume_default()) - volume) >= getVolumeStep())
            get_gui_settings()->set_value<double>(settings_notification_volume, volume);

        _w->setText(ql1s("%1 %2%").arg(QT_TRANSLATE_NOOP("settings", "Notification volume:")).arg(_i * getVolumeStep() * 100), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
    };

    constexpr int markCount = static_cast<int>(1.0 / getVolumeStep());
    const double currentVolume = get_gui_settings()->get_value<double>(settings_notification_volume, settings_notification_volume_default());
    const int index = std::clamp(static_cast<int>(currentVolume / getVolumeStep()), 0, markCount);

    return GeneralCreator::addProgresser(
        _scrollArea,
        _mainLayout,
        markCount,
        index,
        [on_change_volume](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_volume(true, w, aw, i); },
        [on_change_volume](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_volume(false, w, aw, i); }
    );
}

void GeneralSettingsWidget::Creator::initNotifications(NotificationSettings* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS NotificationsPage scrollArea"));
    layout->addWidget(scrollArea);

    {
        _parent->sounds_ = GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Play sounds"),
                get_gui_settings()->get_value<bool>(settings_sounds_enabled, true),
                [](bool enabled)
                {
#ifndef STRIP_VOIP
                    GetDispatcher()->getVoipController().setMuteSounds(!enabled);
#endif
                    if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != enabled)
                        get_gui_settings()->set_value<bool>(settings_sounds_enabled, enabled);
                }, -1, qsl("AS NotificationsPage playSoundSetting"));

        mainLayout->addSpacing(getItemSpacing());
        auto notificationVolume = createVolumeSlider(mainLayout, scrollArea);
        if (!get_gui_settings()->get_value(settings_sounds_enabled, true))
        {
            notificationVolume.setEnabled(false);
        }
        connect(_parent->sounds_, &Ui::SidebarCheckboxButton::checked, scrollArea, [_parent, notificationVolume]() mutable
            {
                notificationVolume.setEnabled(_parent->sounds_->isChecked());
            });
        mainLayout->addSpacing(getItemSpacing());

        auto outgoingSoundWidgets = GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Outgoing messages sound"),
                get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, settings_outgoing_message_sound_enabled_default()),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, settings_outgoing_message_sound_enabled_default()) != enabled)
                        get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, enabled);
                }, -1, qsl("AS NotificationsPage outgoingSoundSetting"));
        if (!get_gui_settings()->get_value<bool>(settings_sounds_enabled, true))
        {
            outgoingSoundWidgets->setChecked(false);
            outgoingSoundWidgets->setEnabled(false);
        }

        connect(_parent->sounds_, &Ui::SidebarCheckboxButton::checked, scrollArea, [_parent, outgoingSoundWidgets]()
        {
            if (const bool c = _parent->sounds_->isChecked(); !c)
            {
                outgoingSoundWidgets->setChecked(false);
                outgoingSoundWidgets->setEnabled(false);
                if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, settings_outgoing_message_sound_enabled_default()) != c)
                    get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, c);
            }
            else
            {
                outgoingSoundWidgets->setEnabled(true);
            }
        });

        _parent->notifications_ = GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Show notifications"),
                get_gui_settings()->get_value<bool>(settings_notify_new_messages, true),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value<bool>(settings_notify_new_messages, true) != enabled)
                        get_gui_settings()->set_value<bool>(settings_notify_new_messages, enabled);
                }, -1, qsl("AS NotificationsPage showNotificationSetting"));

        auto showNoticationsWithActiveUI = GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Show notifications when app in focus"),
            get_gui_settings()->get_value<bool>(settings_notify_new_messages_with_active_ui, settings_notify_new_messages_with_active_ui_default()),
            [](bool enabled)
            {
                if (get_gui_settings()->get_value(settings_notify_new_messages_with_active_ui, settings_notify_new_messages_with_active_ui_default()) != enabled)
                    get_gui_settings()->set_value(settings_notify_new_messages_with_active_ui, enabled);
            }, -1, qsl("AS NotificationsPage showNoticationsWithActiveUiSetting"));
        if (!get_gui_settings()->get_value<bool>(settings_notify_new_messages, true))
        {
            showNoticationsWithActiveUI->setChecked(false);
            showNoticationsWithActiveUI->setEnabled(false);
        }

        connect(_parent->notifications_, &Ui::SidebarCheckboxButton::checked, scrollArea, [_parent, showNoticationsWithActiveUI]()
        {
            if (const bool c = _parent->notifications_->isChecked(); !c)
            {
                showNoticationsWithActiveUI->setChecked(false);
                showNoticationsWithActiveUI->setEnabled(false);
                if (get_gui_settings()->get_value(settings_notify_new_messages_with_active_ui, settings_notify_new_messages_with_active_ui_default()) != c)
                    get_gui_settings()->set_value(settings_notify_new_messages_with_active_ui, c);
            }
            else
            {
                showNoticationsWithActiveUI->setEnabled(true);
            }
        });

        if (Ui::MyInfo()->haveConnectedEmail())
        {
            GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Show mail notifications"),
                    get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true),
                    [](bool enabled)
                    {
                        if (get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true) != enabled)
                            get_gui_settings()->set_value<bool>(settings_notify_new_mail_messages, enabled);
                    }, -1, qsl("AS NotificationsPage showMailNotificationSettings"));
        }


        auto onMessageHideChange = [](bool enabled)
        {
            if (get_gui_settings()->get_value<bool>(settings_hide_message_notification, false) != enabled)
                get_gui_settings()->set_value<bool>(settings_hide_message_notification, enabled);
        };

        auto messageHideSwitch = GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Hide message text in notifications"),
            get_gui_settings()->get_value<bool>(settings_hide_message_notification, false),
            onMessageHideChange, -1, qsl("AS NotificationsPage hideTextInNotificationSetting"));
        connect(messageHideSwitch, &SidebarCheckboxButton::checked, onMessageHideChange);

        auto onSenderHideChange = [](bool enabled)
        {
            if (get_gui_settings()->get_value<bool>(settings_hide_sender_notification, false) != enabled)
                get_gui_settings()->set_value<bool>(settings_hide_sender_notification, enabled);
        };

        auto senderHideSwitch = GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Hide sender name in notifications"),
            get_gui_settings()->get_value<bool>(settings_hide_sender_notification, false),
            onSenderHideChange, -1, qsl("AS NotificationsPage hideSenderInNotificationSetting"));
        connect(senderHideSwitch, &SidebarCheckboxButton::checked, onSenderHideChange);

        connect(messageHideSwitch, &SidebarCheckboxButton::disabledClicked, []() { if (Features::hideMessageTextEnabled()) showSecurityToast(); });
        connect(senderHideSwitch, &SidebarCheckboxButton::disabledClicked, []() { if (Features::hideMessageInfoEnabled()) showSecurityToast(); });

        auto updateNotificationSettings = [switcherMsg = QPointer(messageHideSwitch), switcherSender = QPointer(senderHideSwitch)]()
        {
            const bool forceHideInfo = Features::hideMessageInfoEnabled();
            const bool forceHideText = forceHideInfo || Features::hideMessageTextEnabled();

            if (forceHideText)
                get_gui_settings()->set_value<bool>(settings_hide_message_notification, true);
            if (forceHideInfo)
                get_gui_settings()->set_value<bool>(settings_hide_sender_notification, true);

            const bool textHidden = forceHideText || get_gui_settings()->get_value<bool>(settings_hide_message_notification, false);
            const bool infoHidden = forceHideInfo || get_gui_settings()->get_value<bool>(settings_hide_sender_notification, false);

            if (switcherMsg)
            {
                switcherMsg->setChecked(textHidden);
                switcherMsg->setEnabled(!forceHideText);
            }

            if (switcherSender)
            {
                switcherSender->setChecked(infoHidden);
                switcherSender->setEnabled(!(forceHideInfo || !textHidden));
            }

            auto onMsgHideChanged = [switcherSender](bool _on)
            {
                if (!switcherSender)
                    return;
                if (!_on)
                    switcherSender->setChecked(false);
                switcherSender->setEnabled(_on);
            };

            if (switcherMsg)
                connect(switcherMsg, &SidebarCheckboxButton::checked, onMsgHideChanged);
        };

        connect(GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, updateNotificationSettings);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, updateNotificationSettings);
        updateNotificationSettings();

        if constexpr (!platform::is_linux())
        {
            GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Animate taskbar icon"),
                get_gui_settings()->get_value<bool>(settings_alert_tray_icon, false),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value<bool>(settings_alert_tray_icon, false) != enabled)
                        get_gui_settings()->set_value<bool>(settings_alert_tray_icon, enabled);
                }, -1, qsl("AS NotificationsPage animateTaskbarIconSettings"));
        }

        GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Show unreads counter in window title"),
            get_gui_settings()->get_value<bool>(settings_show_unreads_in_title, false),
            [](bool enabled)
            {
                if (get_gui_settings()->get_value<bool>(settings_show_unreads_in_title, false) != enabled)
                    get_gui_settings()->set_value<bool>(settings_show_unreads_in_title, enabled);
            }, -1, qsl("AS NotificationsPage showUnreadsCountItTitleSettings"));
    }
}


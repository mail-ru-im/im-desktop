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

using namespace Ui;

void GeneralSettingsWidget::Creator::initNotifications(NotificationSettings* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
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
        auto outgoingSoundWidgets = GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Outgoing messages sound"),
                get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, false),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != enabled)
                        get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, enabled);
                }, -1, qsl("AS NotificationsPage outgoingSoundSetting"));
        if (!get_gui_settings()->get_value(settings_sounds_enabled, true))
        {
            outgoingSoundWidgets->setChecked(false);
            outgoingSoundWidgets->setEnabled(false);
        }
        connect(_parent->sounds_, &Ui::SidebarCheckboxButton::checked, scrollArea, [_parent, outgoingSoundWidgets]()
        {
            bool c = _parent->sounds_->isChecked();
            if (!c)
            {
                outgoingSoundWidgets->setChecked(false);
                outgoingSoundWidgets->setEnabled(false);
                if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != c)
                    get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, c);
            }
            else
            {
                outgoingSoundWidgets->setEnabled(true);
                outgoingSoundWidgets->setEnabled(true);
            }
        });
        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Show notifications"),
                get_gui_settings()->get_value<bool>(settings_notify_new_messages, true),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value<bool>(settings_notify_new_messages, true) != enabled)
                        get_gui_settings()->set_value<bool>(settings_notify_new_messages, enabled);
                }, -1, qsl("AS NotificationsPage showNotificationSetting"));

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

        if (Features::showNotificationsTextSettings())
        {
            GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Hide message text in notifications"),
                get_gui_settings()->get_value<bool>(settings_hide_message_notification, false),
                [](bool enabled)
                {
                    if (get_gui_settings()->get_value<bool>(settings_hide_message_notification, false) != enabled)
                        get_gui_settings()->set_value<bool>(settings_hide_message_notification, enabled);
                }, -1, qsl("AS NotificationsPage hideTextInNotificationSetting"));
        }

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


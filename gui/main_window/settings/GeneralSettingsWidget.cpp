#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "controls/TextEmojiWidget.h"
#include "controls/FlatMenu.h"
#include "core_dispatcher.h"
#include "gui_settings.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"
#include "main_window/smiles_menu/Store.h"
#include "ContactUs.h"
#include "SettingsForTesters.h"
#include "SessionsPage.h"
#include "SettingsTab.h"
#ifdef HAS_WEB_ENGINE
#include "../WebAppPage.h"
#endif

#include "../common.shared/config/config.h"
#include "styles/ThemesContainer.h"
#include "controls/RadioTextRow.h"


namespace Ui
{
    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* _parent)
        : QStackedWidget(_parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));
    }

    GeneralSettingsWidget::~GeneralSettingsWidget() = default;

    void GeneralSettingsWidget::setType(int _type)
    {
        Utils::CommonSettingsType type = (Utils::CommonSettingsType)_type;

        if (type == Utils::CommonSettingsType::CommonSettingsType_General)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
            if (!general_)
            {
                general_ = new GeneralSettings(this);
                general_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initGeneral(general_);
                addWidget(general_);

                Testing::setAccessibleName(general_, qsl("AS GeneralPage"));

                general_->recvUserProxy();
                QObject::connect(Ui::GetDispatcher(), &core_dispatcher::getUserProxy, general_, &GeneralSettings::recvUserProxy);
                QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::downloadPathUpdated, general_, &GeneralSettings::downloadPathUpdated);
            }

            setCurrentWidget(general_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo)
        {
#ifndef STRIP_VOIP
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Voice and video"));
            initVoiceAndVideo();
            setCurrentWidget(voiceAndVideo_.rootWidget);
#endif
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Notifications)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Notifications"));
            if (!notifications_)
            {
                notifications_ = new NotificationSettings(this);
                notifications_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initNotifications(notifications_);
                addWidget(notifications_);

                Testing::setAccessibleName(notifications_, qsl("AS NotificationsPage"));
            }

            setCurrentWidget(notifications_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Appearance)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Appearance"));
            if (!appearance_)
            {
                appearance_ = new AppearanceWidget(this);
                appearance_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initAppearance(appearance_);
                addWidget(appearance_);

                Testing::setAccessibleName(appearance_, qsl("AS AppearancePage"));
            }

            setCurrentWidget(appearance_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_About)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "About app"));
            if (!about_)
            {
                about_ = new QWidget(this);
                about_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initAbout(about_);
                addWidget(about_);

                Testing::setAccessibleName(about_, qsl("AS AboutUsPage"));
            }

            setCurrentWidget(about_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_ContactUs)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(SettingsTab::getContactUsText());
            if (!contactus_)
            {
                contactus_ = new ContactUsWidget(this);
                contactus_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                addWidget(contactus_);

                Testing::setAccessibleName(contactus_, qsl("AS ContactUsPage"));
            }

            setCurrentWidget(contactus_);
            Q_EMIT Utils::InterConnector::instance().generalSettingsContactUsShown();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Language)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Language"));
            if (!language_)
            {
                language_ = new QWidget(this);
                language_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initLanguage(language_);
                addWidget(language_);

                Testing::setAccessibleName(language_, qsl("AS LanguagePage"));
            }

            setCurrentWidget(language_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Shortcuts)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Shortcuts"));
            if (!shortcuts_)
            {
                shortcuts_ = new ShortcutsSettings(this);
                shortcuts_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initShortcuts(shortcuts_);
                addWidget(shortcuts_);

                Testing::setAccessibleName(shortcuts_, qsl("AS ShortcutsPage"));
            }

            setCurrentWidget(shortcuts_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Security)
        {
            QString headerName;
            if (!security_)
            {
                security_ = new QStackedWidget(this);
                security_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                Creator::initSecurity(security_);
                addWidget(security_);

                Testing::setAccessibleName(security_, qsl("AS PrivacyPage"));
            }

            switch (security_->currentIndex())
            {
            case 1:
                headerName = QT_TRANSLATE_NOOP("main_page", "Delete Account");
                break;
            case 2:
                headerName = QT_TRANSLATE_NOOP("settings", "Session List");
                break;
            default:
                headerName = QT_TRANSLATE_NOOP("main_page", "Privacy");
                break;
            }

            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(headerName);
            setCurrentWidget(security_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Stickers)
        {
            if (!stickersStore_)
            {
                stickersStore_ = new Stickers::Store(this);
                stickersStore_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                addWidget(stickersStore_);

                connect(stickersStore_, &Stickers::Store::back, this, []()
                {
                    Q_EMIT Utils::InterConnector::instance().myProfileBack();
                });
                Testing::setAccessibleName(stickersStore_, qsl("AS StickersPage"));
            }

            setCurrentWidget(stickersStore_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Debug)
        {
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("settings", "Advanced Settings"));
            if (!debugSettings_)
            {
                debugSettings_ = new SettingsForTesters(this);
                debugSettings_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                addWidget(debugSettings_);

                Testing::setAccessibleName(debugSettings_, qsl("AS AdditionalSettingsPage"));
            }

            setCurrentWidget(debugSettings_);
        }
    }

    bool GeneralSettingsWidget::isSessionsShown() const
    {
        return security_ && currentWidget() == security_ && qobject_cast<SessionsPage*>(security_->currentWidget());
    }


    bool GeneralSettingsWidget::isWebViewShown() const
    {
#if HAS_WEB_ENGINE
        return security_ && currentWidget() == security_ && qobject_cast<WebAppPage*>(security_->currentWidget());
#else
        return false;
#endif
    }

    void GeneralSettingsWidget::navigateBack()
    {
        if (isSessionsShown() || isWebViewShown())
        {
            setCurrentWidget(security_);
            security_->setCurrentIndex(0);
        }
    }

#ifndef STRIP_VOIP
    void GeneralSettingsWidget::onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices)
    {
        std::vector<voip_proxy::device_desc> devices;
        devices = _devices;
        // Remove non camera devices.
        devices.erase(std::remove_if(devices.begin(), devices.end(), [](const voip_proxy::device_desc& desc)
        {
            return (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type != voip_proxy::kvoipDeviceCamera);
        }), devices.end());

        QMenu* menu = nullptr;
        std::vector<DeviceInfo>* deviceList = nullptr;
        TextEmojiWidget* currentSelected = nullptr;
        bool addDefaultDevice = false;

        switch (deviceType)
        {
        case voip_proxy::kvoipDevTypeAudioCapture:
            menu = voiceAndVideo_.audioCaptureDevices;
            deviceList = &voiceAndVideo_.aCapDeviceList;
            currentSelected = voiceAndVideo_.aCapSelected;
            addDefaultDevice = true;
            break;
        case voip_proxy::kvoipDevTypeAudioPlayback:
            menu = voiceAndVideo_.audioPlaybackDevices;
            deviceList = &voiceAndVideo_.aPlaDeviceList;
            currentSelected = voiceAndVideo_.aPlaSelected;
            addDefaultDevice = true;
            break;
        case voip_proxy::kvoipDevTypeVideoCapture:
            menu = voiceAndVideo_.videoCaptureDevices;
            deviceList = &voiceAndVideo_.vCapDeviceList;
            currentSelected = voiceAndVideo_.vCapSelected;
            break;
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            im_assert(false);
            return;
        }

        if (!menu || !deviceList)
            return;

        deviceList->clear();
        menu->clear();
        if (currentSelected)
            currentSelected->setText(QString());

#ifdef _WIN32
        voip_proxy::device_desc defaultDeviceDescription;
        if (addDefaultDevice && !devices.empty())
        {
            DeviceInfo di;
            di.name = QT_TRANSLATE_NOOP("settings", "By default").toStdString() + " (" + devices[0].name + ')';
            di.uid  = DEFAULT_DEVICE_UID;

            defaultDeviceDescription.name = di.name;
            defaultDeviceDescription.uid  = di.uid;
            defaultDeviceDescription.dev_type = deviceType;

            deviceList->push_back(di);
        }
#endif

        using namespace voip_proxy;

        const device_desc* selectedDesc = nullptr;
        for (const device_desc& desc : devices)
        {
            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            deviceList->push_back(di);

            if (user_selected_device_.count(deviceType) > 0 && user_selected_device_[deviceType] == di.uid)
                selectedDesc = &desc;
        }

#ifdef _WIN32
        // For default device select
        if (addDefaultDevice && !devices.empty())
        {
            if (!selectedDesc || (user_selected_device_.count(deviceType) > 0 && user_selected_device_[deviceType] == DEFAULT_DEVICE_UID))
                selectedDesc = &defaultDeviceDescription;
        }
#endif

        // Fill menu
        for (const DeviceInfo& device : *deviceList)
        {
            const auto text = QString::fromStdString(device.name);
            auto a = menu->addAction(text);
            a->setProperty(FlatMenu::sourceTextPropName(), text);
        }

        if (currentSelected)
        {
            // User selected item has most priority, then first element in list.
            const device_desc* desc = selectedDesc ? selectedDesc : (!devices.empty() ? &devices[0] : nullptr);
            if (desc)
            {
#ifdef _WIN32
                currentSelected->setText(QString::fromStdString((addDefaultDevice && desc == &defaultDeviceDescription) ? defaultDeviceDescription.name : desc->name));
#else
                currentSelected->setText(QString::fromStdString(desc->name));
#endif
            }
        }
    }

    void GeneralSettingsWidget::setActiveDevice(const voip_proxy::device_desc& _description)
    {
        applyDefaultDeviceLogic(_description);
        Ui::GetDispatcher()->getVoipController().setActiveDevice(_description);

        QString settingsName;
        switch (_description.dev_type) {
        case voip_proxy::kvoipDevTypeAudioPlayback: settingsName = ql1s(settings_speakers);   break;
        case voip_proxy::kvoipDevTypeAudioCapture:  settingsName = ql1s(settings_microphone); break;
        case voip_proxy::kvoipDevTypeVideoCapture:  settingsName = ql1s(settings_webcam);     break;
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            im_assert(!"unexpected device type");
            return;
        };

        user_selected_device_[_description.dev_type] = _description.uid;

        const auto uid = QString::fromStdString(_description.uid);
        if (get_gui_settings()->get_value<QString>(settingsName, QString()) != uid)
            get_gui_settings()->set_value<QString>(settingsName, uid);
    }

    void GeneralSettingsWidget::initVoiceAndVideo()
    {
        if (voiceAndVideo_.rootWidget)
            return;

        voiceAndVideo_.rootWidget = new QWidget(this);
        voiceAndVideo_.rootWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Creator::initVoiceVideo(voiceAndVideo_.rootWidget, voiceAndVideo_);

        Ui::GetDispatcher()->getVoipController().setRequestSettings();
        Ui::GetDispatcher()->getVoipController().loadSettings([this](voip_proxy::device_desc& desc){
            user_selected_device_[desc.dev_type] = desc.uid;
        });
        // Fill device list.
        for (int i = voip_proxy::kvoipDevTypeAudioCapture; i <= voip_proxy::kvoipDevTypeVideoCapture; i += 1)
        {
            voip_proxy::EvoipDevTypes type = (voip_proxy::EvoipDevTypes)i;
            onVoipDeviceListUpdated(type, Ui::GetDispatcher()->getVoipController().deviceList(type));
        }

        addWidget(voiceAndVideo_.rootWidget);

        QObject::connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipDeviceListUpdated, this, &GeneralSettingsWidget::onVoipDeviceListUpdated);
    }

    bool GeneralSettingsWidget::getDefaultDeviceFlag(const voip_proxy::EvoipDevTypes& type)
    {
#ifdef _WIN32
        QString defaultFlagSettingName;
        switch (type)
        {
        case voip_proxy::kvoipDevTypeAudioPlayback: defaultFlagSettingName = ql1s(settings_speakers_is_default);   break;
        case voip_proxy::kvoipDevTypeAudioCapture:  defaultFlagSettingName = ql1s(settings_microphone_is_default); break;
        default:
            return false;
        };
        return !defaultFlagSettingName.isEmpty() ? get_gui_settings()->get_value<bool>(defaultFlagSettingName, false) : false;
#else
        return false;
#endif
    }

    void GeneralSettingsWidget::applyDefaultDeviceLogic(const voip_proxy::device_desc& _description)
    {
#ifdef _WIN32
        QString defaultFlagSettingName;
        switch (_description.dev_type)
        {
        case voip_proxy::kvoipDevTypeAudioPlayback:
            defaultFlagSettingName = ql1s(settings_speakers_is_default);
            break;
        case voip_proxy::kvoipDevTypeAudioCapture:
            defaultFlagSettingName = ql1s(settings_microphone_is_default);
            break;
        default:
            return;
        }
        get_gui_settings()->set_value<bool>(defaultFlagSettingName, _description.uid == DEFAULT_DEVICE_UID);
#endif
    }
#endif

    GeneralSettings::GeneralSettings(QWidget* _parent)
        : QWidget(_parent)
        , connectionTypeChooser_(nullptr)
        , downloadPathChooser_(nullptr)
        , launchMinimized_(nullptr)
        , smartreplySwitcher_(nullptr)
        , spellCheckerSwitcher_(nullptr)
        , suggestsEmojiSwitcher_(nullptr)
        , suggestsWordsSwitcher_(nullptr)
        , statusSwitcher_(nullptr)
        , previewLinks_(nullptr)
        , reactionsSwitcher_(nullptr)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &GeneralSettings::onOmicronUpdate);
    }

    void GeneralSettings::recvUserProxy()
    {
        if (!connectionTypeChooser_)
            return;

        auto userProxy = Utils::get_proxy_settings();
        connectionTypeChooser_->setText(Utils::ProxySettings::proxyTypeStr(userProxy->type_));
    }

    void GeneralSettings::downloadPathUpdated()
    {
        if (downloadPathChooser_)
            downloadPathChooser_->setText(Ui::getDownloadPath());
    }

    void GeneralSettings::onOmicronUpdate()
    {
        if (smartreplySwitcher_)
            smartreplySwitcher_->setVisible(Features::isSmartreplyEnabled());
        if (spellCheckerSwitcher_)
            spellCheckerSwitcher_->setVisible(Features::isSpellCheckEnabled());
        if (suggestsEmojiSwitcher_)
            suggestsEmojiSwitcher_->setVisible(Features::isSuggestsEnabled());
        if (suggestsWordsSwitcher_)
            suggestsWordsSwitcher_->setVisible(Features::isSuggestsEnabled());
        if (statusSwitcher_)
            statusSwitcher_->setVisible(Features::isStatusEnabled());
        if (reactionsSwitcher_)
            reactionsSwitcher_->setVisible(Features::reactionsEnabled());
    }

    void GeneralSettings::showEvent(QShowEvent*)
    {
        onOmicronUpdate();
        if (previewLinks_)
            previewLinks_->setVisible(config::get().is_on(config::features::snippet_in_chat));
    }

    NotificationSettings::NotificationSettings(QWidget* _parent)
        : QWidget(_parent)
        , sounds_(nullptr)
        , notifications_(nullptr)
    {
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &NotificationSettings::value_changed);
    }

    void NotificationSettings::value_changed(const QString& name)
    {
        if (sounds_ && name == ql1s(settings_sounds_enabled))
            sounds_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
        if (notifications_ && name == ql1s(settings_notify_new_messages))
            notifications_->setChecked(get_gui_settings()->get_value<bool>(settings_notify_new_messages, true));
    }

    ShortcutsSettings::ShortcutsSettings(QWidget* _parent)
            : QWidget(_parent)
            , statuses_(nullptr)
    {
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &ShortcutsSettings::onSettingsChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &ShortcutsSettings::onOmicronUpdate);
    }

    void ShortcutsSettings::onSettingsChanged(const QString& _name)
    {
        if (statuses_ && _name == ql1s(settings_allow_statuses))
            statuses_->setVisible(Statuses::isStatusEnabled());
    }
    void ShortcutsSettings::onOmicronUpdate()
    {
        if (statuses_)
            statuses_->setVisible(Statuses::isStatusEnabled());
    }

    void ShortcutsSettings::showEvent(QShowEvent *_event)
    {
        onOmicronUpdate();
        onSettingsChanged(ql1s(settings_allow_statuses));
    }

    AppearanceWidget::AppearanceWidget(QWidget* _parent)
        :QWidget(_parent)
        , list_(new SimpleListWidget(Qt::Vertical))
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::additionalThemesChanged, this, &AppearanceWidget::resetThemesList, Qt::QueuedConnection);
    }

    void AppearanceWidget::resetThemesList()
    {
        list_->clear();
        list_->setCurrentIndex(-1);

        const auto themesList = Styling::getThemesContainer().getAvailableThemes();
        const auto current = Styling::getThemesContainer().getCurrentThemeId();
        const auto it = std::find_if(themesList.begin(), themesList.end(), [&current](const auto& _p) { return _p.first == current; });
        const auto currentIdx = it != themesList.end() ? int(std::distance(themesList.begin(), it)) : std::numeric_limits<int>::max();

        for (const auto& [id, name] : themesList)
        {
            auto row = new RadioTextRow(name);
            Testing::setAccessibleName(row, u"AS AppearancePage " % id);
            list_->addItem(row);
        }

        list_->setCurrentIndex(currentIdx);

        QObject::connect(list_, &SimpleListWidget::clicked, list_, [this, themesList](int _idx)
        {
            if (_idx < 0 && _idx >= int(themesList.size()))
                return;

            if (const auto clickedTheme = themesList[_idx].first; clickedTheme != Styling::getThemesContainer().getCurrentThemeId())
            {
                list_->setCurrentIndex(_idx);
                Styling::getThemesContainer().setCurrentTheme(clickedTheme);
            }
        }, Qt::UniqueConnection);
    }

    SimpleListWidget* AppearanceWidget::getList() const
    {
        return list_;
    }
}
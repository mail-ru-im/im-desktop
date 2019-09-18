#include "stdafx.h"
#include "GeneralSettingsWidget.h"


#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "styles/ThemeParameters.h"
#include "main_window/smiles_menu/Store.h"
#include "ContactUs.h"

#define DEFAULT_DEVICE_UID "default_device"

namespace Ui
{
    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* _parent)
        : QStackedWidget(_parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStyleSheet(qsl("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    }

    GeneralSettingsWidget::~GeneralSettingsWidget()
    {
        //
    }

    void GeneralSettingsWidget::setType(int _type)
    {
        Utils::CommonSettingsType type = (Utils::CommonSettingsType)_type;

        if (type == Utils::CommonSettingsType::CommonSettingsType_General)
        {
            if (!general_)
            {
                general_ = new GeneralSettings(this);
                general_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initGeneral(general_);
                addWidget(general_);

                Testing::setAccessibleName(general_, qsl("AS settings general_"));

                general_->recvUserProxy();
                QObject::connect(Ui::GetDispatcher(), &core_dispatcher::getUserProxy, general_, &GeneralSettings::recvUserProxy);
                QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::downloadPathUpdated, general_, &GeneralSettings::downloadPathUpdated);
            }

            setCurrentWidget(general_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo)
        {
            initVoiceAndVideo();

            setCurrentWidget(voiceAndVideo_.rootWidget);
            if (devices_.empty())
                Ui::GetDispatcher()->getVoipController().setRequestSettings();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Notifications)
        {
            if (!notifications_)
            {
                notifications_ = new NotificationSettings(this);
                notifications_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initNotifications(notifications_);
                addWidget(notifications_);

                Testing::setAccessibleName(notifications_, qsl("AS settings notifications_"));
            }

            setCurrentWidget(notifications_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Appearance)
        {
            if (!appearance_)
            {
                appearance_ = new QWidget(this);
                appearance_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initAppearance(appearance_);
                addWidget(appearance_);

                Testing::setAccessibleName(appearance_, qsl("AS settings appearance_"));
            }

            setCurrentWidget(appearance_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_About)
        {
            if (!about_)
            {
                about_ = new QWidget(this);
                about_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initAbout(about_);
                addWidget(about_);

                Testing::setAccessibleName(about_, qsl("AS settings about_"));
            }

            setCurrentWidget(about_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_ContactUs)
        {
            if (!contactus_)
            {
                contactus_ = new ContactUsWidget(this);
                contactus_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                addWidget(contactus_);

                Testing::setAccessibleName(contactus_, qsl("AS settings contactus_"));
            }

            setCurrentWidget(contactus_);
            emit Utils::InterConnector::instance().generalSettingsContactUsShown();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachPhone)
        {
            if (!attachPhone_)
            {
                attachPhone_ = new QWidget(this);
                attachPhone_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initAttachPhone(attachPhone_);
                addWidget(attachPhone_);

                Testing::setAccessibleName(attachPhone_, qsl("AS settings attachPhone_"));
            }

            setCurrentWidget(attachPhone_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Language)
        {
            if (!language_)
            {
                language_ = new QWidget(this);
                language_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initLanguage(language_);
                addWidget(language_);

                Testing::setAccessibleName(language_, qsl("AS settings language_"));
            }

            setCurrentWidget(language_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Shortcuts)
        {
            if (!shortcuts_)
            {
                shortcuts_ = new QWidget(this);
                shortcuts_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initShortcuts(shortcuts_);
                addWidget(shortcuts_);

                Testing::setAccessibleName(shortcuts_, qsl("AS settings shortcuts_"));
            }

            setCurrentWidget(shortcuts_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Security)
        {
            if (!security_)
            {
                security_ = new QWidget(this);
                security_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                Creator::initSecurity(security_);
                addWidget(security_);

                Testing::setAccessibleName(shortcuts_, qsl("AS settings security_"));
            }

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
                    emit Utils::InterConnector::instance().myProfileBack();
                });
            }

            setCurrentWidget(stickersStore_);
        }
    }

    void GeneralSettingsWidget::onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices)
    {
        devices_ = _devices;

        // Remove non camera devices.
        devices_.erase(std::remove_if(devices_.begin(), devices_.end(), []( const voip_proxy::device_desc& desc)
        {
            return (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture  &&  desc.video_dev_type != voip_proxy::kvoipDeviceCamera);
        }), devices_.end());

        QMenu* menu =  nullptr;
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
            assert(false);
            return;
        }

        if (!menu || !deviceList)
        {
            return;
        }

        Testing::setAccessibleName(menu, qsl("AS voip menu"));

        deviceList->clear();
        menu->clear();
        if (currentSelected)
        {
            currentSelected->setText(QString());
        }

#ifdef _WIN32
        voip_proxy::device_desc defaultDeviceDescription;
        if (addDefaultDevice && !devices_.empty())
        {
            DeviceInfo di;
            di.name = QT_TRANSLATE_NOOP("settings", "By default").toStdString() + " (" + devices_[0].name + ')';
            di.uid  = DEFAULT_DEVICE_UID;

            defaultDeviceDescription.name = di.name;
            defaultDeviceDescription.uid  = di.uid;
            defaultDeviceDescription.dev_type = deviceType;

            deviceList->push_back(di);
        }
#endif

        using namespace voip_proxy;

        const device_desc* selectedDesc = nullptr;
        const device_desc* activeDesc   = nullptr;
        for (unsigned ix = 0; ix < devices_.size(); ix++)
        {
            const device_desc& desc = devices_[ix];

            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            deviceList->push_back(di);

            if (user_selected_device_.count(deviceType) > 0 && user_selected_device_[deviceType] == di.uid)
            {
                selectedDesc = &desc;
            }
            if (desc.isActive)
            {
                activeDesc = &desc;
            }
        }

#ifdef _WIN32
        // For default device select
        if (addDefaultDevice && !devices_.empty())
        {
            if (user_selected_device_.count(deviceType) > 0 && user_selected_device_[deviceType] == DEFAULT_DEVICE_UID)
            {
                selectedDesc = &defaultDeviceDescription;
            }
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
            // User selected item has most priority, then voip active device, then first element in list.
            const device_desc* desc = selectedDesc ? selectedDesc
                : (activeDesc ? activeDesc
                    : (!devices_.empty() ? &devices_[0] : nullptr));

            if (desc)
            {
                std::string realActiveDeviceID = setActiveDevice(*desc);
#ifdef _WIN32
                currentSelected->setText(QString::fromStdString((realActiveDeviceID == DEFAULT_DEVICE_UID) ? defaultDeviceDescription.name : desc->name));
#else
                currentSelected->setText(QString::fromStdString(desc->name));
#endif
            }
        }
    }

    std::string  GeneralSettingsWidget::setActiveDevice(const voip_proxy::device_desc& _description)
    {
        std::string runTimeUid = _description.uid;
        voip_proxy::device_desc description = applyDefaultDeviceLogic(_description, runTimeUid);
        Ui::GetDispatcher()->getVoipController().setActiveDevice(description);

        QString settingsName;
        switch (description.dev_type) {
        case voip_proxy::kvoipDevTypeAudioPlayback: { settingsName = ql1s(settings_speakers);  break; }
        case voip_proxy::kvoipDevTypeAudioCapture: { settingsName = ql1s(settings_microphone); break; }
        case voip_proxy::kvoipDevTypeVideoCapture: { settingsName = ql1s(settings_webcam);     break; }
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(!"unexpected device type");
            return runTimeUid;
        };

        user_selected_device_[description.dev_type] = runTimeUid;

        const auto uid = QString::fromStdString(description.uid);
        if (get_gui_settings()->get_value<QString>(settingsName, QString()) != uid)
            get_gui_settings()->set_value<QString>(settingsName, uid);

        return runTimeUid;
    }

    void GeneralSettingsWidget::initVoiceAndVideo()
    {
        if (voiceAndVideo_.rootWidget)
            return;

        voiceAndVideo_.rootWidget = new QWidget(this);
        voiceAndVideo_.rootWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Creator::initVoiceVideo(voiceAndVideo_.rootWidget, voiceAndVideo_);

        // Fill device list.
        for (int i = voip_proxy::kvoipDevTypeAudioCapture; i <= voip_proxy::kvoipDevTypeVideoCapture; i += 1)
        {
            voip_proxy::EvoipDevTypes type = (voip_proxy::EvoipDevTypes)i;
            onVoipDeviceListUpdated(type, Ui::GetDispatcher()->getVoipController().deviceList(type));
        }

        addWidget(voiceAndVideo_.rootWidget);

        auto setActiveDevice = [this](const voip_proxy::EvoipDevTypes& type)
        {
            QString settingsName;

            switch (type)
            {
            case voip_proxy::kvoipDevTypeAudioPlayback:
            {
                settingsName = ql1s(settings_speakers);
                break;
            }
            case voip_proxy::kvoipDevTypeAudioCapture:
            {
                settingsName = ql1s(settings_microphone);
                break;
            }
            case voip_proxy::kvoipDevTypeVideoCapture:
            {
                settingsName = ql1s(settings_webcam);
                break;
            }
            case voip_proxy::kvoipDevTypeUndefined:
            default:
                assert(!"unexpected device type");
                return;
            };

            QString val = get_gui_settings()->get_value<QString>(settingsName, QString());
            bool applyDefaultDevice = getDefaultDeviceFlag(type);

            if (!val.isEmpty())
            {
                voip_proxy::device_desc description;
                description.uid = applyDefaultDevice ? DEFAULT_DEVICE_UID : val.toStdString();
                description.dev_type = type;

                this->setActiveDevice(description);
            }
        };

        setActiveDevice(voip_proxy::kvoipDevTypeAudioPlayback);
        setActiveDevice(voip_proxy::kvoipDevTypeAudioCapture);
        setActiveDevice(voip_proxy::kvoipDevTypeVideoCapture);

        QObject::connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipDeviceListUpdated, this, &GeneralSettingsWidget::onVoipDeviceListUpdated);
    }

    bool GeneralSettingsWidget::getDefaultDeviceFlag(const voip_proxy::EvoipDevTypes& type)
    {
#ifdef _WIN32
        QString defaultFlagSettingName;

        switch (type)
        {
        case voip_proxy::kvoipDevTypeAudioPlayback: { defaultFlagSettingName = ql1s(settings_speakers_is_default);  break; }
        case voip_proxy::kvoipDevTypeAudioCapture: {  defaultFlagSettingName = ql1s(settings_microphone_is_default); break; }
        default:
            return false;
        };

        return !defaultFlagSettingName.isEmpty() ?
            get_gui_settings()->get_value<bool>(defaultFlagSettingName, false) : false;
#else
        return false;
#endif
    }

    voip_proxy::device_desc GeneralSettingsWidget::applyDefaultDeviceLogic(const voip_proxy::device_desc& _description, std::string& runtimeDeviceUid)
    {
#ifdef _WIN32
        QString defaultFlagSettingName;
        runtimeDeviceUid = _description.uid;
        voip_proxy::device_desc res = _description;

        switch (_description.dev_type)
        {
            case voip_proxy::kvoipDevTypeAudioPlayback:
            {
                defaultFlagSettingName = ql1s(settings_speakers_is_default);
                break;
            }
            case voip_proxy::kvoipDevTypeAudioCapture:
            {
                defaultFlagSettingName = ql1s(settings_microphone_is_default);
                break;
            }
            default: return res;
        }

        const std::vector<voip_proxy::device_desc>& devList =
            Ui::GetDispatcher()->getVoipController().deviceList(res.dev_type);

        if (!devList.empty())
        {
            // If there is no default setting, we try to set current device to default, if user use default device.
            if (!get_gui_settings()->contains_value(defaultFlagSettingName) && devList[0].uid == res.uid && !res.uid.empty())
            {
                res.uid = DEFAULT_DEVICE_UID;
                runtimeDeviceUid = DEFAULT_DEVICE_UID;
            }

            // Default device is first in device list.
            if (res.uid == DEFAULT_DEVICE_UID)
            {
                res.uid = devList[0].uid;
            }

            get_gui_settings()->set_value<bool>(defaultFlagSettingName, runtimeDeviceUid == DEFAULT_DEVICE_UID);
        }
        return res;
#else
        return _description;
#endif
    }


    GeneralSettings::GeneralSettings(QWidget* _parent)
        : QWidget(_parent)
        , connectionTypeChooser_(nullptr)
        , downloadPathChooser_(nullptr)
        , launchMinimized_(nullptr)
    {
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

    NotificationSettings::NotificationSettings(QWidget* _parent)
        : QWidget(_parent)
        , sounds_(nullptr)
    {
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &NotificationSettings::value_changed);
    }

    void NotificationSettings::value_changed(const QString& name)
    {
        if (name == ql1s(settings_sounds_enabled))
        {
            QSignalBlocker sb(sounds_);
            sounds_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
        }
    }
}

#pragma once

#include "../../controls/GeneralCreator.h"
#include "../../voip/VoipProxy.h"
#include "main_window/sidebar/SidebarUtils.h"

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    namespace Stickers
    {
        class Store;
    }

    class TextEmojiWidget;

    class ConnectionSettingsWidget;

    class GeneralSettings : public QWidget
    {
        Q_OBJECT

    public:
        GeneralSettings(QWidget * _parent);

        TextEmojiWidget* connectionTypeChooser_;
        TextEmojiWidget* downloadPathChooser_;
        SidebarCheckboxButton* launchMinimized_;

    public Q_SLOTS:
        void recvUserProxy();
        void downloadPathUpdated();
    };

    class NotificationSettings : public QWidget
    {
        Q_OBJECT

    public:
        NotificationSettings(QWidget * _parent);

        Ui::SidebarCheckboxButton* sounds_;

    private Q_SLOTS:
       void value_changed(const QString&);
    };

    class GeneralSettingsWidget : public QStackedWidget
    {
        struct DeviceInfo
        {
            std::string name;
            std::string uid;
        };

        Q_OBJECT

    private:
        std::vector< voip_proxy::device_desc > devices_;
        // Hold user selected devices.
        std::unordered_map<uint8_t, std::string> user_selected_device_;

    private:
        struct VoiceAndVideoOptions
        {
            QWidget* rootWidget = nullptr;

            std::vector<DeviceInfo> aCapDeviceList;
            std::vector<DeviceInfo> aPlaDeviceList;
            std::vector<DeviceInfo> vCapDeviceList;

            TextEmojiWidget* aCapSelected = nullptr;
            TextEmojiWidget* aPlaSelected = nullptr;
            TextEmojiWidget* vCapSelected = nullptr;

            QMenu* audioCaptureDevices = nullptr;
            QMenu* audioPlaybackDevices = nullptr;
            QMenu* videoCaptureDevices = nullptr;
        }
        voiceAndVideo_;

        GeneralSettings* general_ = nullptr;
        NotificationSettings* notifications_ = nullptr;
        QWidget* appearance_ = nullptr;
        QWidget* about_ = nullptr;
        QWidget* contactus_ = nullptr;
        QWidget* attachPhone_ = nullptr;
        QWidget* language_ = nullptr;
        QWidget* shortcuts_ = nullptr;
        QWidget* security_ = nullptr;
        Stickers::Store* stickersStore_ = nullptr;


        struct Creator
        {
            static void initAbout(QWidget* _parent);
            static void initGeneral(GeneralSettings* _parent);
            static void initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo);
            static void initAppearance(QWidget* _parent);
            static void initNotifications(NotificationSettings* _parent);
            static void initContactUs(QWidget* _parent);
            static void initAttachPhone(QWidget* _parent);
            static void initLanguage(QWidget* _parent);
            static void initShortcuts(QWidget* _parent);
            static void initSecurity(QWidget* _parent);
        };

    public:
        GeneralSettingsWidget(QWidget* _parent = nullptr);
        ~GeneralSettingsWidget();

        void setType(int _type);

        //@return uid of set device.
        std::string setActiveDevice(const voip_proxy::device_desc& _description);

    private:
        void initVoiceAndVideo();

        bool getDefaultDeviceFlag(const voip_proxy::EvoipDevTypes& type);
        voip_proxy::device_desc applyDefaultDeviceLogic(const voip_proxy::device_desc& _description, std::string& runtimeDeviceUid);
        void onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices);
    };
}

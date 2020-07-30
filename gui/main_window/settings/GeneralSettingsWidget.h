#pragma once

#include "../../controls/GeneralCreator.h"
#include "../../voip/VoipProxy.h"

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
    class SessionsPage;
    class ConnectionSettingsWidget;

    class GeneralSettings : public QWidget
    {
        Q_OBJECT

    public:
        GeneralSettings(QWidget * _parent);

        TextEmojiWidget* connectionTypeChooser_;
        TextEmojiWidget* downloadPathChooser_;
        SidebarCheckboxButton* launchMinimized_;
        SidebarCheckboxButton* smartreplySwitcher_;
        SidebarCheckboxButton* spellCheckerSwitcher_;
        SidebarCheckboxButton* suggestsEmojiSwitcher_;
        SidebarCheckboxButton* suggestsWordsSwitcher_;
        SidebarCheckboxButton* statusSwitcher_;
        SidebarCheckboxButton* previewLinks_;
        SidebarCheckboxButton* reactionsSwitcher_;

    public Q_SLOTS:
        void recvUserProxy();
        void downloadPathUpdated();
        void onOmicronUpdate();

    protected:
        void showEvent(QShowEvent*) override;
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

    class ShortcutsSettings : public QWidget
    {
    Q_OBJECT

    public:
        ShortcutsSettings(QWidget* _parent);

        QWidget* statuses_;

    private Q_SLOTS:
        void onSettingsChanged(const QString &_name);
        void onOmicronUpdate();

    protected:
        void showEvent(QShowEvent *_event);
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
        ShortcutsSettings* shortcuts_ = nullptr;
        QWidget* security_ = nullptr;
        Stickers::Store* stickersStore_ = nullptr;
        QWidget* debugSettings_ = nullptr;
        SessionsPage* sessions_ = nullptr;

        struct Creator
        {
            static void initAbout(QWidget* _parent);
            static void initGeneral(GeneralSettings* _parent);
            static void initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo);
            static void initAppearance(QWidget* _parent);
            static void initNotifications(NotificationSettings* _parent);
            static void initContactUs(QWidget* _parent);
            static void initLanguage(QWidget* _parent);
            static void initShortcuts(ShortcutsSettings* _parent);
            static void initSecurity(QWidget* _parent);
        };

    public:
        GeneralSettingsWidget(QWidget* _parent = nullptr);
        ~GeneralSettingsWidget();

        void setType(int _type);
        void setActiveDevice(const voip_proxy::device_desc& _description);
        bool isSessionsShown() const;

    private:
        void initVoiceAndVideo();

        void initSessionsPage();

        bool getDefaultDeviceFlag(const voip_proxy::EvoipDevTypes& type);
        void applyDefaultDeviceLogic(const voip_proxy::device_desc& _description);
        void onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices);
    };
}

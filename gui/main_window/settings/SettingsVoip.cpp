#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../sounds/SoundsManager.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo)
{
    Ui::GetSoundsManager(); // Start device monitoring

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("QWidget{border: none; background-color: transparent;}"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);
    Utils::ApplyStyle( mainWidget, Styling::getParameters().getGeneralSettingsQss());

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(36), Utils::scale_value(20), Utils::scale_value(36));

    Testing::setAccessibleName(mainWidget, qsl("AS VoiceAndVideoPage mainWidget"));
    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS VoiceAndVideoPage scrollArea"));
    layout->addWidget(scrollArea);

    auto __deviceChanged = [&_voiceAndVideo, _parent](const int ix, const voip_proxy::EvoipDevTypes dev_type)
    {
        im_assert(ix >= 0);
        if (ix < 0)
            return;

        std::vector<DeviceInfo>* devList = NULL;
        switch (dev_type)
        {
        case voip_proxy::kvoipDevTypeAudioPlayback:
            devList = &_voiceAndVideo.aPlaDeviceList; break;
        case voip_proxy::kvoipDevTypeAudioCapture:
            devList = &_voiceAndVideo.aCapDeviceList; break;
        case voip_proxy::kvoipDevTypeVideoCapture:
            devList = &_voiceAndVideo.vCapDeviceList; break;
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            im_assert(!"unexpected device type");
            return;
        };

        im_assert(devList);
        if (devList->empty())
            return;

        im_assert(ix < (int)devList->size());
        const DeviceInfo& info = (*devList)[ix];

        GeneralSettingsWidget* settingsWidget = qobject_cast<GeneralSettingsWidget*>(_parent->parent());

        if (settingsWidget)
        {
            Testing::setAccessibleName(settingsWidget, qsl("AS VoiceAndVideoPage settingsWidget"));
            voip_proxy::device_desc description;
            description.name = info.name;
            description.uid = info.uid;
            description.dev_type = dev_type;

            settingsWidget->setActiveDevice(description);
        }
    };

    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Microphone:"),
            false,
            vs,
            0,
            -1,
            [__deviceChanged](const QString&, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioCapture);
            });

        _voiceAndVideo.audioCaptureDevices = di.menu;
        _voiceAndVideo.aCapSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.audioCaptureDevices, qsl("AS VoiceAndVideoPage audioCaptureDevices"));
        Testing::setAccessibleName(_voiceAndVideo.aCapSelected, qsl("AS VoiceAndVideoPage audioCaptureDevicesSelected"));
        mainLayout->addSpacing(Utils::scale_value(20));
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Speakers:"),
            false,
            vs,
            0,
            -1,
            [__deviceChanged](const QString&, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioPlayback);
            });

        _voiceAndVideo.audioPlaybackDevices = di.menu;
        _voiceAndVideo.aPlaSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.audioPlaybackDevices, qsl("AS VoiceAndVideoPage audioPlaybackDevices"));
        Testing::setAccessibleName(_voiceAndVideo.aPlaSelected, qsl("AS VoiceAndVideoPage  audioPlaybackDevicesSelected"));
        mainLayout->addSpacing(Utils::scale_value(20));
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Webcam:"),
            false,
            vs,
            0,
            -1,
            [__deviceChanged](const QString&, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoipDevTypeVideoCapture);
            });

        _voiceAndVideo.videoCaptureDevices = di.menu;
        _voiceAndVideo.vCapSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.videoCaptureDevices, qsl("AS VoiceAndVideoPage videoCaptureDevices"));
        Testing::setAccessibleName(_voiceAndVideo.vCapSelected, qsl("AS VoiceAndVideoPage videoCaptureDevicesSelected"));
        mainLayout->addSpacing(Utils::scale_value(20));
    }
}

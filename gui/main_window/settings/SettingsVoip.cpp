#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);
    Utils::ApplyStyle( mainWidget, Styling::getParameters().getGeneralSettingsQss());

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(36), Utils::scale_value(20), Utils::scale_value(36));

    Testing::setAccessibleName(mainWidget, qsl("AS settings voip mainWidget"));
    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS settings voip scrollArea"));
    layout->addWidget(scrollArea);

    auto __deviceChanged = [&_voiceAndVideo, _parent](const int ix, const voip_proxy::EvoipDevTypes dev_type)
    {
        assert(ix >= 0);
        if (ix < 0)
        {
            return;
        }

        std::vector<DeviceInfo>* devList = NULL;
        switch (dev_type)
        {
        case voip_proxy::kvoipDevTypeAudioPlayback:
        {
            devList = &_voiceAndVideo.aPlaDeviceList; break;
        }
        case voip_proxy::kvoipDevTypeAudioCapture:
        {
            devList = &_voiceAndVideo.aCapDeviceList; break;
        }
        case voip_proxy::kvoipDevTypeVideoCapture:
        {
            devList = &_voiceAndVideo.vCapDeviceList; break;
        }
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(!"unexpected device type");
            return;
        };

        assert(devList);
        if (devList->empty()) { return; }

        assert(ix < (int)devList->size());
        const DeviceInfo& info = (*devList)[ix];

        GeneralSettingsWidget* settingsWidget = qobject_cast<GeneralSettingsWidget*>(_parent->parent());

        if (settingsWidget)
        {
            Testing::setAccessibleName(settingsWidget, qsl("AS voip settingsWidget"));
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
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioCapture);
        },
            [](bool) -> QString { return QString(); });

        _voiceAndVideo.audioCaptureDevices = di.menu;
        _voiceAndVideo.aCapSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.audioCaptureDevices, qsl("AS voip audioCaptureDevices"));
        Testing::setAccessibleName(_voiceAndVideo.aCapSelected, qsl("AS voip aCapSelected"));
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
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioPlayback);
        },
            [](bool) -> QString { return QString(); });

        _voiceAndVideo.audioPlaybackDevices = di.menu;
        _voiceAndVideo.aPlaSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.audioPlaybackDevices, qsl("AS voip audioPlaybackDevices"));
        Testing::setAccessibleName(_voiceAndVideo.aPlaSelected, qsl("AS voip aPlaSelected"));
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
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeVideoCapture);
        },
            [](bool) -> QString { return QString(); });

        _voiceAndVideo.videoCaptureDevices = di.menu;
        _voiceAndVideo.vCapSelected = di.currentSelected;
        Testing::setAccessibleName(_voiceAndVideo.videoCaptureDevices, qsl("AS voip videoCaptureDevices"));
        Testing::setAccessibleName(_voiceAndVideo.vCapSelected, qsl("AS voip vCapSelected"));
        mainLayout->addSpacing(Utils::scale_value(20));
    }
}

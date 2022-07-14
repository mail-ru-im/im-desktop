#include "stdafx.h"
#include "MicroAlert.h"
#include "VideoOverlay.h"
#include "../utils/utils.h"

namespace Ui
{
    class VideoOverlayPrivate
    {
    public:
        VideoOverlay* q;
        MicroAlert* microAlert_ = nullptr;

        VideoOverlayPrivate(VideoOverlay* _q) : q(_q) {}

        void createMicroAlert()
        {
            microAlert_ = new MicroAlert(q);
            microAlert_->installEventFilter(q);
            microAlert_->setState(MicroAlert::MicroAlertState::Expanded);
            microAlert_->setIssue(MicroIssue::NoPermission);
        }

        void createLayout()
        {
            const int margin = MicroAlert::alertOffset();
            QHBoxLayout* alertLayout = ::Utils::emptyHLayout();
            alertLayout->setContentsMargins(margin, margin, margin, margin);
            alertLayout->addWidget(microAlert_, 1);
            alertLayout->addStretch();

            QVBoxLayout* layout = ::Utils::emptyVLayout(q);
            layout->addLayout(alertLayout);
            layout->addStretch();
        }

        void initUi()
        {
            createMicroAlert();
            createLayout();
        }
    };

    VideoOverlay::VideoOverlay(QWidget* _parent)
        : ShapedWidget(_parent)
        , d(std::make_unique<VideoOverlayPrivate>(this))
    {
        d->initUi();
    }

    VideoOverlay::~VideoOverlay() = default;

    void VideoOverlay::showMicroAlert(MicroIssue _issue)
    {
        d->microAlert_->setState(MicroAlert::MicroAlertState::Expanded);
        d->microAlert_->setIssue(_issue);
        if (_issue != MicroIssue::None)
            show();
        else
            hide();
    }
}

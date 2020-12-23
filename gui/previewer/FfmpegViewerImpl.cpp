#include "stdafx.h"

#include "../utils/utils.h"
#include "../utils/gui_metrics.h"

#include "FfmpegViewerImpl.h"
#ifndef STRIP_AV_MEDIA
#include "../main_window/mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA
#include "../gui_settings.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

namespace
{
    auto minVideoSize()
    {
        return Utils::scale_value(380);
    }
}

std::unique_ptr<Previewer::AbstractViewer> Previewer::FFMpegViewer::create(const MediaData& _mediaData, const QSize& _viewportSize, QWidget* _parent, const bool _firstOpen)
{
    assert(_parent);
    auto viewer = std::unique_ptr<AbstractViewer>(new FFMpegViewer(_mediaData, _viewportSize, _parent, _firstOpen));

    return viewer;
}

void Previewer::FFMpegViewer::hide()
{
    ffplayer_->setPaused(true, true);
    ffplayer_->hide();
}

void Previewer::FFMpegViewer::show()
{
    ffplayer_->showAs();
    if (ffplayer_->isPausedByUser())
        ffplayer_->setPaused(false, true);
}

void Previewer::FFMpegViewer::bringToBack()
{
#ifdef __APPLE__
    MacSupport::showNSModalPanelWindowLevel(ffplayer_.get());
#endif
}

void Previewer::FFMpegViewer::showOverAll()
{
#ifdef __APPLE__
    MacSupport::showOverAll(ffplayer_.get());
#endif
}

QWidget* Previewer::FFMpegViewer::getParentForContextMenu() const
{
    return ffplayer_.get();
}

Previewer::FFMpegViewer::FFMpegViewer(const MediaData& _mediaData,
                                      const QSize& _viewportSize,
                                      QWidget* _parent,
                                      const bool _firstOpen)
    :   AbstractViewer(_viewportSize, _parent),
        parent_(_parent),
        fullscreen_(false)
{
    uint32_t flags = Ui::DialogPlayer::Flags::enable_control_panel |
                     Ui::DialogPlayer::Flags::gpu_renderer |
                     Ui::DialogPlayer::Flags::as_window;

    QFileInfo fileInfo(_mediaData.fileName);
    if (Utils::is_image_extension(fileInfo.suffix().toLower()))
        flags |= Ui::DialogPlayer::Flags::is_gif;

    if (_mediaData.attachedPlayer)
    {
        ffplayer_ = std::make_unique<Ui::DialogPlayer>(_mediaData.attachedPlayer, _parent);

        auto callInit = [this, _viewportSize]()
        {
            ffplayer_->showAs();
            const auto videoSize = ffplayer_->getVideoSize();
            init({videoSize.width(), videoSize.height()}, _viewportSize, minVideoSize());
        };

        if (ffplayer_->state() == QMovie::NotRunning)
            connect(ffplayer_.get(), &Ui::DialogPlayer::loaded, this, callInit);
        else
            callInit();

        ffplayer_->setClippingPath(QPainterPath());
        ffplayer_->start(true);
    }
    else
    {
        ffplayer_ = std::make_unique<Ui::DialogPlayer>(_parent, flags, _mediaData.preview);

        connect(ffplayer_.get(), &Ui::DialogPlayer::loaded, this, [this, _firstOpen, _viewportSize, _mediaData]()
        {
            ffplayer_->showAs();
            const auto videoSize = ffplayer_->getVideoSize();
            init({videoSize.width(), videoSize.height()}, _viewportSize, minVideoSize());

            int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);
            ffplayer_->setVolume(volume);
            ffplayer_->setGotAudio(_mediaData.gotAudio);
            ffplayer_->setMute(false);
            ffplayer_->start(true);

            if (_firstOpen)
                statistic::getGuiMetrics().eventGalleryVideoLoaded();

        });

        connect(ffplayer_.get(), &Ui::DialogPlayer::firstFrameReady, this, &FFMpegViewer::loaded);

        ffplayer_->openMedia(_mediaData.fileName);
    }

    connect(ffplayer_.get(), &Ui::DialogPlayer::fullScreenClicked, this, [this]()
    {
        if (fullscreen_)
            ffplayer_->showAsNormal();
        else
            ffplayer_->showAsFullscreen();

        fullscreen_ = !fullscreen_;

        Q_EMIT fullscreenToggled(fullscreen_);

    }, Qt::QueuedConnection);

    connect(ffplayer_.get(), &Ui::DialogPlayer::playClicked, this, &FFMpegViewer::playClicked);

    connect(ffplayer_.get(), &Ui::DialogPlayer::mouseWheelEvent, this, [this](const QPoint& _delta)
    {
        Q_EMIT mouseWheelEvent(_delta);

    });

    connect(ffplayer_.get(), &Ui::DialogPlayer::mouseDoubleClicked, this, &FFMpegViewer::doubleClicked, Qt::QueuedConnection); // direct connection leads to crash in qt internals
    connect(ffplayer_.get(), &Ui::DialogPlayer::mouseRightClicked, this, &FFMpegViewer::rightClicked, Qt::QueuedConnection); // direct connection leads to crash in qt internals

    ffplayer_->setReplay(true);
    ffplayer_->setCursor(Qt::PointingHandCursor);
}

Previewer::FFMpegViewer::~FFMpegViewer()
{
    ffplayer_->close();

    auto attachedPlayer = ffplayer_->getAttachedPlayer();
    if (attachedPlayer)
    {
        attachedPlayer->setPaused(true, true);
        attachedPlayer->setMute(true);
        attachedPlayer->setFillClient(Utils::isPanoramic(ffplayer_->getVideoSize()));
    }
}

QSize Previewer::FFMpegViewer::calculateInitialSize() const
{
    return ffplayer_->getVideoSize();
}

void Previewer::FFMpegViewer::doPaint(QPainter& _painter, const QRect& _source, const QRect& _target)
{
}

void Previewer::FFMpegViewer::doResize(const QRect& _source, const QRect& _target)
{
    const QPoint ptLeftTop = parent_->mapToGlobal({(parent_->width() - _target.width())/2, _target.topLeft().y()});
    ffplayer_->setGeometry(QRect(ptLeftTop.x(), ptLeftTop.y(), _target.width(), _target.height()));
}

bool Previewer::FFMpegViewer::closeFullscreen()
{
    if (!fullscreen_)
        return false;

    ffplayer_->showAsNormal();

    fullscreen_ = false;

    return true;
}

bool Previewer::FFMpegViewer::isZoomSupport() const
{
    return true;
}
#include "stdafx.h"
#include "VideoPlayer.h"

#include "../../utils/utils.h"
#include "app_config.h"
#include "../../gui_settings.h"

#include "controls/CustomButton.h"

#include "../../utils/InterConnector.h"
#include "../MainWindow.h"
#include "../history_control/MessageStyle.h"

#include "FFMpegPlayer.h"
#include "FrameRenderer.h"

#include "../../fonts.h"

#include "../gui/memory_stats/FFmpegPlayerMemMonitor.h"

#include "previewer/Drawable.h"
#include "previewer/toast.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#include "macos/MetalRenderer.h"
#endif

namespace
{
    constexpr std::chrono::milliseconds hideTimeout() noexcept { return std::chrono::seconds(2); }
    constexpr std::chrono::milliseconds hideTimeoutShort() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds unloadTimeout() noexcept { return std::chrono::seconds(20); }
    constexpr std::chrono::milliseconds unloadTimeoutLottie() noexcept { return std::chrono::seconds(5); }
}

namespace Ui
{
    int getMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 16 : 8);
    }

    int getVolumeSliderHeight(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 80 : 48);
    }

    int getControlPanelSoundHeight(bool _isFullScreen)
    {
        return (int) getVolumeSliderHeight(_isFullScreen) + getMargin(_isFullScreen);
    }

    int getControlPanelButtonSize(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 32 : 24);
    }

    int getSoundButtonVolumeSpacerHeight(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    int getSoundsWidgetHeight(bool _isFullScreen)
    {
        return (int) getControlPanelButtonSize(_isFullScreen) + getControlPanelSoundHeight(_isFullScreen);
    }

    int getControlPanelMaxHeight(bool _isFullScreen)
    {
        return (int) getSoundsWidgetHeight(_isFullScreen) + 2 * getMargin(_isFullScreen) + getSoundButtonVolumeSpacerHeight(_isFullScreen);
    }

    int getVolumeSliderWidth(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 28 : 24);
    }

    int getTimeRightMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    int getTimeBottomMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    QSize getSoundButtonIconSize()
    {
        return QSize(20,20);
    }

    QColor getSoundButtonDefaultColor()
    {
        return QColor(255, 255, 255, 255 * 0.8);
    }

    bool useMetalRenderer() noexcept
    {
#ifdef __APPLE__
        return MacSupport::isMetalSupported();
#else
        return false;
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // VideoProgressSlider
    //////////////////////////////////////////////////////////////////////////
    InstChangedSlider::InstChangedSlider(Qt::Orientation _orientation, QWidget* _parent = nullptr)
        : QSlider(_orientation, _parent)
    {

    }

    void InstChangedSlider::mousePressEvent(QMouseEvent* _event)
    {
        int64_t sliderWidth = width();

        if (_event->button() == Qt::LeftButton)
        {
            int64_t value = 0;

            if (orientation() == Qt::Vertical)
            {
                value = (int64_t) minimum() + ((int64_t) (maximum() - (int64_t) minimum()) * ((int64_t) height() - (int64_t) _event->y())) / (int64_t) height();
            }
            else
            {
                value = (int64_t) minimum() + (((int64_t) maximum() - (int64_t) minimum()) * (int64_t) _event->x()) / (int64_t) sliderWidth;
            }

            setValue((int32_t) value);

            _event->accept();

            Q_EMIT sliderMoved(value);
            Q_EMIT sliderReleased();
        }

        QSlider::mousePressEvent(_event);
    }

    void InstChangedSlider::wheelEvent(QWheelEvent* _event)
    {
        QSlider::wheelEvent(_event);

        Q_EMIT sliderMoved(value());
        Q_EMIT sliderReleased();
    }

    //////////////////////////////////////////////////////////////////////////
    // VideoPlayerControlPanel
    //////////////////////////////////////////////////////////////////////////
    VideoPlayerControlPanel::VideoPlayerControlPanel(
        VideoPlayerControlPanel* _copyFrom,
        QWidget* _parent,
        FFMpegPlayer* _player,
        const QString& _mode)

        :   QWidget(_parent),
            duration_(_copyFrom ? _copyFrom->duration_ : 0),
            position_(_copyFrom ? _copyFrom->position_ : 0),
            player_(_player),
            lastSoundMode_(_copyFrom ? _copyFrom->lastSoundMode_ : SoundMode::ActiveOff),
            gotAudio_(_copyFrom ? _copyFrom->gotAudio_ : false),
            fullscreen_(_mode == u"full_screen"),
            soundOnByUser_(false)
    {
        setProperty("mode", _mode);

        setStyleSheet(Utils::LoadStyle(qsl(":/qss/mstyles")));

        int32_t volume = get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

        QVBoxLayout* rootLayout = new QVBoxLayout();
        rootLayout->setContentsMargins(0, 0, 0, 0);

        gradient_ = new QWidget(this);
        gradient_->setObjectName(qsl("control_panel_gradient"));
        gradient_->setProperty("mode", _mode);
        gradient_->show();

        baseLayout_ = new QHBoxLayout();
        baseLayout_->setContentsMargins(getMargin(isFullScreen()), getMargin(isFullScreen()), getMargin(isFullScreen()), getMargin(isFullScreen()));
        baseLayout_->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

        {
            playLayout_ = new QVBoxLayout();
            playLayout_->setContentsMargins(0, getControlPanelSoundHeight(isFullScreen()), 0, 0);
            playLayout_->setSpacing(0);
            playLayout_->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

            playButton_ = new QPushButton(this);
            playButton_->setObjectName(qsl("VideoPlayerPlayButton"));
            playButton_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
            playButton_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));
            playButton_->setCursor(Qt::PointingHandCursor);
            playButton_->setCheckable(true);

            playLayout_->addWidget(playButton_);
            baseLayout_->addLayout(playLayout_);
        }

        {
            progressSliderLayout_ = new QVBoxLayout();
            progressSliderLayout_->setContentsMargins(0, getControlPanelSoundHeight(false), 0, 0);
            progressSliderLayout_->setSpacing(0);
            progressSliderLayout_->setAlignment(Qt::AlignBottom);

            progressSlider_ = new InstChangedSlider(Qt::Orientation::Horizontal, this);
            progressSlider_->setObjectName(qsl("VideoProgressSlider"));
            progressSlider_->setProperty("mode", _mode);
            progressSlider_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
            progressSlider_->setCursor(Qt::PointingHandCursor);
            progressSliderLayout_->addWidget(progressSlider_);
            baseLayout_->addLayout(progressSliderLayout_);
        }

        {
            timeLayout_ = new QVBoxLayout();
            timeLayout_->setContentsMargins(
                getTimeRightMargin(isFullScreen()),
                getControlPanelSoundHeight(isFullScreen()),
                getTimeRightMargin(isFullScreen()),
                getTimeBottomMargin(isFullScreen()));

            timeLayout_->setSpacing(0);
            timeLayout_->setAlignment(Qt::AlignBottom);

            timeRight_ = new QLabel(this);
            timeRight_->setObjectName(qsl("VideoTimeProgress"));
            timeRight_->setProperty("mode", _mode);
            timeRight_->setText(qsl("0:00"));

            timeLayout_->addWidget(timeRight_);
            baseLayout_->addLayout(timeLayout_);
        }

        {
            soundWidget_ = new QWidget(this);
            soundWidget_->setFixedHeight(getSoundsWidgetHeight(isFullScreen()));
            soundWidget_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));

            QVBoxLayout* soundLayout = new QVBoxLayout();
            soundLayout->setContentsMargins(0, 0, 0, 0);
            soundLayout->setSpacing(0);
            soundLayout->setAlignment(Qt::AlignBottom);

            soundWidget_->setLayout(soundLayout);
            soundWidget_->installEventFilter(this);
            {
                auto dummySoundButtonWidget = new QWidget(this); // needed to override cursor when button is disabled
                dummySoundButtonWidget->setCursor(Qt::ArrowCursor);
                dummySoundButtonWidget->setFixedSize({getControlPanelButtonSize(isFullScreen()), getControlPanelButtonSize(isFullScreen())});
                auto dummySoundButtonLayout = Utils::emptyHLayout(dummySoundButtonWidget);

                soundButton_ = new CustomButton(this);
                soundButton_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
                soundButton_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));
                soundButton_->setDisabledImage(qsl(":/videoplayer/no_sound"), getSoundButtonDefaultColor(), getSoundButtonIconSize());
                soundButton_->setCursor(Qt::PointingHandCursor);
                soundButton_->installEventFilter(this);
                dummySoundButtonLayout->addWidget(soundButton_);

                volumeContainer_ = new QWidget(this);
                volumeContainer_->setFixedWidth(getVolumeSliderWidth(isFullScreen()));
                soundLayout->addWidget(volumeContainer_);
                {
                    QHBoxLayout* volumeLayout = new QHBoxLayout();
                    volumeLayout->setContentsMargins(0, 0, 0, 0);
                    volumeLayout->setSpacing(0);
                    volumeContainer_->setLayout(volumeLayout);

                    volumeSlider_ = new InstChangedSlider(Qt::Orientation::Vertical, this);
                    volumeSlider_->setOrientation(Qt::Orientation::Vertical);
                    volumeSlider_->setObjectName(qsl("VideoVolumeSlider"));
                    volumeSlider_->setProperty("mode", _mode);
                    volumeSlider_->setMinimum(0);
                    volumeSlider_->setMaximum(100);
                    volumeSlider_->setPageStep(10);
                    volumeSlider_->setFixedHeight(getVolumeSliderHeight(isFullScreen()));
                    volumeSlider_->setCursor(Qt::PointingHandCursor);
                    volumeSlider_->setSliderPosition(volume);
                    volumeSlider_->setFixedWidth(getVolumeSliderWidth(isFullScreen()));
                    volumeSlider_->hide();
                    volumeLayout->addWidget(volumeSlider_);
                }

                soundButtonVolumeSpacer_ = new QWidget();
                soundButtonVolumeSpacer_->setFixedHeight(getSoundButtonVolumeSpacerHeight(isFullScreen()));
                soundLayout->addWidget(soundButtonVolumeSpacer_);
                soundLayout->addWidget(dummySoundButtonWidget);
            }
            baseLayout_->addWidget(soundWidget_);
        }

        {
            fullScreenLayout_ = new QVBoxLayout();
            fullScreenLayout_->setSpacing(0);
            fullScreenLayout_->setContentsMargins(0, getControlPanelSoundHeight(false), 0, 0);
            fullScreenLayout_->setAlignment(Qt::AlignBottom);

            fullscreenButton_ = new QPushButton(this);
            fullscreenButton_->setObjectName(qsl("VideoPlayerFullscreenButton"));
            fullscreenButton_->setFixedHeight(getControlPanelButtonSize(false));
            fullscreenButton_->setFixedWidth(getControlPanelButtonSize(false));
            fullscreenButton_->setCursor(Qt::PointingHandCursor);
            fullscreenButton_->setVisible(!isFullScreen());

            normalscreenButton_ = new QPushButton(this);
            normalscreenButton_->setObjectName(qsl("VideoPlayerNormalscreenButton"));
            normalscreenButton_->setFixedHeight(getControlPanelButtonSize(false));
            normalscreenButton_->setFixedWidth(getControlPanelButtonSize(false));
            normalscreenButton_->setCursor(Qt::PointingHandCursor);
            normalscreenButton_->setVisible(isFullScreen());

            fullScreenLayout_->addWidget(fullscreenButton_);
            fullScreenLayout_->addWidget(normalscreenButton_);

            baseLayout_->addLayout(fullScreenLayout_);
        }

        rootLayout->addLayout(baseLayout_);

        setLayout(rootLayout);

        connectSignals(_player);

        setMouseTracking(true);

        setSoundMode(lastSoundMode_);
        setGotAudio(gotAudio_);

        if (_copyFrom)
        {
            videoDurationChanged(duration_);
            videoPositionChanged(_copyFrom->progressSlider_->value());
            playButton_->setChecked(_copyFrom->playButton_->isChecked());
            volumeSlider_->setSliderPosition(_copyFrom->volumeSlider_->sliderPosition());


            connect(progressSlider_, &QSlider::sliderMoved, _copyFrom->progressSlider_, [_copyFrom](int _value)
            {
                _copyFrom->videoPositionChanged(_value);
            });
        }
    }

    VideoPlayerControlPanel::~VideoPlayerControlPanel()
    {
    }

    void VideoPlayerControlPanel::showVolumeControl(const bool _isShow)
    {
        volumeSlider_->setVisible(_isShow);
    }

    void VideoPlayerControlPanel::paintEvent(QPaintEvent* _e)
    {
        QStyleOption style_option;
        style_option.init(this);

        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &style_option, &p, this);

        if (isSeparateWindowMode())
            p.fillRect(rect(), QColor(0,0,0,1)); // hack to accept mouse events in transparent window

        return QWidget::paintEvent(_e);
    }

    bool VideoPlayerControlPanel::eventFilter(QObject* _obj, QEvent* _event)
    {
        QObject* objectSoundButton = qobject_cast<QObject*>(soundButton_);
        QObject* objectSoundWidget = qobject_cast<QObject*>(soundWidget_);

        if (objectSoundButton == _obj && QEvent::Enter == _event->type() && soundButton_->isEnabled())
        {
            showVolumeControl(true);
        }
        else if (objectSoundWidget == _obj && QEvent::Leave == _event->type())
        {
            showVolumeControl(false);
        }

        return QObject::eventFilter(_obj, _event);
    }

    void VideoPlayerControlPanel::mousePressEvent(QMouseEvent* _event)
    {
        if (isSeparateWindowMode() && _event->button() == Qt::LeftButton)
        {
            if (!isPause())
            {
                player_->pause();
                player_->setPausedByUser(true);
            }
            else
            {
                player_->play(true /* _init */);
                player_->setPausedByUser(false);
            }
        }

        QWidget::mousePressEvent(_event);
    }

    void VideoPlayerControlPanel::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::RightButton && rect().contains(_event->pos()))
            Q_EMIT mouseRightClicked();

        QWidget::mouseReleaseEvent(_event);
    }

    QString getDurationString(const qint64& _duration, const qint64& _init_duration)
    {
        constexpr qint64 one_hour = (1000 * 60 * 60);
        constexpr qint64 one_minute = (1000 * 60);
        constexpr qint64 one_sec = (1000);

        qint64 hours = _duration / one_hour;
        qint64 minutes = (_duration - hours * one_hour) / one_minute;
        qint64 seconds = (_duration - hours * one_hour - minutes * one_minute) / one_sec;

        qint64 duration_hours = _init_duration / one_hour;
        qint64 duration_minutes = (_init_duration - duration_hours * one_hour) / one_minute;

        const QString hourString = duration_hours != 0 ? QString::asprintf("%02d:", (int)hours) : QString();
        const QString minutesString = duration_minutes >= 10 ? QString::asprintf("%02d:", (int)minutes) : QString::asprintf("%01d:", (int)minutes);
        const QString secondsString = QString::asprintf("%02d", (int) seconds);

        return hourString % minutesString % secondsString;
    }

    QString getZeroTime(const qint64& _init_duration)
    {
        constexpr qint64 one_hour = (1000 * 60 * 60);
        constexpr qint64 one_minute = (1000 * 60);

        qint64 duration_hours = _init_duration / one_hour;
        qint64 duration_minutes = (_init_duration - duration_hours * one_hour) / one_minute;

        const QString hourString = duration_hours != 0 ? QString::asprintf("%02d:", 0) : QString();
        const QString minutesString = duration_minutes >= 10 ? QString::asprintf("%02d:", 0) : QString::asprintf("%01d:", 0);
        const QString secondsString = QString::asprintf("%02d", 0);

        return hourString % minutesString % secondsString;
    }

    void VideoPlayerControlPanel::resizeEvent(QResizeEvent* _event)
    {
        const auto button_size = getControlPanelButtonSize(isFullscreen());

        progressSlider_->setFixedHeight(getControlPanelButtonSize(isFullscreen()));

        volumeContainer_->setFixedWidth(getVolumeSliderWidth(isFullscreen()));

        volumeSlider_->setFixedHeight(getVolumeSliderHeight(isFullscreen()));
        volumeSlider_->setFixedWidth(getVolumeSliderWidth(isFullscreen()));

        auto font = Fonts::appFontScaled(isFullscreen() ? 18 : 14, Fonts::FontWeight::SemiBold);
        QFontMetrics m(font);

        timeRight_->setVisible(_event->size().width() >= Utils::scale_value(200));
        progressSlider_->setVisible(_event->size().width() >= Utils::scale_value(96));

        if (_event->size().width() < Utils::scale_value(68))
        {
            fullscreenButton_->setFixedWidth(0);
            normalscreenButton_->setFixedWidth(0);
        }
        else
        {
            fullscreenButton_->setFixedSize(button_size, button_size);
            normalscreenButton_->setFixedSize(button_size, button_size);
        }

        if (_event->size().width() < Utils::scale_value(48))
        {
            soundWidget_->setFixedWidth(0);
        }
        else
        {
            soundWidget_->setFixedHeight(getSoundsWidgetHeight(isFullscreen()));
            soundWidget_->setFixedWidth(getControlPanelButtonSize(isFullscreen()));

            soundButton_->setFixedHeight(getControlPanelButtonSize(isFullscreen()));
            soundButton_->setFixedWidth(getControlPanelButtonSize(isFullscreen()));
        }

        playButton_->setFixedSize(button_size, button_size);

        const QString mode = isFullscreen() ? qsl("full_screen") : qsl("dialog");

        timeRight_->setProperty("mode", mode);
        progressSlider_->setProperty("mode", mode);
        volumeSlider_->setProperty("mode", mode);

        const int gradientHeight = isFullscreen() ? Utils::scale_value(64) : Utils::scale_value(40);
        const int gradientWidth = _event->size().width();
        gradient_->setFixedSize(gradientWidth, gradientHeight);
        gradient_->move(QPoint(0, _event->size().height() - gradientHeight));
    }

    void VideoPlayerControlPanel::videoDurationChanged(const qint64 _duration)
    {
        duration_ = _duration;

        progressSlider_->setMinimum(0);
        progressSlider_->setMaximum((int) _duration);
        progressSlider_->setPageStep(_duration/10);

        timeRight_->setText(getDurationString(_duration, duration_));

        auto font = Fonts::appFontScaled((isFullscreen() ? 18 : 14), Fonts::FontWeight::SemiBold);
        QFontMetrics m(font);
        timeRight_->setFixedWidth(m.horizontalAdvance(getZeroTime(duration_)));
    }

    void VideoPlayerControlPanel::videoPositionChanged(const qint64 _position)
    {
        position_ = _position;

        if (!progressSlider_->isSliderDown())
            progressSlider_->setSliderPosition((int) _position);

        timeRight_->setText(getDurationString(duration_ - position_, duration_));
    }

    void VideoPlayerControlPanel::connectSignals(FFMpegPlayer* _player)
    {
        connect(_player, &FFMpegPlayer::durationChanged, this, &VideoPlayerControlPanel::videoDurationChanged);
        connect(_player, &FFMpegPlayer::positionChanged, this, &VideoPlayerControlPanel::videoPositionChanged);
        connect(_player, &FFMpegPlayer::paused, this, &VideoPlayerControlPanel::playerPaused);
        connect(_player, &FFMpegPlayer::played, this, &VideoPlayerControlPanel::playerPlayed);

        connect(progressSlider_, &QSlider::sliderMoved, this, [this](int /*_value*/)
        {
            player_->setPosition(progressSlider_->value());
        });


        connect(playButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            if (_checked)
            {
                player_->pause();
                player_->setPausedByUser(true);
            }
            else
            {
                player_->play(true /* _init */);
                player_->setPausedByUser(false);
            }

            Q_EMIT playClicked(_checked);
        });

        connect(volumeSlider_, &QSlider::sliderMoved, this, [this](int _value)
        {
            setVolume(_value, false);
        });

        connect(volumeSlider_, &QSlider::sliderReleased, this, [this]()
        {
            auto pos = volumeSlider_->sliderPosition();

            if (pos != 0)
            {
                player_->setRestoreVolume(pos);

                Ui::get_gui_settings()->set_value<int32_t>(setting_mplayer_volume, pos);
            }
        });


        connect(soundButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            if (volumeSlider_->sliderPosition() == 0)
                restoreVolume();
            else
                setVolume(0, false);

            soundOnByUser_ = true;
        });

        connect(fullscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            const QString mode = property("mode").toString();

            if (mode != u"dialog")
            {
                fullscreenButton_->setVisible(false);
                normalscreenButton_->setVisible(true);
            }

            Q_EMIT signalFullscreen(true);
        });

        connect(normalscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            const QString mode = property("mode").toString();

            if (mode != u"dialog")
            {
                normalscreenButton_->setVisible(false);
                fullscreenButton_->setVisible(true);
            }

            Q_EMIT signalFullscreen(false);
        });

        connect(_player, &FFMpegPlayer::mediaFinished, this, [this]()
        {
            playButton_->setChecked(true);
            progressSlider_->setValue(0);
        });
    }

    void VideoPlayerControlPanel::playerPaused()
    {
        if (playButton_->isChecked())
            return;

        playButton_->setChecked(true);
    }

    void VideoPlayerControlPanel::playerPlayed()
    {
        if (!playButton_->isChecked())
            return;

        playButton_->setChecked(false);
    }

    bool VideoPlayerControlPanel::isFullscreen() const
    {
        return fullscreen_;
    }

    bool VideoPlayerControlPanel::isPause() const
    {
        return playButton_->isChecked();
    }

    void VideoPlayerControlPanel::setPause(const bool& _pause)
    {
        playButton_->setChecked(_pause);
    }

    VideoPlayerControlPanel::SoundMode VideoPlayerControlPanel::getSoundMode(const int32_t _volume)
    {
        if (_volume == 0)
            return SoundMode::ActiveOff;
        else if (_volume < 50)
            return SoundMode::ActiveLow;
        else
            return SoundMode::ActiveHigh;
    }

    void VideoPlayerControlPanel::setVolume(const int32_t _value, const bool _toRestore)
    {
        player_->setVolume(_value);

        player_->setMute(_value == 0);

        volumeSlider_->setValue(_value);

        if (_toRestore)
        {
            player_->setRestoreVolume(_value);

            Ui::get_gui_settings()->set_value<int32_t>(setting_mplayer_volume, _value);
        }

        updateSoundButtonState();
    }

    int32_t VideoPlayerControlPanel::getVolume() const
    {
        return player_->getVolume();
    }

    void VideoPlayerControlPanel::restoreVolume()
    {
        volumeSlider_->setValue(player_->getRestoreVolume());

        player_->setVolume(player_->getRestoreVolume());

        player_->setMute(false);

        updateSoundButtonState();
    }

    void VideoPlayerControlPanel::updateVolume(const bool _hovered)
    {
        if (!VideoPlayerControlPanel::isFullscreen() && !soundOnByUser_ && player_->getRestoreVolume() && get_gui_settings()->get_value<bool>(settings_hoversound_video, settings_hoversound_video_default()))
            setVolume(_hovered ? player_->getRestoreVolume() : 0, _hovered);
    }

    void VideoPlayerControlPanel::setGotAudio(bool _gotAudio)
    {
        gotAudio_ = _gotAudio;
        soundButton_->setDisabled(!_gotAudio);
        soundButton_->setCursor(_gotAudio ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void VideoPlayerControlPanel::setSeparateWindowMode()
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setCursor(Qt::PointingHandCursor);
        separateWindow_ = true;
    }

    bool VideoPlayerControlPanel::isSeparateWindowMode() const
    {
        return separateWindow_;
    }

    bool VideoPlayerControlPanel::isOverControls(const QPoint& _pos) const
    {
        auto controlsRect = geometry();
        controlsRect.setTop(controlsRect.bottom() - getControlPanelMaxHeight(fullscreen_));
        return controlsRect.contains(_pos);
    }

    void VideoPlayerControlPanel::updateSoundButtonState()
    {
        auto newSoundMode = getSoundMode(player_->isMute() ? 0 : player_->getVolume());
        if (newSoundMode != lastSoundMode_)
        {
            lastSoundMode_ = newSoundMode;
            setSoundMode(newSoundMode);
        }
    }

    void VideoPlayerControlPanel::setSoundMode(VideoPlayerControlPanel::SoundMode _mode)
    {
        static auto color = getSoundButtonDefaultColor();
        static auto size = getSoundButtonIconSize();

        switch (_mode)
        {
            case SoundMode::ActiveOff:
                soundButton_->setDefaultImage(qsl(":/videoplayer/off_sound"), color, size);
                break;
            case SoundMode::ActiveLow:
                soundButton_->setDefaultImage(qsl(":/videoplayer/middle_sound"), color, size);
                break;
            case SoundMode::ActiveHigh:
                soundButton_->setDefaultImage(qsl(":/videoplayer/max_sound"), color, size);
                break;
        }

        soundButton_->setHoverColor(Qt::white);
    }

    //////////////////////////////////////////////////////////////////////////
    // DialogPlayer
    //////////////////////////////////////////////////////////////////////////
    DialogPlayer::DialogPlayer(QWidget* _parent, const uint32_t _flags, const QPixmap& _preview)
        : QWidget(_parent)
        , attachedPlayer_(nullptr)
        , isLottie_(_flags & DialogPlayer::Flags::lottie)
        , isLottieInstantPreview_(_flags & DialogPlayer::Flags::lottie_instant_preview)
        , showControlPanel_(_flags & DialogPlayer::Flags::enable_control_panel)
        , isGalleryView_(false)
        , preview_(_preview)
    {
        if (DialogPlayer::Flags::as_window & _flags)
        {
            if constexpr (platform::is_linux())
                setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
            else
                setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

#if defined(_WIN32)
            winId();
            QWindowsWindowFunctions::setHasBorderInFullScreen(windowHandle(), true);
#endif // WIN

            setCursor(Qt::PointingHandCursor);
        }

        renderer_ = createRenderer(this, _flags & DialogPlayer::Flags::gpu_renderer, _flags & DialogPlayer::Flags::dialog_mode);
        renderer_->updateFrame(_preview.toImage());
        ffplayer_ = new FFMpegPlayer(this, renderer_, false);

        connect(ffplayer_, &FFMpegPlayer::mediaFinished, this, [this]
        {
            if (!replay_)
                setPaused(true, true);

            if (dialogControls_)
                dialogControls_->setPosition(0);
        });

        if (GetAppConfig().WatchGuiMemoryEnabled())
            FFmpegPlayerMemMonitor::instance().watchFFmpegPlayer(ffplayer_);

        init(_parent, (_flags &DialogPlayer::Flags::is_gif), (_flags &DialogPlayer::Flags::is_sticker));

        isFullScreen_ = DialogPlayer::Flags::as_window & _flags;

        rootLayout_->addWidget(renderer_->getWidget());

        const auto isDialogMode = _flags & DialogPlayer::Flags::dialog_mode;

        if (showControlPanel_ || isDialogMode)
        {
            initControlPanel(nullptr, (_flags & DialogPlayer::Flags::as_window) ? qsl("window") : qsl("dialog"));
            controlPanel_->hide();
            if (_flags & DialogPlayer::Flags::gpu_renderer && useMetalRenderer())
                controlPanel_->setSeparateWindowMode();
        }

        if (isDialogMode)
            dialogControls_ = createControls(_flags);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, [this]()
        {
            setPaused(true, false);
        });
    }

    DialogPlayer::DialogPlayer(DialogPlayer* _attached, QWidget* _parent)
        : QWidget(_parent)
        , ffplayer_(_attached->ffplayer_)
        , attachedPlayer_(_attached)
        , mediaPath_(_attached->mediaPath_)
        , isGif_(_attached->isGif_)
        , isLottie_(_attached->isLottie_)
        , isLottieInstantPreview_(_attached->isLottieInstantPreview_)
        , isSticker_(_attached->isSticker_)
        , showControlPanel_(true)
    {

        if (platform::is_linux())
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
        else
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        init(_parent, isGif_, isSticker_);

        ffplayer_->setParent(nullptr);
        renderer_ = createRenderer(this, true /*gpu*/, false /*dialog mode*/);
        ffplayer_->setRenderer(renderer_);

        rootLayout_->addWidget(renderer_->getWidget());
        isFullScreen_ = true;

#if defined(_WIN32)
        winId();
        QWindowsWindowFunctions::setHasBorderInFullScreen(windowHandle(), true);
#endif // WIN

        initControlPanel(_attached->controlPanel_.get(), qsl("window"));
        if (useMetalRenderer())
            controlPanel_->setSeparateWindowMode();

        attachedPlayer_->setAttachedPlayer(this);

        connect(qApp, &QApplication::aboutToQuit, this, [this]() // To fix crash in case of exiting app from fullscreen gallery by the hotkey
        {                                                        // when video is playing
            attachedPlayer_ = nullptr;
        });
    }

    DialogPlayer::~DialogPlayer()
    {
        if (attachedPlayer_)
        {
            ffplayer_->setParent(attachedPlayer_);
            attachedPlayer_->setVolume(getVolume(), false);
            attachedPlayer_->setAttachedPlayer(nullptr);
            attachedPlayer_->renderer_->updateFrame(renderer_->getActiveImage());
        }
    }

    void DialogPlayer::init(QWidget* _parent, const bool _isGif, const bool _isSticker)
    {
        controlsShowed_ = false;

        parent_ = _parent;
        isLoad_ = false;
        isGif_ = _isGif;
        isSticker_ = _isSticker;
        soundOnByUser_ = false;

        rootLayout_ = Utils::emptyVLayout(this);

        connect(ffplayer_, &FFMpegPlayer::mouseMoved, this, &DialogPlayer::playerMouseMoved);
        connect(ffplayer_, &FFMpegPlayer::mouseLeaveEvent, this, &DialogPlayer::playerMouseLeaved);
        connect(ffplayer_, &FFMpegPlayer::fileLoaded, this, &DialogPlayer::onLoaded);
        connect(ffplayer_, &FFMpegPlayer::firstFrameReady, this, &DialogPlayer::onFirstFrameReady);
        setMouseTracking(true);

        installEventFilter(this);

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::galeryClosed, this, [this]()
        {
            isGalleryView_ = false;
        });
    }

    void DialogPlayer::moveToScreen()
    {
        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();

        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        setGeometry(screenGeometry);
    }

    void DialogPlayer::paintEvent(QPaintEvent* _e)
    {
        QStyleOption style_option;
        style_option.init(this);

        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &style_option, &p, this);
    }

    bool DialogPlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        auto updateVolume = [this](bool _hovered)
        {
            if (controlPanel_ && !isGalleryView_ && !isFullScreen_ && !soundOnByUser_ && !isGif_ && !isLottie_)
            {
                controlPanel_->updateVolume(_hovered);
                if (state() == QMovie::MovieState::Running)
                    setMute(controlPanel_->getVolume() == 0);
            }
        };

        const auto objIsPanel = _obj && _obj == qobject_cast<QObject*>(controlPanel_.get());

        switch (_event->type())
        {
            case QEvent::Enter:
            {
                updateVolume(true);
                if (objIsPanel && timerHide_)
                    timerHide_->stop();
                break;
            }
            case QEvent::Leave:
            {
                updateVolume(false);
                if (objIsPanel)
                    startTimerHide(hideTimeoutShort());
                break;
            }
            case QEvent::MouseMove:
            {
                if (objIsPanel && controlPanel_ && controlPanel_->isSeparateWindowMode())
                {
                    QMouseEvent* me = static_cast<QMouseEvent*>(_event);
                    if (controlPanel_->isOverControls(me->pos()))
                    {
                        if (timerHide_)
                            timerHide_->stop();
                    }
                    else
                    {
                        startTimerHide(hideTimeout());
                    }
                }
                break;
            }

            case QEvent::KeyPress:
            {
                QKeyEvent *ke = static_cast<QKeyEvent *>(_event);

                if (ke->key() == Qt::Key_Escape)
                {
                    changeFullScreen();
                }

                return true;
            }
            default:
                break;
        }

        return QObject::eventFilter(_obj, _event);
    }

    void DialogPlayer::timerHide()
    {
        showControlPanel(false);
        controlsShowed_ = false;
    }

    void DialogPlayer::showControlPanel(const bool _isShow)
    {
        if (!showControlPanel_)
            return;

        im_assert(controlPanel_);

        updateControlsGeometry();

        if (_isShow)
        {
            controlPanel_->show();
            controlPanel_->raise();
            if constexpr (platform::is_apple())
            {
                // Fix the toast not receiving mouse events
                ToastManager::instance()->raiseBottomToastIfPresent();
            }
        }
        else
        {
            controlPanel_->hide();
        }
    }

    void DialogPlayer::playerMouseMoved()
    {
        showControlPanel();
    }

    void DialogPlayer::updateControlsGeometry()
    {
        if (!controlPanel_)
            return;

        QRect panelGeom;
        if (controlPanel_->isSeparateWindowMode())
        {
            panelGeom = geometry();
        }
        else
        {
            panelGeom = rect();
            panelGeom.setTop(panelGeom.bottom() - getControlPanelMaxHeight(controlPanel_->isFullscreen()));
        }

        controlPanel_->setGeometry(panelGeom);
    }

    void DialogPlayer::showControlPanel()
    {
        if (isGif_ || isLottie_)
            return;

        if (controlsShowed_)
            return;

        startTimerHide(hideTimeout());

        controlsShowed_ = true;
        showControlPanel(true);
    }

    void DialogPlayer::playerMouseLeaved()
    {
        startTimerHide(hideTimeoutShort());
    }

    bool DialogPlayer::openMedia(const QString& _mediaPath)
    {
        isLoad_ = true;
        mediaPath_ = _mediaPath;

        return ffplayer_->openMedia(_mediaPath, isLottie_, isLottieInstantPreview_);
    }

    void DialogPlayer::setMedia(const QString &_mediaPath)
    {
        mediaPath_ = _mediaPath;

        if (dialogControls_)
            dialogControls_->setState(MediaControls::Paused);

        updateVisibility(visible_);

        update();
    }

    void DialogPlayer::setPaused(const bool _paused, const bool _byUser)
    {
        if (!_paused)
        {
            start(true);
            setPausedByUser(false);
        }
        else
        {
            ffplayer_->pause();

            if (controlPanel_)
                controlPanel_->setPause(_paused);

            if (ffplayer_->getStarted())
                showControlPanel();

            setPausedByUser(_byUser);

            Q_EMIT paused();
        }

        if (dialogControls_ && !Utils::InterConnector::instance().isMultiselect())
            dialogControls_->setState(_paused ? MediaControls::Paused : MediaControls::Playing);
    }

    void DialogPlayer::setPausedByUser(const bool _paused)
    {
        ffplayer_->setPausedByUser(_paused);
    }

    bool DialogPlayer::isPausedByUser() const
    {
        return ffplayer_->isPausedByUser();
    }

    QMovie::MovieState DialogPlayer::state() const
    {
        return ffplayer_->state();
    }

    void DialogPlayer::setVolume(const int32_t _volume, bool _toRestore)
    {
        if (controlPanel_)
            controlPanel_->setVolume(_volume, _toRestore);
    }

    int32_t DialogPlayer::getVolume() const
    {
        return controlPanel_ ? controlPanel_->getVolume() : 0;
    }

    void DialogPlayer::setMute(bool _mute)
    {
        if (controlPanel_)
        {
            if (_mute)
                controlPanel_->setVolume(0, false);
            else
                controlPanel_->restoreVolume();
        }

        if (dialogControls_)
            dialogControls_->setMute(_mute);
    }

    void DialogPlayer::start(bool _start)
    {
        if (_start && ffplayer_->state() == QMovie::NotRunning)
            openMedia(mediaPath_);

        ffplayer_->play(_start);

        if (_start)
        {
            if (controlPanel_)
                controlPanel_->setPause(false);
            ffplayer_->setPausedByUser(false);

            if (dialogControls_)
                dialogControls_->setState(MediaControls::Playing);
        }
    }

    void DialogPlayer::updateSize(const QRect& _sz)
    {
        setGeometry(_sz);

        showControlPanel(controlsShowed_);

        if (dialogControls_)
            dialogControls_->setRect(QRect({0, 0}, _sz.size()));
    }

    void DialogPlayer::changeFullScreen()
    {
        bool isFullScreen = controlPanel_ && controlPanel_->isFullscreen();
        fullScreen(!isFullScreen);
    }

    void DialogPlayer::initControlPanel(VideoPlayerControlPanel* _copyFrom, const QString& _mode)
    {
        controlPanel_ = std::make_unique<Ui::VideoPlayerControlPanel>(_copyFrom, this, ffplayer_, _mode);
        controlPanel_->installEventFilter(this);

        connect(controlPanel_.get(), &VideoPlayerControlPanel::signalFullscreen, this, &DialogPlayer::fullScreen);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::playClicked, this, &DialogPlayer::playClicked);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::mouseRightClicked, this, &DialogPlayer::timerHide);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::mouseRightClicked, this, &DialogPlayer::mouseRightClicked);
    }

    void DialogPlayer::initTimerHide()
    {
        if (timerHide_)
            return;

        timerHide_ = new QTimer(this);
        timerHide_->setSingleShot(true);
        connect(timerHide_, &QTimer::timeout, this, &DialogPlayer::timerHide);
    }

    void DialogPlayer::startTimerHide(std::chrono::milliseconds _timeout)
    {
        initTimerHide();
        timerHide_->start(_timeout);
    }

    void DialogPlayer::startUnloadTimer()
    {
        if (!unloadTimer_)
        {
            unloadTimer_ = new QTimer(this);
            unloadTimer_->setSingleShot(true);
            unloadTimer_->setInterval(isLottie_ ? unloadTimeoutLottie() : unloadTimeout());
            connect(unloadTimer_, &QTimer::timeout, this, &DialogPlayer::unloadMedia);
        }
        unloadTimer_->start();
    }

    void DialogPlayer::stopUnloadTimer()
    {
        if (unloadTimer_)
            unloadTimer_->stop();
    }

    bool DialogPlayer::isAutoPlay()
    {
        if (isSticker_ || isLottie_)
            return true;
        else if (isGif_)
            return get_gui_settings()->get_value(settings_autoplay_gif, true);
        else
            return get_gui_settings()->get_value(settings_autoplay_video, true);
    }

    MediaControls* DialogPlayer::createControls(const uint32_t _flags)
    {
        std::set<MediaControls::ControlType> controls;
        controls.insert(MediaControls::Play);
        controls.insert(MediaControls::Fullscreen);

        if (_flags & Flags::is_gif)
        {
            controls.insert(MediaControls::GifLabel);
        }
        else
        {
            controls.insert(MediaControls::Duration);
            controls.insert(MediaControls::Mute);
        }

        uint32_t options = MediaControls::PlayClickable;

        if (_flags & Flags::compact_mode)
            options |= MediaControls::CompactMode;

        if (_flags & Flags::short_no_sound)
            options |= MediaControls::ShortNoSound;

        auto dialogControls = new MediaControls(controls, options, this);

        connect(dialogControls, &MediaControls::fullscreen, this, [this]()
        {
            isGalleryView_ = !isGalleryView_;
            Q_EMIT openGallery();
        });
        connect(dialogControls, &MediaControls::mute, this, &DialogPlayer::setMute);
        connect(dialogControls, &MediaControls::mute, this, [this]() { soundOnByUser_ = true; });
        connect(dialogControls, &MediaControls::clicked, this, &DialogPlayer::mouseClicked);
        connect(ffplayer_, &FFMpegPlayer::positionChanged, this, [this, dialogControls](qint64 _position)
        {
            if (!attachedPlayer_)
                dialogControls->setPosition(_position / 1000);
        });
        connect(dialogControls, &MediaControls::play, this, [this](bool _enable) { setPaused(_enable, true); });
        connect(dialogControls, &MediaControls::copyLink, this, &DialogPlayer::copyLink);

        dialogControls->raise();

        return dialogControls;
    }

    void DialogPlayer::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            if (rect().contains(_event->pos()))
            {
                if (isFullScreen())
                {
                    const auto paused = controlPanel_ ? controlPanel_->isPause() : false;
                    setPaused(!paused, true);
                    Q_EMIT playClicked(!paused);
                }

                if (!isGif_ && !isLottie_)
                    Q_EMIT mouseClicked();
            }

        }
        else if (_event->button() == Qt::RightButton)
        {
            if (rect().contains(_event->pos()))
                Q_EMIT mouseRightClicked();
        }
        QWidget::mouseReleaseEvent(_event);
    }

    void DialogPlayer::mouseDoubleClickEvent(QMouseEvent *_event)
    {
        QWidget::mouseDoubleClickEvent(_event);

        if (_event->button() == Qt::LeftButton)
            Q_EMIT mouseDoubleClicked();
    }

    void DialogPlayer::showAsFullscreen()
    {
        normalModePosition_ = geometry();

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();

        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        if constexpr (!platform::is_apple())
            hide();

        updateSize(screenGeometry);

        if constexpr (platform::is_windows() || platform::is_linux())
            showFullScreen();

        showControlPanel(controlsShowed_);

        show();
    }

    void DialogPlayer::showAs()
    {
        show();

#ifdef __APPLE__
        MacSupport::showOverAll(this);
#endif

        showControlPanel(controlsShowed_);
    }

    void DialogPlayer::showAsNormal()
    {
        showNormal();

#ifdef __APPLE__
        MacSupport::showOverAll(this);
#endif

        setGeometry(normalModePosition_);

        showControlPanel(controlsShowed_);
    }

    void DialogPlayer::fullScreen(const bool _checked)
    {
        Q_EMIT fullScreenClicked();
    }

    void DialogPlayer::moveToTop()
    {
        raise();
        if (controlPanel_)
            controlPanel_->raise();
    }

    bool DialogPlayer::isFullScreen() const
    {
        return isFullScreen_;
    }

    void DialogPlayer::setIsFullScreen(bool _isFullScreen)
    {
        isFullScreen_ = _isFullScreen;
    }

    bool DialogPlayer::inited()
    {
        return ffplayer_->getStarted();
    }

    void DialogPlayer::setPreview(QPixmap _preview)
    {
        ffplayer_->setPreview(_preview);
        preview_ = _preview;
    }

    void DialogPlayer::setPreview(QImage _preview)
    {
        preview_ = QPixmap::fromImage(_preview);
        ffplayer_->setPreview(preview_);
    }

    void DialogPlayer::setPreviewFromFirstFrame()
    {
        if (auto frame = getFirstFrame(); !frame.isNull())
            setPreview(std::move(frame));
    }

    QPixmap DialogPlayer::getActiveImage() const
    {
        return ffplayer_->getActiveImage();
    }

    void DialogPlayer::onLoaded()
    {
        Q_EMIT loaded();
    }

    void DialogPlayer::onFirstFrameReady()
    {
        Q_EMIT firstFrameReady();
    }

    void DialogPlayer::setLoadingState(bool _isLoad)
    {
        //qDebug() << "setLoadingState " << _isLoad << ", old: " << isLoad_ << ", " << mediaPath_;

        if (_isLoad == isLoad_)
            return;
        isLoad_ = _isLoad;

        if (isLoad_)
        {
            //qDebug() << "reload media " << mediaPath_;
            ffplayer_->openMedia(mediaPath_);
        }
        else
        {
            //qDebug() << "unload media " << mediaPath_;
            ffplayer_->stop();
        }
    }

    void DialogPlayer::unloadMedia()
    {
        ffplayer_->stop();

        if (!preview_.isNull())
            ffplayer_->setPreview(preview_);

        if (dialogControls_)
            dialogControls_->setPosition(0);
    }

    void DialogPlayer::setHost(QWidget* _host)
    {
        parent_ = _host;
    }

    bool DialogPlayer::isGif() const
    {
        return isGif_;
    }

    void DialogPlayer::setClippingPath(QPainterPath _clippingPath)
    {
        renderer_->setClippingPath(_clippingPath);
    }

    void DialogPlayer::setAttachedPlayer(DialogPlayer* _player)
    {
        attachedPlayer_ = _player;

        if (!_player)
        {
            ffplayer_->setParent(nullptr);
            ffplayer_->setReplay(replay_);
            ffplayer_->setRenderer(renderer_);
            if (dialogControls_)
                dialogControls_->raise();
        }
        else
        {
            stopUnloadTimer();
        }
    }

    DialogPlayer* DialogPlayer::getAttachedPlayer() const
    {
        return attachedPlayer_;
    }

    QSize DialogPlayer::getVideoSize() const
    {
        if (!ffplayer_)
            return QSize();

        return ffplayer_->getVideoSize();
    }

    void DialogPlayer::setReplay(bool _enable)
    {
        replay_ = _enable;

        if (ffplayer_)
            ffplayer_->setReplay(replay_);
    }

    void DialogPlayer::setFillClient(const bool _fill)
    {
        renderer_->setFillClient(_fill);
    }

    const QString& DialogPlayer::mediaPath() const noexcept
    {
        return mediaPath_;
    }

    void DialogPlayer::setDuration(int32_t _duration)
    {
        if (dialogControls_)
            dialogControls_->setDuration(_duration);
    }

    void DialogPlayer::setGotAudio(bool _gotAudio)
    {
        if (dialogControls_)
            dialogControls_->setGotAudio(_gotAudio);

        if (controlPanel_)
            controlPanel_->setGotAudio(_gotAudio);
    }

    void DialogPlayer::setSiteName(const QString& _siteName)
    {
        if (dialogControls_)
            dialogControls_->setSiteName(_siteName);
    }

    void DialogPlayer::setFavIcon(const QPixmap& _favicon)
    {
        if (dialogControls_)
            dialogControls_->setFavicon(_favicon);
    }

    void DialogPlayer::updateVisibility(bool _visible)
    {
        visible_ = _visible;

        if (attachedPlayer_ || mediaPath_.isEmpty() || (ffplayer_->state() == QMovie::NotRunning && !visible_))
            return;

        stopUnloadTimer();

        if (!_visible)
            startUnloadTimer();

        auto play = _visible && isAutoPlay() && !isPausedByUser();
        auto byUser = !play && isPausedByUser();

        setPaused(!play, byUser);
    }

    QWidget* DialogPlayer::getParentForContextMenu() const
    {
        return controlPanel_.get();
    }

    int DialogPlayer::bottomLeftControlsWidth() const
    {
        return dialogControls_ ? dialogControls_->bottomLeftControlsWidth() : 0;
    }

    QImage DialogPlayer::getFirstFrame() const
    {
        return ffplayer_->getFirstFrame();
    }

    void DialogPlayer::wheelEvent(QWheelEvent* _event)
    {
        Q_EMIT mouseWheelEvent(_event->angleDelta());

        QWidget::wheelEvent(_event);
    }

    void DialogPlayer::keyPressEvent(QKeyEvent* _event)
    {
        QWidget::keyPressEvent(_event);
    }

    void DialogPlayer::mouseMoveEvent(QMouseEvent* _event)
    {
        playerMouseMoved();
        QWidget::mouseMoveEvent(_event);
    }

    void DialogPlayer::resizeEvent(QResizeEvent* _event)
    {
        updateControlsGeometry();
        QWidget::resizeEvent(_event);
    }

    std::unique_ptr<FrameRenderer> DialogPlayer::createRenderer(QWidget* _parent, bool _useGPU, bool _dialogMode)
    {
        std::unique_ptr<FrameRenderer> renderer;

#if defined(__linux__)
        if (_dialogMode)
            renderer = std::make_unique<DialogRenderer>(_parent);
        else
            renderer = std::make_unique<GDIRenderer>(_parent);
#else
        if (_useGPU && (platform::is_apple() || platform::is_windows()))
        {
#ifdef __APPLE__
            if (useMetalRenderer())
                renderer = std::make_unique<MetalRenderer>(_parent);
            else
#endif
                if constexpr (platform::is_apple())
                {
                    renderer = std::make_unique<MacOpenGLRenderer>(_parent);
                }
                else
                {
                    auto checkOpenGL = []()
                    {
                        QOpenGLContext context;
                        return context.create();
                    };
                    static bool openGLSupported = checkOpenGL();

                    if (openGLSupported)
                        renderer = std::make_unique<OpenGLRenderer>(_parent);
                    else
                        renderer = std::make_unique<GDIRenderer>(_parent);
                }
        }
        else
        {
            if (_dialogMode)
                renderer = std::make_unique<DialogRenderer>(_parent);
            else
                renderer = std::make_unique<GDIRenderer>(_parent);

            if (_useGPU)
                renderer->setFillColor(Qt::black);
        }
#endif //__linux__ || __APPLE__

        return renderer;
    }
}

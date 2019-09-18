#include "stdafx.h"
#include "VideoPlayer.h"

#include "../../utils/utils.h"
#include "app_config.h"
#include "../../gui_settings.h"

#include "controls/CustomButton.h"

#include "../../utils/InterConnector.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../history_control/MessageStyle.h"

#include "FFMpegPlayer.h"
#include "../../fonts.h"

#include "../gui/memory_stats/FFmpegPlayerMemMonitor.h"

#include "previewer/Drawable.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#include "macos/MetalRenderer.h"
#endif


namespace
{
    QString durationStr(int32_t _duration)
    {
        QTime t(0, 0);
        t = t.addSecs(_duration);
        QString durationStr;
        if (t.hour())
            durationStr += QString::number(t.hour()) % ql1c(':');
        durationStr += QString::number(t.minute()) % ql1c(':');
        if (t.second() < 10)
            durationStr += ql1c('0');
        durationStr += QString::number(t.second());
        return durationStr;
    }
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

            emit sliderMoved(value);
            emit sliderReleased();
        }

        QSlider::mousePressEvent(_event);
    }

    void InstChangedSlider::wheelEvent(QWheelEvent* _event)
    {
        QSlider::wheelEvent(_event);

        emit sliderMoved(value());
        emit sliderReleased();
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
            positionSliderTimer_(new QTimer(this)),
            fullscreen_(_mode == ql1s("full_screen"))
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
        if (!_isShow)
        {
            volumeSlider_->hide();
        }
        else
        {
            volumeSlider_->show();
        }
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
        if (isSeparateWindowMode())
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

    QString getDurationString(const qint64& _duration, const qint64& _init_duration)
    {
        const qint64 one_hour = (1000 * 60 * 60);
        const qint64 one_minute = (1000 * 60);
        const qint64 one_sec = (1000);

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
        const qint64 one_hour = (1000 * 60 * 60);
        const qint64 one_minute = (1000 * 60);

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
        duration_ = _duration;;

        progressSlider_->setMinimum(0);
        progressSlider_->setMaximum((int) _duration);
        progressSlider_->setPageStep(_duration/10);

        timeRight_->setText(getDurationString(_duration, duration_));

        auto font = Fonts::appFontScaled((isFullscreen() ? 18 : 14), Fonts::FontWeight::SemiBold);
        QFontMetrics m(font);
        timeRight_->setFixedWidth(m.width(getZeroTime(duration_)));
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

            positionSliderTimer_->stop();
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

            emit playClicked(_checked);
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
            {
                restoreVolume();
            }
            else
            {
                setVolume(0, false);
            }
        });

        connect(fullscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            const QString mode = property("mode").toString();

            if (mode != ql1s("dialog"))
            {
                fullscreenButton_->setVisible(false);
                normalscreenButton_->setVisible(true);
            }

            emit signalFullscreen(true);
        });

        connect(normalscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            const QString mode = property("mode").toString();

            if (mode != ql1s("dialog"))
            {
                normalscreenButton_->setVisible(false);
                fullscreenButton_->setVisible(true);
            }

            emit signalFullscreen(false);
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
    // TextIconBase
    //////////////////////////////////////////////////////////////////////////

    class TextIconBase : public BDrawable
    {
    public:
        TextIconBase()
        {
            label_.setVerticalPosition(TextRendering::VerPosition::MIDDLE);
        }
        ~TextIconBase() = default;
        void draw(QPainter &_p) override;
        void setRect(const QRect& _rect) override;

        Label label_;
        Icon icon_;

    protected:
        virtual int iconLeftMargin() { return Utils::scale_value(6); }
        virtual int labelMargin() { return Utils::scale_value(4); }

        virtual void setLabelRect(const QRect& _rect) = 0;
    };

    void TextIconBase::draw(QPainter &_p)
    {
        if (!visible_)
            return;

        BDrawable::draw(_p);
        label_.draw(_p);
        icon_.draw(_p);
    }

    void TextIconBase::setRect(const QRect &_rect)
    {
        Drawable::setRect(_rect);

        auto iconSize = icon_.pixmapSize();
        auto iconRect = _rect;
        iconRect.setLeft(_rect.left() + iconLeftMargin());
        iconRect.setTop(_rect.top() + (_rect.height() - iconSize.height()) / 2);
        iconRect.setSize(iconSize);
        icon_.setRect(iconRect);

        setLabelRect(_rect);
    }

    //////////////////////////////////////////////////////////////////////////
    // StaticTextIcon
    //////////////////////////////////////////////////////////////////////////

    class StaticTextIcon : public TextIconBase
    {
    private:
        void setLabelRect(const QRect& _rect) override;
    };

    void StaticTextIcon::setLabelRect(const QRect& _rect)
    {
        auto iconRect = icon_.rect();
        auto yOffset = _rect.height() / 2;
        yOffset += 1;                       //!HACK until TextUnit fixed

        auto labelRect = _rect;
        labelRect.setLeft(iconRect.right() + labelMargin());
        label_.setYOffset(yOffset);
        label_.setRect(labelRect);
    }

    //////////////////////////////////////////////////////////////////////////
    // DynamicTextIcon
    //////////////////////////////////////////////////////////////////////////

    class DynamicTextIcon : public TextIconBase
    {
    private:
        void setLabelRect(const QRect& _rect) override;
    };

    void DynamicTextIcon::setLabelRect(const QRect& _rect)
    {
        auto textWidth = label_.desiredWidth();

        auto yOffset = _rect.height() / 2;
        yOffset += 1;                       //!HACK until TextUnit fixed

        auto labelRect = _rect;
        labelRect.setLeft(_rect.right());
        labelRect.setRight(_rect.right() + textWidth + labelMargin());
        label_.setYOffset(yOffset);
        label_.setRect(labelRect);

        auto rect = rect_;
        rect.setWidth(rect_.width() + labelRect.width());

        BDrawable::setRect(rect);
    }

    enum ControlType : short
    {
        Fullscreen,
        Mute,
        NoSound,
        Duration,
        EmptyDuration,
        GifLabel,
        Play
    };

    //////////////////////////////////////////////////////////////////////////
    // DialogControls_p
    //////////////////////////////////////////////////////////////////////////

    class DialogControls_p
    {
    public:

        enum AnimationDirection
        {
            None,
            Forward,
            Back
        };

        int commonMargin() const { return Ui::MessageStyle::getHiddenControlsShift(); }

        QColor defaultBackground() const { return QColor(0, 0, 0, static_cast<int>(0.5 * 255)); }

        QColor hoveredBackground() const { return QColor(0, 0, 0, static_cast<int>(0.6 * 255)); }

        QColor pressedBackground() const { return QColor(0, 0, 0, static_cast<int>(0.7 * 255)); }

        QSize objectSize(ControlType _type) const
        {
            switch (_type)
            {
                case Mute:
                case Fullscreen:
                case EmptyDuration:
                    return Utils::scale_value(QSize(28, 20));

                case NoSound: // no sound item takes rect for icon and expands it based on label size
                    return Utils::scale_value(QSize(26, 20));

                case Duration:
                    return Utils::scale_value(QSize(55, 20));

                case GifLabel:
                    return Utils::scale_value(QSize(36, 20));

                case Play:
                    return Utils::scale_value(QSize(48, 48));
            }

            return QSize();
        }

        QPoint objectPosition(ControlType _type, const QSize& _size) const
        {
            switch (_type)
            {
                case NoSound:
                    return {commonMargin(), _size.height() - objectSize(NoSound).height() - commonMargin()};
                case Mute:
                    return {commonMargin(), _size.height() - objectSize(Mute).height() - commonMargin()};
                case Fullscreen:
                    return {_size.width() - objectSize(Fullscreen).width() - commonMargin(), commonMargin()};

                case Duration:
                case GifLabel:
                case EmptyDuration:
                    return {commonMargin(), commonMargin()};

                case Play:
                    return {(_size.width() - objectSize(Play).width()) / 2, (_size.height() - objectSize(Play).height()) / 2};
            }

            return QPoint();
        }

        QPoint animationStartPosition(ControlType _type, const QSize& _size) const
        {
            switch (_type)
            {
                case Mute:
                    return {objectPosition(Mute, _size).x(), objectPosition(Mute, _size).y() + commonMargin()};
                case NoSound:
                    return {objectPosition(NoSound, _size).x(), objectPosition(NoSound, _size).y() + commonMargin()};
                case Fullscreen:
                    return {objectPosition(Fullscreen, _size).x(), objectPosition(Fullscreen, _size).y() - commonMargin()};

                case Duration:
                case GifLabel:
                case EmptyDuration:
                    return {commonMargin(), 0};

                case Play:
                    return objectPosition(Play, _size);
            }

            return QPoint();
        }

        QPoint animationPosition(ControlType _type, const QSize& _size, int _progress) const // progress is from 1 to 100
        {
            if (_progress == 100)
                return objectPosition(_type, _size);

            auto start = animationStartPosition(_type, _size);
            auto end = objectPosition(_type, _size);

            auto dx = QPoint(end - start).x() * _progress / 100;
            auto dy = QPoint(end - start).y() * _progress / 100;

            return start + QPoint(dx, dy);
        }

        int animationProgress(int _animationDuration, int _elapsedTime) const
        {
            int progress = static_cast<double>(_elapsedTime) / _animationDuration * 100;
            if (currentAnimation_ == Back)
                progress = 100 - progress;

            return progress;
        }

        void createDuration(int32_t _duration)
        {
            auto duration = std::make_unique<StaticTextIcon>();
            auto durationUnit = TextRendering::MakeTextUnit(durationStr(_duration));
            durationUnit->init(Fonts::appFontScaled(11, Fonts::FontWeight::Normal), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            duration->label_.setTextUnit(std::move(durationUnit));
            duration->icon_.setPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/duration_time_icon"), QSize(14, 14), Qt::white));
            duration->background_ = defaultBackground();
            duration->setBorderRadius(Utils::scale_value(10));

            objects_[Duration] = std::move(duration);
        }

        void createNoSound()
        {
            auto noSound = std::make_unique<DynamicTextIcon>();
            auto noSoundUnit = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("videoplayer", "No sound"));
            noSoundUnit->init(Fonts::appFontScaled(11, Fonts::FontWeight::Normal), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            noSound->label_.setTextUnit(std::move(noSoundUnit));
            noSound->icon_.setPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/mute_video_icon"), QSize(20, 20), QColor(255, 255, 255, 255 * 0.34)));
            noSound->background_ = defaultBackground();
            noSound->setBorderRadius(Utils::scale_value(10));

            objects_[NoSound] = std::move(noSound);
        }

        void createMute()
        {
            auto muteButton = std::make_unique<BButton>();
            muteButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/mute_video_icon"), QSize(20, 20), Qt::white));
            muteButton->setCheckable(true);
            muteButton->setCheckedPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/sound_on_icon"), QSize(20, 20), Qt::white));
            muteButton->setPixmapCentered(true);
            muteButton->setBorderRadius(Utils::scale_value(10));
            muteButton->background_ = defaultBackground();
            muteButton->hoveredBackground_ = hoveredBackground();
            muteButton->pressedBackground_ = pressedBackground();

            objects_[Mute] = std::move(muteButton);
        }

        void createObjects(bool _isGif, int32_t _duration)
        {
            auto fullscreenButton = std::make_unique<BButton>();
            fullscreenButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/full_screen_icon"), QSize(20, 20), Qt::white));
            fullscreenButton->setPixmapCentered(true);
            fullscreenButton->setBorderRadius(Utils::scale_value(10));
            fullscreenButton->background_ = defaultBackground();
            fullscreenButton->hoveredBackground_ = hoveredBackground();
            fullscreenButton->pressedBackground_ = pressedBackground();

            objects_[Fullscreen] = std::move(fullscreenButton);

            if (!_isGif)
            {
                createMute();

                if (_duration)
                {
                    createDuration(_duration);
                }
                else
                {
                    auto duration = std::make_unique<BButton>();
                    duration->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/duration_time_icon"), QSize(14, 14), Qt::white));
                    duration->setPixmapCentered(true);
                    duration->setBorderRadius(Utils::scale_value(10));
                    duration->background_ = defaultBackground();

                    objects_[EmptyDuration] = std::move(duration);
                }
            }
            else
            {
                auto gifLabel = std::make_unique<BLabel>();
                auto gifUnit = TextRendering::MakeTextUnit(qsl("GIF"));
                gifUnit->init(Fonts::appFontScaled(13, Fonts::FontWeight::SemiBold), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
                gifLabel->setTextUnit(std::move(gifUnit));
                gifLabel->background_ = defaultBackground();
                gifLabel->setBorderRadius(Utils::scale_value(10));
                gifLabel->setVerticalPosition(TextRendering::VerPosition::MIDDLE);
                gifLabel->setYOffset(objectSize(GifLabel).height() / 2);

                objects_[GifLabel] = std::move(gifLabel);
            }

            auto playButton = std::make_unique<BButton>();
            playButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/video_pause"), QSize(28, 28)));
            playButton->setCheckable(true);
            playButton->setCheckedPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/video_play"), QSize(28, 28)));
            playButton->setPixmapCentered(true);
            playButton->setBorderRadius(Utils::scale_value(24));
            playButton->background_ = defaultBackground();
            playButton->hoveredBackground_ = hoveredBackground();
            playButton->pressedBackground_ = pressedBackground();

            objects_[Play] = std::move(playButton);
        }

        QRect clickArea(ControlType _type)
        {
            switch (_type)
            {
                case Mute:
                case Fullscreen:
                    return objects_[_type]->rect().adjusted(-commonMargin(), -commonMargin(), commonMargin(), commonMargin());
                default:
                    return objects_[_type]->rect();
            }
        }

        void updateObjectsPositions(const QRect &_rect, int _animationProgress = 100)
        {
            for (auto & [type, object] : objects_)
            {
                QPoint position;

                if (currentlyAnimated_.count(type))
                    position = animationPosition(type, _rect.size(), _animationProgress);
                else
                    position = objectPosition(type, _rect.size());

                object->setRect(QRect(position, objectSize(type)));
            }
        }

        bool visibilityForCurrentState(ControlType _type, bool _mouseOver = true)
        {
            switch (state_)
            {
                case DialogControls::Preview:
                    switch (_type)
                    {
                        case Duration:
                        case GifLabel:
                        case EmptyDuration:
                            return true;
                        default:
                            return false;
                    }

                case DialogControls::Paused:
                    switch (_type)
                    {
                        case Mute:
                        case NoSound:
                        case Fullscreen:
                            return _mouseOver;
                        default:
                            return true;
                    }
                case DialogControls::Playing:
                    return _mouseOver;
            }

            return false;
        }

        void hideAnimatedObjects()
        {
            for (auto & [type, object] : objects_)
            {
                if (currentlyAnimated_.count(type))
                    object->setVisible(false);
            }
        }

        void updateVisibility(bool _mouseOver = true)
        {
            for (auto & [type, object] : objects_)
                object->setVisible(visibilityForCurrentState(type, _mouseOver));
        }

        void showAnimated(bool _mouseOver = true)
        {
            if (!currentlyAnimated_.empty())
            {
                queuedAnimation_ = Forward;
                return;
            }

            for (auto & [type, object] : objects_)
            {
                if (!object->visible())
                    currentlyAnimated_.insert(type);
            }

            if (currentlyAnimated_.empty())
                return;

            updateVisibility(_mouseOver);

            animationTimer_.start();
            updateTimer_.start();
            currentAnimation_ = DialogControls_p::Forward;
        }

        void hideAnimated()
        {
            if (!currentlyAnimated_.empty())
            {
                queuedAnimation_ = Back;
                return;
            }

            for (auto & [type, object] : objects_)
            {
                if (object->visible() && !visibilityForCurrentState(type, false))
                    currentlyAnimated_.insert(type);
            }

            if (currentlyAnimated_.empty())
                return;

            animationTimer_.start();
            updateTimer_.start();
            currentAnimation_ = DialogControls_p::Back;
        }

        QTimer animationTimer_;
        QTimer updateTimer_;
        QTimer hideTimer_;
        bool doubleClick_;
        int32_t duration_ = 0;
        std::map<ControlType, std::unique_ptr<Drawable>> objects_;
        AnimationDirection currentAnimation_;
        AnimationDirection queuedAnimation_;
        DialogControls::State state_ = DialogControls::Preview;
        bool isGif_ = false;

        std::set<ControlType> currentlyAnimated_;
    };

    static auto animationTimeout = static_cast<int>(MessageStyle::getHiddenControlsAnimationTime().count());
    static auto updateInterval = 10;
    static auto hideTimeout = 3200;

    //////////////////////////////////////////////////////////////////////////
    // DialogControls
    //////////////////////////////////////////////////////////////////////////

    DialogControls::DialogControls(bool _isGif, int32_t _duration, QWidget *_parent)
        : QWidget(_parent)
        , d(new DialogControls_p())
    {
        d->createObjects(_isGif, _duration);
        d->updateObjectsPositions(rect());
        d->isGif_ = _isGif;

        d->animationTimer_.setSingleShot(true);
        d->animationTimer_.setInterval(animationTimeout);

        connect(&d->animationTimer_, &QTimer::timeout, this, [this]()
        {
            if (d->currentAnimation_ == DialogControls_p::Forward)
                d->updateObjectsPositions(rect());
            else
                d->hideAnimatedObjects();

            d->currentlyAnimated_.clear();
            d->updateTimer_.stop();
            update();

            if (d->queuedAnimation_ == DialogControls_p::Forward)
                d->showAnimated();
            else if (d->queuedAnimation_ == DialogControls_p::Back)
                d->hideAnimated();

            d->queuedAnimation_ = DialogControls_p::None;
        });

        d->updateTimer_.setInterval(updateInterval);

        connect(&d->updateTimer_, &QTimer::timeout, this, [this]()
        {
            update();
        });

        d->hideTimer_.setInterval(hideTimeout);
        d->hideTimer_.setSingleShot(true);

        connect(&d->hideTimer_, &QTimer::timeout, this, [this]()
        {
            if (d->state_ == Playing)
                d->hideAnimated();
        });

        d->updateVisibility();

        setMouseTracking(true);
    }

    DialogControls::~DialogControls()
    {
    }

    void DialogControls::setState(DialogControls::State _state)
    {
        auto stateChanged = d->state_ != _state;

        d->state_ = _state;

        if (_state == Playing)
            d->hideTimer_.start();

        auto & play = d->objects_[Play];
        if (play)
            play->setChecked(_state != Playing);

        auto mouseOver = rect().contains(mapFromGlobal(QCursor::pos()));

        if (_state == Paused)
            d->showAnimated(mouseOver);
        else if (_state == Playing && stateChanged)
            d->hideAnimated();

        update();
    }

    void DialogControls::setRect(const QRect& _rect)
    {
        setGeometry(_rect);
        d->updateObjectsPositions(_rect);
    }

    void DialogControls::setMute(bool _enable)
    {
        if (d->objects_.count(Mute))
            d->objects_[Mute]->setChecked(!_enable);
    }

    void DialogControls::setDuration(int32_t _duration)
    {
        auto emptyDurationIt = d->objects_.find(EmptyDuration);
        if (emptyDurationIt != d->objects_.end())
            d->objects_.erase(emptyDurationIt);

        auto durationIt = d->objects_.find(Duration);
        if (durationIt == d->objects_.end())
        {
            d->createDuration(_duration);
            d->updateObjectsPositions(rect());
            d->duration_ = _duration;
        }

        update();
    }

    void DialogControls::setPosition(int32_t _position)
    {
        auto durationIt = d->objects_.find(Duration);
        if (durationIt != d->objects_.end())
        {
            auto duration = dynamic_cast<TextIconBase*>(durationIt->second.get());
            duration->label_.setText(durationStr(d->duration_ - _position));
            update(duration->rect());
        }
    }

    void DialogControls::setGotAudio(bool _gotAudio)
    {
        if (d->isGif_)
            return;

        if (_gotAudio)
        {
            auto noSoundIt = d->objects_.find(NoSound);
            if (noSoundIt != d->objects_.end())
                d->objects_.erase(noSoundIt);

            auto muteIt = d->objects_.find(Mute);
            if (muteIt == d->objects_.end())
                d->createMute();
        }
        else
        {
            auto muteIt = d->objects_.find(Mute);
            if (muteIt != d->objects_.end())
                d->objects_.erase(muteIt);

            auto noSoundIt = d->objects_.find(NoSound);
            if (noSoundIt == d->objects_.end())
                d->createNoSound();
        }

        d->updateObjectsPositions(rect());
        d->updateVisibility();
        update();
    }

    void DialogControls::paintEvent(QPaintEvent* _event)
    {
        Q_UNUSED(_event)

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        auto progress = 100;

        if (d->animationTimer_.isActive())
        {
            progress = d->animationProgress(animationTimeout, animationTimeout - d->animationTimer_.remainingTime());
            d->updateObjectsPositions(rect(), progress);
        }

        for (auto & [type, object] : d->objects_)
        {
            auto drawWithOpacity = d->currentlyAnimated_.count(type);
            if (drawWithOpacity)
                p.setOpacity(progress / 100.);
            else
                p.setOpacity(1);

            object->draw(p);
        }
    }

    void DialogControls::mouseMoveEvent(QMouseEvent* _event)
    {
        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_event->pos());
            auto needUpdate = object->hovered() != overObject;
            object->setHovered(overObject);

            if (needUpdate)
                update(object->rect());
        }

        QWidget::mouseMoveEvent(_event);

        d->showAnimated(true);
        d->hideTimer_.start();
    }

    void DialogControls::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        onMousePressed(_event->pos());

        MainPage::instance()->setFocusOnInput();
    }

    void DialogControls::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        auto overAnyObject = false;

        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_event->pos()) && object->clickable();
            auto pressed = object->pressed();
            object->setPressed(false);

            if ((pressed) && overObject && object->checkable())
                object->setChecked(!object->checked());

            update(object->rect());

            if (pressed && overObject)
                 onClick(type);

            overAnyObject |= overObject;
        }

        if (!overAnyObject)
        {
            if (!d->doubleClick_)
                onDefaultClick();
            else
                onDoubleClick();
        }

        d->doubleClick_ = false;
    }

    void DialogControls::mouseDoubleClickEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        d->doubleClick_ = true;

        onMousePressed(_event->pos());
    }

    void DialogControls::enterEvent(QEvent* _event)
    {
        Q_UNUSED(_event)
        d->showAnimated();
    }

    void DialogControls::leaveEvent(QEvent* _event)
    {
        Q_UNUSED(_event)
        d->hideAnimated();
    }

    void DialogControls::onClick(ControlType _type)
    {
        switch (_type)
        {
            case Fullscreen:
            {
                emit fullscreen();
                break;
            }
            case Mute:
            {
                auto & muteButton = d->objects_[Mute];
                if (muteButton)
                    emit mute(!muteButton->checked());
                break;
            }
            case Play:
            {
                auto & playButton = d->objects_[Play];
                if (playButton)
                {
                    auto enablePlay = playButton->checked();
                    d->state_ = enablePlay ? Paused : Playing;
                    emit play(enablePlay);
                }

                break;
            }
            default:
                break;
        }
        d->hideTimer_.start();
    }

    void DialogControls::onDefaultClick()
    {
        auto & playButton = d->objects_[Play];
        if (playButton && d->state_ != Preview)
        {
            auto checked = playButton->checked();
            playButton->setChecked(!checked);
            d->state_ = !checked ? Paused : Playing;
            emit play(!checked);
        }
        else
        {
            emit clicked();
        }
    }

    void DialogControls::onDoubleClick()
    {
        if (d->state_ != Preview)
            emit fullscreen();
    }

    void DialogControls::onMousePressed(const QPoint& _pos)
    {
        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_pos);
            auto needUpdate = object->pressed() != overObject;
            object->setPressed(overObject);

            if (needUpdate)
                update(object->rect());
        }
    }

    const auto unloadTimeout = 20000;

    //////////////////////////////////////////////////////////////////////////
    // DialogPlayer
    //////////////////////////////////////////////////////////////////////////
    DialogPlayer::DialogPlayer(QWidget* _parent, const uint32_t _flags, const QPixmap& _preview, const int32_t _duration)
        : QWidget(_parent)
        , attachedPlayer_(nullptr)
        , showControlPanel_(_flags & DialogPlayer::Flags::enable_control_panel)
        , preview_(_preview)
    {
        const QString mode = ((_flags & DialogPlayer::Flags::as_window) ? qsl("window") : qsl("dialog"));

        if (DialogPlayer::Flags::as_window & _flags)
        {
            if constexpr (platform::is_linux())
                setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
            else
                setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

            setCursor(Qt::PointingHandCursor);
        }

        if (_flags & DialogPlayer::Flags::gpu_renderer && useMetalRenderer())
            setParent(nullptr);

        renderer_ = createRenderer(this, _flags & DialogPlayer::Flags::gpu_renderer);
        renderer_->updateFrame(_preview);
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

        init(_parent, (_flags &DialogPlayer::Flags::is_gif),(_flags &DialogPlayer::Flags::is_sticker));

        isFullScreen_ = DialogPlayer::Flags::as_window & _flags;

        rootLayout_->addWidget(renderer_->getWidget());

        controlPanel_ = std::make_unique<Ui::VideoPlayerControlPanel>(nullptr, this, ffplayer_, mode);
        controlPanel_->installEventFilter(this);
        controlPanel_->hide();

        if (_flags & DialogPlayer::Flags::gpu_renderer && useMetalRenderer())
        {
            controlPanel_->setSeparateWindowMode();
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationDeactivated, this, &DialogPlayer::hide);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationActivated, this, &DialogPlayer::show);
        }

        connect(controlPanel_.get(), &VideoPlayerControlPanel::signalFullscreen, this, &DialogPlayer::fullScreen);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::playClicked, this, &DialogPlayer::playClicked);

        if (_flags & DialogPlayer::Flags::enable_dialog_controls)
            dialogControls_ = createControls(isGif_, _duration);

        unloadTimer_.setSingleShot(true);
        unloadTimer_.setInterval(unloadTimeout);
        connect(&unloadTimer_, &QTimer::timeout, this, &DialogPlayer::unloadMedia);
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
        , isSticker_(_attached->isSticker_)
        , showControlPanel_(true)
    {

        if (platform::is_linux())
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
        else
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        if (useMetalRenderer())
            setParent(nullptr);

        init(_parent, isGif_, isSticker_);

        ffplayer_->setParent(nullptr);
        renderer_ = createRenderer(this, true);
        ffplayer_->setRenderer(renderer_);

        rootLayout_->addWidget(renderer_->getWidget());
        isFullScreen_ = true;

        controlPanel_ = std::make_unique<Ui::VideoPlayerControlPanel>(_attached->controlPanel_.get(), this, ffplayer_, qsl("window"));
        controlPanel_->installEventFilter(this);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::signalFullscreen, this, &DialogPlayer::fullScreen);
        connect(controlPanel_.get(), &VideoPlayerControlPanel::playClicked, this, &DialogPlayer::playClicked);

        if (useMetalRenderer())
        {
            controlPanel_->setSeparateWindowMode();
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationDeactivated, this, &DialogPlayer::hide);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationActivated, this, &DialogPlayer::show);
        }

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

        timerHide_ = new QTimer(this);
        timerMousePress_ = new QTimer(this);

        parent_ = _parent;
        isLoad_ = false;
        isGif_ = _isGif;
        isSticker_ = _isSticker;

        rootLayout_ = Utils::emptyVLayout(this);

        connect(timerHide_, &QTimer::timeout, this, &DialogPlayer::timerHide);
        connect(timerMousePress_, &QTimer::timeout, this, &DialogPlayer::timerMousePress);
        connect(ffplayer_, &FFMpegPlayer::mouseMoved, this, &DialogPlayer::playerMouseMoved);
        connect(ffplayer_, &FFMpegPlayer::mouseLeaveEvent, this, &DialogPlayer::playerMouseLeaved);
        connect(ffplayer_, &FFMpegPlayer::fileLoaded, this, &DialogPlayer::onLoaded);
        connect(ffplayer_, &FFMpegPlayer::firstFrameReady, this, &DialogPlayer::onFirstFrameReady);
        setMouseTracking(true);

        installEventFilter(this);
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
        switch (_event->type())
        {
            case QEvent::Enter:
            {
                QObject* objectcontrolPanel = qobject_cast<QObject*>(controlPanel_.get());
                if (_obj == objectcontrolPanel)
                {
                    timerHide_->stop();
                }
                break;
            }
            case QEvent::Leave:
            {
                QObject* objectcontrolPanel = qobject_cast<QObject*>(controlPanel_.get());
                if (_obj == objectcontrolPanel)
                {
                    timerHide_->start(hideTimeoutShort);
                }
                break;
            }
            case QEvent::MouseMove:
            {
                QObject* objectcontrolPanel = qobject_cast<QObject*>(controlPanel_.get());
                if (_obj == objectcontrolPanel && controlPanel_->isSeparateWindowMode())
                {
                    QMouseEvent* me = static_cast<QMouseEvent*>(_event);
                    if (controlPanel_->isOverControls(me->pos()))
                        timerHide_->stop();
                    else
                        timerHide_->start(hideTimeout);
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
        timerHide_->stop();
        controlsShowed_ = false;
    }

    void DialogPlayer::showControlPanel(const bool _isShow)
    {
        if (!showControlPanel_)
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

        if (_isShow)
        {
            controlPanel_->show();
            controlPanel_->raise();
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

    void DialogPlayer::showControlPanel()
    {
        if (isGif_)
        {
            return;
        }

        if (controlsShowed_)
            return;

        timerHide_->stop();
        timerHide_->start(hideTimeout);

        controlsShowed_ = true;
        showControlPanel(true);
    }

    void DialogPlayer::playerMouseLeaved()
    {
        timerHide_->stop();
        timerHide_->start(hideTimeoutShort);
    }

    bool DialogPlayer::openMedia(const QString& _mediaPath)
    {
        isLoad_ = true;
        mediaPath_ = _mediaPath;

        return ffplayer_->openMedia(_mediaPath);
    }

    void DialogPlayer::setMedia(const QString &_mediaPath)
    {
        mediaPath_ = _mediaPath;

        if (dialogControls_)
            dialogControls_->setState(DialogControls::Paused);

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
            controlPanel_->setPause(_paused);

            if (ffplayer_->getStarted())
                showControlPanel();

            setPausedByUser(_byUser);

            emit paused();
        }

        if (dialogControls_)
            dialogControls_->setState(_paused ? DialogControls::Paused : DialogControls::Playing);
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
        controlPanel_->setVolume((double) _volume, _toRestore);
    }

    int32_t DialogPlayer::getVolume() const
    {
        return (int32_t) controlPanel_->getVolume();
    }

    void DialogPlayer::setMute(bool _mute)
    {
        if (_mute)
        {
            controlPanel_->setVolume(0, false);
        }
        else
        {
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
                dialogControls_->setState(DialogControls::Playing);
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
        bool isFullScreen = controlPanel_->isFullscreen();

        fullScreen(!isFullScreen);

    }

    bool DialogPlayer::isAutoPlay()
    {
        if (isSticker_)
            return true;
        else if (isGif_)
            return get_gui_settings()->get_value(settings_autoplay_gif, true);
        else
            return get_gui_settings()->get_value(settings_autoplay_video, true);
    }

    DialogControls*DialogPlayer::createControls(bool _isGif, int32_t _duration)
    {
        auto dialogControls = new DialogControls(_isGif, _duration, this);

        connect(dialogControls, &DialogControls::fullscreen, this, &DialogPlayer::openGallery);
        connect(dialogControls, &DialogControls::mute, this, &DialogPlayer::setMute);
        connect(dialogControls, &DialogControls::clicked, this, &DialogPlayer::mouseClicked);
        connect(ffplayer_, &FFMpegPlayer::positionChanged, this, [this, dialogControls](qint64 _position)
        {
            if (!attachedPlayer_)
                dialogControls->setPosition(_position / 1000);
        });
        connect(dialogControls, &DialogControls::play, this, [this](bool _enable) { setPaused(_enable, true); });

        dialogControls->raise();

        return dialogControls;
    }

    void DialogPlayer::timerMousePress()
    {
        moveToTop();

        setPaused(!controlPanel_->isPause(), true);

        timerMousePress_->stop();
    }

    void DialogPlayer::mouseReleaseEvent(QMouseEvent* _event)
    {
        if(_event->button() == Qt::LeftButton)
        {
            if (rect().contains(_event->pos()))
            {
                if (isFullScreen())
                {
                    setPaused(!controlPanel_->isPause(), true);
                    emit playClicked(controlPanel_->isPause());
                }

                if (!isGif())
                    emit mouseClicked();
            }

        }
        QWidget::mouseReleaseEvent(_event);
    }

    void DialogPlayer::mouseDoubleClickEvent(QMouseEvent *_event)
    {
        QWidget::mouseDoubleClickEvent(_event);

        if (_event->button() == Qt::LeftButton)
            emit mouseDoubleClicked();
    }

    void DialogPlayer::showAsFullscreen()
    {
        normalModePosition_ = geometry();

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();

        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        if constexpr (!platform::is_apple())
            hide();

        updateSize(screenGeometry);

        if constexpr (platform::is_windows())
            showMaximized();
        else if constexpr (platform::is_linux())
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
        emit fullScreenClicked();
    }

    void DialogPlayer::moveToTop()
    {
        raise();
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

    QPixmap DialogPlayer::getActiveImage() const
    {
        return ffplayer_->getActiveImage();
    }

    void DialogPlayer::onLoaded()
    {
        emit loaded();
    }

    void DialogPlayer::onFirstFrameReady()
    {
        emit firstFrameReady();
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

    QString DialogPlayer::mediaPath()
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

        controlPanel_->setGotAudio(_gotAudio);
    }

    void DialogPlayer::updateVisibility(bool _visible)
    {
        visible_ = _visible;

        if (attachedPlayer_ || mediaPath_.isEmpty() || (ffplayer_->state() == QMovie::NotRunning && !visible_))
            return;

        unloadTimer_.stop();

        if (!_visible)
            unloadTimer_.start();

        auto play = _visible && isAutoPlay() && !isPausedByUser();
        auto byUser = !play && isPausedByUser();

        setPaused(!play, byUser);
    }

    void DialogPlayer::wheelEvent(QWheelEvent* _event)
    {
        emit mouseWheelEvent(_event->angleDelta());

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

    std::unique_ptr<FrameRenderer> DialogPlayer::createRenderer(QWidget* _parent, bool _gpu)
    {
        std::unique_ptr<FrameRenderer> renderer;

#if defined(__linux__)
        renderer = std::make_unique<GDIRenderer>(_parent);
#else
        if (_gpu && (platform::is_windows_vista_or_late() || platform::is_apple()))
        {
#ifdef __APPLE__
            if (useMetalRenderer())
                renderer = std::make_unique<MetalRenderer>(_parent);
            else
#endif
                renderer = std::make_unique<OpenGLRenderer>(_parent);
        }
        else
        {
            renderer = std::make_unique<GDIRenderer>(_parent);
            if (_gpu)
                renderer->setFillColor(Qt::black);
        }
#endif //__linux__ || __APPLE__

        return renderer;
    }

}

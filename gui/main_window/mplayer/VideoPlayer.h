#pragma once

#include "../history_control/complex_message/MediaControls.h"

namespace Ui
{
    class FFMpegPlayer;
    class CustomButton;

    class InstChangedSlider : public QSlider
    {
        Q_OBJECT

    protected:

        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void wheelEvent(QWheelEvent* _event) override;

    public:

        InstChangedSlider(Qt::Orientation _orientation, QWidget* _parent);
    };


    class VideoPlayerControlPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void signalFullscreen(bool _checked);
        void mouseMoved();
        void playClicked(bool _paused);
        void mouseRightClicked();

    public Q_SLOTS:

        void videoDurationChanged(const qint64 _duration);
        void videoPositionChanged(const qint64 _position);

        void playerPaused();
        void playerPlayed();

    private:

        enum class SoundMode
        {
            ActiveOff,
            ActiveLow,
            ActiveHigh
        };

        InstChangedSlider* progressSlider_;

        QLabel* timeLeft_;
        QLabel* timeRight_;

        qint64 duration_;
        qint64 position_;

        FFMpegPlayer* player_;

        QPushButton* playButton_;

        QWidget* soundWidget_;
        CustomButton* soundButton_;
        SoundMode lastSoundMode_;

        bool gotAudio_;

        QWidget* gradient_;

        InstChangedSlider* volumeSlider_;

        QPushButton* fullscreenButton_;
        QPushButton* normalscreenButton_;

        QWidget* volumeContainer_;
        QVBoxLayout* fullScreenLayout_;
        QHBoxLayout* baseLayout_;
        QVBoxLayout* playLayout_;
        QVBoxLayout* progressSliderLayout_;
        QVBoxLayout* soundLayout_;
        QWidget* soundButtonVolumeSpacer_;
        QVBoxLayout* timeLayout_;

        const bool fullscreen_;

        bool separateWindow_ = false;
        bool soundOnByUser_;

    private:

        void connectSignals(FFMpegPlayer* _player);
        SoundMode getSoundMode(const int32_t _volume);
        void showVolumeControl(const bool _isShow);
        void updateSoundButtonState();
        void setSoundMode(SoundMode _mode);

    protected:

        void paintEvent(QPaintEvent* _e) override;

        bool eventFilter(QObject* _obj, QEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void resizeEvent(QResizeEvent * event) override;

    public:

        VideoPlayerControlPanel(
            VideoPlayerControlPanel* _copyFrom,
            QWidget* _parent,
            FFMpegPlayer* _player,
            const QString& _mode = qsl("dialog"));

        virtual ~VideoPlayerControlPanel();

        bool isFullscreen() const;

        bool isPause() const;
        void setPause(const bool& _pause);

        void setVolume(const int32_t _volume, const bool _toRestore);
        int32_t getVolume() const;
        void restoreVolume();
        void updateVolume(const bool _hovered);

        void setGotAudio(bool _gotAudio);

        void setSeparateWindowMode();
        bool isSeparateWindowMode() const;

        bool isOverControls(const QPoint& _pos) const;
    };

    class Drawable;

    class CustomButton;
    class FrameRenderer;

    class MediaControls;

    class DialogPlayer : public QWidget
    {
        Q_OBJECT

    private:

        FFMpegPlayer* ffplayer_;

        std::unique_ptr<VideoPlayerControlPanel> controlPanel_;

        QPointer<DialogPlayer> attachedPlayer_;

        QString mediaPath_;
        QWidget* rootWidget_;

        QTimer* timerHide_ = nullptr;
        bool controlsShowed_;
        QWidget* parent_;
        QVBoxLayout* rootLayout_;
        std::shared_ptr<FrameRenderer> renderer_;

        bool isFullScreen_;
        bool isLoad_;
        bool isGif_;
        bool isLottie_ = false;
        bool isLottieInstantPreview_ = false;
        bool isSticker_;
        const bool showControlPanel_;
        bool replay_ = false;
        bool visible_ = false;
        bool isGalleryView_;
        bool soundOnByUser_;

        QTimer* unloadTimer_ = nullptr;

        QRect normalModePosition_;

        QPointer<MediaControls> dialogControls_;

        QPixmap preview_;

        void init(QWidget* _parent, const bool _isGif, const bool _isSticker);
        void moveToScreen();

    public:

        enum Flags
        {
            is_gif                  = 1 << 0,
            is_sticker              = 1 << 1,
            enable_control_panel    = 1 << 2,
            gpu_renderer            = 1 << 3,
            as_window               = 1 << 4,
            dialog_mode             = 1 << 5,
            compact_mode            = 1 << 6,
            short_no_sound          = 1 << 7,
            lottie                  = 1 << 8,
            lottie_instant_preview  = 1 << 9,
        };

        void showAs();
        void showAsFullscreen();
        void showAsNormal();

        void start(bool _start);

        DialogPlayer(QWidget* _parent, const uint32_t _flags = 0, const QPixmap &_preview = QPixmap());
        DialogPlayer(DialogPlayer* _attached, QWidget* _parent);
        virtual ~DialogPlayer();

        bool openMedia(const QString& _mediaPath);

        void setMedia(const QString& _mediaPath);

        void setPaused(const bool _paused, const bool _byUser);
        QMovie::MovieState state() const;

        void setPausedByUser(const bool _paused);
        bool isPausedByUser() const;

        void setVolume(const int32_t _volume, bool _toRestore = true);
        int32_t getVolume() const;
        void setMute(bool _mute);

        void updateSize(const QRect& _sz);

        void moveToTop();

        void setHost(QWidget* _host);

        bool isFullScreen() const;

        void setIsFullScreen(bool _is_full_screen);

        bool inited();

        void setPreview(QPixmap _preview);
        void setPreview(QImage _preview);
        void setPreviewFromFirstFrame();
        QPixmap getActiveImage() const;

        void setLoadingState(bool _isLoad);

        void unloadMedia();

        bool isGif() const;

        void setClippingPath(QPainterPath _clippingPath);

        void setAttachedPlayer(DialogPlayer* _player);
        DialogPlayer* getAttachedPlayer() const;

        QSize getVideoSize() const;

        void setReplay(bool _enable);

        void setFillClient(const bool _fill);

        const QString& mediaPath() const noexcept;

        void setDuration(int32_t _duration);

        void setGotAudio(bool _gotAudio);

        void setSiteName(const QString& _siteName);

        void setFavIcon(const QPixmap& _favicon);

        void updateVisibility(bool _visible);

        QWidget* getParentForContextMenu() const;

        int bottomLeftControlsWidth() const;

        QImage getFirstFrame() const;
        const QPixmap& getPreview() const noexcept { return preview_; }

    public Q_SLOTS:

        void fullScreen(const bool _checked);

        void timerHide();
        void showControlPanel(const bool _isShow);
        void playerMouseMoved();
        void playerMouseLeaved();
        void onLoaded();
        void onFirstFrameReady();

    Q_SIGNALS:
        void loaded();
        void paused();
        void closed();
        void firstFrameReady();
        void mouseClicked();
        void mouseRightClicked();
        void mouseDoubleClicked();
        void fullScreenClicked();
        void mouseWheelEvent(const QPoint& _delta);
        void openGallery();
        void playClicked(bool _paused);
        void copyLink();

    protected:

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void mouseDoubleClickEvent(QMouseEvent* _event) override;
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void wheelEvent(QWheelEvent* _event) override;
        virtual void keyPressEvent(QKeyEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
    private:
        void showControlPanel();
        void changeFullScreen();

        void initControlPanel(VideoPlayerControlPanel* _copyFrom, const QString& _mode);
        void initTimerHide();
        void startTimerHide(std::chrono::milliseconds _timeout);

        void startUnloadTimer();
        void stopUnloadTimer();

        bool isAutoPlay();

        MediaControls* createControls(const uint32_t _flags);

        std::unique_ptr<FrameRenderer> createRenderer(QWidget* _parent, bool _useGPU, bool _dialogMode);
    };
}

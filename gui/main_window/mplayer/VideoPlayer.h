#pragma once

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

        QTimer* positionSliderTimer_;

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

    private:

        void connectSignals(FFMpegPlayer* _player);
        SoundMode getSoundMode(const int32_t _volume);
        void showVolumeControl(const bool _isShow);
        void updateSoundButtonState();
        void setSoundMode(SoundMode _mode);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void resizeEvent(QResizeEvent * event) override;

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

        void setGotAudio(bool _gotAudio);

        void setSeparateWindowMode();
        bool isSeparateWindowMode() const;

        bool isOverControls(const QPoint& _pos) const;
    };

    class DialogControls_p;

    enum ControlType : short;

    class DialogControls : public QWidget
    {
        Q_OBJECT
    public:
        enum State
        {
            Preview,
            Playing,
            Paused
        };

        DialogControls(bool _isGif, int32_t _duration, QWidget* _parent = nullptr);
        ~DialogControls();

        void setState(State _state);
        void setRect(const QRect& _rect);
        void setMute(bool _enable);
        void setDuration(int32_t _duration);
        void setPosition(int32_t _position);
        void setGotAudio(bool _gotAudio);

    Q_SIGNALS:
        void fullscreen();
        void mute(bool _enable);
        void play(bool _enable);
        void clicked();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        void onClick(ControlType _type);
        void onDefaultClick();
        void onDoubleClick();
        void onMousePressed(const QPoint& _pos);

        std::unique_ptr<DialogControls_p> d;
    };


    class Drawable;

    class CustomButton;
    class FrameRenderer;

    class DialogPlayer : public QWidget
    {
        Q_OBJECT

    private:

        FFMpegPlayer* ffplayer_;

        std::unique_ptr<VideoPlayerControlPanel> controlPanel_;

        QPointer<DialogPlayer> attachedPlayer_;

        QString mediaPath_;
        QWidget* rootWidget_;

        QTimer* timerHide_;
        QTimer* timerMousePress_;
        bool controlsShowed_;
        QWidget* parent_;
        QVBoxLayout* rootLayout_;
        std::shared_ptr<FrameRenderer> renderer_;

        static const uint32_t hideTimeout = 2000;
        static const uint32_t hideTimeoutShort = 100;

        bool isFullScreen_;
        bool isLoad_;
        bool isGif_;
        bool isSticker_;
        const bool showControlPanel_;
        bool replay_ = false;
        bool visible_ = false;

        QTimer unloadTimer_;

        QRect normalModePosition_;

        QPointer<DialogControls> dialogControls_;

        QPixmap preview_;

        void init(QWidget* _parent, const bool _isGif, const bool _isSticker);
        void moveToScreen();

    public:

        enum Flags
        {
            is_gif = 1 << 0,
            is_sticker = 1 << 1,
            enable_control_panel = 1 << 2,
            gpu_renderer = 1 << 3,
            as_window = 1 << 4,
            enable_dialog_controls = 1 << 5
        };

        void showAs();
        void showAsFullscreen();
        void showAsNormal();

        void start(bool _start);

        DialogPlayer(QWidget* _parent, const uint32_t _flags = 0, const QPixmap &_preview = QPixmap(), const int32_t _duration = 0);
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

        QString mediaPath();

        void setDuration(int32_t _duration);

        void setGotAudio(bool _gotAudio);

        void updateVisibility(bool _visible);

    public Q_SLOTS:

        void fullScreen(const bool _checked);

        void timerHide();
        void timerMousePress();
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
        void mouseDoubleClicked();
        void fullScreenClicked();
        void mouseWheelEvent(const QPoint& _delta);
        void openGallery();
        void playClicked(bool _paused);

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

        bool isAutoPlay();

        DialogControls* createControls(bool _isGif, int32_t _duration);

        std::unique_ptr<FrameRenderer> createRenderer(QWidget* _parent, bool _openGL);
    };
}

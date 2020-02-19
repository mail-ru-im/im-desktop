#pragma once

namespace Ui
{
    class MediaControls_p;

    class MediaControls : public QWidget
    {
        Q_OBJECT
    public:
        enum State
        {
            Preview,
            Playing,
            Paused
        };

        enum ControlType
        {
            Fullscreen,
            Mute,
            NoSound,
            Duration,
            EmptyDuration,
            GifLabel,
            Play,
            SiteName,
            CopyLink
        };

        enum ControlsOptions
        {
            None                = 0,
            PlayClickable       = 1 << 0,
            ShowPlayOnPreview   = 1 << 1,
            CompactMode         = 1 << 2,
            ShortNoSound        = 1 << 3,
        };

        MediaControls(const std::set<ControlType>& _controls, uint32_t _options, QWidget* _parent = nullptr);
        ~MediaControls();

        void setState(State _state);
        void setRect(const QRect& _rect);
        void setMute(bool _enable);
        void setDuration(int32_t _duration);
        void setPosition(int32_t _position);
        void setGotAudio(bool _gotAudio);
        void setSiteName(const QString& _siteName);
        void setFavicon(const QPixmap& _favicon);
        int bottomLeftControlsWidth() const;

    Q_SIGNALS:
        void fullscreen();
        void mute(bool _enable);
        void play(bool _enable);
        void clicked();
        void copyLink();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onAnimationTimeout();

    private:
        void onClick(ControlType _type);
        void onDefaultClick();
        void onDoubleClick();
        void onMousePressed(const QPoint& _pos);

        std::unique_ptr<MediaControls_p> d;
    };
}

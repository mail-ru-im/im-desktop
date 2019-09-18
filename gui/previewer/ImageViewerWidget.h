#pragma once

namespace Ui
{
    class DialogPlayer;
}

namespace Previewer
{
    class AbstractViewer;

    struct MediaData
    {
        QPixmap preview;
        QString fileName;
        bool gotAudio;
        Ui::DialogPlayer* attachedPlayer = nullptr;
    };

    class ImageViewerWidget final
        : public QLabel
    {
        Q_OBJECT
    Q_SIGNALS:
        void clicked();
        void fullscreenToggled(bool _enabled);
        void playClicked(bool _paused);

    public:
        explicit ImageViewerWidget(QWidget* _parent);
        ~ImageViewerWidget();

        void showMedia(const MediaData& _mediaData);

        void showPixmap(const QPixmap& _pixmap, const QSize& _originalImageSize, bool _isVideoPreview);


        bool isZoomSupport() const;

        void zoomIn(const QPoint &_anchor = QPoint());
        void zoomOut(const QPoint &_anchor = QPoint());
        void resetZoom();

        void scaleBy(double _scaleFactorDiff, const QPoint& _anchor = QPoint());
        void scale(double _scaleFactor, const QPoint& _anchor = QPoint());

        bool canZoomIn() const;
        bool canZoomOut() const;

        void reset();

        void connectExternalWheelEvent(std::function<void(const QPoint&)> _func);
        bool closeFullscreen();

        void setVideoWidthOffset(int _offset);
        void setViewSize(const QSize& _viewSize);

        void hide();

        void show();

        void bringToBack();
        void showOverAll();

    signals:
        void imageClicked();
        void zoomChanged();
        void closeRequested();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;

        void paintEvent(QPaintEvent* _event) override;

    private Q_SLOTS:
        void onViewerLoadTimeout();

    private:
        double getScaleStep() const;

        double getZoomInValue() const;
        double getZoomOutValue() const;

        double getZoomValue(const int _step) const;

        int currentStep() const;

        QSize maxVideoSize(const QSize& _initialViewport) const;

    private:
        QPoint mousePos_;

        int zoomStep_;
        int videoWidthOffset_;
        QSize viewSize_;

        QWidget* parent_;
        bool firstOpen_;

        std::unique_ptr<AbstractViewer> viewer_;
        std::unique_ptr<AbstractViewer> tmpViewer_;

        QTimer viewerLoadTimer_;
        MediaData currentData_;
    };
}

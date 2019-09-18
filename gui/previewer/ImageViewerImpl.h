#pragma once

#include "ImageViewerWidget.h"

namespace Ui
{
    class DialogPlayer;
}

namespace Previewer
{
    class AbstractViewer
        : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void mouseWheelEvent(const QPoint& _delta);
        void doubleClicked();
        void loaded();
        void fullscreenToggled(bool _enabled);
        void playCLicked(bool _paused);

    public:
        virtual ~AbstractViewer();

        bool canScroll() const;

        QRect rect() const;

        virtual void scale(double _newScaleFactor, QPoint _anchor = QPoint());
        virtual void scaleBy(double _newScaleFactor, QPoint _anchor = QPoint()) {}
        virtual void move(const QPoint& _offset);

        void paint(QPainter& _painter);

        double getPreferredScaleFactor() const;
        double getScaleFactor() const;

        double actualScaleFactor() const;

        virtual bool isZoomSupport() const;
        virtual bool closeFullscreen();

        virtual void hide() {}
        virtual void show() {}

        virtual void bringToBack() {}
        virtual void showOverAll() {}

    protected:
        explicit AbstractViewer(const QSize& _viewportSize, QWidget* _parent);

        void init(const QSize& _imageSize, const QSize &_viewport, const int _minSize);

    protected slots:
        void repaint();

    protected:
        virtual void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) = 0;
        virtual void doResize(const QRect& _source, const QRect& _target);

        double getPreferredScaleFactor(const QSize& _imageSize, const int _minSize) const;

        void fixBounds(const QSize& _bounds, QRect& _child);

        QRect getTargetRect(double _scaleFactor) const;

        bool canScroll_;

        QSize imageSize_;

        QRect viewportRect_;

        QRect targetRect_;
        QRect fragment_;

        QPoint offset_;

        QRect initialViewport_;

        double preferredScaleFactor_;
        double scaleFactor_;
    };


    class GifViewer : public AbstractViewer
    {
        Q_OBJECT
    public:
        static std::unique_ptr<AbstractViewer> create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

    private:
        GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QMovie gif_;
    };

    class JpegPngViewer : public AbstractViewer
    {
    public:
        static std::unique_ptr<AbstractViewer> create(const QPixmap& _image,
                                                      const QSize& _viewportSize,
                                                      QWidget* _parent,
                                                      const QSize &_initialViewport,
                                                      const bool _isVideoPreview = false);

        virtual void scale(double _newScaleFactor, QPoint _anchor = QPoint()) override;
        virtual void scaleBy(double _scaleFactorDiff, QPoint _anchor = QPoint()) override;
        virtual void move(const QPoint& _offset) override;

    private:
        JpegPngViewer(const QPixmap& _image,
                      const QSize& _viewportSize,
                      QWidget* _parent,
                      const QSize& _initialViewport);

        void constrainOffset();

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QPoint additionalOffset_;
        QPixmap originalImage_;
        bool smoothUpdate_;
        QTimer smoothUpdateTimer_;
    };

    class FFMpegViewer : public AbstractViewer
    {
        Q_OBJECT

    public:

        static std::unique_ptr<AbstractViewer> create(const MediaData& _mediaData,
                                                      const QSize& _viewportSize,
                                                      QWidget* _parent,
                                                      const bool _firstOpen);
        void hide() override;
        void show() override;
        void bringToBack() override;
        void showOverAll() override;

    private:

        QWidget* parent_;

        bool fullscreen_;
        QRect normalSize_;

        FFMpegViewer(const MediaData& _mediaData,
                     const QSize& _viewportSize,
                     QWidget* _parent,
                     const bool _firstOpen);
        virtual ~FFMpegViewer();

        QSize calculateInitialSize() const;

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;
        void doResize(const QRect& _source, const QRect& _target) override;
        bool closeFullscreen() override;
        bool isZoomSupport() const override;
    private:

        std::unique_ptr<Ui::DialogPlayer> ffplayer_;
    };
}

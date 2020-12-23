#pragma once

#include "ImageViewerImpl.h"

namespace Ui
{
    class DialogPlayer;
}
namespace Previewer
{
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

        QWidget* getParentForContextMenu() const override;

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
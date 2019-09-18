#pragma once

#include "controls/TextUnit.h"
#include "styles/WallpaperId.h"

namespace Styling
{
    using WallpaperPtr = std::shared_ptr<class ThemeWallpaper>;
}

namespace Ui
{
    class BackgroundWidget;
    class FlowLayout;
    class CheckBox;
    class WallpaperPreviewWidget;
    class ScrollAreaWithTrScrollBar;

    class DragOverlay : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void fileDropped(const QString& _path, QPrivateSignal);
        void imageDropped(const QPixmap& _pixmap, QPrivateSignal);

    public:
        DragOverlay(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void dragEnterEvent(QDragEnterEvent* _e) override;
        void dragLeaveEvent(QDragLeaveEvent* _e) override;
        void dragMoveEvent(QDragMoveEvent* _e) override;
        void dropEvent(QDropEvent* _e) override;

    private:
        void onTimer();
        void onResized(const QPixmap& _pixmap);

        QTimer* dragMouseoverTimer_;
    };

    class WallpaperSelectWidget : public QWidget
    {
        Q_OBJECT

    public:
        WallpaperSelectWidget(QWidget* _parent, const QString& _aimId = QString(), bool _fromSettings = true);
        ~WallpaperSelectWidget();

        void onOkClicked();

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void dragEnterEvent(QDragEnterEvent *) override;
        void dragLeaveEvent(QDragLeaveEvent *) override;
        void dragMoveEvent(QDragMoveEvent *) override;

    private:
        void onUserWallpaperClicked(const Styling::WallpaperId& _id);
        void onWallpaperClicked(const Styling::WallpaperId& _id);
        void onImageLoaded(const QPixmap& pixmap);
        void onUserWallpaperTileChanged(const bool _tile);
        void onFileSelected(const QString& _path);

        void fillWallpapersList();
        void touchScrollStateChanged(QScroller::State _st);
        bool isSetToAllChecked() const;
        void updateSelection(const Styling::WallpaperId& _id);

        void showDragOverlay();
        void hideDragOverlay();

        FlowLayout* thumbLayout_;
        WallpaperPreviewWidget* preview_;
        CheckBox* setToAll_;
        DragOverlay* dragOverlay_;
        ScrollAreaWithTrScrollBar* scrollArea_;

        QString targetContact_;
        TextRendering::TextUnitPtr contactName_;
        Styling::WallpaperId selectedId_;

        Styling::WallpaperPtr userWallpaper_;
        bool fromSettings_;
    };

    void showWallpaperSelectionDialog(const QString& _aimId, bool _fromSettings = true);
}
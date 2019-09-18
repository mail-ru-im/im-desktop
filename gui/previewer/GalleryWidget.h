#pragma once

#include "../controls/TextUnit.h"

namespace Data
{
    struct Image;
}

namespace Ui
{
    class CustomButton;
    class ActionButtonWidget;
    class DialogPlayer;
    class Button;
}

namespace Previewer
{
    class ImageViewerWidget;
    class Toast;

    class GalleryFrame;

    class NavigationButton;

    class ContentLoader;
    class GalleryLoader;

    class GalleryWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit GalleryWidget(QWidget* _parent);
        ~GalleryWidget();

        void openGallery(const QString& _aimId, const QString& _link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer = nullptr);
        void openAvatar(const QString& _aimId);

        bool isClosing() const;

    Q_SIGNALS:
        void finished();
        void closed();

    public slots:
        void closeGallery();

        void showMinimized();
        void showFullScreen();
        void escape();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        bool event(QEvent* _event) override;

    private slots:
        void updateControlsHeight(int _add);

        void onPrevClicked();
        void onNextClicked();

        void onSaveAs();

        void onMediaLoaded();
        void onPreviewLoaded();
        void onImageLoadingError();

        void updateNavButtons();

        void onZoomIn();
        void onZoomOut();

        void onResetZoom();

        void onWheelEvent(const QPoint& _delta);
        void onEscapeClick();

        void onSave();
        void onForward();
        void onGoToMessage();
        void onCopy();
        void onOpenContact();

        void onDelayTimeout();

        void updateZoomButtons();

        void onItemSaved(const QString& _path);
        void onItemSaveError();

        void clicked();

        void onFullscreenToggled(bool _enabled);

        void onPlayClicked(bool _paused);

    private:

        void moveToScreen();

        QString getDefaultText() const;
        QString getInfoText(int _n, int _total) const;

        void updateControls();

        void showProgress();

        void connectContainerEvents();

        bool hasPrev() const;
        void prev();

        bool hasNext() const;
        void next();

        void updateControlsFrame();

        QSize getViewSize();

        QPoint getToastPos();

        ContentLoader* createGalleryLoader(const QString& _aimId, const QString& _link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer = nullptr);

        ContentLoader* createAvatarLoader(const QString& _aimId);

        void init();

        void bringToBack();

        void showOverAll();

    private:
        QString aimId_;

        ImageViewerWidget* imageViewer_;

        GalleryFrame* controlsWidget_;

        QTimer* scrollTimer_;
        QTimer* delayTimer_;

        bool firstOpen_;

        NavigationButton* nextButton_;
        NavigationButton* prevButton_;

        std::unique_ptr<ContentLoader> contentLoader_;
        Ui::ActionButtonWidget* progressButton_;

        int controlWidgetHeight_;

        bool isClosing_;
    };


    class NavigationButton : public QWidget
    {
        Q_OBJECT

    public:
        NavigationButton(QWidget* _parent);
        ~NavigationButton();

        void setHoveredPixmap(const QPixmap &_pixmap);
        void setDefaultPixmap(const QPixmap &_pixmap);

        void setFixedSize(int _width, int _height);
        void setFixedSize(const QSize& _size) { setFixedSize(_size.width(), _size.height()); }
        void setVisible(bool _visible) override;

    Q_SIGNALS:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;

    private:
        QPixmap hoveredPixmap_;
        QPixmap defaultPixmap_;
        QPixmap activePixmap_;
    };
}

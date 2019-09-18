#pragma once

namespace Ui
{
    class BackgroundWidget;

    class InputBgWidget : public QWidget
    {
        Q_OBJECT

    public:
        InputBgWidget(QWidget* _parent, BackgroundWidget* _bgWidget);
        void setOverlayColor(const QColor& _color);
        void setBgVisible(const bool _isVisible);
        void forceUpdate();

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void showEvent(QShowEvent*) override;

    private:
        void dropBg();
        void requestBg();
        void onBlurred(const QPixmap& _result, qint64 _srcCacheKey);
        void onWallpaperChanged();

    private:
        BackgroundWidget* chatBg_;
        bool isBgVisible_ = false;
        QPixmap cachedBg_;
        QColor overlay_;

        QTimer resizeTimer_;
        QSize requestedSize_;
        qint64 requestedKey_ = -1;
    };
}
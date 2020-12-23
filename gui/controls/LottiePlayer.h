#pragma once

namespace Ui
{
    class LottiePLayerPrivate;
    class LottiePlayer : public QObject
    {
    Q_OBJECT

    Q_SIGNALS:
        void firstFrameReady() const;

    public:
        LottiePlayer(QWidget* _parent, const QString& _path, QRect _rect, const QPixmap& _preview = QPixmap());
        ~LottiePlayer();
        void setRect(QRect _rect);
        void onVisibilityChanged(bool _visible);
        const QPixmap& getPreview() const noexcept;
        const QString& getPath() const noexcept;

    private:
        bool eventFilter(QObject* _obj, QEvent* _event);

    private:
        std::unique_ptr<LottiePLayerPrivate> d;
    };
}

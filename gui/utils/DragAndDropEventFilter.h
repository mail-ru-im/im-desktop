#pragma once

namespace Utils
{
    class DragAndDropEventFilter : public QObject
    {
        Q_OBJECT

    private Q_SLOTS:
        void onTimer();

    public:
        DragAndDropEventFilter(QWidget* _parent);

    Q_SIGNALS:
        void dragPositionUpdate(QPoint _p, QPrivateSignal);
        void dropMimeData(QPoint _p, const QMimeData* _data, QPrivateSignal);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        QTimer* dragMouseoverTimer_;
    };
} // namespace Utils

#pragma once

namespace Ui
{
    class SplitterHandle;

    class Splitter : public QSplitter
    {
        Q_OBJECT

    public:

        explicit Splitter(Qt::Orientation, QWidget* _parent);
        ~Splitter();

        QSplitterHandle *createHandle() override;

        QWidget* replaceWidgetImpl(int _index, QWidget* _widget);  // use replaceWidget since qt 5.9

    signals:
        void moved(int desiredPos, int resultPos, int index, QPrivateSignal) const;

    private:
        void movedImpl(int desiredPos, int resultPos, int index) const
        {
            emit moved(desiredPos, resultPos, index, QPrivateSignal());
        }

        bool shouldShowWidget(const QWidget *w) const;

        friend class SplitterHandle;
    };

    class SplitterHandle : public QSplitterHandle
    {
        Q_OBJECT

    public:

        explicit SplitterHandle(Qt::Orientation, QSplitter* _parent);
        ~SplitterHandle();

        void updateStyle();

    protected:
        void mouseMoveEvent(QMouseEvent *) override;
        void mousePressEvent(QMouseEvent *) override;
        void mouseReleaseEvent(QMouseEvent *) override;
        void paintEvent(QPaintEvent *) override;

    private:
        int mouseOffset_ = 0;

        int pick(QPoint _point) const noexcept;
    };
}

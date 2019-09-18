#pragma once

namespace Ui
{
    class LabelEx : public QLabel
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        LabelEx(QWidget* _parent);
        void setColor(const QColor &_color);
        void setColors(const QColor &_normalColor, const QColor &_hoverColor, const QColor &_activeColor);

        void setElideMode(Qt::TextElideMode _mode);
        Qt::TextElideMode elideMode() const;

    protected:
        void paintEvent(QPaintEvent *_event) override;
        void resizeEvent(QResizeEvent *_event) override;
        void mouseReleaseEvent(QMouseEvent *_event) override;
        void mousePressEvent(QMouseEvent *_event) override;
        void enterEvent(QEvent *_event) override;
        void leaveEvent(QEvent *_event) override;

    private:
        void updateColor();
        void updateCachedTexts();

    private:
        Qt::TextElideMode elideMode_;
        QString cachedText_;
        QString cachedElidedText_;
        QPoint pos_;
        QColor normalColor_;
        QColor hoverColor_;
        QColor activeColor_;
        bool hovered_;
        bool pressed_;
    };
}
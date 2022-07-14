#pragma once

#include "styles/ThemeColor.h"

namespace Ui
{
    class LabelEx : public QLabel
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        LabelEx(QWidget* _parent);
        void setColor(const Styling::ThemeColorKey& _color);
        void setColors(const Styling::ThemeColorKey& _normalColor, const Styling::ThemeColorKey& _hoverColor, const Styling::ThemeColorKey& _activeColor);

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
        void updateCachedTexts();
        void setColor(const QColor& _color);

    private Q_SLOTS:
        void updateColor();

    private:
        Qt::TextElideMode elideMode_;
        QString cachedText_;
        QString cachedElidedText_;
        QPoint pos_;
        Styling::ColorContainer currentColor_;
        Styling::ThemeColorKey normalColor_;
        Styling::ThemeColorKey hoverColor_;
        Styling::ThemeColorKey activeColor_;
        bool hovered_;
        bool pressed_;
    };
}
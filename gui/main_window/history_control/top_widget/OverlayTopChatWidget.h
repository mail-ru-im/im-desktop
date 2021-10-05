#pragma once

#include "controls/TextUnit.h"

namespace Ui
{
    class OverlayTopChatWidget : public QWidget
    {
        Q_OBJECT;

    public:
        explicit OverlayTopChatWidget(QWidget* _parent = nullptr);
        ~OverlayTopChatWidget();

        void setBadgeText(const QString& _text);
        void setPosition(const QPoint& _pos);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QString text_;
        TextRendering::TextUnitPtr textUnit_;
        QPoint pos_;
    };
}
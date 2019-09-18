#pragma once

#include "../controls/ClickWidget.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class ExpandButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        explicit ExpandButton(QWidget* _parent = nullptr);
        ~ExpandButton();

        void setBadgeText(const QString& _text);

    protected:
        bool event(QEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;

    private:
        QString badgeText_;
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
    };
}
#pragma once

#include "controls/TransparentScrollBar.h"

namespace Ui
{
    class MessagesScrollbar : public QScrollBar
    {
        Q_OBJECT

    Q_SIGNALS:
        void hoverChanged(bool _hovered);

    public Q_SLOTS:
        void onAutoScrollTimer();
        virtual void setVisible(bool visible) override;

    public:
        explicit MessagesScrollbar(QWidget *page);

        bool canScrollDown() const;
        bool isInFetchRangeToBottom(const int32_t scrollPos) const;
        bool isInFetchRangeToTop(const int32_t scrollPos) const;
        void scrollToBottom();
        bool isAtBottom() const;
        bool isVisible() const;

        void fadeOut();
        void fadeIn();

        bool isHovered() const;

        static int32_t getPreloadingDistance();

    protected:
        bool event(QEvent *_event) override;

        void setHovered(bool _hovered);

    private:
        QTimer *AutoscrollEnablerTimer_;

        TransparentAnimation OpacityAnimation_;

        bool CanScrollDown_;
        bool IsHovered_;
    };

}

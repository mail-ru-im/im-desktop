#pragma once

#include "controls/ClickWidget.h"

namespace Data
{
    class SmartreplySuggest;
}

namespace Ui
{
    namespace Smiles
    {
        class StickerPreview;
    }

    class SmartReplyItem;
    class ScrollButton;

    class SmartReplyWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needHide(QPrivateSignal) const;
        void sendSmartreply(const Data::SmartreplySuggest& _suggest, QPrivateSignal) const;

    public:
        SmartReplyWidget(QWidget* _parent, bool _withSideSpacers);

        bool addItems(const std::vector<Data::SmartreplySuggest>& _suggests);

        void clearItems();
        void clearDeletedItems();
        void markItemsToDeletion();
        void clearItemsForMessage(const qint64 _msgId);

        void setBackgroundColor(const QColor& _color);
        void scrollToNewest();

        bool isUnderMouse() const noexcept { return isUnderMouse_; }

        void setNeedHideOnLeave();

        int itemCount() const { return getItems().size(); }
        int getTotalWidth() const;

    protected:
        void resizeEvent(QResizeEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void wheelEvent(QWheelEvent* _event) override;
        void hideEvent(QHideEvent*) override;

    private:
        void showStickerPreview(const QString& _stickerId);
        void hideStickerPreview();
        void onItemsMouseMoved(QPoint _pos);

        bool addItem(std::unique_ptr<SmartReplyItem>&& _newItem);

        QList<SmartReplyItem*> getItems() const;

        void onItemSelected(const Data::SmartreplySuggest& _suggest);
        void onAfterClickTimer();

        void scrollToRight();
        void scrollToLeft();

        void recalcGeometry();
        void updateEdges();
        void updateEdgeGradients();
        void updateItemMargins();

        void clearOldItems(const int64_t _newestMsgId);
        void clearItemsEqLessThan(const int64_t _msgId);
        void deleteItem(QWidget* _item);
        void deleteItemBySuggest(const Data::SmartreplySuggest& _suggest);
        bool isItemVisible(QWidget* _item) const;

    private:
        enum class Direction
        {
            left,
            right
        };
        void scrollStep(const Direction _direction);

    private:
        ScrollButton* scrollLeft_;
        ScrollButton* scrollRight_;

        QScrollArea* scrollArea_;
        QWidget* contentWidget_;
        QPropertyAnimation* animScroll_;
        QTimer afterClickTimer_;

        Smiles::StickerPreview* stickerPreview_;

        bool hasSideSpacers_;
        bool bgTransparent_;
        bool isUnderMouse_;
        bool needHideOnLeave_;
    };
}
#pragma once

#include "types/chat.h"

namespace Qml
{
    class GalleryTabsModel;
}

namespace Ui
{
    enum class MediaContentType;
    class ListViewWithTrScrollBar;

    class ExpandedGalleryPopup : public QWidget
    {
        Q_OBJECT

    public:
        explicit ExpandedGalleryPopup(QWidget* _parent = nullptr);
        ~ExpandedGalleryPopup();

    Q_SIGNALS:
        void hidden();
        void itemClicked(MediaContentType _type);

    protected:
        void hideEvent(QHideEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        int itemCount() const;
        int evaluateWidth() const;
        int evaluateHeight() const;

    private Q_SLOTS:
        void updateSize();
        void onItemClick(const QModelIndex& _current);

    private:
        Qml::GalleryTabsModel* model_;
        ListViewWithTrScrollBar* listView_;
    };
} // namespace Ui

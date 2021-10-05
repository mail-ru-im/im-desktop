#pragma once

#include "PageBase.h"
#include "types/chat.h"

namespace Ui
{
    class GalleryList;
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    class GalleryPopup;
    enum class MediaContentType;

    class GalleryPage : public PageBase
    {
        Q_OBJECT

    public:
        explicit GalleryPage(const QString& _aimId, QWidget* _parent = nullptr);
        void pageOpen() override;
        void pageLeave() override;
        QWidget* getTopWidget() const override;
        const QString& aimId() const override { return aimId_; }

        static QString createGalleryAimId(const QString& _aimId);
        static bool isGalleryAimId(const QString& _aimId);

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void setContentType(MediaContentType _type);
        void onTitleArrowClicked();
        void onRoleChanged(const QString& _aimId);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void closeGallery();

        void initState();
        void initPopup();

        void updatePopupCounters();
        void updatePopupSizeAndPos();

        QString getContact() const;

    private:
        QString aimId_;
        Data::DialogGalleryState state_;
        MediaContentType currentType_;

        GalleryList* gallery_;
        HeaderTitleBar* titleBar_;
        GalleryPopup* galleryPopup_;
        HeaderTitleBarButton* backButton_;
    };
}
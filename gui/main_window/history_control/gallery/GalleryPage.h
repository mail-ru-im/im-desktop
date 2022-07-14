#pragma once

#include "../PageBase.h"
#include "types/chat.h"

namespace Ui
{
    class GalleryList;
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    class ExpandedGalleryPopup;
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
        void updateWidgetsTheme() override;

    private:
        void onRoleChanged(const QString& _aimId);
        void closeGallery();

        void initPopup();

        void updatePopupPosition();

        QString getContact() const;

    private Q_SLOTS:
        void onShowPopupClicked(bool _popupNeedVisible);
        void onGalleryPopupHide();
        void setContentType(MediaContentType _type);
        void onTabsCountChanged();

    private:
        QString aimId_;

        GalleryList* gallery_;
        HeaderTitleBar* titleBar_ = nullptr;

        QPointer<ExpandedGalleryPopup> galleryPopup_;
        HeaderTitleBarButton* backButton_ = nullptr;

        QQuickWidget* tabs_;
    };
}
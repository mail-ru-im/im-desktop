#pragma once

#include "styles/WallpaperId.h"

namespace Ui
{
    class BackgroundWidget;

    namespace ComplexMessage
    {
        class ComplexMessageItem;
    }

    struct MessagePreviewInfo
    {
        QString text_;
        QTime time_;
        QString senderFriendly_;
        bool isEdited_ = false;
    };
    using PreviewMessagesVector = std::vector<MessagePreviewInfo>;

    class WallpaperPreviewWidget : public QWidget
    {
    public:
        WallpaperPreviewWidget(QWidget* _parent = nullptr, const PreviewMessagesVector& _messages = {});
        void updateFor(const QString& _aimId);

    protected:
        void resizeEvent(QResizeEvent*);
        void showEvent(QShowEvent*);

    private:
        void onWallpaperAvailable(const Styling::WallpaperId& _id);
        void onFontParamsChanged();
        void onResize();

        BackgroundWidget* bg_;
        QString aimId_;
    };
}
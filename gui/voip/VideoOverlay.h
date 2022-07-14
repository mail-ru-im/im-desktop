#pragma once
#include "VoipProxy.h"
#include "ShapedWidget.h"

namespace Ui
{
    class VideoOverlay : public ShapedWidget
    {
         Q_OBJECT
    public:
        explicit VideoOverlay(QWidget* _parent);
        ~VideoOverlay();

        void showMicroAlert(MicroIssue _issue);

    private:
        std::unique_ptr<class VideoOverlayPrivate> d;
    };

}

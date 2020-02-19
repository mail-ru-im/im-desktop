#pragma once
#include <QtWidgets/qwidget.h>

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_specific
{
    class GraphicsPanel : public QWidget
    {
    public:
        static GraphicsPanel* create(QWidget* _parent, std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo, bool titlePar);

        GraphicsPanel(QWidget* _parent) : QWidget(_parent) {}
        virtual ~GraphicsPanel() { }

        virtual WId frameId() const;

        virtual void addPanels(std::vector<QPointer<Ui::BaseVideoPanel>>& _panels) {}
        virtual void clearPanels() {}
        virtual void fullscreenModeChanged(bool _fullscreen) {}
        virtual void fullscreenAnimationStart() {}
        virtual void fullscreenAnimationFinish() {}
        virtual void windowWillDeminiaturize() {}
        virtual void windowDidDeminiaturize() {}
        virtual void createdTalk() {}
        virtual void startedTalk() {}
    };
}

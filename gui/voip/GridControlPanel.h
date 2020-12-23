#include "../voip/CommonUI.h"

namespace Ui
{
    class CustomButton;
    class GridButton;

    class GridControlPanel : public BaseTopVideoPanel
    {
        Q_OBJECT

    Q_SIGNALS:
        void updateConferenceMode(voip_manager::VideoLayout _layout);
    public:
        explicit GridControlPanel(QWidget* _p);

        void fadeIn(unsigned int) override {};
        void fadeOut(unsigned int) override {};
        void forceFinishFade() override {};

        void changeConferenceMode(voip_manager::VideoLayout _layout);
        void onChangeConferenceMode();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void changeEvent(QEvent* _e) override;

    private:
        QWidget* rootWidget_;
        QHBoxLayout* rootLayout_;
        GridButton* changeGridButton_;
        voip_manager::VideoLayout videoLayout_;
    };
}
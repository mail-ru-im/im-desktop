#include "../voip/CommonUI.h"

namespace Ui
{
    class CustomButton;
    class DialogButton;
    class TextEmojiWidget;

    enum class MicroIssue
    {
        None,
        NoPermission,
        NotFound
    };

    class MicroAlert : public QWidget
    {
        Q_OBJECT
    public:

        enum class MicroAlertState
        {
            Expanded,
            Collapsed,
        };

        explicit MicroAlert(QWidget* _p);
        void setIssue(MicroIssue _issue);

        void setState(MicroAlertState _state);
        bool isCollapsed() const { return state_ == MicroAlertState::Collapsed; }

        static int alertOffset();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;

    private:
        void openSettings() const;
        void updateTextSize();

        CustomButton* microButton_;
        TextEmojiWidget* text_;
        DialogButton* settingsButton_;
        CustomButton* closeButton_;
        MicroAlertState state_;
        MicroIssue issue_;
    };
}

namespace Utils
{
    void showPermissionDialog(QWidget*);
}

#include "../voip/CommonUI.h"

namespace Ui
{
    class CustomButton;
    class DialogButton;
    class TextEmojiWidget;
    class GeneralDialog;

    class ResizeDialogEventFilter : public QObject
    {
        Q_OBJECT
    public:
        ResizeDialogEventFilter(GeneralDialog* _dialog, QObject* _parent);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _e) override;

    private:
        GeneralDialog* dialog_;
    };

    enum class MicroIssue
    {
        None,
        Incoming,
        NoPermission,
        NotFound
    };

    class MicroAlert : public BaseTopVideoPanel
    {
        Q_OBJECT
    public:
        explicit MicroAlert(QWidget* _p);
        void setIssue(MicroIssue _issue);

        void fadeIn(unsigned int) override {};
        void fadeOut(unsigned int) override {};
        void forceFinishFade() override {};
        void updatePosition(const QWidget&) override;

        enum class MicroAlertState
        {
            Full,
            Collapsed,
        };

        void setState(MicroAlertState _state);
        bool isCollapsed() const { return state_ == MicroAlertState::Collapsed; }

    protected:
        void paintEvent(QPaintEvent* _e) override;
        bool eventFilter(QObject* _o, QEvent* _e) override;
        void changeEvent(QEvent* _e) override;

    private:
        void openSettings() const;
        void updateSize();

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
    using MicroPermissionCallback = std::function<void(Ui::MicroIssue)>;
    void checkMicroPermission(QWidget*, MicroPermissionCallback);
}
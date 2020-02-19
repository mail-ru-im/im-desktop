#pragma once

namespace Ui
{
    class ProgressAnimation;
    class CustomButton;
    class TextWidget;

    class CheckForUpdateButton : public QWidget
    {
        Q_OBJECT

    public:
        explicit CheckForUpdateButton(QWidget* _parent = nullptr);
        ~CheckForUpdateButton();

    private:
        void onClick();
        void onUpdateReady();
        void onUpToDate(int64_t _seq, bool _isNetworkError);

        void startAnimation();
        void stopAnimation();

        void showUpdateReadyToast();

    private:
        ProgressAnimation* animation_ = nullptr;
        CustomButton* icon_ = nullptr;
        TextWidget* text_ = nullptr;

        std::optional<int64_t> seq_;

        bool isUpdateReady_ = false;
    };
}
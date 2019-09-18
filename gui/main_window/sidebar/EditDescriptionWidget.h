#pragma once

namespace Ui
{
    class InputEdit;
    class TextBrowserEx;
    class DialogButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class EditDescriptionWidget: public QWidget
    {
        Q_OBJECT

    public:
        struct FormData
        {
            QString description_;
        };

    public:
        explicit EditDescriptionWidget(QWidget* _parent = nullptr, const FormData& _initData = FormData());
        ~EditDescriptionWidget();

        FormData getFormData() const;
        void clearData();

        void setButtonsPair(const QPair<DialogButton*, DialogButton*>& _buttonsPair);

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void onFormChanged();
        void okClicked();
        void cancelClicked();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _e) override;

    private:
        bool isFormComplete() const;
        void updateDescriptionHeight();

    private:
        std::unique_ptr<TextRendering::TextUnit> headerUnit_;
        std::unique_ptr<TextRendering::TextUnit> hintUnit_;
        std::unique_ptr<TextRendering::TextUnit> counterUnit_;
        InputEdit* description_;
        DialogButton* okButton_;
        DialogButton* cancelButton_;
        int descriptionHeight_;
        bool modifiedViewPort_;
        bool wrongDescription_;
    };
}

#pragma once

namespace Ui
{
    class LineEditEx;
    class DialogButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class EditNameWidget: public QWidget
    {
        Q_OBJECT

    public:
        struct FormData
        {
            QString firstName_;
            QString middleName_;
            QString lastName_;
        };

    public:
        explicit EditNameWidget(QWidget* _parent = nullptr, const FormData& _initData = FormData());
        ~EditNameWidget();

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
        void keyPressEvent(QKeyEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        bool isFormComplete() const;

        void connectToLineEdit(LineEditEx *_lineEdit, LineEditEx *_onUp, LineEditEx *_onDown) const;

    private:
        QVBoxLayout* globalLayout_ = nullptr;
        std::unique_ptr<TextRendering::TextUnit> headerUnit_;
        LineEditEx* firstName_ = nullptr;
        LineEditEx* middleName_ = nullptr;
        LineEditEx* lastName_ = nullptr;
        DialogButton* okButton_ = nullptr;
        DialogButton* cancelButton_ = nullptr;
    };
}

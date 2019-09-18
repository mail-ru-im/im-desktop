#pragma once

namespace Ui
{
    class NickLineEdit;
    class DialogButton;
    class TextWidget;
    class TextUnitLabel;
    class LoaderSpinner;

    namespace TextRendering
    {
        class TextUnit;
    }

    class EditNicknameWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void changed();

    public:
        struct FormData
        {
            QString nickName_;
        };

        EditNicknameWidget(QWidget* _parent = nullptr, const FormData& _initData = FormData());
        ~EditNicknameWidget();

        FormData getFormData() const;
        void clearData();

        void setButtonsPair(const QPair<DialogButton*, DialogButton*>& _buttonsPair);
        void setToastParent(QWidget* _parent);

        void setStatFrom(std::string_view _from);

    private Q_SLOTS:
        void onFormChanged();
        void onFormReady();
        void onSameNick();
        void onServerError(bool _repeateOn);
        void onNickSet();
        void onSetNickTimeout();
        void okClicked();
        void cancelClicked();
        void cancelSetNickClicked();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        bool isFormComplete() const;
        QString getNickUrl(const QString& _nick) const;

        void showToast(const QString& _text) const;

    private:
        std::unique_ptr<TextRendering::TextUnit> headerUnit_;
        NickLineEdit* nickName_;
        TextUnitLabel* nickLabel_;
        TextUnitLabel* urlLabel_;
        TextWidget* nickLabelText_;
        TextWidget* ruleText_;
        TextWidget* urlLabelText_;
        DialogButton* okButton_;
        DialogButton* cancelButton_;
        bool nickReady_;
        LoaderSpinner* setNickSpinner_;
        QTimer* setNickTimeout_;
        QString changeOkButtonText_;
        QWidget* toastParent_;
        std::string statFrom_;
        bool nickSet_;
    };

    void showEditNicknameDialog(EditNicknameWidget* _widget);
}

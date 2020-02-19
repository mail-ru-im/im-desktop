#pragma once

namespace Ui
{
    class LabelEx;
    class CommonInputContainer;
    class DialogButton;
    class ContactAvatarWidget;

    class IntroduceYourself : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void profileUpdated();
        void avatarSet();

    public:
        IntroduceYourself(const QString& _aimid, const QString& _display_name, QWidget* parent);
        ~IntroduceYourself();

        void UpdateParams(const QString& _aimid, const QString& _display_name);

    private Q_SLOTS:
        void TextChanged();
        void UpdateProfile();
        void RecvResponse(int _error);
        void avatarChanged();

    private:
        ContactAvatarWidget* contactAvatar_;
        CommonInputContainer*     firstNameEdit_;
        CommonInputContainer*     lastNameEdit_;
        DialogButton*   nextButton_;
        QVBoxLayout *main_layout_;
        QString aimid_;
        void setButtonActive(bool _is_active);
        void init(QWidget *parent);
    };
}

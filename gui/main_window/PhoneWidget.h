#pragma once

#include "../types/common_phone.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class LineEditEx;
    class DialogButton;

    enum class PhoneWidgetState
    {
        ABOUT_STATE = 0,
        ENTER_PHONE_STATE = 1,
        ENTER_CODE_SATE = 2,
        FINISH_STATE = 3,
    };

    class PhoneWidget : public QWidget
    {
        Q_OBJECT
    public:
        PhoneWidget(QWidget* _parent, const PhoneWidgetState& _state = PhoneWidgetState::ABOUT_STATE, const QString& _label = QString(), const QString& _about = QString(), bool _canClose = true, bool _forceAttach = false);
        virtual ~PhoneWidget();

        void setState(const PhoneWidgetState& _state);
        bool succeeded() const { return state_ == PhoneWidgetState::FINISH_STATE || loggedOut_; }

    protected:
        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void mouseReleaseEvent(QMouseEvent* _e) override;
        virtual void mouseMoveEvent(QMouseEvent* _e) override;
        virtual void showEvent(QShowEvent* _e) override;
        virtual void keyPressEvent(QKeyEvent* _e) override;

    private:
        void init();
        void sendCode();
        void resetActions();
        void callPhone();
        void logout();

        void updateTextOnGetSmsResult();

    Q_SIGNALS:
        void requestClose();

    private Q_SLOTS:
        void nextClicked();
        void cancelClicked();
        void codeChanged(const QString& _code);
        void codeFocused();
        void phoneChanged(const QString& _phone);
        void smsCodeChanged(const QString& _code);
        void phoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data);
        void getSmsResult(int64_t, int, int, const QString&, const QString& _ckecks);
        void loginResult(int64_t, int);
        void onTimer();
        void countrySelected(const QString&);

    private:
        PhoneWidgetState state_;

        std::unique_ptr<TextRendering::TextUnit> label_;
        std::unique_ptr<TextRendering::TextUnit> about_first_;
        std::unique_ptr<TextRendering::TextUnit> about_second_;
        std::unique_ptr<TextRendering::TextUnit> about_phone_enter_;
        std::unique_ptr<TextRendering::TextUnit> change_phone_;
        std::unique_ptr<TextRendering::TextUnit> phone_hint_;
        std::unique_ptr<TextRendering::TextUnit> send_again_;
        std::unique_ptr<TextRendering::TextUnit> phone_call_;
        std::unique_ptr<TextRendering::TextUnit> sms_error_;
        std::unique_ptr<TextRendering::TextUnit> about_phone_changed_;
        std::unique_ptr<TextRendering::TextUnit> changed_phone_number_;
        std::unique_ptr<TextRendering::TextUnit> logout_;

        QPixmap icon_;
        QPixmap mark_;

        DialogButton* next_;
        DialogButton* cancel_;

        LineEditEx* countryCode_;
        LineEditEx* phoneNumber_;
        LineEditEx* enteredPhoneNumber_;
        LineEditEx* smsCode_;

        QCompleter* completer_;

        int64_t sendSeq_;
        int codeLength_;
        QString checks_;
        int secRemaining_;
        QTimer* timer_;
        bool showSmsError_;
        QString ivrUrl_;
        QString labelText_;
        QString aboutText_;
        bool canClose_;
        bool forceAttach_;
        bool loggedOut_;
        bool showPhoneHint_;
    };
}

#pragma once
#include "LineEditEx.h"
#include "TextEditEx.h"

namespace Ui
{
    class PhoneCodePicker;
    class RotatingSpinner;

    enum class InputRole
    {
        Common,
        Password,
        Code,
        Phone
    };

    class BaseInput : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void textEdited();
        void textChanged(const QString &);
        void emptyTextBackspace();
        void enterPressed();
        void leftArrow();
        void rightArrow();

    public:
        explicit BaseInput(QWidget* _parent);
        virtual ~BaseInput() = default;

        virtual void setText(const QString& _text);
        virtual QString text() const;
        QFontMetrics getFontMetrics() const;
        virtual void setFont(const QFont& _font);

        virtual void setEchoMode(QLineEdit::EchoMode _mode);
        virtual void setPlaceholderText(const QString& _text);
        virtual void setValidator(const QValidator *_v);
        virtual void setMaxLength(const int _len);

        virtual void setCodeLength(int _len) {};
        virtual void setPhone(const QString& _phone) {};

        virtual void clear();
        virtual void setCursorMoveStyle(Qt::CursorMoveStyle _style);

        int getWidth() const;
        bool atBegin() const;
        bool atEnd() const;

        virtual void prevInput();
        virtual void nextInput();

        virtual void selectAll();
        virtual void deselectAll();
        virtual bool isAllSelected() const;
        virtual int lastInput() const;

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;
        void focusInEvent(QFocusEvent *_e) override;
        virtual void initInput(const int _size);
        virtual LineEditEx* current() const;
        virtual bool isLast() const;

    private:
        std::vector<LineEditEx*> inputs_;
        QHBoxLayout* globalLayout_;
        size_t currentInput_;
    };

    class CommonInput : public BaseInput
    {
        Q_OBJECT

    public:
        CommonInput(QWidget* _parent, const InputRole _role = InputRole::Common);
        ~CommonInput() = default;

        void setCodeLength(int _len) override;
        void setText(const QString& _text) override;

    private:
        InputRole role_;
        int codeLength_;
    };

    class PhoneInput : public BaseInput
    {
        Q_OBJECT

    public:
        PhoneInput(QWidget* _parent);
        ~PhoneInput() = default;

        void setPhone(const QString& _phone) override;

    private:
        QString phone_;
    };

    class CodeInput : public BaseInput
    {
        Q_OBJECT

    Q_SIGNALS:
        void codeChanged(const QString& _code);

    public:
        CodeInput(QWidget* _parent, const QString& _code = QString());
        ~CodeInput() = default;

        void setText(const QString& _code) override;

    private:
        QString code_;
    };

    class PhoneInputWidget : public QWidget
    {
        Q_OBJECT

    public:
        PhoneInputWidget(QWidget* _parent);
        ~PhoneInputWidget() = default;

    private:
        QLabel* countryPicker_;
        PhoneInput* input_;
    };

    class BaseInputContainer : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void textEdited();
        void textChanged(const QString &);
        void emptyTextBackspace();
        void enterPressed();

    public:
        BaseInputContainer(QWidget* _parent, InputRole _role = InputRole::Common);
        virtual ~BaseInputContainer() = default;

        virtual void setText(const QString& _text);
        virtual QString text() const;
        virtual void setPlaceholderText(const QString& _text);
        virtual void clear();

        void setPhone(const QString& _phone);
        void setPhoneCode(const QString& _code);
        QString getPhoneCode();
        QString getPhone();
        void setCodeLength(int _len);

        void startSpinner() const;
        void stopSpinner() const;

        virtual void setPhoneFocus() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent *_event) override;

    private:
        void updateCountryCode();
        PhoneCodePicker* getPhoneCodePicker() const;

        enum ResizeMode
        {
            Force,
            Common
        };
        void updateSpacerWidth(const ResizeMode _force) const;

    private:
        QLayout* globalLayout_;
        QWidget* leftWidget_;
        RotatingSpinner* rightWidget_;
        CodeInput* code_;
        BaseInput* input_;
        QSpacerItem* leftSpacer_;
        QSpacerItem* rightSpacer_;
    };

    class CommonInputContainer : public BaseInputContainer
    {
        Q_OBJECT

    public:
        CommonInputContainer(QWidget* _parent, InputRole _role = InputRole::Common) : BaseInputContainer(_parent, _role) {};
        ~CommonInputContainer() = default;
    };

    class CodeInputContainer : public BaseInputContainer
    {
        Q_OBJECT

    public:
        CodeInputContainer(QWidget* _parent) : BaseInputContainer(_parent, InputRole::Code) {};
        ~CodeInputContainer() = default;
    };

    class PhoneInputContainer : public BaseInputContainer
    {
        Q_OBJECT

    Q_SIGNALS:
        void countrySelected(const QString&);

    public:
        PhoneInputContainer(QWidget* _parent) : BaseInputContainer(_parent, InputRole::Phone) {};
        ~PhoneInputContainer() = default;
    };

    class TextEditInputContainer : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void textEdited();
        void textChanged(const QString &);

    public:
        TextEditInputContainer(QWidget* _parent);
        virtual ~TextEditInputContainer() = default;

        virtual void setText(const QString& _text);
        virtual QString getPlainText() const;
        virtual void setPlaceholderText(const QString& _text);
        virtual void clear();

        QTextDocument* document() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent *_event) override;

    private:
        void updateInputHeight();

    private:
        TextEditEx* input_;
    };
}
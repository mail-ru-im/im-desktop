#pragma once

namespace Ui
{
    enum class DialogButtonRole
    {
        CANCEL,
        CONFIRM,
        CONFIRM_DELETE,
        DISABLED,
        RESTART,

        DEFAULT = CONFIRM
    };

    class DialogButton : public QPushButton
    {
        Q_OBJECT

    public:
        explicit DialogButton(QWidget* _parent, const QString _text, const DialogButtonRole _role = DialogButtonRole::DEFAULT);
        void changeRole(const DialogButtonRole _role);
    protected:
        void paintEvent(QPaintEvent *_e) override;
        void leaveEvent(QEvent *_e) override;
        void enterEvent(QEvent *_e) override;
        void mousePressEvent(QMouseEvent *_e) override;
        void mouseReleaseEvent(QMouseEvent *_e) override;
        void changeEvent(QEvent *_e) override;
        void focusInEvent(QFocusEvent *_e) override;
        void focusOutEvent(QFocusEvent *_e) override;
        void keyPressEvent(QKeyEvent *_e) override;
    private:
        void setBackgroundColor();
        void setBorderColor();
        void setTextColor();
        void updateTextColor();

    private:
        bool hovered_;
        bool pressed_;
        QString text_;
        DialogButtonRole role_;

        QColor bgColor_;
        QColor bgColorHover_;
        QColor bgColorPress_;
        QColor borderColor_;
        QColor borderColorHover_;
        QColor borderColorPress_;
        QColor textColor_;
        QColor textColorHover_;
        QColor textColorPress_;
    };
}

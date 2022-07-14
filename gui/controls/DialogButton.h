#pragma once

#include "styles/ThemeColor.h"

namespace Ui
{
    enum class DialogButtonRole
    {
        CANCEL,
        CONFIRM,
        CONFIRM_DELETE,
        DISABLED,
        RESTART,
        CUSTOM,

        DEFAULT = CONFIRM
    };

    class DialogButton : public QPushButton
    {
        Q_OBJECT

    public:
        explicit DialogButton(QWidget* _parent, const QString& _text, const DialogButtonRole _role = DialogButtonRole::DEFAULT);
        void changeRole(const DialogButtonRole _role);
        bool isEnabled() const;
        void updateWidth();
        void setEnabled(bool _isEnabled);
        void setBackgroundColor(const Styling::ThemeColorKey& _normal, const Styling::ThemeColorKey& _hover, const Styling::ThemeColorKey& _pressed);
        void setBorderColor(const Styling::ThemeColorKey& _normal, const Styling::ThemeColorKey& _hover, const Styling::ThemeColorKey& _pressed);
        void setTextColor(const Styling::ThemeColorKey& _normal, const Styling::ThemeColorKey& _hover, const Styling::ThemeColorKey& _pressed);
    protected:
        void paintEvent(QPaintEvent *_e) override;
        void leaveEvent(QEvent *_e) override;
        void enterEvent(QEvent *_e) override;
        void mousePressEvent(QMouseEvent *_e) override;
        void mouseReleaseEvent(QMouseEvent *_e) override;
        void focusInEvent(QFocusEvent *_e) override;
        void focusOutEvent(QFocusEvent *_e) override;
        void keyPressEvent(QKeyEvent *_e) override;
    private:
        void setBackgroundColor();
        void setBorderColor();
        void setTextColor();

    private:
        QString text_;
        DialogButtonRole initRole_;
        DialogButtonRole role_;

        Styling::ColorContainer bgColor_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer bgColorHover_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer bgColorPress_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer borderColor_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer borderColorHover_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer borderColorPress_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer textColor_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer textColorHover_ = Styling::ColorContainer{ Qt::transparent };
        Styling::ColorContainer textColorPress_ = Styling::ColorContainer{ Qt::transparent };
        bool hovered_ = false;
        bool pressed_ = false;
    };
}

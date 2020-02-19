#pragma once

#include "../animation/animation.h"

#include "../controls/ClickWidget.h"

namespace Ui
{
    class LabelEx;
    class TextWidget;

    namespace TextRendering
    {
        class TextUnit;
    }

    class PlaceholderButton : public ClickableWidget
    {
        Q_OBJECT
    public:
        explicit PlaceholderButton(QWidget* _parent);
        ~PlaceholderButton();

        void setText(const QString& _text);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> text_;
    };

    class Placeholder : public QWidget
    {
        Q_OBJECT
    public:
        enum class Type
        {
            Recents,
            Contacts
        };

        Placeholder(QWidget* _parent, Type);
        void setPictureOnlyView(bool _isPictureOnly);

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void updateLink();

    private:
        const Type type_;
        QString link_;
        QWidget* noRecentsWidget_ = nullptr;
        TextWidget* noRecentsPromt_ = nullptr;
        TextWidget* noRecentsTextWidget_ = nullptr;
        bool isPicrureOnly_;
    };

    class RecentsPlaceholder : public Placeholder
    {
        Q_OBJECT
    public:
        RecentsPlaceholder(QWidget* _parent = nullptr);
    };

    class ContactsPlaceholder : public Placeholder
    {
        Q_OBJECT
    public:
        ContactsPlaceholder(QWidget* _parent = nullptr);
    };

    class RotatingSpinner : public QWidget
    {
        Q_OBJECT
    public:
        RotatingSpinner(QWidget* _parent = nullptr);
        ~RotatingSpinner();

        void startAnimation(const QColor& _spinnerColor = QColor(), const QColor& _bgColor = QColor());
        void stopAnimation();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        anim::Animation anim_;
        int currentAngle_;
        QColor mainColor_;
        QColor bgColor_;
    };
}

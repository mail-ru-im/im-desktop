#pragma once

#include "../animation/animation.h"

namespace Ui
{
    class LabelEx;

    class RecentsPlaceholder : public QWidget
    {
    public:
        RecentsPlaceholder(QWidget* _parent = nullptr);
        void setPictureOnlyView(bool _isPictureOnly);
    protected:
        void resizeEvent(QResizeEvent* _event) override;
    private:
        QWidget* noRecentsWidget_;
        LabelEx* noRecentsLabel_;
        QString originalLabel_;
        bool isPicrureOnly_;
    };

    class ContactsPlaceholder : public QWidget
    {
    public:
        ContactsPlaceholder(QWidget* _parent = nullptr);
    };

    class DialogPlaceholder : public QWidget
    {
    public:
        DialogPlaceholder(QWidget* _parent = nullptr);
    };

    class RotatingSpinner : public QWidget
    {
    public:
        RotatingSpinner(QWidget* _parent = nullptr);
        ~RotatingSpinner();

        void startAnimation(const QColor& _spinnerColor = QColor(), const QColor& _bgColor = QColor());
        void stopAnimation();

    protected:
        void paintEvent(QPaintEvent* _event);

    private:
        anim::Animation anim_;
        int currentAngle_;
        QColor mainColor_;
        QColor bgColor_;
    };
}

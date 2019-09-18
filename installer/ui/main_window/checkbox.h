#pragma once

namespace installer
{
    namespace ui
    {
        class BundleCheckBox : public QAbstractButton
        {
            Q_OBJECT

        public:
            BundleCheckBox(QWidget* _parent, const QString& _text);

        protected:
            void enterEvent(QEvent* _e) override;
            void leaveEvent(QEvent* _e) override;
            void paintEvent(QPaintEvent* _e) override;

        private:
            QWidget* icon_;
            QLabel* label_;

            void setHovered(const bool _hovered);
        };
    }
}
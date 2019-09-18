#pragma once

namespace Ui
{
    class smiles_Widget;

    namespace Smiles
    {
        class AttachedView
        {
            QWidget* View_;
            QWidget* ViewParent_;

        public:

            AttachedView(QWidget* _view, QWidget* _viewParent = nullptr);

            QWidget* getView() const;
            QWidget* getViewParent() const;
        };


        class AddButton : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS :
            void clicked();

        public:
            AddButton(QWidget* _parent);

        protected:
            void paintEvent(QPaintEvent* _e) override;
        };


        //////////////////////////////////////////////////////////////////////////
        // TabButton
        //////////////////////////////////////////////////////////////////////////

        class TabButton : public QPushButton
        {
            Q_OBJECT

        public:
            TabButton(QWidget* _parent, const QPixmap& _pixmap);

            void AttachView(const AttachedView& _view);
            const AttachedView& GetAttachedView() const;

            void setFixed(const bool _isFixed);
            bool isFixed() const;

        protected:
            void paintEvent(QPaintEvent* _e) override;

        private:
            AttachedView attachedView_;
            bool fixed_;
            QPixmap pixmap_;
        };

        //////////////////////////////////////////////////////////////////////////
        // Toolbar
        //////////////////////////////////////////////////////////////////////////

        class ToolbarScrollArea : public QScrollArea
        {
            Q_OBJECT
        public:
            ToolbarScrollArea(QWidget* _parent) : QScrollArea(_parent) {}
            void setRightMargin(const int _margin)
            {
                setViewportMargins(0, 0, _margin, 0);
            }
        };


        class Toolbar : public QFrame
        {
            Q_OBJECT

        public:
            enum class buttonsAlign
            {
                center = 1,
                left = 2,
                right = 3
            };

        private:

            enum class direction
            {
                left = 0,
                right = 1
            };

            buttonsAlign align_;
            QHBoxLayout* horLayout_;
            std::list<TabButton*> buttons_;
            ToolbarScrollArea* viewArea_;

            AddButton* buttonStore_;

            QPropertyAnimation* AnimScroll_;

            void addButton(TabButton* _button);
            void scrollStep(direction _direction);

        private Q_SLOTS:
            void touchScrollStateChanged(QScroller::State);
            void buttonStoreClick();

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void resizeEvent(QResizeEvent * _e) override;
            void wheelEvent(QWheelEvent* _e) override;

        public:
            Toolbar(QWidget* _parent, buttonsAlign _align);
            ~Toolbar();

            void Clear(const bool _delFixed = false);

            TabButton* addButton(const QPixmap& _icon);
            TabButton* selectedButton() const;

            void scrollToButton(TabButton* _button);

            const std::list<TabButton*>& GetButtons() const;

            void addButtonStore();

            void selectNext();
            void selectPrevious();

            void selectFirst();
            void selectLast();

            bool isFirstSelected() const;
            bool isLastSelected() const;
        };
    }
}
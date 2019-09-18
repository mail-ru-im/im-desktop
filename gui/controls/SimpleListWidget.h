#pragma once

namespace Ui
{
    class SimpleListItem : public QWidget
    {
        Q_OBJECT

    public:
        explicit SimpleListItem(QWidget* _parent = nullptr);
        ~SimpleListItem();

        virtual void setSelected(bool _value);
        virtual bool isSelected() const;

        virtual void setCompactMode(bool _value);
        virtual bool isCompactMode() const;

        void setPressed(const bool _isPressed);
        bool isPressed() const noexcept;

        bool isHovered() const noexcept;

        void setHoverStateCursor(Qt::CursorShape _value);
        Qt::CursorShape getHoverStateCursor() const noexcept;

    Q_SIGNALS:
        void hoverChanged(bool, QPrivateSignal) const;

    protected:
        bool event(QEvent* _event) override;

    private:
        bool isHovered_ = false;
        bool isPressed_ = false;

        Qt::CursorShape hoverStateCursor_;
    };

    class SimpleListWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit SimpleListWidget(Qt::Orientation _o, QWidget* _parent = nullptr);
        ~SimpleListWidget();

        int addItem(SimpleListItem* _tab);

        void setCurrentIndex(int _index);
        int getCurrentIndex() const noexcept;

        bool isValidIndex(int _index) const noexcept;

        SimpleListItem* itemAt(int _index) const;

        int count() const noexcept;

        void clear();
        void clearSelection();

        void setSpacing(const int _spacing);

    Q_SIGNALS:
        void clicked(int, QPrivateSignal) const;
        void rightClicked(int, QPrivateSignal) const;
        void shown(QPrivateSignal) const;

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        void updatePressedState();

    private:
        int currentIndex_ = -1;
        std::vector<SimpleListItem*> items_;

        struct PressedIndex
        {
            int index_ = -1;
            Qt::MouseButton button_ = Qt::NoButton;
        } pressedIndex_;
    };
}
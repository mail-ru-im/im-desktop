#pragma once

#include "controls/SimpleListWidget.h"

namespace Utils
{
    class OpacityEffect;
}

namespace Ui
{
    class HistoryControlPageItem;

    enum class MenuButtonType
    {
        Thread,
        Reaction,
    };

    class CornerMenuButton : public SimpleListItem
    {
        Q_OBJECT

    public:
        CornerMenuButton(MenuButtonType _type, QWidget* _parent);

        void setIcon(QStringView _iconPath);
        void setActiveIcon(QStringView _iconPath);
        MenuButtonType getType() const noexcept { return type_; }

        void setSelected(bool _value) override;
        bool isSelected() const override { return isSelected_; }

        std::vector<Qt::MouseButton> acceptedButtons() const override { return { Qt::LeftButton }; }

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        MenuButtonType type_;
        QString iconNormal_;
        QString iconActive_;
        bool isSelected_ = false;
    };

    class CornerMenuContainer : public QWidget
    {
        Q_OBJECT

    public:
        CornerMenuContainer(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _e) override;
    };

    class CornerMenu : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void buttonClicked(MenuButtonType _button, QPrivateSignal) const;
        void hidden(QPrivateSignal) const;
        void mouseLeft(QPrivateSignal) const;

    public:
        CornerMenu(QWidget* _parent);

        void clear();
        void addButton(MenuButtonType _type);
        void addButton(MenuButtonType _type, QStringView _iconNormal, QStringView _iconActive);
        QRect getButtonRect(MenuButtonType _type) const; // in global coords, empty if not found

        void showAnimated();
        void hideAnimated();
        void forceHide();

        void setForceVisible(bool _forceVisible);
        bool isForceVisible() const noexcept { return forceVisible_; }

        void setButtonSelected(MenuButtonType _type, bool _selected);

        int buttonsCount() const;
        bool hasButtons() const;
        bool hasButtonOfType(MenuButtonType _type) const;

        void updateHoverState();

        static QSize calcSize(int _buttonCount);
        static int shadowOffset();

    private:
        void initAnimation();
        void animate(QAbstractAnimation::Direction _dir);
        void onItemClicked(int _idx);

    protected:
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void wheelEvent(QWheelEvent* _e) override;

    private:
        CornerMenuButton* getButton(MenuButtonType _type) const;

    private:
        QVariantAnimation* animation_ = nullptr;
        Utils::OpacityEffect* opacityEffect_ = nullptr;
        SimpleListWidget* listWidget_ = nullptr;
        bool forceVisible_ = false;
    };
}
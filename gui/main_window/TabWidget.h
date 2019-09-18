#pragma once

namespace Ui
{
    class TabBar;

    class TabWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit TabWidget(QWidget* _parent = nullptr);
        ~TabWidget();

        void hideTabbar();
        void showTabbar();

        int addTab(QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive);

        void setCurrentIndex(int _index);
        int currentIndex() const noexcept;

        TabBar* tabBar() const;

        void setBadgeText(int _index, const QString& _text);
        void setBadgeIcon(int _index, const QString& _icon);

    Q_SIGNALS:
        void tabBarClicked(int, QPrivateSignal) const;
        void currentChanged(int, QPrivateSignal) const;

    private:
        void onTabClicked(int _index);

    private:
        QStackedWidget* pages_;
        TabBar* tabbar_;
    };
}
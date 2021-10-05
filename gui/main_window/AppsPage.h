#pragma once

namespace Ui
{
    class MainPage;
    class AppsBar;
    class AppBarItem;
    class SemitransparentWindowAnimated;

    enum class AppPageType
    {
        messenger,
        tasks,
        org_structure,
        calendar,
        settings,
        calls,
        contacts,
    };

    class AppsNavigationBar : public QWidget
    {
        Q_OBJECT
    public:
        explicit AppsNavigationBar(QWidget* _parent = nullptr);

        AppBarItem* addTab(AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive);
        AppBarItem* addBottomTab(AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive);

        void removeTab(AppPageType _type);
        bool contains(AppPageType _type) const;
        static int defaultWidth();

    public Q_SLOTS:
        void selectTab(AppPageType _type);
        void selectTab(AppsBar* _bar, int _index);
        void onRightClick(AppsBar* _bar, int _index);

    Q_SIGNALS:
        void appTabClicked(AppPageType _newTab, AppPageType _oldTab, QPrivateSignal);
        void reloadWebPage(AppPageType _pageType, QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;

        AppPageType getTabType(AppsBar* _bar, int _index) const;
        AppPageType currentPage() const;
        AppBarItem* addTab(AppsBar* _bar, int _index, AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive);
        std::pair<AppsBar*, int> getBarAndIndexByType(AppPageType _type);

    private:
        AppsBar* appsBar_;
        AppsBar* bottomAppsBar_;
    };

    class AppsPage : public QWidget
    {
        Q_OBJECT
    public:
        explicit AppsPage(QWidget* _parent = nullptr);
        ~AppsPage();

        MainPage* messengerPage() const;

        bool isMessengerPage() const;
        bool isContactDialog() const;
        bool isTasksPage() const;
        void resetPages();
        void prepareForShow();

        void showMessengerPage();

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        bool containsTab(AppPageType _type) const;
        Ui::AppBarItem* addTab(AppPageType _type, QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive);
        void removeTab(AppPageType _type, bool showDefaultPage = true);
        void selectTab(AppPageType _tabIndex);
        void addCallsTab();

        void updateWebPages();
        void updatePagesAimsids();
        void updateNavigationBarVisibility();
        void onAuthParamsChanged(bool _aimsidChanged);
        void onConfigChanged();
        void onGuiSettingsChanged(const QString& _key);

    private Q_SLOTS:
        void onTabChanged(AppPageType _pageType);
        void onTabClicked(AppPageType _pageType, AppPageType _oldTab);
        void loggedIn();
        void chatUnreadChanged();
        void reloadWebAppPage(AppPageType _pageType);

    private:
        QStackedWidget* pages_;
        std::unordered_map<AppPageType, QPointer<QWidget>> pagesByType_;
        MainPage* messengerPage_;
        AppsNavigationBar* navigationBar_;
        AppBarItem* messengerItem_;
        SemitransparentWindowAnimated* semiWindow_;
        bool authDataValid_ = false;
    };
}
namespace Ui
{
    class MainPage;
    class AppsBar;
    class AppBarItem;
    class SemitransparentWindowAnimated;

    class AppsNavigationBar : public QWidget
    {
        Q_OBJECT
    public:
        explicit AppsNavigationBar(QWidget* _parent = nullptr);
        AppBarItem* addTab(const QString& _name, const QString& _icon, const QString& _iconActive);
        static int defaultWidth();

    public Q_SLOTS:
        void selectTab(int index);

    Q_SIGNALS:
        void tabSelected(int _tabIndex, QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        AppsBar* appsBar_;
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
        void resetMessengerPage();
        void prepareForShow();

        void showMessengerPage();

        void addTab(QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive);

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void selectTab(int _tabIndex);

    private Q_SLOTS:
        void onTabSelected(int _tabIndex);

    private:
        QStackedWidget* pages_;
        MainPage* messengerPage_;
        AppsNavigationBar* navigationBar_;
        SemitransparentWindowAnimated* semiWindow_;
    };
}
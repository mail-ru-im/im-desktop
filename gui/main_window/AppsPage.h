#pragma once
#include "../core_dispatcher.h"

namespace Logic
{
    enum class KeepWebPage;
}

namespace Ui
{
    class MainPage;
    class WebAppPage;
    class AppsBar;
    class AppBarItem;
    class SemitransparentWindowAnimated;
    class CustomButton;
    class ConnectionWidget;
    class GradientWidget;
    class ContactAvatarWidget;

    namespace TextRendering
    {
        class TextUnit;
    }

    struct AppPageData
    {
        QString type_;
        QString name_;
        QString icon_;
        QString iconActive_;

        AppPageData(const QString& _type, const QString& _name, const QString& _icon, const QString& _iconActive)
            : type_(_type)
            , name_(_name)
            , icon_(_icon)
            , iconActive_(_iconActive)
        {}
    };

    struct AppsHeaderData
    {
        AppsHeaderData(const QString& _title = {})
            : title_(_title)
            , leftButtonEnabled_(true)
            , rightButtonEnabled_(true)
        {
        }

        bool isEmpty() const { return title_.isEmpty(); }

        void setTitle(const QString& _title) { title_ = _title; }
        void setLeftButton(const QString& _text, const QString& _color, bool _enabled);
        void setRightButton(const QString& _text, const QString& _color, bool _enabled);

        void clear();

        QString title() const { return title_; }
        QString leftButtonText() const { return leftButtonText_; }
        QString rightButtonText() const { return rightButtonText_; }
        QString leftButtonColor() const { return leftButtonColor_; }
        QString rightButtonColor() const { return rightButtonColor_; }
        bool leftButtonEnabled() const { return leftButtonEnabled_; }
        bool rightButtonEnabled() const { return rightButtonEnabled_; }

    private:
        QString title_;
        QString leftButtonText_;
        QString rightButtonText_;
        QString leftButtonColor_;
        QString rightButtonColor_;
        bool leftButtonEnabled_;
        bool rightButtonEnabled_;
    };

    class AppsHeader : public QWidget
    {
        Q_OBJECT
    public:
        explicit AppsHeader(QWidget* _parent = nullptr);
        void setData(const QString& _type, const AppsHeaderData& _data);
        const QString& currentType() const { return currentType_; }
        void updateVisibility(bool _isVisible);

    Q_SIGNALS:
        void leftClicked(const QString& _type);
        void rightClicked(const QString& _type);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void setTitleText(const QString& _text);

    private Q_SLOTS:
        void connectionStateChanged(const Ui::ConnectionState& _state);

    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> title_;
        CustomButton* left_;
        CustomButton* right_;
        ConnectionWidget* connectionWidget_;
        Ui::ConnectionState state_;
        QString currentType_;
         bool visible_;
    };

    class AppsNavigationBar : public QWidget
    {
        Q_OBJECT
    public:
        explicit AppsNavigationBar(QWidget* _parent = nullptr);

        AppBarItem* addTab(const std::vector<QString>& _order, const AppPageData& _data);
        AppBarItem* addBottomTab(const AppPageData& _data);
        void updateTabIcons(const QString& _type);

        void removeTab(const QString& _type);
        bool contains(const QString& _type) const;
        static int defaultWidth();

        AppsBar* bottomAppsBar() const { return bottomAppsBar_; };
        QString currentPage() const;
        void clearAppsSelection();
        void updateTabInfo(const QString& _type);
        void resetAvatar();

    public Q_SLOTS:
        void selectTab(const QString& _type, bool _emitClick = true);
        void selectTab(AppsBar* _bar, int _index, bool _emitClick = true);
        void onRightClick(AppsBar* _bar, int _index);
        void onScroll(int _value);

    Q_SIGNALS:
        void appTabClicked(const QString& _newTab, const QString& _oldTab, QPrivateSignal);
        void reloadWebPage(const QString& _pageType, QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

        QString getTabType(AppsBar* _bar, int _index) const;
        AppBarItem* addTab(AppsBar* _bar, int _index, const AppPageData& _data);
        std::pair<AppsBar*, int> getBarAndIndexByType(const QString& _type);

        void addScrollArea(QVBoxLayout* _rootLayout);

    private:
        void resetGradient();

    private:
        AppsBar* appsBar_ = nullptr;
        AppsBar* bottomAppsBar_ = nullptr;
        QScrollArea* scrollArea_ = nullptr;
        GradientWidget* gradientTop_ = nullptr;
        GradientWidget* gradientBottom_ = nullptr;
        ContactAvatarWidget* avatarWidget_ = nullptr;
    };

    class AppsPage : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void reopenMessengerDialog(const QString& _aimId);
    public:
        explicit AppsPage(QWidget* _parent = nullptr);
        ~AppsPage();

        MainPage* messengerPage() const;
        WebAppPage* tasksPage() const;

        bool isMessengerPage() const;
        bool isContactDialog() const;
        bool isTasksPage() const;
        bool isFeedPage() const;
        void resetPages();
        void prepareForShow();

        void showMessengerPage();

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;
        int getHeaderHeight() const;
        void onSettingsClicked();
        void setHeaderVisible(bool _isVisible);

        void setPageToReopen(const QString& _aimId);

    protected:
        void resizeEvent(QResizeEvent* _event) override;
#ifdef HAS_WEB_ENGINE
        WebAppPage* getWebPage(const QString& _id);
#endif

    private:
        QString getCurrentPage() const;
        bool containsTab(const QString& _type) const;
        Ui::AppBarItem* addTab(QWidget* _widget, const AppPageData& _data, const QString& _defaultTitle = {}, bool _selected = false);
        void removeTab(const QString& _type, bool showDefaultPage = true);
        void addCallsTab();
        void updateCallsTab();
        void addSettingsTab();

        void updateWebPages();

        void updatePagesAimsids();
        void updateNavigationBarVisibility();
        void onAuthParamsChanged(bool _aimsidChanged);
        void onConfigChanged();
        void onMyInfoReceived();
        void onLogout();
        void updateTabIcon(const QString& _id);
        void updateTabCounter(const QString& _miniAppId, int _counter);
        void initNavigationBar();
        void addMessengerPage();
        void addInterConnectorConnections();
        void addDispatcherConnections();

    private Q_SLOTS:
        void openApp(const QString& _appId, const QString& _urlQuery = {}, const QString& _urlFragment = {});
        void selectTab(const QString& _appType, bool _emitClick = true);
        void onTabChanged(const QString& _pageType);
        void onTabClicked(const QString& _pageType, const QString& _oldTab);
        void loggedIn();
        void chatUnreadChanged();
        void reloadWebAppPage(const QString& _pageType);
        void onSetTitle(const QString& _type, const QString& _title);
        void onSetLeftButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled);
        void onSetRightButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled);
        void composeEmail(const QString& _email);
        void appUnavailable(const QString& _id);

        void onAppsUpdate(const QString& _selectedId);
        void onRemoveTab(const QString& _id);
        void onRemoveApp(const QString& _id, Logic::KeepWebPage _keepAppPage);
        void onUpdateAppFields(const QString& _id);
        void onRemoveHiddenPage(const QString& _id);
        void onThemeChanged();

        void reopenDialog(const QString& _aimid);

    private:
        QStackedWidget* pages_;

        struct PageData
        {
            QPointer<QWidget> page_;
            QPointer<QWidget> tabItem_;
        };

        std::vector<QString> appTabsOrder_;
        std::unordered_map<QString, PageData> pagesByType_;
        std::unordered_map<QString, AppsHeaderData> headerData_;
        MainPage* messengerPage_;
        AppsNavigationBar* navigationBar_;
        AppBarItem* messengerItem_;
        SemitransparentWindowAnimated* semiWindow_;
        AppsHeader* header_;
        QString currentMessengerDialog_;
        bool authDataValid_ = false;
#ifdef HAS_WEB_ENGINE
        void updateWebPage(const QString& _id);
#endif
    };
}

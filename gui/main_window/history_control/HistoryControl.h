#pragma once

#include "types/message.h"
#include "styles/WallpaperId.h"
#include "main_window/EscapeCancellable.h"
#include "controls/BackgroundWidget.h"

namespace core
{
    enum class typing_status;
}

namespace Ui
{
    using highlightsV = std::vector<QString>;

    enum class ConnectionState;
    class ConnectionWidget;
    enum class FrameCountMode;
    enum class ClosePage;

    class PageBase;
    class HistoryControlPage;
    class FeedPage;
    class ConnectionWidget;
    class BubblePlateWidget;
    class SelectDialogWidget;
    class StartCallDialogWidget;

    enum class PageOpenedAs;

    class EmptyConnectionInfoPage : public BackgroundWidget
    {
    public:
        enum class OnlineWidgetType
        {
            Chat,
            Calls,
        };

        explicit EmptyConnectionInfoPage(QWidget* _parent = nullptr);
        void setPageType(OnlineWidgetType _pageType);

    private:
        void onGlobalWallpaperChanged();
        void connectionStateChanged(const Ui::ConnectionState& _state);
        void updateCentralWidget();

    private:
        void onGlobalThemeChanged();

    private:
        QWidget* dialogWidget_;
        BubblePlateWidget* bubblePlate_;
        ConnectionWidget* connectionWidget_;
        SelectDialogWidget* selectDialogWidget_;
        StartCallDialogWidget* startCallDialogWidget_;

        ConnectionState connectionState_;
        OnlineWidgetType pageType_;
    };


    class HistoryControl : public QWidget, public IEscapeCancellable
    {
        Q_OBJECT

    Q_SIGNALS:
        void needUpdateWallpaper(const QString& _aimId, QPrivateSignal) const;
        void setTopWidget(const QString, QWidget*, QPrivateSignal) const;

    public Q_SLOTS:
        void contactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights = {}, bool _ignoreScroll = false);

        void addPageToDialogHistory(const QString& _aimId);
        void clearDialogHistory();
        void switchToPrevDialogPage(const bool _calledFromKeyboard);

        void switchToEmpty();
    public:
        HistoryControl(QWidget* parent);
        ~HistoryControl();

        void cancelSelection();
        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        PageBase* getPage(const QString& _aimId) const;
        bool hasMessageUnderCursor() const;

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        void scrollHistoryToBottom(const QString& _contact) const;
        const QString& currentAimId() const;
        void setFrameCountMode(FrameCountMode _mode);
        FrameCountMode getFrameCountMode() const noexcept { return frameCountMode_; }

        void setPageOpenedAs(PageOpenedAs _openedAs);

    private Q_SLOTS:
        void updatePages();
        void closeDialog(const QString& _aimId);
        void leaveDialog(const QString& _aimId);
        void mainPageChanged();
        void onContactSelected(const QString& _aimId);
        void onBackToChats();

        void updateUnreads();
        void onGlobalWallpaperChanged();
        void onGlobalThemeChanged();
        void onContactWallpaperChanged(const QString& _aimId);
        void onWallpaperAvailable(const Styling::WallpaperId& _id);
        void onActiveDialogHide(const QString& _aimId, Ui::ClosePage _closePage);

    private:
        HistoryControlPage* getCurrentHistoryPage() const;
        PageBase* getCurrentPage() const;
        void updateWallpaper(const QString& _aimId = QString()) const;
        void updateEmptyPageWallpaper() const;

        void rememberCurrentDialogTime();
        void suggestNotifyUser(const QString& _contact, const QString& _smsContext);

        void suspendBackgroundPages();

        void openGallery(const QString& _contact);

        PageBase* createPage(const QString& _aimId);

        QHash<QString, PageBase*> pages_;
        QMap<QString, QTime> times_;
        QString current_;
        QTimer* timer_;
        QStackedWidget* stackPages_;
        EmptyConnectionInfoPage* emptyPage_;
        std::deque<QString> dialogHistory_;
        FrameCountMode frameCountMode_;
        QString unreadText_;

        struct
        {
            QString aimId_;
            qint64 messageId_ = -1;
            void clear()
            {
                aimId_.clear();
                messageId_ = -1;
            }
        } lastInitedParams_;

        PageOpenedAs openedAs_;
    };
}

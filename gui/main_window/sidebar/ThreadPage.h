#pragma once

#include "Sidebar.h"

namespace Data
{
    class DlgState;
}

namespace Ui
{
    class BackgroundWidget;
    class HistoryControlPage;
    class TopPanelWidget;
    class RecentsTab;
    enum class FrameCountMode;

    enum class Tab
    {
        Main = 0,
        Search = 1,
    };

    class ThreadPage : public SidebarPage
    {
        Q_OBJECT

    public:
        ThreadPage(QWidget* _parent);
        ~ThreadPage();

        void initFor(const QString& _aimId, SidebarParams _params = {}) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;
        QString parentAimId() const override { return parentAimId_; }
        void onPageBecomeVisible() override;
        bool hasInputOrHistoryFocus() const override;
        bool hasSearchFocus() const override;
        void updateWidgetsTheme() override;

    private:
        void dlgStates(const QVector<Data::DlgState>& _states);
        void startUnloadTimer();
        void stopUnloadTimer();
        void unloadPage();

        void initSearch();
        void changeTab(Tab _tab);

        void onSearchStart(const QString& _aimId);
        void onSearchEnd();

        void updateBackButton();

    private Q_SLOTS:
        void onScrollThreadToMsg(const QString& _threadId, int64_t _msgId, const highlightsV& _highlights = {});
        void onChatRoleChanged(const QString& _aimId);
        void onBackClicked(bool _searchActive);

    private:
        QStackedWidget* stackedWidget_ = nullptr;
        BackgroundWidget* bg_ = nullptr;
        HistoryControlPage* page_ = nullptr;
        QWidget* searchWidget_ = nullptr;
        TopPanelWidget* topWidget_ = nullptr;
        RecentsTab* recentsTab_ = nullptr;
        QTimer* unloadTimer_ = nullptr;
        QString parentAimId_;
        FrameCountMode mode_;
    };
}

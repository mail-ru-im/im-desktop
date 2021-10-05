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
    enum class FrameCountMode;

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

    private:
        void dlgStates(const QVector<Data::DlgState>& _states);
        void startUnloadTimer();
        void stopUnloadTimer();
        void unloadPage();
        
    private Q_SLOTS:
        void onScrollThreadToMsg(const QString& _threadId, int64_t _msgId);
        void onChatRoleChanged(const QString& _aimId);

    private:
        BackgroundWidget* bg_ = nullptr;
        HistoryControlPage* page_ = nullptr;
        QTimer* unloadTimer_ = nullptr;
        QString parentAimId_;
    };
}

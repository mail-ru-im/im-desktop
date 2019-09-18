#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <unordered_set>
#include "../../app_config.h"
#include "controls/GeneralCreator.h"
#include "main_window/sidebar/SidebarUtils.h"


namespace Ui
{
    class GeneralDialog;
    class TextEditEx;
    class LineEditEx;
    class ContextMenu;
    class CustomButton;

    class MemoryUsageWidget : public QWidget
    {
        Q_OBJECT
    public:
        MemoryUsageWidget(QWidget* parent);

    public Q_SLOTS:
        void onRefresh();

    private Q_SLOTS:
        void onMemoryUsageReportReady(const Memory_Stats::Response& _response,
                                      Memory_Stats::RequestId _req_id);

    private:
        void init();
        void redrawReportTextEdit(const Memory_Stats::Response& _response);
        QString stringForReport(const Memory_Stats::MemoryStatsReport& _report);
        QString stringForSubcategory(const Memory_Stats::MemoryStatsReport::MemoryStatsSubcategory& _subcategory);
        static QString typeString(Memory_Stats::StatType _type);

        bool isPending(Memory_Stats::RequestId _req_id);
        void addPending(Memory_Stats::RequestId _req_id);
        void removePending(Memory_Stats::RequestId _req_id);

    private:
        QVBoxLayout* mainLayout_ = nullptr;
        Ui::TextEditEx* reportText_ = nullptr;
        QPushButton* refreshButton_ = nullptr;
        QPushButton* copyTextButton_ = nullptr;
        QLabel* ramUsageLabel_ = nullptr;

        qint64 archiveIndexSize_ = 0;
        qint64 archiveGallerySize_ = 0;

        std::unordered_set<Memory_Stats::RequestId> pendingRequests_;
    };

    class MemorySettingsWidget: public QWidget
    {
    public:
        enum class Mode
        {
            SwitcherOnly,
            FullLayout
        };

    public:
        MemorySettingsWidget(Mode _mode, QWidget* _parent = nullptr);

        void notifyCacheDialogsTimeoutChanged(const QString& newTimeout);
        void addDockWidget(bool doAdd);

    private:
        Ui::SidebarCheckboxButton* stickDockWidgets_;
        GeneralCreator::DropperInfo cacheDialogsDropper_;
        MemoryUsageWidget* memoryUsageWidget_ = nullptr;
        QVBoxLayout* globalLayout_ = nullptr;
    };

    class SettingsForTestersDialog : public QDialog
    {
        Q_OBJECT

    public:
        SettingsForTestersDialog(const QString& _labelText, QWidget* _parent);
        ~SettingsForTestersDialog();

        using QDialog::show;
        bool show();

    private:
        void initViewElementsFrom(const AppConfig& appConfig);

    Q_SIGNALS:
        void openLogsPath(const QString& logsPath);

    private Q_SLOTS:
        void onOpenLogsPath(const QString& logsPath);
        void onToggleFullLogMode(bool checked);
        void onToggleUpdateble(bool checked);
        void onToggleShowMsgIdsMenu(bool checked);
        void onToggleSaveRTPDumps(bool checked);
        void onToggleServerSearch(bool checked);
        void notifyCacheDialogsTimeoutChanged(const QString& newTimeout);
        void onToggleMemUsageWidget();

    private:
        QGridLayout* gridLayout_ = nullptr;
        QVBoxLayout* globalLayout_ = nullptr;
        QWidget* mainWidget_ = nullptr;

        Ui::CustomButton* memUsageToggleBtn_ = nullptr;
        QVBoxLayout* memUsageLayout_ = nullptr;

        Ui::CustomButton* openLogsBtn_ = nullptr;
        Ui::CustomButton* clearCacheBtn_ = nullptr;
        Ui::CustomButton* clearAvatarsBtn_ = nullptr;
        Ui::CustomButton* resetUserAgreement_ = nullptr;
        Ui::CustomButton* logCurrentMessagesModel_ = nullptr;
        Ui::SidebarCheckboxButton* fullLogModeCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* updatebleCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devShowMsgIdsCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devSaveCallRTPdumpsCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devServerSearchCheckbox_ = nullptr;

        Ui::SidebarCheckboxButton* stickDockWidgets_;
        GeneralCreator::DropperInfo cacheDialogsDropper_;
        MemoryUsageWidget* memoryUsageWidget_ = nullptr;

        MemorySettingsWidget* memorySettingsWidget_ = nullptr;
        std::unique_ptr<GeneralDialog>  mainDialog_;
    };

}

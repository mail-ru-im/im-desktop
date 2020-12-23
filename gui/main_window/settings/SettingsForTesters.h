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

    class SettingsForTesters : public QWidget
    {
        Q_OBJECT

    public:
        SettingsForTesters(QWidget* _parent);
        ~SettingsForTesters();

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
        void onToggleDevId(bool checked);
        void onToggleWatchGuiMemory(bool checked);

    private:
        QVBoxLayout* mainLayout_ = nullptr;
        QWidget* mainWidget_ = nullptr;

        Ui::CustomButton* openLogsBtn_ = nullptr;
        Ui::CustomButton* clearCacheBtn_ = nullptr;
        Ui::CustomButton* clearAvatarsBtn_ = nullptr;
        Ui::CustomButton* logCurrentMessagesModel_ = nullptr;
        Ui::CustomButton* logMemorySnapshot_ = nullptr;
        Ui::SidebarCheckboxButton* fullLogModeCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* updatebleCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devShowMsgIdsCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devSaveCallRTPdumpsCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devServerSearchCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* devCustomIdCheckbox_ = nullptr;
        Ui::SidebarCheckboxButton* watchGuiMemoryCheckbox_ = nullptr;

        Ui::CustomButton* sendDevStatistic_ = nullptr;

        Ui::SidebarCheckboxButton* stickDockWidgets_;
        GeneralCreator::DropperInfo cacheDialogsDropper_;
        std::unique_ptr<GeneralDialog>  mainDialog_;
    };

}

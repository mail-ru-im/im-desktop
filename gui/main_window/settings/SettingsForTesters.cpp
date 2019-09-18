#include "SettingsForTesters.h"

#include <QDesktopServices>
#include <QDockWidget>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <set>

#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "controls/GeneralDialog.h"
#include "controls/GeneralCreator.h"
#include "controls/CustomButton.h"
#include "controls/DialogButton.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../AcceptAgreementInfo.h"
#include "controls/TextEditEx.h"
#include "controls/LineEditEx.h"
#include "../../controls/TransparentScrollBar.h"
#include "gui_settings.h"

#include "../contact_list/ContactListModel.h"

#include "../../styles/ThemeParameters.h"
#include "main_window/sidebar/SidebarUtils.h"

namespace
{
    constexpr auto DIALOG_WIDTH = 360;
    constexpr auto LEFT_OFFSET = 12;
    constexpr auto MARGIN = 10;
    constexpr auto ITEM_HEIGHT = 44;
    constexpr auto SMALL_BTN_WIDTH = 24;
    constexpr auto SMALL_BTN_HEIGHT = 20;
    constexpr auto DEFAULT_REFRESH_MEMUSAGE_TIMEOUT = 1000;
}

namespace Ui
{

MemorySettingsWidget::MemorySettingsWidget(Mode _mode, QWidget *_parent)
    : QWidget(_parent)
    , stickDockWidgets_(nullptr)
{
    globalLayout_ = Utils::emptyVLayout(this);

    auto mw = Utils::InterConnector::instance().getMainWindow();

    stickDockWidgets_ = GeneralCreator::addSwitcher(this, globalLayout_,
                                                    QT_TRANSLATE_NOOP("popup_window", "Stick memory usage widget"),
                                                    mw->hasMemoryDockWidget(),
                                                    [](bool enabled) -> QString {
                                                        Q_UNUSED(enabled);
                                                        return QString();
                                                    },
                                                    Utils::scale_value(36));
    connect(stickDockWidgets_, &Ui::SidebarCheckboxButton::checked, this, &MemorySettingsWidget::addDockWidget);

    if (_mode == Mode::SwitcherOnly)
        return;

    std::set<int> cacheVals = { 0, GetAppConfig().CacheHistoryControlPagesFor(), 1000 };
    std::vector<QString> vals;
    int selected = 0, j = 0;
    for (auto cache: cacheVals)
    {
        if (cache == GetAppConfig().CacheHistoryControlPagesFor())
            selected = j;

        vals.push_back(QString::number(cache));
        ++j;
    }

    // set up memory usage layout
    cacheDialogsDropper_ = GeneralCreator::addDropper(
                this,
                globalLayout_,
                QT_TRANSLATE_NOOP("popup_window", "Cache dialogs for (milliseconds)"),
                false,
                vals,
                selected /* selected index */,
                -1 /* width */,
                [this](QString v, int, TextEmojiWidget*)
                {
                    notifyCacheDialogsTimeoutChanged(v);
                },
                [](bool) -> QString { return QString(); });

    memoryUsageWidget_ = new MemoryUsageWidget(this);
    memoryUsageWidget_->setFixedWidth(DIALOG_WIDTH / 2);
    memoryUsageWidget_->setStyleSheet(qsl("background-color: rgb(255,0,0); margin:5px; border:1px solid rgb(0, 255, 0);"));

    globalLayout_->addWidget(memoryUsageWidget_);
}

void MemorySettingsWidget::notifyCacheDialogsTimeoutChanged(const QString& newTimeout)
{
    bool ok;
    int newValue = newTimeout.toInt(&ok);

    if (!ok || newValue < 0)
        return;

    AppConfig appConfig = GetAppConfig();
    appConfig.SetCacheHistoryControlPagesFor(newValue);

    ModifyAppConfig(std::move(appConfig), [](core::icollection*) {}, this);
}

void MemorySettingsWidget::addDockWidget(bool doAdd)
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();

    if (!doAdd)
    {
        mainWindow->removeMemoryDockWidget();
        return;
    }

    if (mainWindow->hasMemoryDockWidget())
        return;

    AppConfig appConfig = GetAppConfig();
    appConfig.SetWatchGuiMemoryEnabled(doAdd);
    ModifyAppConfig(std::move(appConfig));

    auto dockWidget = new QDockWidget(qsl("Debug"), mainWindow);
    auto memUsageWidget = new MemoryUsageWidget(dockWidget);
    dockWidget->setWidget(memUsageWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable |  QDockWidget::DockWidgetFloatable);
    dockWidget->setAttribute(Qt::WA_DeleteOnClose);

    connect(dockWidget, &QObject::destroyed,
            this, [this](QObject*){
        QSignalBlocker blocker(stickDockWidgets_);
        stickDockWidgets_->setChecked(false);
    });

    auto dockWidgetTimer = new QTimer(dockWidget);
    connect(dockWidgetTimer, &QTimer::timeout,
            memUsageWidget, &MemoryUsageWidget::onRefresh);
    dockWidgetTimer->setSingleShot(false);
    dockWidgetTimer->start(DEFAULT_REFRESH_MEMUSAGE_TIMEOUT);

    mainWindow->addMemoryDockWidget(Qt::LeftDockWidgetArea, dockWidget);
}

SettingsForTestersDialog::SettingsForTestersDialog(const QString &_labelText, QWidget* _parent)
    : QDialog(_parent),
      mainWidget_(new QWidget(this)),
      openLogsBtn_(nullptr),
      clearCacheBtn_(nullptr),
      clearAvatarsBtn_(nullptr),
      fullLogModeCheckbox_(nullptr),
      devShowMsgIdsCheckbox_(nullptr),
      devSaveCallRTPdumpsCheckbox_(nullptr),
      memoryUsageWidget_(new MemoryUsageWidget(this))
{
    // Set up main widget
    mainWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mainWidget_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
    auto settingsStyle = qsl("Ui--CustomButton#menu_item{border-radius: 8dip; border-style: 1px solid; border-color: %1; }Ui--CustomButton:hover#menu_item { background-color: %2;}")
        .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY),
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
    Utils::ApplyStyle(mainWidget_, settingsStyle);

    auto setupButton = [this](const auto& _icon, const auto& _text, int _row, int _column, int _rowSpan, int _columnSpan)
    {
        auto btn = new CustomButton(this, _icon, QSize(20, 20));
        Styling::Buttons::setButtonDefaultColors(btn);
        btn->setText(_text);
        btn->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        btn->setMouseTracking(true);
        btn->setIconAlignment(Qt::AlignLeft);
        btn->setIconOffsets(Utils::scale_value(LEFT_OFFSET), 0);
        btn->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
        btn->setObjectName(qsl("menu_item"));

        Utils::grabTouchWidget(btn);

        gridLayout_->addWidget(btn, _row, _column, _rowSpan, _columnSpan);

        return btn;
    };

    // Set up global layout grid: two items buttons
    globalLayout_ = Utils::emptyVLayout(mainWidget_);

    gridLayout_ = Utils::emptyGridLayout();
    gridLayout_->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    gridLayout_->setSpacing(Utils::scale_value(MARGIN));

    openLogsBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Open logs path"), 0, 0, 1, -1);
    Utils::ApplyStyle(openLogsBtn_, settingsStyle);
    clearCacheBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Clear cache"), 2, 0, 1, -1);
    clearAvatarsBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Clear avatars"), 3, 0, 1, -1);
    logCurrentMessagesModel_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Log messagesModel"), 4, 0, 1, -1);

    connect(this, &SettingsForTestersDialog::openLogsPath, this, &SettingsForTestersDialog::onOpenLogsPath);
    connect(openLogsBtn_, &QPushButton::clicked, this, [this]() {
        GetDispatcher()->post_message_to_core("get_logs_path", nullptr, this,
                                              [this](core::icollection* _coll) {
            Ui::gui_coll_helper coll(_coll, false);
            const QString logsPath = QString::fromUtf8(coll.get_value_as_string("path"));
            emit openLogsPath(logsPath);
        });
    });

    connect(logCurrentMessagesModel_, &QPushButton::clicked, this, []() {
        if (const auto contact = Logic::getContactListModel()->selectedContact(); !contact.isEmpty())
            emit Utils::InterConnector::instance().logHistory(contact);
    });

    connect(clearCacheBtn_, &QPushButton::clicked, this, [this]() {
        GetDispatcher()->post_message_to_core("remove_content_cache", nullptr, this,
                                              [this](core::icollection* _coll) {
            Q_UNUSED(this);

            Ui::gui_coll_helper coll(_coll, false);
            auto last_error = coll.get_value_as_int("last_error");
            assert(!last_error);
            Q_UNUSED(last_error);
        });
    });

    connect(clearAvatarsBtn_, &QPushButton::clicked, this, [this]() {
        GetDispatcher()->post_message_to_core("clear_avatars", nullptr, this,
                                              [this](core::icollection* _coll) {
            Q_UNUSED(this);

            Ui::gui_coll_helper coll(_coll, false);
            auto last_error = coll.get_value_as_int("last_error");
            assert(!last_error);
            Q_UNUSED(last_error);
        });
    });

    auto appConfig = GetAppConfig();

    fullLogModeCheckbox_ = GeneralCreator::addSwitcher(this, gridLayout_,
                                                       QT_TRANSLATE_NOOP("popup_window", "Enable full log mode"),
                                                       appConfig.IsFullLogEnabled(),
                                                       [](bool enabled) -> QString {
                                                           Q_UNUSED(enabled);
                                                           return QString();
                                                       },
                                                       Utils::scale_value(36));

    updatebleCheckbox_ = GeneralCreator::addSwitcher(this, gridLayout_,
                                                     QT_TRANSLATE_NOOP("popup_window", "Updatable"),
                                                     appConfig.IsUpdateble(),
                                                     [](bool enabled) -> QString {
                                                         Q_UNUSED(enabled);
                                                         return QString();
                                                     },
                                                     Utils::scale_value(36));

    devShowMsgIdsCheckbox_ = GeneralCreator::addSwitcher(this, gridLayout_,
                                                         QT_TRANSLATE_NOOP("popup_window", "Display message IDs"),
                                                         appConfig.IsShowMsgIdsEnabled(),
                                                         [](bool enabled) -> QString {
                                                             Q_UNUSED(enabled);
                                                             return QString();
                                                         },
                                                         Utils::scale_value(36));

    devSaveCallRTPdumpsCheckbox_ = GeneralCreator::addSwitcher(this, gridLayout_,
                                                               QT_TRANSLATE_NOOP("popup_window", "Save call RTP dumps"),
                                                               appConfig.IsSaveCallRTPEnabled(),
                                                               [](bool enabled) -> QString { return QString(); });

    devServerSearchCheckbox_ = GeneralCreator::addSwitcher(this, gridLayout_,
                                                           QT_TRANSLATE_NOOP("popup_window", "Server search"),
                                                           appConfig.IsServerSearchEnabled(),
                                                           [](bool enabled) -> QString { return QString(); },
                                                           Utils::scale_value(36));

    connect(fullLogModeCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTestersDialog::onToggleFullLogMode);
    connect(updatebleCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTestersDialog::onToggleUpdateble);
    connect(devShowMsgIdsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTestersDialog::onToggleShowMsgIdsMenu);
    connect(devSaveCallRTPdumpsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTestersDialog::onToggleSaveRTPDumps);
    connect(devServerSearchCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTestersDialog::onToggleServerSearch);

    Testing::setAccessibleName(openLogsBtn_, qsl("AS testers openLogsBtn_"));
    Testing::setAccessibleName(clearCacheBtn_, qsl("AS testers clearCacheBtn_"));
    Testing::setAccessibleName(clearAvatarsBtn_, qsl("AS testers clearAvatarsBtn_"));

    mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_, parentWidget(), false, true, false);
    mainDialog_->addAcceptButton(QT_TRANSLATE_NOOP("popup_window", "CLOSE"), true);

    memorySettingsWidget_ = new MemorySettingsWidget(MemorySettingsWidget::Mode::SwitcherOnly, this);
    gridLayout_->addWidget(memorySettingsWidget_ /*, 3, 0, -1, -1*/);
    memorySettingsWidget_->setVisible(true);

    globalLayout_->addLayout(gridLayout_);

    auto updateUrlLayout = Utils::emptyHLayout();
    updateUrlLayout->setContentsMargins(MARGIN, 0, MARGIN, 0);
    auto updateUrlEdit = new LineEditEx(this);
    updateUrlEdit->setPlaceholderText(QT_TRANSLATE_NOOP("popup_window", "Update url, empty for default"));
    auto checkUpdateButton = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "CHECK"));

    connect(checkUpdateButton, &DialogButton::clicked, this, [updateUrlEdit]()
    {
        gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("url", updateUrlEdit->text());
        GetDispatcher()->post_message_to_core("update/check", collection.get());
    });

    updateUrlLayout->addWidget(updateUrlEdit);
    updateUrlLayout->addSpacing(Utils::scale_value(15));
    updateUrlLayout->addWidget(checkUpdateButton);

    globalLayout_->addLayout(updateUrlLayout);

    GeneralDialog::Options options;
    options.preferredSize_ = QSize(Utils::scale_value(DIALOG_WIDTH), -1);

    mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_, parentWidget(),
                                                       false, true, true, true, options);
    mainDialog_->addAcceptButton(QT_TRANSLATE_NOOP("popup_window", "CLOSE"), true);
}

void SettingsForTestersDialog::initViewElementsFrom(const AppConfig& appConfig)
{
    if (fullLogModeCheckbox_->isChecked() != appConfig.IsFullLogEnabled())
        fullLogModeCheckbox_->setChecked(appConfig.IsFullLogEnabled());

    if (devShowMsgIdsCheckbox_->isChecked() != appConfig.IsShowMsgIdsEnabled())
        devShowMsgIdsCheckbox_->setChecked(appConfig.IsShowMsgIdsEnabled());

    if (devSaveCallRTPdumpsCheckbox_->isChecked() != appConfig.IsSaveCallRTPEnabled())
        devSaveCallRTPdumpsCheckbox_->setChecked(appConfig.IsSaveCallRTPEnabled());
}

bool SettingsForTestersDialog::show()
{
    mainDialog_->setButtonActive(true);
    return mainDialog_->showInCenter();
}

void SettingsForTestersDialog::onOpenLogsPath(const QString &logsPath)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(logsPath));
}

void SettingsForTestersDialog::onToggleFullLogMode(bool checked)
{
    AppConfig appConfig = GetAppConfig();
    appConfig.SetFullLogEnabled(checked);

    ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}

void SettingsForTestersDialog::onToggleUpdateble(bool checked)
{
    AppConfig appConfig = GetAppConfig();
    appConfig.SetUpdateble(checked);

    ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}


void SettingsForTestersDialog::onToggleShowMsgIdsMenu(bool checked)
{
    AppConfig appConfig = GetAppConfig();
    appConfig.SetShowMsgIdsEnabled(checked);

    ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}

void SettingsForTestersDialog::notifyCacheDialogsTimeoutChanged(const QString& newTimeout)
{
    bool ok;
    int newValue = newTimeout.toInt(&ok);

    if (!ok || newValue < 0)
        return;

    AppConfig appConfig = GetAppConfig();
    appConfig.SetCacheHistoryControlPagesFor(newValue);

    /*      Disabled this logic in HistoryControl::updatePages
    //      const auto time = GetAppConfig().CacheHistoryControlPagesFor();
    //        for (auto iter = times_.begin(); iter != times_.end(); )
    //      {
    //        if (iter.value().secsTo(currentTime) >= time && iter.key() != current_)
    //        {
    //            closeDialog(iter.key());
    //            iter = times_.erase(iter);
    //        }
    //        else
    //        {
    //            ++iter;
    //        }
    //      }
    */

    ModifyAppConfig(std::move(appConfig), [this](core::icollection*) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}

void SettingsForTestersDialog::onToggleMemUsageWidget()
{
    bool showing = memUsageToggleBtn_->isChecked();
    QGraphicsOpacityEffect* fade_effect = new QGraphicsOpacityEffect(memorySettingsWidget_);
    memorySettingsWidget_->setGraphicsEffect(fade_effect);

    QPropertyAnimation *animation = new QPropertyAnimation(fade_effect, "opacity");
    animation->setEasingCurve(showing ? QEasingCurve::InOutQuad
                                      : QEasingCurve::OutQuad);
    animation->setDuration(200);

    qreal startVal = 0., endVal = 1.;
    if (!showing)
    {
        // We are hiding
        std::swap(startVal, endVal);
    }

    animation->setStartValue(startVal);
    animation->setEndValue(endVal);

    animation->start(QPropertyAnimation::DeleteWhenStopped);
    if (showing)
    {
        memorySettingsWidget_->setVisible(showing);
    }
    else
    {
        connect(animation, &QPropertyAnimation::finished,
                this, [this, showing](){
                    memorySettingsWidget_->setVisible(showing);
                });
    }
}

MemoryUsageWidget::MemoryUsageWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void MemoryUsageWidget::onRefresh()
{
    GetDispatcher()->post_message_to_core("request_memory_usage", nullptr, this,
                                          [this](core::icollection* _coll) {
        Q_UNUSED(this);

        Ui::gui_coll_helper coll(_coll, false);
        auto requestId = coll.get_value_as_int64("request_id");
        addPending(requestId);
    });

    GetDispatcher()->post_message_to_core("get_ram_usage", nullptr, this,
                                          [this](core::icollection* _coll) {
        Ui::gui_coll_helper coll(_coll, false);

        auto ramUsage = coll.get_value_as_int64("ram_used");
        auto realMemUsage = coll.get_value_as_int64("real_mem");
        archiveIndexSize_ = coll.get_value_as_int64("archive_index");
        archiveGallerySize_ = coll.get_value_as_int64("archive_gallery");

        ramUsageLabel_->setText(QT_TRANSLATE_NOOP("popup_window", "Current memory usage: ")
                                 + QChar(QChar::LineSeparator) + QT_TRANSLATE_NOOP("popup_window", "Memory: ") + Utils::formatFileSize(ramUsage) + qsl("; ")
                                + QChar(QChar::LineSeparator) + QT_TRANSLATE_NOOP("popup_window", "Real memory: ") + Utils::formatFileSize(realMemUsage)  + qsl("; "));
    });
}

void MemoryUsageWidget::onMemoryUsageReportReady(const Memory_Stats::Response &_response,
                                                 Memory_Stats::RequestId _req_id)
{
    if (!isPending(_req_id))
        return; // This wasn't our request

    redrawReportTextEdit(_response);
}

void MemoryUsageWidget::init()
{
    mainLayout_ = Utils::emptyVLayout(this);

    reportText_ = new Ui::TextEditEx(this, Fonts::appFontScaled(12, Fonts::FontWeight::Light), QColor(ql1s("#000000")), false, false);
    reportText_->setFrameStyle(QFrame::Box);
    reportText_->setStyleSheet(qsl("background: transparent;"));
    reportText_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    reportText_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    reportText_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    reportText_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    reportText_->setMinimumSize(reportText_->sizeHint());

    ramUsageLabel_ = new QLabel(this);
    ramUsageLabel_->setFont(Fonts::appFontScaled(16));
    ramUsageLabel_->setText(qsl("--"));

    mainLayout_->addWidget(ramUsageLabel_);
    mainLayout_->addWidget(reportText_);
    mainLayout_->setContentsMargins(0, MARGIN, 0, 0);

    // For btn that's only image , change refresh btn to some icon?
    auto setupSmallButton = [this](const auto& _icon, const auto& _text)
    {
        auto btn = new CustomButton(this, _icon, QSize(SMALL_BTN_WIDTH, SMALL_BTN_HEIGHT));
        Styling::Buttons::setButtonDefaultColors(btn);
        btn->setText(_text);
        btn->setFixedSize(Utils::scale_value(QSize(SMALL_BTN_WIDTH, SMALL_BTN_HEIGHT)));
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

        mainLayout_->addWidget(btn);
        return btn;
    };
    Q_UNUSED(setupSmallButton);

    auto setupButton = [this](const auto& _text)
    {
        auto btn = new QPushButton(_text, this);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        btn->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));

        Utils::grabTouchWidget(btn);
        mainLayout_->addWidget(btn);

        return btn;
    };

    copyTextButton_ = setupButton(QT_TRANSLATE_NOOP("popup_window", "Copy text"));
    connect(copyTextButton_, &QPushButton::clicked, this, [this](){
        QApplication::clipboard()->setText(ramUsageLabel_->text() + QChar(QChar::LineSeparator)
                                           + reportText_->getPlainText());
    });
    mainLayout_->addWidget(copyTextButton_);

    refreshButton_ = setupButton(QT_TRANSLATE_NOOP("popup_window", "Refresh"));
    connect(refreshButton_, &QPushButton::clicked, this, &MemoryUsageWidget::onRefresh);

    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::memoryUsageReportReady, this, &MemoryUsageWidget::onMemoryUsageReportReady);
    onRefresh();
}

void MemoryUsageWidget::redrawReportTextEdit(const Memory_Stats::Response &_response)
{
    reportText_->clear();

    qint64 total = 0;

    QString text;
    auto reports(_response.reports_);

    // distracting
    /*
        reports.sort([](const common::memory_stats::memory_stats_report& lhs, const common::memory_stats::memory_stats_report& rhs){
            return lhs.getOccupiedMemory() > rhs.getOccupiedMemory();
        });
    */

    for (auto report: reports)
    {
        text += stringForReport(report);
        text += QChar(QChar::LineSeparator);
        text += QChar(QChar::LineSeparator);
        total += report.getOccupiedMemory();
    }

    text += qsl("archive index: ") + Utils::formatFileSize(archiveIndexSize_);
    text += QChar(QChar::LineSeparator);
    text += QChar(QChar::LineSeparator);
    text += qsl("gallery: ") + Utils::formatFileSize(archiveGallerySize_);
    text += QChar(QChar::LineSeparator);
    text += QChar(QChar::LineSeparator);

    total += archiveIndexSize_;
    total += archiveGallerySize_;

    text.prepend(QT_TRANSLATE_NOOP("popup_window", "Total accounted for: ")
                 + Utils::formatFileSize(total)
                 + QChar(QChar::LineSeparator)
                 + QChar(QChar::LineSeparator));

    text.replace(QChar(QChar::LineSeparator), qsl("<br>"));

    reportText_->setHtml(text);
}

QString MemoryUsageWidget::stringForReport(const Memory_Stats::MemoryStatsReport &_report)
{
    QString reportString;
    reportString += typeString(_report.getStatType());
    reportString += QChar(QChar::Space);
    reportString += Utils::formatFileSize(_report.getOccupiedMemory());

    for (const auto &subcat: _report.getSubcategories())
    {
        reportString += QChar(QChar::LineSeparator);
        reportString += stringForSubcategory(subcat);
    }

    return reportString;
}

void SettingsForTestersDialog::onToggleSaveRTPDumps(bool checked)
{
    AppConfig appConfig = GetAppConfig();
    appConfig.SetSaveCallRTPEnabled(checked);

    ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}

void SettingsForTestersDialog::onToggleServerSearch(bool checked)
{
    AppConfig appConfig = GetAppConfig();
    appConfig.SetServerSearchEnabled(checked);

    ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
        initViewElementsFrom(GetAppConfig());
    }, this);
}

SettingsForTestersDialog::~SettingsForTestersDialog()
{
}

QString MemoryUsageWidget::stringForSubcategory(const Memory_Stats::MemoryStatsReport::MemoryStatsSubcategory &_subcategory)
{
    QString subcatString;

    subcatString += qsl("--");
    subcatString += QString::fromStdString(_subcategory.getName());
    subcatString += QChar(QChar::Space);
    subcatString += Utils::formatFileSize(_subcategory.getOccupiedMemory());

    return subcatString;
}

QString MemoryUsageWidget::typeString(Memory_Stats::StatType _type)
{
    switch (_type)
    {
    case Memory_Stats::StatType::CachedAvatars:
        return QT_TRANSLATE_NOOP("popup_window", "Cached avatars");
        break;
    case Memory_Stats::StatType::CachedThemes:
        return QT_TRANSLATE_NOOP("popup_window", "Cached themes");
        break;
    case Memory_Stats::StatType::CachedEmojis:
        return QT_TRANSLATE_NOOP("popup_window", "Cached emojis");
        break;
    case Memory_Stats::StatType::CachedPreviews:
        return QT_TRANSLATE_NOOP("popup_window", "Cached previews");
        break;
    case Memory_Stats::StatType::CachedStickers:
        return QT_TRANSLATE_NOOP("popup_window", "Cached stickers");
        break;
    case Memory_Stats::StatType::VoipInitialization:
        return QT_TRANSLATE_NOOP("popup_window", "VOIP initialization");
        break;
    case Memory_Stats::StatType::VideoPlayerInitialization:
        return QT_TRANSLATE_NOOP("popup_window", "Video players");
        break;
    default:
        return QString();
    }
}

bool MemoryUsageWidget::isPending(Memory_Stats::RequestId _req_id)
{
    return pendingRequests_.count(_req_id);
}

void MemoryUsageWidget::addPending(Memory_Stats::RequestId _req_id)
{
    pendingRequests_.insert(_req_id);
}

void MemoryUsageWidget::removePending(Memory_Stats::RequestId _req_id)
{
    pendingRequests_.erase(_req_id);
}

}

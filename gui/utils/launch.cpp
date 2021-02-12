#include "stdafx.h"
#include "launch.h"
#include "application.h"
#include "../types/chat.h"
#include "../types/contact.h"
#include "../types/link_metadata.h"
#include "../types/message.h"
#include "../types/typing.h"
#include "../types/masks.h"
#include "../types/session_info.h"
#include "../cache/stickers/stickers.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/history_control/history/History.h"
#include "../main_window/tray/RecentMessagesAlert.h"
#include "../voip/quality/ShowQualityStarsPopupConfig.h"
#include "../voip/quality/ShowQualityReasonsPopupConfig.h"
#include "../core_dispatcher.h"
#include "utils/utils.h"
#include "gui_metrics.h"
#include "cache/emoji/EmojiCode.h"
#include "styles/WallpaperId.h"
#include "sys/sys.h"
#include "controls/ClickWidget.h"


#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
#include "../../common.shared/win32/crash_handler.h"
#endif
#include "../common.shared/crash_report/crash_reporter.h"
#endif // !STRIP_CRASH_HANDLER

#include "../common.shared/config/config.h"

#ifndef STRIP_AV_MEDIA
#include "media/ptt/AudioUtils.h"
#include "media/ptt/AudioRecorder2.h"
#endif // !STRIP_AV_MEDIA

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef __linux__
#include <signal.h>
#endif //__linux__

#ifdef ICQ_QT_STATIC

#ifdef _WIN32
    Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif //_WIN32

#ifdef __linux__
    Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
#endif //__linux__

#ifdef __APPLE__
    Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
    Q_IMPORT_PLUGIN(QMacHeifPlugin)
    Q_IMPORT_PLUGIN(QMacJp2Plugin)
#endif //__APPLE__

    Q_IMPORT_PLUGIN(QGifPlugin)
    Q_IMPORT_PLUGIN(QICNSPlugin)
    Q_IMPORT_PLUGIN(QICOPlugin)
    Q_IMPORT_PLUGIN(QJpegPlugin)
    Q_IMPORT_PLUGIN(QTgaPlugin)
    Q_IMPORT_PLUGIN(QTiffPlugin)
    Q_IMPORT_PLUGIN(QWbmpPlugin)
    Q_IMPORT_PLUGIN(QWebpPlugin)
    Q_IMPORT_PLUGIN(QSvgPlugin)

#endif

namespace
{
    std::vector<std::pair<std::string_view, std::string_view>> CustomCmdArgs = {
    #if defined(__APPLE__)
        { "-platform", "cocoa:fontengine=freetype"}
    #elif defined(_WIN32)
        //{ "-platform", "windows:fontengine=freetype" }
    #elif defined(__linux__)

    #endif
    };

    void handleRlimits()
    {
#ifdef __APPLE__
        rlimit r;
        getrlimit(RLIMIT_NOFILE, &r);
        qInfo() << ql1s("initial limit for file descriptors: ") << r.rlim_cur;
        qInfo() << ql1s("max limit for file descriptors: ") << r.rlim_max;

        r.rlim_cur = std::min((rlim_t)10240, r.rlim_max);
        auto result = setrlimit(RLIMIT_NOFILE, &r);
        qInfo() << ((result == 0) ? ql1s("limit for file descriptors was succesfully updated") : ql1s("failed to update limit for file descriptors"));

        r.rlim_cur = 0;
        getrlimit(RLIMIT_NOFILE, &r);
        qInfo() << ql1s("new limit for file descriptors: ") << r.rlim_cur;
#endif //__APPLE__
    }
}

launch::CommandLineParser::CommandLineParser(int _argc, char* _argv[])
    : isUrlCommand_(false)
    , isVersionCommand_(false)
{
    if (_argc > 0)
    {
        executable_ = QString::fromUtf8(_argv[0]);

        for (int i = 1; i < _argc; ++i)
        {
            const auto arg = _argv[i];
            if (arg == std::string_view("-urlcommand"))
            {
                isUrlCommand_ = true;

                ++i;

                if (i >= _argc)
                    break;

                urlCommand_ = QString::fromUtf8(_argv[i]);
            }
            else if (!platform::is_windows() && i == 1 && arg == std::string_view("--version"))
            {
                isVersionCommand_ = true;
                break;
            }
        }
    }
}

launch::CommandLineParser::~CommandLineParser() = default;

bool launch::CommandLineParser::isUrlCommand() const
{
    return isUrlCommand_;
}

bool launch::CommandLineParser::isVersionCommand() const
{
    return isVersionCommand_;
}

const QString& launch::CommandLineParser::getUrlCommand() const
{
    return urlCommand_;
}

const QString& launch::CommandLineParser::getExecutable() const
{
    return executable_;
}

static const QString& getLoggingCategoryFilter()
{
    static const QString filter = []()
    {
        const auto logCategory = [](QStringView _cat, const bool _enabled) -> QString
        {
            const auto value = _enabled ? u"true" : u"false";
            return _cat % u".debug=" % value;
        };

        const QStringList cats =
        {
            logCategory(u"ffmpegPlayer", false),
            logCategory(u"fileSharingBlock", false),
            logCategory(u"friendlyContainer", false),
            logCategory(u"clModel", false),
            logCategory(u"history", false),
            logCategory(u"historyPage", false),
            logCategory(u"searchModel", false),
            logCategory(u"messageSearcher", false),
            logCategory(u"mentionsModel", false),
            logCategory(u"maskManager", false),
            logCategory(u"heads", false),
            logCategory(u"bgWidget", false),
            logCategory(u"ptt", false),
            logCategory(u"localPeer", false),
            logCategory(u"soundManager", false)
        };
        return cats.join(u'\n');
    }();
    return filter;
}

static QtMessageHandler defaultHandler = nullptr;

static void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtWarningMsg:
    {
        constexpr QStringView whiteList[] = {
            u"libpng warning:"
        };

        if (std::none_of(std::begin(whiteList), std::end(whiteList), [&msg](auto str) { return msg.startsWith(str); }))
        {
            assert(false);
        }
    }
    break;
    default:
        break;
    }

    if (defaultHandler)
        defaultHandler(type, context, msg);
}

int launch::main(int _argc, char* _argv[])
{
    static_assert(Q_BYTE_ORDER == Q_LITTLE_ENDIAN);

    if constexpr (build::is_debug())
        defaultHandler = qInstallMessageHandler(debugMessageHandler);

    handleRlimits();

    int result;
    bool restarting;

#ifdef __linux__
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
            assert(false);
#else
#ifndef STRIP_CRASH_HANDLER
    crash_system::reporter::instance();
#ifdef _WIN32
    crash_system::reporter::instance().set_process_exception_handlers();
    crash_system::reporter::instance().set_thread_exception_handlers();
#endif //_WIN32
#endif // !STRIP_CRASH_HANDLER
#endif //__linux__



    std::vector<char*> v_args;

    QString commandLine;

    {
        statistic::getGuiMetrics().eventStarted();

        static bool isLaunched = false;
        if (isLaunched)
            return 0;
        isLaunched = true;

        if (_argc == 0)
        {
            assert(false);
            return -1;
        }

        if constexpr (build::is_testing())
        {
            for (int i = 0; i < _argc; ++i)
                v_args.push_back(strdup(_argv[i]));
        }
        else
        {
            v_args.push_back(strdup(_argv[0]));
        }

        auto windowMode = Utils::Application::MainWindowMode::Normal;
        for (int i = 1; i < _argc; ++i)
        {
            if (std::string_view(_argv[i]) == std::string_view("/startup"))
            {
                v_args.push_back(strdup(_argv[i]));
                windowMode = Utils::Application::MainWindowMode::Minimized;
            }
        }

        for (const auto& [key, value] : CustomCmdArgs)
        {
            v_args.push_back(strdup(key.data()));
            v_args.push_back(strdup(value.data()));
        }

        int argCount = int(v_args.size());

        Utils::Application app(argCount, v_args.data());
        QLoggingCategory::setFilterRules(getLoggingCategoryFilter());

        commandLine = qApp->arguments().constFirst();

        CommandLineParser cmd_parser(_argc, _argv);

        if (cmd_parser.isVersionCommand())
        {
            qInfo() << qPrintable(Utils::getVersionPrintable());
            return 0;
        }

        if (!app.isMainInstance())
            return app.switchInstance(cmd_parser);

        if (app.updating(windowMode))
            return 0;

        if (app.init(cmd_parser))
        {
            qRegisterMetaType<std::shared_ptr<Data::ContactList>>("std::shared_ptr<Data::ContactList>");
            qRegisterMetaType<std::shared_ptr<Data::Buddy>>("std::shared_ptr<Data::Buddy>");
            qRegisterMetaType<std::shared_ptr<Data::UserInfo>>("std::shared_ptr<Data::UserInfo>");
            qRegisterMetaType<Data::MessageBuddies>("Data::MessageBuddies");
            qRegisterMetaType<std::shared_ptr<Data::ChatInfo>>("std::shared_ptr<Data::ChatInfo>");
            qRegisterMetaType<QVector<Data::ChatInfo>>("QVector<Data::ChatInfo>");
            qRegisterMetaType<QVector<Data::ChatMemberInfo>>("QVector<Data::ChatMemberInfo>");
            qRegisterMetaType<Data::DlgState>("Data::DlgState");
            qRegisterMetaType<Data::CommonChatInfo>("Data::CommonChatInfo");
            qRegisterMetaType<hist::scroll_mode_type>("hist::scroll_mode_type");
            qRegisterMetaType<Logic::MessageKey>("Logic::MessageKey");
            qRegisterMetaType<Logic::UpdateChatSelection>("Logic::UpdateChatSelection");
            qRegisterMetaType<QVector<Logic::MessageKey>>("QVector<Logic::MessageKey>");
            qRegisterMetaType<QScroller::State>("QScroller::State");
            qRegisterMetaType<Ui::MessagesBuddiesOpt>("Ui::MessagesBuddiesOpt");
            qRegisterMetaType<Ui::MessagesNetworkError>("Ui::MessagesNetworkError");
            qRegisterMetaType<QSystemTrayIcon::ActivationReason>("QSystemTrayIcon::ActivationReason");
            qRegisterMetaType<Logic::TypingFires>("Logic::TypingFires");
            qRegisterMetaType<int64_t>("int64_t");
            qRegisterMetaType<int32_t>("int32_t");
            qRegisterMetaType<uint64_t>("uint64_t");
            qRegisterMetaType<uint32_t>("uint32_t");
            qRegisterMetaType<Data::LinkMetadata>("Data::LinkMetadata");
            qRegisterMetaType<QSharedPointer<QMovie>>("QSharedPointer<QMovie>");
            qRegisterMetaType<Data::Quote>("Data::Quote");
            qRegisterMetaType<Data::QuotesVec>("Data::QuotesVec");
            qRegisterMetaType<QVector<Data::DlgState>>("QVector<Data::DlgState>");
            qRegisterMetaType<std::shared_ptr<Ui::Stickers::Set>>("std::shared_ptr<Ui::Stickers::Set>");
            qRegisterMetaType<Ui::Stickers::StickerData>("Ui::Stickers::StickerData");
            qRegisterMetaType<Ui::Stickers::StickerLoadDataV>("Ui::Stickers::StickerLoadDataV");
            qRegisterMetaType<Data::MessageBuddySptr>("Data::MessageBuddySptr");
            qRegisterMetaType<Ui::AlertType>("Ui::AlertType");
            qRegisterMetaType<Emoji::EmojiCode>("Emoji::EmojiCode");
            qRegisterMetaType<Utils::CloseWindowInfo>("Utils::CloseWindowInfo");
            qRegisterMetaType<Utils::SidebarVisibilityParams>("Utils::SidebarVisibilityParams");
            qRegisterMetaType<QVector<Data::DialogGalleryEntry>>("QVector<Data::DialogGalleryEntry>");
            qRegisterMetaType<Data::FileSharingMeta>("Data::FileSharingMeta");
            qRegisterMetaType<Data::FileSharingDownloadResult>("Data::FileSharingDownloadResult");
            qRegisterMetaType<common::tools::patch_version>("common::tools::patch_version");
            qRegisterMetaType<hist::FetchDirection>("hist::FetchDirection");
            qRegisterMetaType<std::map<QString, int32_t>>("std::map<QString, int32_t>");
            qRegisterMetaType<Styling::WallpaperId>("Styling::WallpaperId");
            qRegisterMetaType<std::vector<Data::Mask>>("std::vector<Data::Mask>");
            qRegisterMetaType<Data::MentionMap>("Data::MentionMap");
            qRegisterMetaType<std::chrono::seconds>("std::chrono::seconds");
            qRegisterMetaType<std::chrono::milliseconds>("std::chrono::milliseconds");
            qRegisterMetaType<std::function<void()>>("std::function<void()>");
            qRegisterMetaType<Data::MentionMap>("Data::MentionMap");
            qRegisterMetaType<QVector<double>>("QVector<double>");
            qRegisterMetaType<Ui::ClickType>("Ui::ClickType");
            qRegisterMetaType<std::optional<QString>>("std::optional<QString>");
            qRegisterMetaType<std::vector<Data::SessionInfo>>("std::vector<Data::SessionInfo>");
            qRegisterMetaType<Data::CallInfo>("Data::CallInfo");
            qRegisterMetaType<Data::CallInfoPtr>("Data::CallInfoPtr");
            qRegisterMetaType<Data::CallInfoVec>("Data::CallInfoVec");
            qRegisterMetaType<Statuses::Status>("Statuses::Status");
#ifndef STRIP_AV_MEDIA
            qRegisterMetaType<ptt::State2>("ptt::State2");
            qRegisterMetaType<ptt::Error2>("ptt::Error2");
            qRegisterMetaType<ptt::Buffer>("ptt::Buffer");
            qRegisterMetaType<ptt::StatInfo>("ptt::StatInfo");
#endif // !STRIP_AV_MEDIA

#ifndef STRIP_VOIP
            qRegisterMetaType<Ui::ShowQualityReasonsPopupConfig>("Ui::ShowQualityReasonsPopupConfig");
            qRegisterMetaType<Ui::ShowQualityStarsPopupConfig>("Ui::ShowQualityStarsPopupConfig");
            qRegisterMetaType<voip_proxy::EvoipDevTypes>("voip_proxy::EvoipDevTypes");
            qRegisterMetaType<voip_proxy::device_desc>("voip_proxy::device_desc");
            qRegisterMetaType<voip_proxy::device_desc_vector>("voip_proxy::device_desc_vector");
#endif
        }
        else
        {
            return 1;
        }


        //do not change context(1st argument), it's important
        QT_TRANSLATE_NOOP("QWidgetTextControl", "&Undo");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "&Redo");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "Cu&t");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "&Copy");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "&Paste");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "Delete");
        QT_TRANSLATE_NOOP("QWidgetTextControl", "Select All");

        QT_TRANSLATE_NOOP("QLineEdit", "&Undo");
        QT_TRANSLATE_NOOP("QLineEdit", "&Redo");
        QT_TRANSLATE_NOOP("QLineEdit", "Cu&t");
        QT_TRANSLATE_NOOP("QLineEdit", "&Copy");
        QT_TRANSLATE_NOOP("QLineEdit", "&Paste");
        QT_TRANSLATE_NOOP("QLineEdit", "Delete");
        QT_TRANSLATE_NOOP("QLineEdit", "Select All");

        result = app.exec();
        restarting = qApp->property("restarting").toBool();
    }

    Ui::destroyDispatcher();

    // Cleanup
    for (auto _arg : v_args)
        free(_arg);

    if (restarting)
    {
#ifdef __linux__
        execv(_argv[0], _argv);
#else
        System::launchApplication(commandLine);
#endif
    }

    return result;
}

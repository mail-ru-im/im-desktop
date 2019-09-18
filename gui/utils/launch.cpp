#include "stdafx.h"
#include "launch.h"
#include "application.h"
#include "../types/chat.h"
#include "../types/contact.h"
#include "../types/images.h"
#include "../types/link_metadata.h"
#include "../types/message.h"
#include "../types/typing.h"
#include "../types/masks.h"
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

#include "media/ptt/AudioUtils.h"
#include "media/ptt/AudioRecorder2.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef __linux__
#include <signal.h>
#endif //__linux__

#ifdef ICQ_QT_STATIC

    Q_IMPORT_PLUGIN(QICOPlugin)
    Q_IMPORT_PLUGIN(QSvgPlugin)

    #ifdef _WIN32
        Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
    #endif //_WIN32

    #ifndef __linux__
        Q_IMPORT_PLUGIN(QTiffPlugin)
    #else
        Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
    #endif //__linux__

    #ifdef __APPLE__
        Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
        Q_IMPORT_PLUGIN(QJpegPlugin)
        Q_IMPORT_PLUGIN(QGifPlugin)
    #endif
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
}

void makeBuild(const int _argc, char* _argv[])
{
    const auto path = QFileInfo(ql1s(_argv[0])).fileName();
    if (path.contains(ql1s("agent"), Qt::CaseInsensitive))
    {
        build::is_build_icq = false;
        return;
    }

    for (int i = 1; i < _argc; ++i)
    {
        if (std::string_view(_argv[i]).find("/agent") != std::string_view::npos)
            build::is_build_icq = false;
    }

    if (build::is_biz() || build::is_dit())
        build::is_build_icq = false;
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

launch::CommandLineParser::~CommandLineParser()
{

}

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

static QString getLoggingCategoryFilter()
{
    static QString filter;
    if (filter.isEmpty())
    {
        const auto logCategory = [](std::string_view _cat, const bool _enabled) -> QString
        {
            const auto value = _enabled ? ql1s("true") : ql1s("false");
            return QLatin1String(_cat.data(), int(_cat.size())) % ql1s(".debug=") % value;
        };

        const QStringList cats =
        {
            logCategory("ffmpegPlayer", false),
            logCategory("fileSharingBlock", false),
            logCategory("friendlyContainer", false),
            logCategory("clModel", false),
            logCategory("history", false),
            logCategory("historyPage", false),
            logCategory("searchModel", false),
            logCategory("messageSearcher", false),
            logCategory("mentionsModel", false),
            logCategory("maskManager", false),
            logCategory("heads", false),
            logCategory("bgWidget", false),
            logCategory("ptt", false),

            logCategory("localPeer", false)
        };
        filter = cats.join(ql1c('\n'));
    }
    return filter;
}

static QtMessageHandler defaultHandler = nullptr;

static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtWarningMsg:
    {
        const static auto whiteList = {
            ql1s("libpng warning:")
        };

        if (std::none_of(whiteList.begin(), whiteList.end(), [&msg](auto str) { return msg.startsWith(str); }))
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
    if constexpr (build::is_debug())
        defaultHandler = qInstallMessageHandler(myMessageOutput);
    int result;
    bool restarting;

#ifdef __linux__
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
            assert(false);
#endif //__linux__

    std::vector<char*> v_args;

    QString commandLine;

    {
        makeBuild(_argc, _argv);

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

        if (build::is_testing())
        {
            for (int i = 0; i < _argc; ++i)
                v_args.push_back(strdup(_argv[i]));
        }
        else
        {
            v_args.push_back(strdup(_argv[0]));
        }

        for (int i = 1; i < _argc; ++i)
        {
            if (std::string_view(_argv[i]) == std::string_view("/startup"))
            {
                v_args.push_back(strdup(_argv[i]));
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

        if (app.updating())
            return 0;

        if (app.init(cmd_parser))
        {
            qRegisterMetaType<Data::ImageListPtr>("Data::ImageListPtr");
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
            qRegisterMetaType<Data::MessageBuddySptr>("Data::MessageBuddySptr");
            qRegisterMetaType<Ui::AlertType>("Ui::AlertType");
            qRegisterMetaType<Emoji::EmojiCode>("Emoji::EmojiCode");
            qRegisterMetaType<Ui::ShowQualityReasonsPopupConfig>("Ui::ShowQualityReasonsPopupConfig");
            qRegisterMetaType<Ui::ShowQualityStarsPopupConfig>("Ui::ShowQualityStarsPopupConfig");
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
            qRegisterMetaType<ptt::State2>("ptt::State2");
            qRegisterMetaType<ptt::Error2>("ptt::Error2");
            qRegisterMetaType<std::chrono::seconds>("std::chrono::seconds");
            qRegisterMetaType<std::chrono::milliseconds>("std::chrono::milliseconds");
            qRegisterMetaType<Data::MentionMap>("Data::MentionMap");
            qRegisterMetaType<QVector<double>>("QVector<double>");
            qRegisterMetaType<ptt::Buffer>("ptt::Buffer");
            qRegisterMetaType<ptt::StatInfo>("ptt::StatInfo");
            qRegisterMetaType<Ui::ClickType>("Ui::ClickType");
            qRegisterMetaType<voip_proxy::EvoipDevTypes>("voip_proxy::EvoipDevTypes");
            qRegisterMetaType<voip_proxy::device_desc>("voip_proxy::device_desc");
            qRegisterMetaType<voip_proxy::device_desc_vector>("voip_proxy::device_desc_vector");
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

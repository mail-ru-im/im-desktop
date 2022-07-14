#include "stdafx.h"
#include "UrlUtils.h"
#include "utils.h"

#include "InterConnector.h"
#include "UrlCommand.h"

#include "features.h"
#include "url_config.h"
#include "../app_config.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/version_info.h"

#include "../gui_settings.h"

#include "../controls/GeneralDialog.h"
#include "../main_window/MainWindow.h"
#include "../main_window/mini_apps/MiniAppsUtils.h"
#include "../main_window/mini_apps/MiniAppsContainer.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/RecentsModel.h"
#include "previewer/toast.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <cache/emoji/EmojiDb.h>
#include "macos/mac_support.h"
typedef unsigned char byte;
#elif defined(__linux__)
#define byte uint8_t
Q_LOGGING_CATEGORY(linuxRunCommand, "linuxRunCommand")
#endif


extern "C" int32_t punyencode(const char16_t* _src, int32_t _srcLength, char16_t* _dest, int32_t _destCapacity, int32_t* _pErrorCode);

namespace
{
    constexpr int getUrlInDialogElideSize() noexcept { return 140; }

    constexpr auto redirect() noexcept
    {
        return QStringView(u"&noredirecttologin=1");
    }

    QString getPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru % u":443/cls3564/") % Ui::getUrlConfig().getUrlMailWin();
    }

    QString getFailPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru % u":443/cls3564/") % Ui::getUrlConfig().getUrlMailWin();
    }

    QString getReadMsgPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru % u":443/cln8791/") % Ui::getUrlConfig().getUrlMailRead() % u"?id=";
    }

    QString getBaseMailUrl()
    {
        return u"https://" % Ui::getUrlConfig().getUrlMailAuth() % u"?Login=%1&agent=%2&ver=%3&agentlang=%4";
    }

    QString getMailUrl()
    {
        return getBaseMailUrl() % redirect() % u"&page=" % getPage() % u"&FailPage=" % getFailPage();
    }

    QString getMailOpenUrl()
    {
        return getBaseMailUrl() % redirect() % u"&page=" % getReadMsgPage() % u"%5&lang=%4" % u"&FailPage=" % getReadMsgPage() % u"%5&lang=%4";
    }

    QString msgIdFromUidl(const QString& uidl)
    {
        if (uidl.size() != 16)
        {
            im_assert(!"wrong uidl");
            return QString();
        }

        byte b[8] = { 0 };
        bool ok;
        b[0] = uidl.midRef(0, 2).toUInt(&ok, 16);
        b[1] = uidl.midRef(2, 2).toUInt(&ok, 16);
        b[2] = uidl.midRef(4, 2).toUInt(&ok, 16);
        b[3] = uidl.midRef(6, 2).toUInt(&ok, 16);
        b[4] = uidl.midRef(8, 2).toUInt(&ok, 16);
        b[5] = uidl.midRef(10, 2).toUInt(&ok, 16);
        b[6] = uidl.midRef(12, 2).toUInt(&ok, 16);
        b[7] = uidl.midRef(14, 2).toUInt(&ok, 16);

        return QString::asprintf("%u%010u", *(int*)(b), *(int*)(b + 4 * sizeof(byte)));
    }

    void appendPunycoded(QStringView _source, QString& _encoded)
    {
        static constexpr size_t LOCAL_BUFFER_SIZE = 256;

        if (_source.empty())
            return;

        if (std::all_of(_source.begin(), _source.end(), [](QChar c) { return c.isLetter() || c.isDigit(); }))
        {
            _encoded.append(_source);
            return;
        }

        char16_t buf[LOCAL_BUFFER_SIZE] = { 0 };

        int32_t errCode = 0;
        auto n = punyencode((const char16_t*)_source.utf16(), _source.size(), buf, LOCAL_BUFFER_SIZE, &errCode);

        _encoded.append(u"xn--");
        _encoded.append({ buf, n });
    }

    QString toPunycoded(QStringView _source)
    {
        QString result;
        result.reserve(64);
        auto first = _source.begin();
        auto eos = _source.end();
        while (first != eos)
        {
            auto second = std::find_if(first, eos, [](QChar c) { return c == (u'.') || c == (u':') || c == (u'@'); });
            if (first == second)
            {
                if (second != eos)
                    result.append(*second);
                first = ++second;
                continue;
            }

            qsizetype offset = std::distance(_source.begin(), first);
            qsizetype size = std::distance(first, second);
            appendPunycoded(_source.mid(offset, size), result);
            if (second == eos)
                break;

            result.append(*second);
            first = ++second;
        }
        return result;
    }

#ifndef __APPLE__
    bool isDangerousExtensions(QStringView _ext)
    {
        const static std::vector<QStringView> extensions =
        {
#ifdef _WIN32
            u"scr", u"exe", u"chm", u"cmd", u"pif", u"bat", u"vbs", u"jar", u"ade", u"adp", u"com", u"cpl",
            u"hta", u"ins", u"isp", u"jse", u"lib", u"lnk", u"mde", u"msc", u"msp", u"mst", u"sct", u"shb",
            u"sys", u"vb", u"vbe", u"vxd", u"wsc", u"wsf", u"wsh", u"reg", u"rgs", u"inf", u"msi", u"dll",
            u"cpl", u"job", u"ws", u"vbscript", u"gadget", u"shb", u"shs", u"sct", u"isu", u"inx", u"app", u"ipa",
            u"apk", u"rpm", u"apt", u"deb", u"dmg", u"action", u"command", u"xpi", u"crx", u"js", u"scf", u"url",
            u"ace"
#else
            u"desktop"
#endif
        };

        return std::any_of(extensions.cbegin(), extensions.cend(), [_ext](auto x) { return _ext.compare(x, Qt::CaseInsensitive) == 0; });
    }
#endif

#ifdef __linux__
    bool runCommand(const QByteArray& command)
    {
        auto result = system(command.constData());
        if (result)
        {
            qCDebug(linuxRunCommand) << QString(qsl("Command failed, code: %1, command (in utf8): %2")).arg(result).arg(QString::fromLatin1(command.constData()));
            return false;
        }
        else
        {
            qCDebug(linuxRunCommand) << QString(qsl("Command succeeded, command (in utf8): %2")).arg(QString::fromLatin1(command.constData()));
            return true;
        }
    }

    QByteArray escapeShell(const QByteArray& content)
    {
        auto result = QByteArray();

        auto b = content.constData(), e = content.constEnd();
        for (auto ch = b; ch != e; ++ch)
        {
            if (*ch == ' ' || *ch == '"' || *ch == '\'' || *ch == '\\')
            {
                if (result.isEmpty())
                    result.reserve(content.size() * 2);
                if (ch > b)
                    result.append(b, ch - b);

                result.append('\\');
                b = ch;
            }
        }
        if (result.isEmpty())
            return content;

        if (e > b)
            result.append(b, e - b);

        return result;
    }
#endif

    std::unique_ptr<OpenAppCommand> createOpenSharedMiniAppCommand(QStringView _url)
    {
        const auto& sharingUrl = Ui::getUrlConfig().getMiniappSharing();
        if (sharingUrl.isEmpty())
            return {};

        const QString miniAppUrlString = Utils::addHttpsIfNeeded(sharingUrl);
        if (!_url.startsWith(miniAppUrlString, Qt::CaseInsensitive))
            return {};

        const QUrl miniAppUrl = Utils::fromUserInput(_url.mid(miniAppUrlString.length()));
        auto miniAppId = miniAppUrl.path();

        if (!miniAppId.isEmpty())
            miniAppId.remove(0, 1);

        if (miniAppId.endsWith(u'/'))
            miniAppId.chop(1);

        return std::make_unique<OpenAppCommand>(miniAppId, miniAppUrl.query(), miniAppUrl.fragment());
    }

    std::unique_ptr<OpenAppCommand> createOpenCalendarAppCommand(QStringView _url)
    {
        const auto& calendarAppId = Utils::MiniApps::getCalendarId();
        const auto& app = Logic::GetAppsContainer()->getApp(calendarAppId);
        if (!app.isValid() || !app.isEnabled())
            return {};

        const std::vector<QString>& calendarUrls = app.templateDomains();
        for (const auto& url : calendarUrls)
        {
            if (_url.startsWith(url, Qt::CaseInsensitive))
            {
                const QUrl miniAppUrl = Utils::fromUserInput(_url);
                return std::make_unique<OpenAppCommand>(calendarAppId, miniAppUrl.query(), miniAppUrl.fragment());
            }
        }
        return {};
    }
}

namespace Utils
{
    QStringView normalizeLink(QStringView _link)
    {
        return _link.trimmed();
    }

    QString getDomainUrl()
    {
        const bool useProfileAgent = config::get().is_on(config::features::profile_agent_as_domain_url);
        return useProfileAgent ? Features::getProfileDomainAgent() : Features::getProfileDomain();
    }

    bool isUrlVCS(const QString& _url)
    {
        const auto& vcs_urls = Ui::getUrlConfig().getVCSUrls();
        return std::any_of(vcs_urls.begin(), vcs_urls.end(), [&_url](const auto& u) { return _url.startsWith(u); });
    }

    const std::vector<QString>& urlSchemes()
    {
        const static auto schemes = []() {
            const auto scheme_csv = config::get().string(config::values::register_url_scheme_csv); //url1,descr1,url2,desc2,..
            const auto splitted = QString::fromUtf8(scheme_csv.data(), scheme_csv.size()).split(QLatin1Char(','));
            std::vector<QString> res;
            res.reserve(splitted.size() / 2);
            for (int i = 0; i < splitted.size(); i += 2)
                res.push_back(splitted.at(i));
            return res;
        }();

        return schemes;
    }

    bool isInternalScheme(const QString& scheme)
    {
        const auto& schemes = urlSchemes();
        return std::any_of(schemes.begin(), schemes.end(), [&scheme](const auto& x) { return x.compare(scheme, Qt::CaseInsensitive) == 0; });
    }

    bool isServiceLink(const QString& _url)
    {
        QUrl url(_url);
        if (url.isEmpty())
            return false;

        const QString host = url.host();
        const QString path = url.path();
        const auto pathView = QStringView(path);

        auto startWithServicePath = [](QStringView _path)
        {
            return !_path.isEmpty()
                && (_path.startsWith(url_commandpath_service_getlogs(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_getlogs_with_rtp(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_clear_avatars(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_clear_cache(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_update(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_debug(), Qt::CaseInsensitive)
                    );
        };

        return isInternalScheme(url.scheme())
            && host.compare(url_command_service(), Qt::CaseInsensitive) == 0
            && pathView.startsWith(u'/')
            && startWithServicePath(pathView.mid(1));
    }

    bool isNick(QStringView _text)
    {
        // match alphanumeric str with @, starting with a-zA-Z
        static const QRegularExpression re(qsl("^@[a-zA-Z0-9][a-zA-Z0-9._]*$"), QRegularExpression::UseUnicodePropertiesOption);
        return re.match(_text).hasMatch();
    }

    bool isHashtag(QStringView _text)
    {
        if (!_text.startsWith(u'#'))
            return false;

        // match alphanumeric str, also contains _
        static const auto isTagSymbols = [](QChar _c)
        {
            return _c.isLetterOrNumber() || _c.toLatin1() == '_';
        };

        static const auto isDigitOrPunctOnly = [](QChar _c)
        {
            return _c.isDigit() || _c.toLatin1() == '_' || _c.isPunct();
        };

        return std::all_of(_text.mid(1).cbegin(), _text.cend(), isTagSymbols) && !std::all_of(_text.mid(1).cbegin(), _text.cend(), isDigitOrPunctOnly);
    }

    QString makeNick(const QString& _text)
    {
        return (_text.isEmpty() || _text.contains(u'@')) ? _text : (u'@' % _text);
    }

    QString getEmailFromMentionLink(const QString& _link)
    {
        if (!isMentionLink(_link))
            return {};

        return _link.mid((qsl("@[")).length(), _link.length() - (qsl("@[")).length() - (qsl("]")).length());
    }

    bool doesContainMentionLink(QStringView _text)
    {
        if (const auto idxStart = _text.indexOf(u"@["); idxStart >= 0)
        {
            const auto idxEnd = _text.indexOf(u']', idxStart + 2);
            return idxEnd != -1;
        }
        return false;
    }

    static bool openConfirmDialog(const QString& _label, const QString& _infoText, bool _isSelected, const QString& _setting)
    {
        Utils::CheckableInfoWidget* widget = new Utils::CheckableInfoWidget(nullptr);
        widget->setCheckBoxText(QT_TRANSLATE_NOOP("popup_window", "Don't ask me again"));
        widget->setCheckBoxChecked(_isSelected);
        widget->setInfoText(_infoText);

        Ui::GeneralDialog generalDialog(widget, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(_label);
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Yes"));

        const auto isConfirmed = generalDialog.execute();
        if (isConfirmed)
        {
            if (widget->isChecked())
                Ui::get_gui_settings()->set_value<bool>(_setting, true);
        }

        return isConfirmed;
    }


    void openMailBox(const QString& _email, const QString& _mrimKey, const QString& _mailId)
    {
        core::tools::version_info infoCurrent;
        const auto buildStr = QString::number(infoCurrent.get_build());
        const auto langStr = Utils::GetTranslator()->getLang();
        const auto url = _mailId.isEmpty()
            ? getMailUrl().arg(_email, _mrimKey, buildStr, langStr)
            : getMailOpenUrl().arg(_email, _mrimKey, buildStr, langStr, msgIdFromUidl(_mailId));

        QDesktopServices::openUrl(url);

        Q_EMIT Utils::InterConnector::instance().mailBoxOpened();
    }

    void openAttachUrl(const QString& _email, const QString& _mrimKey, bool _canCancel)
    {
        core::tools::version_info infoCurrent;
        const QStringView canCancel = _canCancel ? u"0" : u"1";
        const QString signed_url = getBaseMailUrl() % redirect() % u"&page=" % Features::attachPhoneUrl() % u"?nocancel=" % canCancel
            % u"&client_code=" % getProductName() % u"&back_url=" % urlSchemes().front() % u"://attach_phone_result";
        const QString final_url = signed_url.arg(_email, _mrimKey, QString::number(infoCurrent.get_build()), Utils::GetTranslator()->getLang());

        QDesktopServices::openUrl(final_url);
    }

    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey)
    {
        core::tools::version_info infoCurrent;

        const QString signed_url = getBaseMailUrl() % redirect() % u"&page=" % _url % u"&FailPage=" % _fail_url;

        const QString final_url = signed_url.arg(_email, _mrimKey, QString::number(infoCurrent.get_build()), Utils::GetTranslator()->getLang());

        QDesktopServices::openUrl(final_url);
    }

    void openFileOrFolder(const QString& _chatAimId, QStringView _path, OpenAt _openAt, OpenWithWarning _withWarning)
    {
        const auto isPublic = Logic::getContactListModel()->isPublic(_chatAimId);
        const auto isStranger = _chatAimId.isEmpty() || Logic::getRecentsModel()->isStranger(_chatAimId) || Logic::getRecentsModel()->isSuspicious(_chatAimId);
        const auto openFile = Features::opensOnClick() && (_openAt == OpenAt::Launcher) && !isPublic && !isStranger;

#ifdef __APPLE__
        if (openFile)
            MacSupport::openFile(_path.toString());
        else
            MacSupport::openFinder(_path.toString());
#else
        const auto pathString = _path.toString();
        auto nativePath = QDir::toNativeSeparators(pathString);
        if (openFile)
        {
            auto exeFile = [&nativePath]()
            {
#ifdef _WIN32
                ShellExecute(0, L"open", nativePath.toStdWString().c_str(), 0, 0, SW_SHOWNORMAL);
#else
                QDesktopServices::openUrl(QString(u"file:///" % nativePath));
#endif
            };

            auto isAvailableWithoutWarning = []()
            {
                return Ui::get_gui_settings()->get_value<bool>(settings_exec_files_without_warning, settings_exec_files_without_warning_default());
            };

            auto isConfirmed = true;
            if (_withWarning == OpenWithWarning::Yes && !isAvailableWithoutWarning() && isDangerousExtensions(QFileInfo(pathString).suffix()))
            {
                isConfirmed = openConfirmDialog(QT_TRANSLATE_NOOP("popup_window", "Run this file?"),
                    QT_TRANSLATE_NOOP("popup_window", "This file may be unsafe"),
                    settings_exec_files_without_warning_default(),
                    qsl(settings_exec_files_without_warning));

            }

            if (isConfirmed)
                exeFile();
        }
        else
        {
#ifdef _WIN32
            nativePath = nativePath.replace(u'"', ql1s("\"\""));
            ShellExecute(0, 0, L"explorer", ql1s("/select,\"%1\"").arg(nativePath).toStdWString().c_str(), 0, SW_SHOWNORMAL);
#else
            QDir dir(nativePath);
            dir.cdUp();
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
#endif
        }
#endif // __APPLE__
    }

    void openFileLocation(const QString& _path)
    {
#ifdef _WIN32
        const QString command = u"explorer /select," % QDir::toNativeSeparators(_path);
        QProcess::startDetached(command);
#else
#ifdef __APPLE__
        MacSupport::openFinder(_path);
#else
        QDir dir(_path);
        dir.cdUp();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
#endif // __APPLE__
#endif //_WIN32
    }


    // following code is workaround for https://bugreports.qt.io/browse/QTBUG-69675
    // and https://bugreports.qt.io/browse/QTBUG-85371 (resolved in Qt 6.3.0):
    // QUrl doesn't handle properly user input contained emoji symbols
    QUrl fromUserInput(QStringView _url)
    {
        static const QStringView httpShemes[] = { u"https", u"http", u"mailto" };

        static auto isPlainLatinUrl = [](QChar _c) // check that we haven't got any emoji in URL
        {
            return _c.isLetter() || _c.isDigit() || _c.isPunct() ||
                   _c == ql1c('+') || _c == ql1c('-') || _c == ql1c('='); // some math related symbols are also allowed
        };

        static auto isPlainLatinAuthority = [](QChar _c) // check that we haven't got any emoji in auth part of URL
        {
            // here we don't check math symbols, since they are not allowed anyway
            return _c.isLetter() || _c.isDigit() || _c.isPunct();
        };

        QUrl urlLink;
        if (std::all_of(_url.begin(), _url.end(), isPlainLatinUrl))
            return QUrl::fromUserInput(_url.toString());

        for (auto scheme : httpShemes)
        {
            if (!_url.startsWith(scheme))
                continue;

            QStringView authority;
            QStringView path;
            QStringView query;
            QStringView fragment;
            QStringView userinfo;

            auto offset = scheme.size();
            if (scheme == u"mailto")
            {
                offset += 1;
                urlLink.setScheme(scheme.toString());
                urlLink.setPath(_url.mid(offset).toString());
                break;
            }

            offset += 3;
            if (offset >= _url.size())
                break;

            auto first = _url.begin() + offset;
            auto last = _url.end();
            auto it = std::find_if(first, last, [](QChar c) { return c == (u'/') || c == (u'?') || c == (u'#'); });
            authority = _url.mid(std::distance(_url.begin(), first), std::distance(first, it));
            if (it != last)
            {
                if (*it == (u'/')) // path found
                {
                    auto end = std::find_if(++it, last, [](QChar c) { return c == (u'?') || c == (u'#'); });
                    path = _url.mid(std::distance(_url.begin(), it-1), std::distance(it-1, end));
                    if (end != last && *end == (u'?')) // query found
                    {
                        it = ++end;
                        end = std::find(it, last, (u'#')); // fragment found
                        query = _url.mid(std::distance(_url.begin(), it), std::distance(it, end));
                    }
                    if (end != last && *end == (u'#'))
                        fragment = _url.mid(std::distance(_url.begin(), ++end));
                }
                else if (*it == (u'?')) // query found
                {
                    auto end = std::find(++it, last, (u'#'));
                    query = _url.mid(std::distance(_url.begin(), it), std::distance(it, end));
                    if (end != last)
                        fragment = _url.mid(std::distance(_url.begin(), ++end));
                }
                else // fragment found
                {
                    fragment = _url.mid(std::distance(_url.begin(), ++it));
                }
            }
            if (authority.size() > 64)
                return QUrl(); // authority cannot be greater than 64 symbols

            if (std::all_of(authority.begin(), authority.end(), isPlainLatinAuthority))
                break;

            const QString authEncoded = toPunycoded(authority);
            urlLink.setScheme(scheme.toString());
            urlLink.setAuthority(authEncoded);
            urlLink.setPath(path.toString(), QUrl::TolerantMode);
            urlLink.setQuery(query.toString());
            urlLink.setFragment(fragment.toString());
            break;
        }

        if (urlLink.isEmpty())
            urlLink = QUrl::fromUserInput(_url.toString());

        return urlLink;
    }

    QString addHttpsIfNeeded(const QString& _url)
    {
        const auto https = qsl("https://");
        return _url.startsWith(https) ? _url : https % _url;
    }

    QUrl addQueryItemsToUrl(QUrl _url, QUrlQuery _urlQuery, AddQueryItemPolicy _policy)
    {
        if (!_urlQuery.isEmpty())
        {
            auto initialQuery = QUrlQuery(_url.query());
            const bool keepExistingItems = AddQueryItemPolicy::KeepExistingItem == _policy;
            auto& sourceQuery = keepExistingItems ? initialQuery : _urlQuery;
            auto& additionalQuery = keepExistingItems ? _urlQuery : initialQuery;
            for (const auto& [key, value] : additionalQuery.queryItems())
            {
                if (!sourceQuery.hasQueryItem(key))
                    sourceQuery.addQueryItem(key, value);
            }
            _url.setQuery(sourceQuery);
        }
        return _url;
    }

    void openUrl(QStringView _url, OpenUrlConfirm _confirm)
    {
        if (Utils::isMentionLink(_url))
        {
            const auto aimId = extractAimIdFromMention(_url);
            if (!aimId.isEmpty())
            {
                std::string resStr;

                switch (openDialogOrProfile(aimId.toString(), OpenDOPParam::aimid))
                {
                case OpenDOPResult::dialog:
                    resStr = "Chat_Opened";
                    break;
                case OpenDOPResult::profile:
                    resStr = "Profile_Opened";
                    break;
                default:
                    break;
                }
                im_assert(!resStr.empty());
                if (!resStr.empty())
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_click_in_chat, { { "Result", std::move(resStr) } });
                return;
            }
        }

        const auto url = _url.toString();
        if (const auto command = UrlCommandBuilder::makeCommand(url, UrlCommandBuilder::Context::Internal); command->isValid())
        {
            command->execute();
            return;
        }

        if (const auto command = createOpenSharedMiniAppCommand(_url); command && command->isValid())
        {
            command->execute();
            return;
        }

        if (const auto command = createOpenCalendarAppCommand(_url); command && command->isValid())
        {
            command->execute();
            return;
        }

        const auto openWithWarning = !Ui::get_gui_settings()->get_value<bool>(settings_open_external_link_without_warning, settings_open_external_link_without_warning_default());

        auto isConfirmed = true;
        if (_confirm == OpenUrlConfirm::Yes && openWithWarning)
        {
            auto elidedUrl = url;
            if (elidedUrl.size() > getUrlInDialogElideSize())
                elidedUrl = elidedUrl.mid(0, getUrlInDialogElideSize()) % u"...";

            isConfirmed = openConfirmDialog(QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to open an external link?"),
                elidedUrl,
                settings_open_external_link_without_warning_default(),
                qsl(settings_open_external_link_without_warning));
        }

        if (!isConfirmed)
            return;

        if (!QDesktopServices::openUrl(fromUserInput(_url)))
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "Unable to open invalid url \"%1\"").arg(_url));
    }

    void registerCustomScheme()
    {
#if defined __linux__ && !defined BUILD_PKG_RPM
        const QString home = QDir::homePath() % u'/';
        const auto executableInfo = QFileInfo(QCoreApplication::applicationFilePath());
        const QString executableName = executableInfo.fileName();
        const QString executableDir = executableInfo.absoluteDir().absolutePath() % u'/';

        const QByteArray urlSchemeName = urlSchemes().front().toUtf8();

        if (QDir(home % u".local/").exists())
        {
            const QString apps = home % u".local/share/applications/";
            const QString icons = home % u".local/share/icons/";

            if (!QDir(apps).exists())
                QDir().mkpath(apps);

            if (!QDir(icons).exists())
                QDir().mkpath(icons);

            const auto oldExecutable = config::get().string(config::values::product_name_old);
            if (!oldExecutable.empty())
                QDir::home().remove(apps % QString::fromUtf8(oldExecutable.data(), oldExecutable.size()) % u"desktop.desktop");

            const QString desktopFileName = executableName % u"desktop.desktop";
            const auto tempPath = QDir::tempPath();
            const QString tempFile = tempPath % u'/' % desktopFileName;
            QDir().mkpath(tempPath);
            QFile f(tempFile);
            if (f.open(QIODevice::WriteOnly))
            {
                const QString icon = icons % executableName % u".png";

                QFile(icon).remove();

                const auto logoLinux = QPixmap(qsl(":/logo/logo_linux"));
                const auto iconName = qsl(":/logo/logo");
                const auto pixmap = logoLinux.isNull() ? Utils::renderSvg(iconName, (QSize(256, 256))) : logoLinux;
                const auto image = pixmap.toImage();
                image.save(icon);

                QFile(iconName).copy(icon);

                const auto appName = getAppTitle();

                QTextStream s(&f);
                s.setCodec("UTF-8");
                s << "[Desktop Entry]\n";
                s << "Version=1.0\n";
                s << "Name=" << appName << '\n';
                s << "Comment=" << QT_TRANSLATE_NOOP("linux_desktop_file", "Official desktop application for the %1 messaging service").arg(appName) << '\n';
                s << "TryExec=" << escapeShell(QFile::encodeName(executableDir + executableName)) << '\n';
                s << "Exec=" << escapeShell(QFile::encodeName(executableDir + executableName)) << " -urlcommand %u\n";
                s << "Icon=" << escapeShell(QFile::encodeName(icon)) << '\n';
                s << "Terminal=false\n";
                s << "Type=Application\n";
                s << "StartupWMClass=" << executableName << '\n';
                s << "Categories=Network;InstantMessaging;Qt;\n";
                s << "MimeType=";
                for (const auto& name : urlSchemes())
                    s << "x-scheme-handler/" << name.toUtf8() << ";";
                s << "\n";
                f.close();

                if (runCommand("desktop-file-install --dir=" % escapeShell(QFile::encodeName(home % u".local/share/applications")) % " --delete-original " % escapeShell(QFile::encodeName(tempFile))))
                {
                    runCommand("update-desktop-database " % escapeShell(QFile::encodeName(home % u".local/share/applications")));
                    runCommand("xdg-mime default " % desktopFileName.toLatin1() % " x-scheme-handler/" % urlSchemeName);
                }
                else // desktop-file-utils not installed, copy by hands
                {
                    auto dekstopFilePath = apps % desktopFileName;
                    if (QFile(tempFile).copy(dekstopFilePath))
                    {
                        runCommand("chmod +x " % dekstopFilePath.toLatin1());
                        runCommand("xdg-mime default " % desktopFileName.toLatin1() % " x-scheme-handler/" % urlSchemeName);
                    }
                    QFile(tempFile).remove();
                }
            }
        }

        if (runCommand("gconftool-2 -t string -s /desktop/gnome/url-handlers/" % urlSchemeName % "/command " % escapeShell(escapeShell(QFile::encodeName(executableDir + executableName)) % " -urlcommand %s")))
        {
            runCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" % urlSchemeName % "/needs_terminal false");
            runCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" % urlSchemeName % "/enabled true");
        }

        QString services;
        if (QDir(home % u".kde4/").exists())
            services = home % u".kde4/share/kde4/services/";
        else if (QDir(home % u".kde/").exists())
            services = home % u".kde/share/kde4/services/";

        if (!services.isEmpty())
        {
            if (!QDir(services).exists())
                QDir().mkpath(services);

            const auto path = services;
            const QString file = path % executableName % u".protocol";
            QFile f(file);
            if (f.open(QIODevice::WriteOnly))
            {
                QTextStream s(&f);
                s.setCodec("UTF-8");
                s << "[Protocol]\n";
                s << "exec=" << QFile::decodeName(escapeShell(QFile::encodeName(executableDir + executableName))) << " -urlcommand %u\n";
                s << "protocol=" << urlSchemeName << '\n';
                s << "input=none\n";
                s << "output=none\n";
                s << "helper=true\n";
                s << "listing=false\n";
                s << "reading=false\n";
                s << "writing=false\n";
                s << "makedir=false\n";
                s << "deleting=false\n";
                f.close();
            }
        }
#endif
    }
}

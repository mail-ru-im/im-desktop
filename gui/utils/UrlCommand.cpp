#include "stdafx.h"

#include "UrlCommand.h"
#include "utils/InterConnector.h"
#include "main_window/contact_list/ContactListModel.h"
#include "cache/stickers/stickers.h"
#include "utils/UrlParser.h"
#include "app_config.h"
#include "url_config.h"
#include "features.h"

namespace
{
    bool checkJoinQuery(const QUrl& _url)
    {
        const QUrlQuery urlQuery(_url);
        const auto items = urlQuery.queryItems();
        const bool join = std::any_of(items.cbegin(), items.cend(), [](const auto& param)
        {
            return param.first == ql1s("join") && param.second == ql1s("1");
        });

        return join;
    }
}


std::shared_ptr<UrlCommand> UrlCommandBuilder::makeCommand(QString _url, Context _context)
{
    auto command = std::make_shared<UrlCommand>();

    if (_url.endsWith(ql1c('/')))
        _url.chop(1);

    const static auto schemesWithSlashes = []() {
        const auto& urlSchemes = Utils::urlSchemes();
        std::vector<QString> schemes;
        schemes.reserve(urlSchemes.size());
        for (const auto& x : urlSchemes)
            schemes.push_back(x % ql1s("://"));
        return schemes;
    }();

    QString currentScheme;
    if (const auto it = std::find_if(schemesWithSlashes.begin(), schemesWithSlashes.end(), [&_url](const auto &x) { return _url.startsWith(x, Qt::CaseInsensitive); }); it != schemesWithSlashes.end())
        currentScheme = *it;

    const auto slashCount = currentScheme.count(ql1c('/'));
    const auto noAdditionalSlashes = _url.count(ql1c('/')) == slashCount; // redundant, but safe

    if (!currentScheme.isEmpty()) // [icq|magent|myteam-messenger|itd-messenger]://uin
    {
        if (_url.indexOf(qsl("attach_phone_result")) != -1)
        {
            command = std::make_shared<PhoneAttachedCommand>(_url.indexOf(qsl("success=1")) != -1, _url.indexOf(qsl("cancel=1")) != -1);
            return command;
        }
        else if (noAdditionalSlashes)
        {
            command = std::make_shared<OpenIdCommand>(_url.mid(currentScheme.size()));
            return command;
        }
    }

    static const auto wwwDot = ql1s("www.");

    const QUrl url(_url);
    const QString scheme = url.scheme();
    const QString host = url.host();
    QStringRef hostRef(&host);
    QString path = url.path();

    if (hostRef.startsWith(wwwDot))
        hostRef = hostRef.mid(wwwDot.size());

    if (!path.isEmpty())
        path.remove(0, 1);

    if (path.endsWith(ql1c('/')))
        path.chop(1);

    if (Utils::isInternalScheme(scheme)) // check for urls starting with icq://, magent://, myteam-messenger://, itd-messenger://
    {
        if (hostRef.compare(ql1s(url_command_join_livechat), Qt::CaseInsensitive) == 0)            // [icq|magent|myteam-messenger|itd-messenger]://chat/stamp[?join=1]
            command = std::make_shared<OpenChatCommand>(path, checkJoinQuery(url));

        else if (hostRef.compare(ql1s(url_command_open_profile), Qt::CaseInsensitive) == 0)        // [icq|magent|myteam-messenger|itd-messenger]://people/uin
            command = std::make_shared<OpenContactCommand>(path);

        else if (hostRef.compare(ql1s(url_command_stickerpack_info), Qt::CaseInsensitive) == 0)    // [icq|magent|myteam-messenger|itd-messenger]://s/store_id
            command = std::make_shared<ShowStickerpackCommand>(path, _context);

        else if (hostRef.compare(ql1s(url_command_service), Qt::CaseInsensitive) == 0)             // [icq|magent|myteam-messenger|itd-messenger]://service/*
            command = std::make_shared<ServiceCommand>(_url);
    }
    else
    {
        static const auto icqCom = ql1s("icq.com");
        static const auto icqIm = ql1s("icq.im");
        static const auto stickerStore = Ui::getUrlConfig().getUrlStickerShare();
        static const auto agentProfile = Features::getProfileDomainAgent();

        const auto parts = path.splitRef(ql1c('/'));

        if (hostRef.compare(icqCom, Qt::CaseInsensitive) == 0)
        {
            const QRegularExpression re(qsl("^\\d*$")); // only numbers
            const auto match = re.match(path);

            Utils::UrlParser p;
            p.process(path);
            const auto isEmail = p.hasUrl() && p.getUrl().is_email();

            if (!path.contains(ql1c('/')) && (match.hasMatch() || isEmail))                     // icq.com/uin, where uin is either an email or a string of numbers
                command = std::make_shared<OpenContactCommand>(path);
            else if (parts.size() > 1 && parts.first().compare(ql1s(url_command_join_livechat), Qt::CaseInsensitive) == 0)  // icq.com/chat/stamp[?join=1]
                command = std::make_shared<OpenChatCommand>(parts[1].toString(), checkJoinQuery(url));
        }
        else if (_url.contains(agentProfile)) // for unparcable by QUrl hosts, containing '/profile'
        {
            const auto profileParts = _url.splitRef(ql1s(url_commandpath_agent_profile) + ql1c('/'));
            if (profileParts.size() > 1)
                command = std::make_shared<OpenIdCommand>(profileParts[1].toString());
        }
        else if (_url.contains(stickerStore)) // cicq.org/s/store_id
        {
            if (parts.size() > 1 && parts.first().compare(ql1s(url_command_stickerpack_info), Qt::CaseInsensitive) == 0)
                command = std::make_shared<ShowStickerpackCommand>(parts[1].toString(), _context);
        }
        else if (hostRef.compare(icqIm, Qt::CaseInsensitive) == 0)
        {
            if (!path.contains(ql1c('/')))                                                      // icq.com/uin
                command = std::make_shared<OpenIdCommand>(path);
        }
    }

    return command;
}

//////////////////////////////////////////////////////////////////////////
// OpenContactCommand
//////////////////////////////////////////////////////////////////////////

OpenContactCommand::OpenContactCommand(const QString& _aimId)
    : aimId_(_aimId)
{
    isValid_ = !aimId_.isEmpty();
}

void OpenContactCommand::execute()
{
    Utils::openDialogOrProfile(aimId_);
}

//////////////////////////////////////////////////////////////////////////
// OpenChatCommand
//////////////////////////////////////////////////////////////////////////

OpenChatCommand::OpenChatCommand(const QString &_stamp, bool _join)
    : stamp_(_stamp)
    , join_(_join)
{
    isValid_ = !stamp_.isEmpty();
}

void OpenChatCommand::execute()
{
    Logic::getContactListModel()->joinLiveChat(stamp_, join_);
}

//////////////////////////////////////////////////////////////////////////
// OpenIdCommand
//////////////////////////////////////////////////////////////////////////

OpenIdCommand::OpenIdCommand(const QString &_id)
    : id_(_id)
{
    isValid_ = !id_.isEmpty();
}

void OpenIdCommand::execute()
{
    emit Utils::InterConnector::instance().openDialogOrProfileById(id_);
}

//////////////////////////////////////////////////////////////////////////
// PhoneAttachedCommand
//////////////////////////////////////////////////////////////////////////

PhoneAttachedCommand::PhoneAttachedCommand(const bool _success, const bool _cancel)
    : success_(_success)
    , cancel_(_cancel)
{
    isValid_ = true;
}

void PhoneAttachedCommand::execute()
{
    if (cancel_)
        emit Utils::InterConnector::instance().phoneAttachmentCancelled();
//     else
//         emit Utils::InterConnector::instance().phoneAttached(success_);
}

//////////////////////////////////////////////////////////////////////////
// ShowStickerpackCommand
//////////////////////////////////////////////////////////////////////////

ShowStickerpackCommand::ShowStickerpackCommand(const QString &_storeId, UrlCommandBuilder::Context _context)
    : storeId_(_storeId)
    , context_(_context)
{
    isValid_ = !storeId_.isEmpty();
}

void ShowStickerpackCommand::execute()
{
    Ui::Stickers::StatContext context;

    if (context_ == UrlCommandBuilder::Context::External)
        context = Ui::Stickers::StatContext::Browser;
    else
        context = Ui::Stickers::StatContext::Chat;

    Ui::Stickers::showStickersPackByStoreId(storeId_, context);
}

//////////////////////////////////////////////////////////////////////////
// ServiceCommand
//////////////////////////////////////////////////////////////////////////

ServiceCommand::ServiceCommand(const QString &_url)
    : url_(_url)
{
    isValid_ = !url_.isEmpty();
}

void ServiceCommand::execute()
{
    emit Utils::InterConnector::instance().gotServiceUrls({ url_ });
}

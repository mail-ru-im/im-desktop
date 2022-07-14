#pragma once

namespace Utils
{
    const std::vector<QString>& urlSchemes();
    bool isInternalScheme(const QString& scheme);

    QUrl fromUserInput(QStringView _url);

    QString addHttpsIfNeeded(const QString& _url);

    enum class AddQueryItemPolicy
    {
        ReplaceIfItemExists,
        KeepExistingItem
    };
    QUrl addQueryItemsToUrl(QUrl _url, QUrlQuery _urlQuery, AddQueryItemPolicy _policy);

    void openMailBox(const QString& _email, const QString& _mrimKey, const QString& _mailId);
    void openAttachUrl(const QString& _email, const QString& _mrimKey, bool _canCancel);
    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey);

    enum class OpenUrlConfirm
    {
        No,
        Yes,
    };
    void openUrl(QStringView _url, OpenUrlConfirm _confirm = OpenUrlConfirm::No);

    enum class OpenAt
    {
        Folder,
        Launcher,
    };

    enum class OpenWithWarning
    {
        No,
        Yes,
    };

    void openFileOrFolder(const QString& _chatAimId, QStringView _path, OpenAt _openAt, OpenWithWarning _withWarning = OpenWithWarning::Yes);
    void openFileLocation(const QString& _path);

    bool isNick(QStringView _text);
    QString makeNick(const QString& _text);

    QString getEmailFromMentionLink(const QString& _link);

    inline bool isMentionLink(QStringView _url)
    {
        return _url.startsWith(u"@[") && _url.endsWith(u']');
    }

    bool isHashtag(QStringView _text);

    bool doesContainMentionLink(QStringView _url);

    bool isServiceLink(const QString& _url);

    QStringView normalizeLink(QStringView _link);

    QString getDomainUrl();

    bool isUrlVCS(const QString& _url);

    void registerCustomScheme();
}

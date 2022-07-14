#pragma once

namespace core
{
    class coll_helper;
}

namespace Ui
{
    class UrlConfig
    {
    public:
        UrlConfig() = default;
        void updateUrls(const core::coll_helper& _coll);

    	[[nodiscard]] const QString& getUrlFilesParser() const noexcept { return filesParse_; }
        [[nodiscard]] const QString& getUrlStickerShare() const noexcept { return stickerShare_; }
        [[nodiscard]] const QString& getUrlProfile() const noexcept { return profile_; }

        [[nodiscard]] const QString& getUrlMailAuth() const noexcept { return mailAuth_; }
        [[nodiscard]] const QString& getUrlMailRedirect() const noexcept { return mailRedirect_; }
        [[nodiscard]] const QString& getUrlMailWin() const noexcept { return mailWin_; }
        [[nodiscard]] const QString& getUrlMailRead() const noexcept { return mailRead_; }

        [[nodiscard]] const QString& getUrlAppUpdate() const noexcept { return appUpdate_; }

        [[nodiscard]] const QString& getBaseBinary() const noexcept { return baseBinary_; }
        [[nodiscard]] const QString& getBase() const noexcept { return base_; }

        [[nodiscard]] const QString& getDi(const bool _dark) const noexcept { return _dark && !diDark_.isEmpty() ? diDark_ : di_; }

        [[nodiscard]] const QString& getTasks() const noexcept { return tasks_; }

        [[nodiscard]] const QString& getCalendar() const noexcept { return calendar_; }

        [[nodiscard]] const QString& getMail() const noexcept { return mail_; }

        [[nodiscard]] const QString& getMiniappSharing() const noexcept { return miniappSharing_; }

        [[nodiscard]] const QString& getTarmMail() const noexcept { return tarm_mail_; }

        [[nodiscard]] const QString& getTarmCalls() const noexcept { return tarm_calls_; }

        [[nodiscard]] const QString& getTarmCloud() const noexcept { return tarm_cloud_; }

        [[nodiscard]] const QString& getConfigHost() const noexcept { return configHost_; }

        [[nodiscard]] const QString& getAvatarsUrl() const noexcept { return avatars_; }

        const QString& getAuthUrl();
        const QString& getRedirectUri();

        const QVector<QString>& getVCSUrls() const noexcept;

        const QVector<QString>& getFsUrls() const noexcept;

    	bool isMailConfigPresent() const noexcept;

    private:
        QString base_;
        QString baseBinary_;

        QString filesParse_;
        QString stickerShare_;
        QString profile_;

        QString mailAuth_;
        QString mailRedirect_;
        QString mailWin_;
        QString mailRead_;

        QString appUpdate_;

        QString di_;
        QString diDark_;

        QString tasks_;
        QString calendar_;
        QString mail_;

        QString miniappSharing_;

        QString tarm_mail_;
        QString tarm_calls_;
        QString tarm_cloud_;

        QString configHost_;

        QString avatars_;

        QString authUrl_;
        QString redirectUri_;

        QVector<QString> vcsUrls_;
    };

    UrlConfig& getUrlConfig();
}
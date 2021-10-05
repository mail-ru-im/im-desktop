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

        [[nodiscard]] const QString& getConfigHost() const noexcept { return configHost_; }

        const QVector<QString>& getVCSUrls() const noexcept;

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

        QString configHost_;

        QVector<QString> vcsUrls_;
    };

    UrlConfig& getUrlConfig();
}
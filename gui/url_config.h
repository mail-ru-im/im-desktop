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

        const QVector<QString>& getVCSUrls() const noexcept;

    	bool isMailConfigPresent() const noexcept;

    private:
        QString filesParse_;
        QString stickerShare_;
        QString profile_;

        QString mailAuth_;
        QString mailRedirect_;
        QString mailWin_;
        QString mailRead_;

        QString appUpdate_;
        QString baseBinary_;

        QVector<QString> vcsUrls_;
    };

    UrlConfig& getUrlConfig();
}
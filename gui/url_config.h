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

    	[[nodiscard]] const QString& getUrlFilesParser() const { return filesParse_; }
        [[nodiscard]] const QString& getUrlStickerShare() const { return stickerShare_; }
        [[nodiscard]] const QString& getUrlProfile() const { return profile_; }

        [[nodiscard]] const QString& getUrlMailAuth() const { return mailAuth_; }
        [[nodiscard]] const QString& getUrlMailRedirect() const { return mailRedirect_; }
        [[nodiscard]] const QString& getUrlMailWin() const { return mailWin_; }
        [[nodiscard]] const QString& getUrlMailRead() const { return mailRead_; }

    	bool isMailConfigPresent() const;

    private:
        QString filesParse_;
        QString stickerShare_;
        QString profile_;

        QString mailAuth_;
        QString mailRedirect_;
        QString mailWin_;
        QString mailRead_;
    };

    UrlConfig& getUrlConfig();
}
#pragma once
#include "types/filesharing_download_result.h"
#include "utils/SvgUtils.h"

using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

namespace Ui
{
    struct AppIcon
    {
        QString id_;
        Utils::StyledPixmap icon_ = Utils::StyledPixmap {};
        QString defaultIconPath_;
        int64_t seq_ = 0;
    };

    class MiniApp : public QObject
    {
        Q_OBJECT

    public:
        MiniApp(const QString& _id,
            const QString& _description,
            const QString& _title,
            const QString& _url,
            const QString& _urlDark,
            const QString& _sn,
            const QString& _stamp,
            const AppIcon& _iconFilled,
            const AppIcon& _iconOutline,
            bool _enabled = false,
            bool _needsAuth = true,
            bool _external = false,
            bool _isServiceApp = false,
            QObject* _parent = nullptr);

        MiniApp(QObject* _parent = nullptr);
        MiniApp(const MiniApp& _other);
        MiniApp& operator=(const MiniApp& _other);
        MiniApp& operator=(MiniApp&& _other);

        bool needUpdate(const MiniApp& _other, bool _isDark) const;

        void setDefaultIcons();
        void unserialize(const rapidjson::Value& _node);

        void setId(const QString& _id);
        const QString& getId() const;

        void setEnabled(bool _val);
        bool isEnabled() const;

        void setService(bool _val);
        bool isServiceApp() const;
        bool isCustomApp() const;

        const QString& getTitle() const;
        const QString& getDescription() const;
        const QString& getUrl(bool _isDark) const;
        bool needsAuth() const;
        bool isExternal() const;
        bool isValid() const;

        const Utils::StyledPixmap& getIcon(bool _active) const;
        void setDefaultIconPaths(const QString& _pathNormal, const QString& _pathActive);
        const QString& getDefaultIconPath(bool _active) const;
        QString getAccessibleName() const;

        const QString& getAimid() const;
        const QString& getStamp() const;

        const std::vector<QString>& templateDomains() const;

    private Q_SLOTS:
        void onIconLoaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void onLoadError(qint64 _seq);

    private:
        void connectSignals();
        void makeCopy(const MiniApp& _other);

    private:
        QString id_;
        QString description_;
        QString name_;
        QString url_;
        QString urlDark_;
        QString sn_;
        QString stamp_;
        AppIcon iconFilled_;
        AppIcon iconOutline_;
        std::vector<QString> templateDomains_;
        bool enabled_ = false;
        bool needsAuth_ = true;
        bool external_ = false;
        bool isServiceApp_ = false;
    };
}

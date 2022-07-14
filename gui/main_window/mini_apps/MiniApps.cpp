#include "stdafx.h"
#include "MiniApps.h"
#include "MiniAppsContainer.h"
#include "MiniAppsUtils.h"

#include "utils/utils.h"
#include "utils/JsonUtils.h"
#include "styles/ThemeParameters.h"
#include "core_dispatcher.h"
#include "url_config.h"

namespace
{

    QSize tabIconSize() noexcept
    {
        return Utils::scale_value(QSize(28, 28));
    }

    auto iconNormalColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto iconActiveColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
    }

    Utils::StyledPixmap getDefaultPixmap(bool _active)
    {
        // TODO: replace icons when designed
        const auto px = Utils::StyledPixmap(qsl(":/favorites_icon"), tabIconSize(), iconNormalColorKey());
        const auto pxActive = Utils::StyledPixmap(qsl(":/favorites_icon"), tabIconSize(), iconActiveColorKey());
        return _active ? pxActive : px;
    }

    QString getDownloadFolder()
    {
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation) % ql1c('/');
    }

    QString getSendUrl(const QString& _fsId)
    {
        return u"https://" % Ui::getUrlConfig().getUrlFilesParser() % u'/' % _fsId;
    }
}

namespace Ui
{
    MiniApp::MiniApp(const QString& _id,
                     const QString& _description,
                     const QString& _title,
                     const QString& _url,
                     const QString& _urlDark,
                     const QString& _sn,
                     const QString& _stamp,
                     const AppIcon& _iconFilled,
                     const AppIcon& _iconOutline,
                     bool _enabled,
                     bool _needsAuth,
                     bool _external,
                     bool _isServiceApp,
                     QObject* _parent)
        : QObject(_parent)
        , id_(_id)
        , description_(_description)
        , name_(_title)
        , url_(_url)
        , urlDark_(_urlDark)
        , sn_(_sn)
        , stamp_(_stamp)
        , iconFilled_(_iconFilled)
        , iconOutline_(_iconOutline)
        , enabled_(_enabled)
        , needsAuth_(_needsAuth)
        , external_(_external)
        , isServiceApp_(_isServiceApp)
    {
        connectSignals();
    }

    MiniApp::MiniApp(QObject* _parent)
        : QObject(_parent)
    {
        connectSignals();
    }

    void MiniApp::setDefaultIcons()
    {
        if (iconFilled_.id_.isEmpty())
            iconOutline_.icon_ = getDefaultPixmap(false);
        if (iconFilled_.id_.isEmpty())
            iconFilled_.icon_ = getDefaultPixmap(true);
    }

    MiniApp::MiniApp(const MiniApp& _other)
    {
        makeCopy(_other);
    }

    MiniApp& MiniApp::operator=(const MiniApp& _other)
    {
        if (this == &_other)
            return *this;

        makeCopy(_other);
        return *this;
    }

    MiniApp& MiniApp::operator=(MiniApp&& _other)
    {
        if (this == &_other)
            return *this;

        id_ = std::exchange(_other.id_, {});
        description_ = std::exchange(_other.description_, {});
        name_ = std::exchange(_other.name_, {});
        url_ = std::exchange(_other.url_, {});
        urlDark_ = std::exchange(_other.urlDark_, {});
        sn_ = std::exchange(_other.sn_, {});
        stamp_ = std::exchange(_other.stamp_, {});
        iconFilled_ = std::exchange(_other.iconFilled_, AppIcon {});
        iconOutline_ = std::exchange(_other.iconOutline_, AppIcon {});
        templateDomains_ = std::exchange(_other.templateDomains_, {});
        enabled_ = std::exchange(_other.enabled_, false);
        needsAuth_ = std::exchange(_other.needsAuth_, false);
        external_ = std::exchange(_other.external_, false);
        isServiceApp_ = std::exchange(_other.isServiceApp_, false);
        connectSignals();
        return *this;
    }

    bool MiniApp::needUpdate(const MiniApp& _other, bool _isDark) const
    {
        return getUrl(_isDark) != _other.getUrl(_isDark)
            || getTitle() != _other.getTitle()
            || getDescription() != _other.getDescription()
            || getDefaultIconPath(false) != _other.getDefaultIconPath(false)
            || getDefaultIconPath(true) != _other.getDefaultIconPath(true);
    }

    void MiniApp::unserialize(const rapidjson::Value& _node)
    {
        if (id_.isEmpty())
            JsonUtils::unserialize_value(_node, "id", id_);

        JsonUtils::unserialize_value(_node, "name", name_);
        if (name_.isEmpty())
            name_ = Utils::MiniApps::getDefaultName(id_);

        JsonUtils::unserialize_value(_node, "description", description_);
        QString baseUrl;
        QString baseUrlDark;
        JsonUtils::unserialize_value(_node, "url", baseUrl);
        url_ = Utils::MiniApps::composeAppUrl(id_, baseUrl, false).toString();
        JsonUtils::unserialize_value(_node, "url-dark", baseUrlDark);
        urlDark_ = Utils::MiniApps::composeAppUrl(id_, baseUrlDark.isEmpty() ? baseUrl : baseUrlDark, true).toString();

        if (!Utils::MiniApps::setDefaultAvatar(*this))
        {
            auto downloadIcon = [this, &_node](const QString& _name, AppIcon& _icon, bool _filled)
            {
                auto iconIdName = _name + qsl("Id");
                JsonUtils::unserialize_value(_node, iconIdName.toStdString().c_str(), _icon.id_);
                if (!_icon.id_.isEmpty())
                    _icon.seq_ = Ui::GetDispatcher()->downloadSharedFile({}, getSendUrl(_icon.id_), false, getDownloadFolder() % id_ % (qsl("/") + _name), true);
                else
                    _icon.icon_ = getDefaultPixmap(_filled);
            };
            downloadIcon(qsl("filledIcon"), iconFilled_, true);
            downloadIcon(qsl("outlinedIcon"), iconOutline_, false);
        }

        JsonUtils::unserialize_value(_node, "sn", sn_);
        JsonUtils::unserialize_value(_node, "stamp", stamp_);
        JsonUtils::unserialize_value(_node, "external", external_);
        JsonUtils::unserialize_value(_node, "needs_auth", needsAuth_);
        JsonUtils::unserialize_value(_node, "enabled", enabled_);

        templateDomains_.clear();
        if (const auto domains = _node.FindMember("template-domains"); domains != _node.MemberEnd() && domains->value.IsArray())
        {
            auto domains_array = domains->value.GetArray();
            templateDomains_.reserve(domains_array.Size() + 2);
            for (const auto& domain : domains_array)
            {
                if (domain.IsString())
                {
                    if (auto domainString = JsonUtils::getString(domain); !domainString.isEmpty())
                        templateDomains_.push_back(Utils::addHttpsIfNeeded(domainString));
                }
            }
        }
        if (!baseUrl.isEmpty())
            templateDomains_.push_back(Utils::addHttpsIfNeeded(baseUrl));
        if (!baseUrlDark.isEmpty())
            templateDomains_.push_back(Utils::addHttpsIfNeeded(baseUrlDark));
    }

    const QString& MiniApp::getId() const
    {
        return id_;
    }

    void MiniApp::setId(const QString& _id)
    {
        id_ = _id;
    }

    const QString& MiniApp::getTitle() const
    {
        return name_;
    }

    const QString& MiniApp::getDescription() const
    {
        return description_;
    }

    const QString& MiniApp::getUrl(bool _isDark) const
    {
        if (_isDark && !urlDark_.isEmpty() || url_.isEmpty())
            return urlDark_;

        return url_;
    }

    bool MiniApp::isEnabled() const
    {
        return enabled_;
    }

    bool MiniApp::needsAuth() const
    {
        return needsAuth_;
    }

    bool MiniApp::isExternal() const
    {
        return external_;
    }

    bool MiniApp::isValid() const
    {
        return !id_.isEmpty();
    }

    const Utils::StyledPixmap& MiniApp::getIcon(bool _active) const
    {
        return _active ? iconFilled_.icon_ : iconOutline_.icon_;
    }

    void MiniApp::setDefaultIconPaths(const QString& _pathNormal, const QString& _pathActive)
    {
        iconFilled_.defaultIconPath_ = _pathActive;
        iconOutline_.defaultIconPath_ = _pathNormal;
    }

    const QString& MiniApp::getDefaultIconPath(bool _active) const
    {
        return _active ? iconFilled_.defaultIconPath_ : iconOutline_.defaultIconPath_;
    }

    QString MiniApp::getAccessibleName() const
    {
        const auto name = qsl("AS ") + id_.at(0).toUpper() + id_.mid(1) + qsl("Page");
        return name;
    }

    const QString& MiniApp::getAimid() const
    {
        return sn_;
    }

    const QString& MiniApp::getStamp() const
    {
        return stamp_;
    }

    const std::vector<QString>& MiniApp::templateDomains() const
    {
        return templateDomains_;
    }

    void MiniApp::setEnabled(bool _val)
    {
        enabled_ = _val;
    }

    void MiniApp::setService(bool _val)
    {
        isServiceApp_ = _val;
    }

    bool MiniApp::isServiceApp() const
    {
        return isServiceApp_;
    }

    bool MiniApp::isCustomApp() const
    {
        return !isServiceApp_;
    }

    void MiniApp::onIconLoaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
    {
        const auto checkIcon = [this, _seq, _result](AppIcon& _ico, bool _filled) -> bool
        {
            if (_ico.seq_ != _seq)
                return false;

            _ico.seq_ = 0;
            QFile file(_result.filename_);
            if (!file.open(QIODevice::ReadOnly))
                return false;

            _ico.icon_ = Utils::StyledPixmap(_result.filename_, {}, _filled ? iconActiveColorKey() : iconNormalColorKey());
            if (_ico.icon_.cachedPixmap().isNull())
                _ico.icon_ = getDefaultPixmap(_filled);
            Q_EMIT Logic::GetAppsContainer()->appIconUpdated(id_);
            return true;
        };

        if (!checkIcon(iconFilled_, true))
            checkIcon(iconOutline_, false);
    }

    void MiniApp::onLoadError(qint64 _seq)
    {
        if (iconFilled_.seq_ == _seq)
        {
            iconFilled_.seq_ = 0;
            return;
        }

        if (iconOutline_.seq_ == _seq)
            iconOutline_.seq_ = 0;
    }

    void MiniApp::connectSignals()
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded, this, &MiniApp::onIconLoaded, Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingError, this, &MiniApp::onLoadError, Qt::UniqueConnection);
    }

    void MiniApp::makeCopy(const MiniApp& _other)
    {
        id_ = _other.id_;
        description_ = _other.description_;
        name_ = _other.name_;
        url_ = _other.url_;
        urlDark_ = _other.urlDark_;
        sn_ = _other.sn_;
        stamp_ = _other.stamp_;
        iconFilled_ = _other.iconFilled_;
        iconOutline_ = _other.iconOutline_;
        templateDomains_ = _other.templateDomains_;
        enabled_ = _other.enabled_;
        needsAuth_ = _other.needsAuth_;
        external_ = _other.external_;
        isServiceApp_ = _other.isServiceApp_;
        connectSignals();
    }
}

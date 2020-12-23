#include "stdafx.h"

#include "../MediaCapturePermissions.h"

#include "utils/win32/RegistryChangeNotifier_win.h"

namespace
{
    enum class AppType
    {
        Classic,
        Store,
    };

    QString getRegistryPermissionKey(media::permissions::DeviceType _devType, AppType _appType)
    {
        QString key;

        if (_devType == media::permissions::DeviceType::Microphone)
            key += ql1s("Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone");
        else if (_devType == media::permissions::DeviceType::Camera)
            key += ql1s("Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam");

        if (!key.isEmpty() && _appType == AppType::Classic)
            key += ql1s("\\NonPackaged");

        return key;
    }
}

namespace media::permissions
{
    Permission checkPermission(DeviceType type)
    {
        auto settings = [](DeviceType _type, AppType _appType) -> std::optional<QString>
        {
            if (auto key = getRegistryPermissionKey(_type, _appType); !key.isEmpty())
                return u"HKEY_CURRENT_USER\\" % key;

            return std::nullopt;
        };

        auto getResult = [&settings](DeviceType type, AppType _appType, Permission& _result)
        {
            if (const auto regPath = settings(type, _appType); regPath)
            {
                QSettings s(*regPath, QSettings::NativeFormat);
                if (s.value(qsl("Value")).toString() == u"Deny")
                    _result = Permission::Denied;
            }
        };

        Permission result = Permission::Allowed;
        getResult(type, AppType::Store, result);

        if (result == Permission::Allowed)
            getResult(type, AppType::Classic, result);

        return result;
    }

    void requestPermission(DeviceType, PermissionCallback _callback)
    {
        if (_callback)
            _callback(true);
    }

    void openPermissionSettings(DeviceType type)
    {
        auto settings = [](DeviceType type) -> std::optional<std::wstring_view>
        {
            if (type == DeviceType::Microphone)
                return std::wstring_view(L"ms-settings:privacy-microphone");
            if (type == DeviceType::Camera)
                return std::wstring_view(L"ms-settings:privacy-webcam");
            return std::nullopt;
        };
        if (const auto s = settings(type); s)
        {
            ShellExecute(
                nullptr,
                L"open",
                s->data(),
                nullptr,
                nullptr,
                SW_SHOWDEFAULT);
        }
    }

    void PermissonsChangeNotifier::start()
    {
        if (!notifier_)
        {
            auto notifier = new Utils::RegistryChangeNotifier(this);
            for (auto devType : { DeviceType::Microphone, DeviceType::Camera })
            {
                permissions_.emplace_back(std::make_pair(devType, checkPermission(devType)));

                for (auto key : { getRegistryPermissionKey(devType, AppType::Store), getRegistryPermissionKey(devType, AppType::Classic) })
                {
                    if (!key.isEmpty())
                        notifier->addLocation(HKEY_CURRENT_USER, key);
                }
            }

            connect(notifier, &Utils::RegistryChangeNotifier::finished, notifier, &QObject::deleteLater);
            connect(notifier, &Utils::RegistryChangeNotifier::changed, this, [this]()
            {
                for (auto& [devType, lastPerm] : permissions_)
                {
                    if (const auto perm = checkPermission(devType); lastPerm != perm)
                    {
                        lastPerm = perm;
                        Q_EMIT deviceChanged(devType);
                    }
                }
            });

            notifier_ = notifier;
            notifier_->start();
        }
    }

    void PermissonsChangeNotifier::stop()
    {
        if (notifier_)
        {
            notifier_->quit();
            permissions_.clear();
        }
    }
}

#pragma once

namespace media
{
namespace permissions
{
    enum class DeviceType
    {
        Microphone,
        Camera,
        Screen // macOS 10.15+ only
    };

    enum class Permission
    {
        NotDetermined = 0,
        Restricted = 1,
        Denied = 2,
        Allowed = 3,
        MaxValue = Allowed
    };

    class PermissonsChangeNotifier : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void deviceChanged(DeviceType);

    public:
        PermissonsChangeNotifier(QObject* _parent) : QObject(_parent) {}

        void start();
        void stop();

    private:
        QPointer<QThread> notifier_;
        std::vector<std::pair<DeviceType, Permission>> permissions_;
    };

    using PermissionCallback = std::function<void(bool)>;

    Permission checkPermission(DeviceType);
    void requestPermission(DeviceType, PermissionCallback);

    void openPermissionSettings(DeviceType);
}
}

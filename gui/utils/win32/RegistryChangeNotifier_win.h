#pragma once

namespace Utils
{
    class RegistryChangeWatcher
    {
        RegistryChangeWatcher(const RegistryChangeWatcher&) = delete;
        RegistryChangeWatcher& operator=(const RegistryChangeWatcher&) = delete;
        RegistryChangeWatcher(RegistryChangeWatcher&&) = delete;
        RegistryChangeWatcher& operator=(RegistryChangeWatcher&&) = delete;

    public:
        RegistryChangeWatcher() = default;
        ~RegistryChangeWatcher();

        void addLocation(HKEY _hive, const QString& _path);
        void addWatchEvent(HANDLE _event);

        bool hasChanged(std::chrono::milliseconds _msTimeout) const;
        bool isEmpty() const;

        bool reload();

        void clear();

    private:
        bool createWatchEvent(HKEY _key);

        std::vector<HANDLE> watchEvents_;
        std::vector<HKEY> registryHandles_;
        mutable std::mutex mutex_;
    };

    class RegistryChangeNotifier : public QThread
    {
        Q_OBJECT

    Q_SIGNALS:
        void changed();

    public:
        RegistryChangeNotifier(QObject* _parent) : QThread(_parent) {}
        ~RegistryChangeNotifier();

        void addLocation(HKEY _hive, const QString& _path);

    protected:
        void run() override;

    private:
        RegistryChangeWatcher watcher_;
        std::atomic_bool quit_ = false;
        HANDLE breakEvent_ = nullptr;
    };
}

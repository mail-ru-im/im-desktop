#include "stdafx.h"

#include "RegistryChangeNotifier_win.h"

namespace Utils
{
    RegistryChangeWatcher::~RegistryChangeWatcher()
    {
        clear();
    }

    void RegistryChangeWatcher::addLocation(HKEY _hive, const QString& _path)
    {
        std::scoped_lock lock(mutex_);
        HKEY openedKey;
        if (RegOpenKeyEx(_hive, reinterpret_cast<const wchar_t*>(_path.utf16()), 0, KEY_READ, &openedKey) != ERROR_SUCCESS)
            return;
        if (!createWatchEvent(openedKey))
            return;
        registryHandles_.emplace_back(openedKey);
    }

    void RegistryChangeWatcher::addWatchEvent(HANDLE _event)
    {
        std::scoped_lock lock(mutex_);
        watchEvents_.emplace_back(_event);
    }

    bool RegistryChangeWatcher::hasChanged(std::chrono::milliseconds _msTimeout) const
    {
        std::scoped_lock lock(mutex_);
        return !watchEvents_.empty() && WaitForMultipleObjects(watchEvents_.size(), watchEvents_.data(), false, static_cast<DWORD>(_msTimeout.count())) < WAIT_OBJECT_0 + watchEvents_.size();
    }

    bool RegistryChangeWatcher::isEmpty() const
    {
        std::scoped_lock lock(mutex_);
        return watchEvents_.empty();
    }

    bool RegistryChangeWatcher::reload()
    {
        std::scoped_lock lock(mutex_);
        for (auto event : std::as_const(watchEvents_))
            CloseHandle(event);
        watchEvents_.clear();

        for (auto key : std::as_const(registryHandles_))
        {
            if (!createWatchEvent(key))
                return false;
        }
        return true;
    }

    void RegistryChangeWatcher::clear()
    {
        std::scoped_lock lock(mutex_);
        for (auto event : std::as_const(watchEvents_))
            CloseHandle(event);
        watchEvents_.clear();

        for (auto key : std::as_const(registryHandles_))
            RegCloseKey(key);
        registryHandles_.clear();
    }

    bool RegistryChangeWatcher::createWatchEvent(HKEY _key)
    {
        static const DWORD filter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;
        auto handle = CreateEvent(nullptr, true, false, nullptr);
        if (RegNotifyChangeKeyValue(_key, true, filter, handle, true) != ERROR_SUCCESS)
        {
            CloseHandle(handle);
            return false;
        }
        watchEvents_.emplace_back(handle);
        return true;
    }

    RegistryChangeNotifier::~RegistryChangeNotifier()
    {
        quit_ = true;
        if (breakEvent_)
            SetEvent(breakEvent_);

        wait();
    }

    void RegistryChangeNotifier::addLocation(HKEY _hive, const QString& _path)
    {
        if (isRunning() && breakEvent_)
            SetEvent(breakEvent_);

        watcher_.addLocation(_hive, _path);
    }

    void RegistryChangeNotifier::run()
    {
        if (watcher_.isEmpty())
            return;

        // add an event to interrupt the watcher
        auto setBreakEvent = [this]()
        {
            breakEvent_ = CreateEvent(nullptr, true, false, nullptr);
            watcher_.addWatchEvent(breakEvent_);
        };
        setBreakEvent();

        while (1)
        {
            auto isChanged = watcher_.hasChanged(std::chrono::milliseconds::max());
            if (quit_.load())
                return;

            if (isChanged)
            {
                Q_EMIT changed();

                if (!watcher_.reload())
                    return;
                // after reload necessary to add the interrupt event again (it was closed)
                setBreakEvent();
            }
        }
    }

}

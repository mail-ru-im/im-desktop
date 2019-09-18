#pragma once

#include <rapidjson/document.h>

#include "omicron_helper.h"

namespace Omicron
{
    class OmicronContainer
    {
    public:
        static OmicronContainer& instance();

        void unserialize(core::coll_helper _collection);
        void cleanup();

        bool boolValue(std::string_view _keyName, bool _defaultValue) const;
        int intValue(std::string_view _keyName, int _defaultValue) const;
        unsigned int uintValue(std::string_view _keyName, unsigned int _defaultValue) const;
        int64_t int64Value(std::string_view _keyName, int64_t _defaultValue) const;
        uint64_t uint64Value(std::string_view _keyName, uint64_t _defaultValue) const;
        double doubleValue(std::string_view _keyName, double _defaultValue) const;
        std::string stringValue(std::string_view _keyName, const std::string& _defaultValue) const;
        QString stringValue(std::string_view _keyName, const QString& _defaultValue) const;
        std::string jsonValue(std::string_view _keyName, const std::string& _defaultValue) const;
        QString jsonValue(std::string_view _keyName, const QString& _defaultValue) const;

    private:
        class SpinLock
        {
            std::atomic_flag locked = ATOMIC_FLAG_INIT;
        public:
            void lock()
            {
                while (locked.test_and_set(std::memory_order_acquire)) { ; }
            }
            void unlock()
            {
                locked.clear(std::memory_order_release);
            }
        };

        std::atomic<bool> init_;
        std::shared_ptr<rapidjson::Document> data_;
        mutable SpinLock dataLock_;

    private:
        OmicronContainer();
        std::shared_ptr<const rapidjson::Document> getData() const;
    };
}

#pragma once

#include "../corelib/collection_helper.h"
#include "utils/StringComparator.h"

namespace Ui
{
    constexpr std::chrono::milliseconds period_for_stats_settings_ms = std::chrono::minutes(24);
    constexpr std::chrono::milliseconds period_for_start_stats_settings_ms = std::chrono::minutes(1);

    class qt_gui_settings : public QObject
    {
        Q_OBJECT

Q_SIGNALS:

        void received();
        void changed(const QString&);

    public:
        using VoipCallsCountMap = std::map<QString, int32_t>;

    private:

        struct settings_value
        {
            std::vector<char> data_;
        };

        std::map<QString, settings_value, Utils::StringComparator>   values_;

        void post_value_to_core(const QString& _name, const settings_value& _val) const;

        void set_value_simple_data(const QString& _name, const char* _data, int _len, bool _postToCore = true)
        {
            auto& val = values_[_name];
            if (!_len)
            {
                val.data_.clear();
            }
            else
            {
                val.data_.resize(_len);
                memcpy(&val.data_[0], _data, _len);
            }

            if (_postToCore)
                post_value_to_core(_name, val);

            Q_EMIT changed(_name);
        }

        template <class t_>
        void set_value_simple(const QString& _name, const t_& _value)
        {
            set_value_simple_data(_name, (const char*) &_value, sizeof(_value));
        }

        template <class t_>
        bool get_value_simple_data(const t_& _name, std::vector<char>& _data) const
        {
            auto iter = values_.find(_name);
            if (iter == values_.end())
                return false;

            _data = iter->second.data_;
            return true;
        }

        template <class t_, class u_>
        t_ get_value_simple(const u_& _name, const t_& _defaultValue) const
        {
            std::vector<char> data;
            if (!get_value_simple_data(_name, data))
                return _defaultValue;

            if (data.size() != sizeof(t_))
            {
                assert(false);
                return _defaultValue;
            }

            t_ val;
            ::memcpy(&val, &data[0], sizeof(t_));

            return val;
        }

    public:

        template <class t_>
        void set_value(const QString& _name, const t_& _value)
        {
            assert(false);
        }

        template <class t_>
        void set_value(const char* _name, const t_& _value)
        {
            set_value(QString::fromLatin1(_name), _value);
        }

        template <class t_>
        void set_value(const char* _name, const t_& _value, const QString& _prefix)
        {
            set_value(_prefix + QString::fromLatin1(_name), _value);
        }

        template <class t_>
        t_ get_value(QStringView _name, const t_& _defaultValue) const
        {
            assert(false);
            return _defaultValue;
        }

        template <class t_>
        t_ get_value(const char* _name, const t_& _defaultValue) const
        {
            assert(false);
            return _defaultValue;
        }

        template <class t_>
        t_ get_value(const char* _name, const t_& _defaultValue, const QString& _prefix) const
        {
            return get_value(_prefix + QString::fromLatin1(_name), _defaultValue);
        }

        template <class t_>
        bool contains_value(const t_& _name) const
        {
            return values_.find(_name) != values_.end();
        }

    private:

        int shadowWidth_;
        bool isLoaded_;
    public:

        qt_gui_settings();

        void unserialize(core::coll_helper _collection);
        void clear();

        void set_shadow_width(int _width);
        int get_shadow_width() const;
        int get_current_shadow_width() const;

        bool getIsLoaded() const { return isLoaded_; };
        void setIsLoaded(bool _isLoaded) { isLoaded_ = _isLoaded; };
    };

    template<> void qt_gui_settings::set_value<QString>(const QString& _name, const QString& _value);
    template<> QString qt_gui_settings::get_value<QString>(QStringView _name, const QString& _defaultValue) const;
    template<> QString qt_gui_settings::get_value<QString>(const char* _name, const QString& _defaultValue) const;
    template<> void qt_gui_settings::set_value<std::string>(const QString& _name, const std::string& _value);
    template<> std::string qt_gui_settings::get_value<std::string>(const char* _name, const std::string& _defaultValue) const;
    template<> void qt_gui_settings::set_value<int>(const QString& _name, const int& _value);
    template <> int qt_gui_settings::get_value<int>(QStringView _name, const int& _defaultValue) const;
    template <> int qt_gui_settings::get_value<int>(const char* _name, const int& _defaultValue) const;
    template<> void qt_gui_settings::set_value<double>(const QString& _name, const double& _value);
    template <> double qt_gui_settings::get_value<double>(QStringView _name, const double& _defaultValue) const;
    template <> double qt_gui_settings::get_value<double>(const char* _name, const double& _defaultValue) const;
    template<> void qt_gui_settings::set_value<bool>(const QString& _name, const bool& _value);
    template <> bool qt_gui_settings::get_value<bool>(QStringView _name, const bool& _defaultValue) const;
    template <> bool qt_gui_settings::get_value<bool>(const char* _name, const bool& _defaultValue) const;
    template<> void qt_gui_settings::set_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _value);
    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(QStringView _name, const std::vector<int32_t>& _defaultValue) const;
    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(const char* _name, const std::vector<int32_t>& _defaultValue) const;
    template<> void qt_gui_settings::set_value<QRect>(const QString& _name, const QRect& _value);
    template<> QRect qt_gui_settings::get_value<QRect>(QStringView _name, const QRect& _defaultValue) const;
    template<> QRect qt_gui_settings::get_value<QRect>(const char* _name, const QRect& _defaultValue) const;
    template<> void qt_gui_settings::set_value<qt_gui_settings::VoipCallsCountMap>(const QString& _name, const qt_gui_settings::VoipCallsCountMap& _value);
    template<> qt_gui_settings::VoipCallsCountMap qt_gui_settings::get_value<qt_gui_settings::VoipCallsCountMap>(QStringView _name, const qt_gui_settings::VoipCallsCountMap& _default) const;
    template<> std::map<int64_t, int64_t> qt_gui_settings::get_value<std::map<int64_t, int64_t>>(QStringView _name, const std::map<int64_t, int64_t>& _def) const;
    template<> void qt_gui_settings::set_value<std::map<int64_t, int64_t>>(const QString& _name, const std::map<int64_t, int64_t>& _value);

    qt_gui_settings* get_gui_settings();

    std::string get_account_setting_name(const std::string& settingName);

    QString getDownloadPath();
    void setDownloadPath(const QString& _path);
    void convertOldDownloadPath();

    void checkDownloadPath();

    QString getSaveAsPath();
    void setSaveAsPath(const QString& _path);
}

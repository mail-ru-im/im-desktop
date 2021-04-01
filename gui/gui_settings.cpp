#include "stdafx.h"
#include <cstring>
#include "gui_settings.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "my_info.h"
#include "../common.shared/config/config.h"

namespace
{
    QString DefaultDownloadsPath()
    {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    }
}

namespace Ui
{
    qt_gui_settings::qt_gui_settings()
        : shadowWidth_(0)
        , isLoaded_(false)
    {

    }

    template<> void qt_gui_settings::set_value<QString>(const QString& _name, const QString& _value)
    {
        const QByteArray arr = _value.toUtf8();
        set_value_simple_data(_name, arr.data(), arr.size() + 1);
    }

    template<> QString qt_gui_settings::get_value<QString>(QStringView _name, const QString& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        return QString::fromUtf8(&data[0]);
    }

    template<> void qt_gui_settings::set_value<qt_gui_settings::VoipCallsCountMap>(const QString& _name, const qt_gui_settings::VoipCallsCountMap& _value)
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);

        for (auto [k, v] : _value)
        {
            ds << k.toStdString().c_str();
            ds << v;
        }

        set_value_simple_data(_name, arr.constData(), arr.size());
    }

    template<> void qt_gui_settings::set_value<std::map<int64_t, int64_t>>(const QString& _name, const std::map<int64_t, int64_t>& _value)
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);

        for (auto [k, v] : _value)
            ds << static_cast<qint64>(k) << static_cast<qint64>(v);

        set_value_simple_data(_name, arr.constData(), arr.size());
    }

    template<> qt_gui_settings::VoipCallsCountMap qt_gui_settings::get_value<qt_gui_settings::VoipCallsCountMap>(QStringView _name, const qt_gui_settings::VoipCallsCountMap& _default) const
    {
        qt_gui_settings::VoipCallsCountMap result;

        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _default;

        QByteArray arr(&data[0], data.size());
        QDataStream ds(arr);

        while (!ds.atEnd())
        {
            char* str; VoipCallsCountMap::mapped_type v;
            ds >> str;

            if (!str || !std::strlen(str))
            {
                im_assert(!"broken VoipCallsCountMap settings");
                break;
            }

            ds >> v;

            result.emplace(QString::fromStdString(std::string(str)), v);
            delete str;
        }

        return result;
    }

    template<> std::map<int64_t, int64_t> Ui::qt_gui_settings::get_value<std::map<int64_t, int64_t>>(QStringView _name, const std::map<int64_t, int64_t>& _def) const
    {
        std::map<int64_t, int64_t> result;

        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _def;

        QByteArray arr(&data[0], data.size());
        QDataStream ds(arr);

        while (!ds.atEnd())
        {
            qint64 k, v;
            ds >> k >> v;

            if (ds.status() != QDataStream::Ok)
            {
                im_assert("int - int map read fail in qt_gui_settings");
                return _def;
            }

            result.emplace(k, v);
        }

        return result;
    }

    template<> QString qt_gui_settings::get_value<QString>(const char* _name, const QString& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        return QString::fromUtf8(&data[0]);
    }

    template<> void qt_gui_settings::set_value<std::string>(const QString& _name, const std::string& _value)
    {
        set_value_simple_data(_name, _value.c_str(), _value.size());
    }

    template<> std::string qt_gui_settings::get_value<std::string>(const char* _name, const std::string& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;
        return std::string(data.begin(), data.end());
    }

    template<> void qt_gui_settings::set_value<int>(const QString& _name, const int& _value)
    {
        set_value_simple(_name, _value);
    }

    template<> void qt_gui_settings::set_value<int64_t>(const QString& _name, const int64_t& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> int qt_gui_settings::get_value<int>(QStringView _name, const int& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template <> int qt_gui_settings::get_value<int>(const char* _name, const int& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template <> int64_t qt_gui_settings::get_value<int64_t>(QStringView _name, const int64_t& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template <> int64_t qt_gui_settings::get_value<int64_t>(const char* _name, const int64_t& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<double>(const QString& _name, const double& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> double qt_gui_settings::get_value<double>(QStringView _name, const double& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template <> double qt_gui_settings::get_value<double>(const char* _name, const double& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<bool>(const QString& _name, const bool& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> bool qt_gui_settings::get_value<bool>(QStringView _name, const bool& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template <> bool qt_gui_settings::get_value<bool>(const char* _name, const bool& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _value)
    {
        if (_value.empty())
        {
            set_value_simple_data(_name, 0, 0);

            return;
        }

        set_value_simple_data(_name, (const char*)&_value[0], (int)_value.size() * sizeof(int32_t));
    }

    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(QStringView _name, const std::vector<int32_t>& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.empty())
            return _defaultValue;

        if ((data.size() % sizeof(int32_t)) != 0)
        {
            im_assert(false);
            return _defaultValue;
        }

        std::vector<int32_t> out_data(data.size()/sizeof(int32_t));
        ::memcpy(&out_data[0], &data[0], data.size());

        return out_data;
    }

    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(const char* _name, const std::vector<int32_t>& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.empty())
            return _defaultValue;

        if ((data.size() % sizeof(int32_t)) != 0)
        {
            im_assert(false);
            return _defaultValue;
        }

        std::vector<int32_t> out_data(data.size() / sizeof(int32_t));
        ::memcpy(&out_data[0], &data[0], data.size());

        return out_data;
    }

    template<> void qt_gui_settings::set_value<QRect>(const QString& _name, const QRect& _value)
    {
        int32_t buffer[4] = {_value.left(), _value.top(), _value.width(), _value.height()};

        set_value_simple_data(_name, (const char*) buffer, sizeof(buffer));
    }

    template<> QRect qt_gui_settings::get_value<QRect>(QStringView _name, const QRect& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.size() != sizeof(int32_t[4]))
        {
            im_assert(false);
            return _defaultValue;
        }

        int32_t* buffer = (int32_t*) &data[0];

        return QRect(buffer[0], buffer[1], buffer[2], buffer[3]);
    }

    template<> QRect qt_gui_settings::get_value<QRect>(const char* _name, const QRect& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.size() != sizeof(int32_t[4]))
        {
            im_assert(false);
            return _defaultValue;
        }

        int32_t* buffer = (int32_t*)&data[0];

        return QRect(buffer[0], buffer[1], buffer[2], buffer[3]);
    }


    void qt_gui_settings::set_shadow_width(int _width)
    {
        shadowWidth_ = _width;
    }

    int qt_gui_settings::get_shadow_width() const
    {
        return shadowWidth_;
    }

    int qt_gui_settings::get_current_shadow_width() const
    {
        return (get_value<bool>(settings_window_maximized, false) == true ? 0 : shadowWidth_);
    }

    void qt_gui_settings::unserialize(core::coll_helper _collection)
    {
        core::iarray* values_array = _collection.get_value_as_array("values");
        if (!values_array)
        {
            im_assert(false);
            return;
        }

        for (int i = 0; i < values_array->size(); ++i)
        {
            const core::ivalue* val = values_array->get_at(i);

            gui_coll_helper coll_val(val->get_as_collection(), false);

            core::istream* idata = coll_val.get_value_as_stream("value");

            int len = idata->size();

            set_value_simple_data(QString::fromUtf8(coll_val.get_value_as_string("name")), (const char*) idata->read(len), len, false);
        }

        Q_EMIT received();
        setIsLoaded(true);
    }

    void qt_gui_settings::clear()
    {
        values_.clear();
        setIsLoaded(false);
    }

    void qt_gui_settings::post_value_to_core(const QString& _name, const settings_value& _val) const
    {
        Ui::gui_coll_helper cl_coll(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(cl_coll->create_stream());

        if (_val.data_.size())
            data_stream->write((const uint8_t*) &_val.data_[0], (uint32_t)_val.data_.size());

        cl_coll.set_value_as_qstring("name", _name);
        cl_coll.set_value_as_stream("value", data_stream.get());

        GetDispatcher()->post_message_to_core("settings/value/set", cl_coll.get());
    }

    qt_gui_settings* get_gui_settings()
    {
        static auto settings = std::make_unique<qt_gui_settings>();
        return settings.get();
    }

    std::string get_account_setting_name(const std::string& _settingName)
    {
        return MyInfo()->aimId().toStdString() + '/' + _settingName;
    }

    QString getDownloadPath()
    {
        checkDownloadPath();

        QString result = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory, QString()));
        return QFileInfo(result.isEmpty() ? DefaultDownloadsPath() : result).canonicalFilePath();// we need canonical path to follow symlinks
    }

    void setDownloadPath(const QString& _path)
    {
        Ui::get_gui_settings()->set_value(settings_download_directory, _path);
    }

    void convertOldDownloadPath()
    {
        const auto& settings = Ui::get_gui_settings();
        if (!settings->contains_value(settings_download_directory_old))
            return;

        auto value = settings->get_value(settings_download_directory_old, QString());
        if (value.isEmpty())
            return;

        auto oldInfo = QFileInfo(QDir::toNativeSeparators(value));
        const auto product = config::get().string(config::values::product_name);
        auto possibleOldInfo = QFileInfo(QDir::toNativeSeparators(DefaultDownloadsPath() % ql1c('/') % QString::fromUtf8(product.data(), product.size())));
        auto newInfo = QFileInfo(DefaultDownloadsPath());
        if (oldInfo == newInfo || oldInfo == possibleOldInfo)
        {
            settings->set_value(settings_download_directory_old, QString());
            return;
        }

        settings->set_value(settings_download_directory, value);
    }

    void checkDownloadPath()
    {
        {
            auto downloadPath = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory, QString()));
            if (!downloadPath.isEmpty() && !QFileInfo::exists(downloadPath))
            {
                Ui::get_gui_settings()->set_value(settings_download_directory, QString());
                Q_EMIT Utils::InterConnector::instance().downloadPathUpdated();
            }
        }

        {
            auto downloadPath = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory_save_as, QString()));
            if (!downloadPath.isEmpty() && !QFileInfo::exists(downloadPath))
                Ui::get_gui_settings()->set_value(settings_download_directory_save_as, QString());
        }
    }

    QString getSaveAsPath()
    {
        QString result = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory_save_as, QString()));
        return (result.isEmpty() ? getDownloadPath() : result);
    }

    void setSaveAsPath(const QString& _path)
    {
        Ui::get_gui_settings()->set_value(settings_download_directory_save_as, _path);
    }
}

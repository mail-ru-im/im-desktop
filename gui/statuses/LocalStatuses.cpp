#include "stdafx.h"

#include "utils/JsonUtils.h"
#include "../utils/features.h"
#include "LocalStatuses.h"
#include "gui_settings.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "rapidjson/writer.h"

namespace
{
    inline QString getDefaultJSONPath()
    {
        return (Ui::get_gui_settings()->get_value(settings_language, QString()) == u"ru") ? qsl(":/statuses/default_statuses") : qsl(":/statuses/default_statuses_eng");
    }

    std::vector<Statuses::LocalStatus> defaultStatuses()
    {
        std::vector<Statuses::LocalStatus> statuses;

        rapidjson::Document doc;
        doc.Parse(Features::getStatusJson());

        if (doc.HasParseError())
        {
            im_assert(QFile::exists(getDefaultJSONPath()));

            QFile file(getDefaultJSONPath());
            if (!file.open(QFile::ReadOnly | QFile::Text))
                return statuses;

            const auto json = file.readAll();

            doc.Parse(json.constData(), json.size());
            if (doc.HasParseError())
            {
                im_assert(false);
                return statuses;
            }
            file.close();
        }

        if (doc.IsArray())
        {
            statuses.reserve(doc.Size());
            for (const auto& item : doc.GetArray())
            {
                Statuses::LocalStatus status;

                JsonUtils::unserialize_value(item, "status", status.status_);
                JsonUtils::unserialize_value(item, "text", status.description_);
                JsonUtils::unserialize_value(item, "duration", status.duration_);

                statuses.push_back(std::move(status));
            }
        }

        return statuses;
    }
}

namespace Statuses
{

using namespace Ui;

class LocalStatuses_p
{
public:

    void updateIndex()
    {
        index_.clear();
        for (auto i = 0u; i < statuses_.size(); i++)
            index_[statuses_[i].status_] = i;
    }

    void overrideStatusDurationsFromSettings()
    {
        for (auto& status : statuses_)
        {
            if (get_gui_settings()->contains_value(status_duration, status.status_))
                status.duration_ = std::chrono::seconds(get_gui_settings()->get_value<int64_t>(status_duration, 0, status.status_));
        }
    }

    void load()
    {
        statuses_ = defaultStatuses();
        migrateOldSettingsData();
        overrideStatusDurationsFromSettings();
        updateIndex();
    }

    void resetToDefaults()
    {
        statuses_ = defaultStatuses();
        updateIndex();
    }

    void migrateOldSettingsData()
    {
        const auto statusesStr = get_gui_settings()->get_value<QString>(statuses_user_statuses, QString());
        const auto statuses = statusesStr.splitRef(ql1c(';'), QString::SkipEmptyParts);

        for (const auto& s : boost::adaptors::reverse(statuses))
        {
            const auto& stat = s.split(ql1c(':'), QString::SkipEmptyParts);
            if (!stat.isEmpty() && stat.size() == 2)
                get_gui_settings()->set_value<int64_t>(status_duration, stat[1].toLongLong(), stat[0].toString());
        }

        get_gui_settings()->set_value(statuses_user_statuses, QString());
    }

    std::vector<LocalStatus> statuses_;
    std::unordered_map<QString, size_t, Utils::QStringHasher> index_;
};

LocalStatuses::LocalStatuses()
    : d(std::make_unique<LocalStatuses_p>())
{
    d->load();

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &LocalStatuses::onOmicronUpdated);
}

LocalStatuses::~LocalStatuses() = default;

LocalStatuses* LocalStatuses::instance()
{
    static LocalStatuses localStatuses;
    return &localStatuses;
}

const LocalStatus& LocalStatuses::getStatus(const QString& _statusCode) const
{

    auto it = d->index_.find(_statusCode);
    if (it != d->index_.end())
        return d->statuses_[it->second];

    static LocalStatus empty;
    return empty;
}

void LocalStatuses::setDuration(const QString& _statusCode, std::chrono::seconds _duration)
{
    auto it = d->index_.find(_statusCode);
    if (it != d->index_.end())
    {
        d->statuses_[it->second].duration_ = _duration;
        get_gui_settings()->set_value<int64_t>(status_duration, _duration.count(), _statusCode);
    }
}

std::chrono::seconds LocalStatuses::statusDuration(const QString& _statusCode) const
{
    auto it = d->index_.find(_statusCode);
    if (it != d->index_.end())
        return d->statuses_[it->second].duration_;

    return std::chrono::seconds::zero();
}

const QString& LocalStatuses::statusDescription(const QString& _statusCode) const
{
    auto it = d->index_.find(_statusCode);
    if (it != d->index_.end())
        return d->statuses_[it->second].description_;

    static QString empty;
    return empty;
}

const std::vector<LocalStatus>& LocalStatuses::statuses() const
{
    return d->statuses_;
}

void LocalStatuses::resetToDefaults()
{
    d->resetToDefaults();
}

void LocalStatuses::onOmicronUpdated()
{
    d->load();
}


}

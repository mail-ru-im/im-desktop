#include "stdafx.h"

#include "profiler.h"

#include "../log/log.h"
#include "../tools/coretime.h"

using namespace core;
using namespace tools;

namespace
{
    struct process_info
    {
        process_info(const std::string &_name, const int64_t _time_started);

        int64_t get_duration() const;

        const std::string name_;

        const int64_t time_started_;

        int64_t time_ended_;
    };

    struct process_stat
    {
        process_stat();

        int64_t get_avg_duration() const;

        int64_t min_duration_;

        int64_t max_duration_;

        int64_t overall_duration_;

        int64_t times_hit_;
    };

    void start_process(const char*_name, const int64_t _process_id, const int64_t _ts);

    void stop_process(const int64_t _process_id, const int64_t _ts);

    std::atomic<int64_t> process_uid_(0);

    std::map<int64_t, process_info> process_info_accum_;

    std::mutex process_info_accum_mutex_;

    bool is_profiling_enabled_ = false;
}

namespace core
{
    namespace profiler
    {
        auto_stop_watch::auto_stop_watch(const char *_process_name)
        {
            assert(_process_name);
            assert(::strlen(_process_name));

            id_ = process_started(_process_name);
        }

        auto_stop_watch::~auto_stop_watch()
        {
            process_stopped(id_);
        }

        void enable(const bool _enable)
        {
            is_profiling_enabled_ = _enable;
        }

        int64_t process_started(const char *_name)
        {
            assert(_name);
            assert(::strlen(_name));

            if (!is_profiling_enabled_)
            {
                return -1;
            }

            const auto process_id = ++process_uid_;

            start_process(_name, process_id, time::now_ms());

            return process_id;
        }

        void process_started(const char *_name, const int64_t _process_id, const int64_t _ts)
        {
            assert(_name);
            assert(::strlen(_name));
            assert(_process_id > std::numeric_limits<int32_t>::max());
            assert(_ts > 0);

            if (!is_profiling_enabled_)
            {
                return;
            }

            start_process(_name, _process_id, _ts);
        }

        void process_stopped(const int64_t _process_id)
        {
            assert(_process_id > 0);

            if (!is_profiling_enabled_)
            {
                return;
            }

            stop_process(_process_id, time::now_ms());
        }

        void process_stopped(const int64_t _process_id, const int64_t _ts)
        {
            assert(_process_id > std::numeric_limits<int32_t>::max());
            assert(_ts > 0);

            if (!is_profiling_enabled_)
            {
                return;
            }

            stop_process(_process_id, _ts);
        }

        void flush_logs()
        {
            if (!is_profiling_enabled_)
            {
                return;
            }

            std::unique_lock<std::mutex> lock(process_info_accum_mutex_);

            std::map<std::string, process_stat> stats;

            for (const auto &pair : process_info_accum_)
            {
                const auto &info = pair.second;

                auto stats_iter = stats.find(info.name_);
                if (stats_iter == stats.end())
                {
                    stats_iter = stats.emplace(info.name_, process_stat()).first;
                }

                auto &stat_entry = stats_iter->second;

                ++stat_entry.times_hit_;
                stat_entry.min_duration_ = std::min(stat_entry.min_duration_, info.get_duration());
                stat_entry.max_duration_ = std::max(stat_entry.max_duration_, info.get_duration());
                stat_entry.overall_duration_ += info.get_duration();
            }

            process_info_accum_.clear();

            lock.unlock();

            typedef std::tuple<std::string, process_stat> process_stat_info;

            std::vector<process_stat_info> sorted_stats(
                std::make_move_iterator(stats.begin()),
                std::make_move_iterator(stats.end())
                );

            stats.clear();

            std::sort(
                sorted_stats.begin(),
                sorted_stats.end(),
                [](const process_stat_info &l, const process_stat_info &r)
            {
                return (
                    std::get<1>(l).overall_duration_ > std::get<1>(r).overall_duration_
                    );
            }
            );

            for (const auto &pair : sorted_stats)
            {
                const auto &process_name = std::get<0>(pair);
                const auto &stat_entry = std::get<1>(pair);

                boost::format process_info_fmt(
                    "process stats\n"
                    "    name = <%s>\n"
                    "    times-hit = <%d>\n"
                    "    min-duration=<%d>\n"
                    "    max-duration=<%d>\n"
                    "    avg-duration=<%d>\n"
                    "    overall-duration=<%d>\n");

                process_info_fmt
                    % process_name
                    % stat_entry.times_hit_
                    % stat_entry.min_duration_
                    % stat_entry.max_duration_
                    % stat_entry.get_avg_duration()
                    % stat_entry.overall_duration_;

                log::info("profiler", process_info_fmt);
            }
        }

    }
}

namespace
{

    void start_process(const char *_name, const int64_t _process_id, const int64_t _ts)
    {
        assert(_name);
        assert(::strlen(_name));
        assert(_process_id > 0);
        assert(_ts > 0);

        std::lock_guard<std::mutex> lock(process_info_accum_mutex_);

        const auto insertion_result = process_info_accum_.emplace(_process_id, process_info(_name, _ts));
        assert(insertion_result.second);
    }

    void stop_process(const int64_t _process_id, const int64_t _ts)
    {
        assert(_process_id > 0);
        assert(_ts > 0);

        std::lock_guard<std::mutex> lock(process_info_accum_mutex_);

        auto iter = process_info_accum_.find(_process_id);
        assert(iter != process_info_accum_.end());

        iter->second.time_ended_ = _ts;
    }

    process_info::process_info(const std::string &_name, const int64_t _time_started)
        : name_(_name)
        , time_started_(_time_started)
        , time_ended_(-1)
    {
        assert(!_name.empty());
        assert(_time_started > 0);
    }

    int64_t process_info::get_duration() const
    {
        assert(time_ended_ > 0);
        return (time_ended_ - time_started_);
    }

    process_stat::process_stat()
        : min_duration_(INT64_MAX)
        , max_duration_(INT64_MIN)
        , overall_duration_(0)
        , times_hit_(0)
    {
    }

    int64_t process_stat::get_avg_duration() const
    {
        assert(times_hit_ > 0);

        return (overall_duration_ / times_hit_);
    }

}
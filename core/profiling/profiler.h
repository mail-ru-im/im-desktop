#pragma once

namespace core
{

    namespace profiler
    {

        class auto_stop_watch : boost::noncopyable
        {
        public:
            auto_stop_watch(const char *_process_name);

            virtual ~auto_stop_watch();

        private:
            int64_t id_;

        };

        void enable(const bool _enable);

        int64_t process_started(const char *_name);

        void process_stopped(const int64_t _process_id);

        void process_started(const char *_name, const int64_t _process_id, const int64_t _ts);

        void process_stopped(const int64_t _process_id, const int64_t _ts);

        void flush_logs();

    }

}
#pragma once

namespace Profiling
{

    class auto_stop_watch
    {
    public:
        auto_stop_watch(const char *_process_name);

        ~auto_stop_watch();

    private:
        qint64 id_;

        std::string name_;

    };

}
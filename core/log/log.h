#pragma once

#if defined(DEBUG)
#define __ENABLE_LOG
#endif

#define DECLARE_OVERLOADS(x)											\
    void x(const std::string &_area, const std::string &_str);			\
    void x(const std::string &_area, const boost::format &_format);

#ifdef __ENABLE_LOG
#define __LOG(x) { x }
#define __WRITE_LOG(type, area, fncname, fmt, params)								\
{																					\
    boost::format format(fncname ", " __FILE__ ", line " __LINEA__"\n" fmt);		\
    format % params;																\
    core::log::type((area), format);												\
}
#else
#define __LOG(x) {}
#define __WRITE_LOG(type, area, fncname, fmt, params) {}
#endif

#define __TRACE(area, fmt, params) __WRITE_LOG(trace, (area), __FUNCTION__, fmt, params)
#define __INFO(area, fmt, params) __WRITE_LOG(info, (area), __FUNCTION__, fmt, params)
#define __WARN(area, fmt, params) __WRITE_LOG(warn, (area), __FUNCTION__, fmt, params)
#define __ERR(area, fmt, params) __WRITE_LOG(err, (area), __FUNCTION__, fmt, params)

#define __NET(fmt, params)                  \
{											\
    boost::format format(fmt);	            \
    format % params;						\
    core::log::net(format);		            \
}

namespace core
{
    namespace log
    {
        void enable_trace_data(const bool _is_enabled);

        void init(const boost::filesystem::wpath &_logs_dir, const bool _is_html);

        DECLARE_OVERLOADS(trace);
        DECLARE_OVERLOADS(info);
        DECLARE_OVERLOADS(warn);
        DECLARE_OVERLOADS(error);

        void net(const boost::format &_format);
        boost::filesystem::wpath get_net_file_path();

        void shutdown();

    }
}

#undef DECLARE_OVERLOADS
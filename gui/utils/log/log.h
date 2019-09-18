#pragma once

#ifdef _DEBUG
#define __ENABLE_LOG
#endif //_DEBUG

#ifdef __ENABLE_LOG
    #define __LOG(x) { x }
    #define __WRITE_LOG(type, area, params)\
    {\
        QString fmt;\
        fmt.reserve(512);\
\
        QTextStream out(&fmt);\
        out << __FUNCTION__ << ", " __FILE__ ", line " __LINEA__ "\n"\
            << params;\
\
        Log::type((area), fmt);\
    }
#else
    #define __LOG(x) {}
    #define __WRITE_LOG(type, area, params) {}
#endif

#define __TRACE(area, params) __WRITE_LOG(trace, (area), params)
#define __INFO(area, params) __WRITE_LOG(info, (area), params)
#define __WARN(area, params) __WRITE_LOG(warn, (area), params)
#define __ERROR(area, params) __WRITE_LOG(error, (area), params)

#define __LOGP(id, value) "    " #id "=<" << (value) << ">\n"

namespace Log
{

    void trace(const QString& area, const QString& text);
    inline void trace(const char* area, const QString& text)
    {
        trace(ql1s(area), text);
    }

    void info(const QString& area, const QString& text);
    inline void info(const char* area, const QString& text)
    {
        info(ql1s(area), text);
    }

    void warn(const QString& area, const QString& text);
    inline void warn(const char* area, const QString& text)
    {
        warn(ql1s(area), text);
    }

    void error(const QString& area, const QString& text);
    inline void error(const char* area, const QString& text)
    {
        error(ql1s(area), text);
    }

    void write_network_log(const std::string& _text);
}

namespace core
{
    enum class file_sharing_function;
    struct file_sharing_content_type;
}

inline QTextStream& operator <<(QTextStream &lhs, const QUrl &uri)
{
    return (lhs << uri.toString());
}

inline QTextStream& operator <<(QTextStream &lhs, const QSize &size)
{
    return (lhs << size.width() << "," << size.height());
}

inline QTextStream& operator <<(QTextStream &lhs, const QSizeF &size)
{
    return (lhs << size.width() << "," << size.height());
}

inline QTextStream& operator <<(QTextStream &lhs, const QRect &rect)
{
    return (lhs << rect.x() << "," << rect.y() << "," << rect.width() << "," << rect.height());
}

inline QTextStream& operator <<(QTextStream &lhs, const QPoint &p)
{
    return (lhs << p.x() << "," << p.y());
}

template<class T>
inline QTextStream& operator <<(QTextStream &lhs, const std::pair<T, T> &pair)
{
    return (lhs << pair.first << "," << pair.second);
}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_content_type& arg);

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_function arg);
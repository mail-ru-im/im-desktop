#include "stdafx.h"

#include "log.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

using namespace Ui;

namespace
{
    void log(QStringView type, QStringView area, QStringView text);
    inline void log(const QString& type, const char* area, const QString& text)
    {
        log(type, QString(ql1s(area)), text);
    }
}

namespace Log
{
    void trace(QStringView area, QStringView text)
    {
        log(u"trace", area, text);
    }

    void info(QStringView area, QStringView text)
    {
        log(u"info", area, text);
    }

    void warn(QStringView area, QStringView text)
    {
        log(u"warn", area, text);
    }

    void error(QStringView area, QStringView text)
    {
        log(u"error", area, text);
    }

    void write_network_log(const std::string& _text)
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("text", _text);

        Ui::GetDispatcher()->post_message_to_core("_network_log", collection.get());
    }
}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_content_type& arg)
{
    using namespace core;

    switch (arg.type_)
    {
        case file_sharing_base_content_type::gif: return (oss << ql1s("type = gif"));
        case file_sharing_base_content_type::image: return (oss << ql1s("type = image"));
        case file_sharing_base_content_type::ptt: return (oss << ql1s("type = ptt"));
        case file_sharing_base_content_type::undefined: return (oss << ql1s("type = undefined"));
        case file_sharing_base_content_type::video: return (oss << ql1s("type = video"));
        case file_sharing_base_content_type::lottie: return (oss << ql1s("type = lottie"));
        default:
            assert(!"unexpected file sharing content type");
    }

    switch (arg.subtype_)
    {
        case file_sharing_sub_content_type::sticker: return (oss << ql1s("subtype = sticker"));
        case file_sharing_sub_content_type::undefined: return (oss << ql1s("subtype = undefined"));
        default:
            assert(!"unexpected file sharing content subtype");
    }

    return (oss << ql1s("#unknown"));
}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_function arg)
{
    using namespace core;

    assert(arg > file_sharing_function::min);
    assert(arg < file_sharing_function::max);

    switch(arg)
    {
        case file_sharing_function::check_local_copy_exists:
            oss << ql1s("check_local_copy_exists");
            break;

        case file_sharing_function::download_file:
            oss << ql1s("download_file");
            break;

        case file_sharing_function::download_file_metainfo:
            oss << ql1s("download_file_metainfo");
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << ql1s("download_preview_metainfo");
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
    }

    return oss;
}

namespace
{
    void log(QStringView type, QStringView area, QStringView text)
    {
        assert(!type.isEmpty());
        assert(!area.isEmpty());
        assert(!text.isEmpty());

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("type", type);
        collection.set_value_as_qstring("area", area);
        collection.set_value_as_qstring("text", text);

        Ui::GetDispatcher()->post_message_to_core("_log", collection.get());
    }
}
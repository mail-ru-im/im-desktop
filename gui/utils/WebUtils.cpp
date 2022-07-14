#include "WebUtils.h"

#include "core_dispatcher.h"
#include "../utils/log/log.h"


void WebUtils::writeLog(const QString& _url, const QString& _message)
{
    auto dispatcher = Ui::GetDispatcher();
    core::coll_helper helper(dispatcher->create_collection(), true);
    helper.set<QString>("source", _url);
    helper.set<QString>("message", _message);
    dispatcher->post_message_to_core("web_log", helper.get());

    QString msg = qsl("source: ") % _url % QChar::LineFeed % qsl("message: ") % _message;
    Log::trace(qsl("web_page"), std::move(msg));
}


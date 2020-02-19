#include "stdafx.h"
#include "InternalServiceHandler.h"

#include "InternalServiceRequest.h"
#include "core_dispatcher.h"
#include "../gui_coll_helper.h"
#include "../InterConnector.h"
#include "main_window/ContactDialog.h"
#include "main_window/MainWindow.h"
#include "../common.shared/config/config.h"

#include "../../../common.shared/version_info_constants.h"
#include "../../../common.shared/config/config.h"
#include "../../memory_stats/gui_memory_monitor.h"

namespace Utils
{

InternalServiceHandler::InternalServiceHandler(InternalServiceRequest *_request)
    : request_(_request)
{

}

InternalServiceHandler::~InternalServiceHandler() = default;

void InternalServiceHandler::start()
{
    const auto type = request_->getCommandType();
    switch (type)
    {
    case InternalServiceRequest::CommandType::ClearCache:
        Utils::clearContentCache();
        break;
    case InternalServiceRequest::CommandType::ClearAvatars:
        Utils::clearAvatarsCache();
        break;
    case InternalServiceRequest::CommandType::OpenDebug:
        emit Utils::InterConnector::instance().showDebugSettings();
        break;
    case InternalServiceRequest::CommandType::CheckForUpdate:
        Utils::checkForUpdates();
        break;
    default:
        break;
    }
}


void InternalServiceHandler::stop()
{
}
}

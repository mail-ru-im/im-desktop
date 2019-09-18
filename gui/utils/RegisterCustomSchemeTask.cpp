#include "RegisterCustomSchemeTask.h"
#include "utils.h"

using namespace Utils;

RegisterCustomSchemeTask::RegisterCustomSchemeTask()
{

}

void RegisterCustomSchemeTask::run()
{
    if (platform::is_linux())
        Utils::registerCustomScheme();

    emit finished();
}

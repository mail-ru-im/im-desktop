#include "stdafx.h"
#include "start_process.h"
#include "../tools.h"

namespace installer
{
    namespace logic
    {
        installer::error start_process()
        {
            const QString icq_exe = QString("\"") % get_icq_exe() % QString("\"");
            const QString systemRoot = QProcessEnvironment::systemEnvironment().value(qsl("SystemRoot"), qsl("C:\WINDOWS"));

            if (!QProcess::startDetached(icq_exe, {}, systemRoot))
                installer::error(errorcode::start_exe, QString("start process: ") + icq_exe);

            return installer::error();
        }
    }
}

#include "stdafx.h"
#include "start_process.h"
#include "../tools.h"

namespace installer
{
    namespace logic
    {
        installer::error start_process()
        {
            const QString icq_exe = ql1c('"') % get_icq_exe() % ql1c('"');
            const QString systemRoot = QProcessEnvironment::systemEnvironment().value(qsl("SystemRoot"), qsl("C:\\WINDOWS"));

            if (!QProcess::startDetached(icq_exe, {}, systemRoot))
                return installer::error(errorcode::start_exe, ql1s("start process: ") % icq_exe);

            return installer::error();
        }
    }
}

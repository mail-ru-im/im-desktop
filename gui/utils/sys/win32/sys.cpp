#include "stdafx.h"

#include "../sys.h"

namespace System
{
    void launchApplication(const QString& _path, const QStringList& _arguments)
    {
        QProcess::startDetached(ql1c('\"') % _path % ql1c('\"'), _arguments);
    }
}

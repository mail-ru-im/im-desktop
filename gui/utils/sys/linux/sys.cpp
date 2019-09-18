#include "stdafx.h"

#include "../sys.h"

namespace System
{
    void launchApplication(const QString& _path)
    {
        QProcess::startDetached(ql1c('\"') % _path % ql1c('\"'));
    }
}

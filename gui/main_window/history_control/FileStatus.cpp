#include "stdafx.h"
#include "FileStatus.h"

#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "previewer/toast.h"

namespace Ui
{
    QSize getFileStatusIconSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QSize getFileStatusIconSizeWithCircle() noexcept
    {
        return Utils::scale_value(QSize(28, 28));
    }

    const QPixmap& getFileStatusIcon(FileStatus _status, Styling::StyleVariable _color)
    {
        using IconColors = std::unordered_map<Styling::StyleVariable, QPixmap>;
        using IconCache = std::unordered_map<FileStatus, IconColors>;

        static IconCache cache;

        const auto path = getFileStatusIconPath(_status);
        if (path.empty())
        {
            im_assert(!"invalid file status path");
            static const QPixmap empty;
            return empty;
        }

        auto& pm = cache[_status][_color];
        if (pm.isNull())
            pm = Utils::renderSvg(path.toString(), getFileStatusIconSize(), Styling::getParameters().getColor(_color));
        return pm;
    }

    QString getFileStatusText(FileStatus _status)
    {
        switch (_status)
        {
        case FileStatus::NoStatus:
            return {};
        case FileStatus::Blocked:
            return QT_TRANSLATE_NOOP("toast", "Administrator has set the restrictions on access to files");
        case FileStatus::AntivirusPending:
            return QT_TRANSLATE_NOOP("toast", "Awaiting antivirus check, patience");
        case FileStatus::AntivirusInProgress:
            return QT_TRANSLATE_NOOP("toast", "Antivirus check in progress, patience");
        case FileStatus::AntivirusInfected:
            return QT_TRANSLATE_NOOP("toast", "File is blocked by the antivirus");
        case FileStatus::AntivirusOk:
            return QT_TRANSLATE_NOOP("toast", "Checked");
        case FileStatus::AntivirusOkButBlocked:
            return QT_TRANSLATE_NOOP("toast", "No access. You may not have enough rights");
        case FileStatus::AntivirusNoCheck:
            return QT_TRANSLATE_NOOP("toast", "File not checked (and never will be). Available to download");
        default:
            break;
        }

        im_assert(false);
        return {};
    }

    void showFileStatusToast(FileStatus _status)
    {
        if (_status == FileStatus::NoStatus)
            return;

        if (const auto text = getFileStatusText(_status); !text.isEmpty())
            Utils::showToastOverMainWindow(text, Utils::defaultToastVerOffset());
    }
}
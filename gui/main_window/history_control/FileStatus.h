#pragma once

namespace Styling
{
    enum class StyleVariable;
}

namespace Ui
{
    enum class FileStatus
    {
        NoStatus,
        Blocked,
        AntivirusPending,
        AntivirusInProgress,
        AntivirusInfected,
        AntivirusOk,
        AntivirusOkButBlocked,
        AntivirusNoCheck,
    };

    QSize getFileStatusIconSize() noexcept;
    QSize getFileStatusIconSizeWithCircle() noexcept;

    constexpr QStringView getFileStatusIconPath(FileStatus _status) noexcept
    {
        switch (_status)
        {
        case FileStatus::Blocked:
            return u":/filesharing/status_blocked";
        case FileStatus::AntivirusPending:
            return u":/filesharing/status_av_pending";
        case FileStatus::AntivirusInProgress:
            return u":/filesharing/status_av_inprogress";
        case FileStatus::AntivirusInfected:
            return u":/filesharing/status_av_infected";
        case FileStatus::AntivirusOk:
            return u":/filesharing/status_av_ok";
        case FileStatus::AntivirusOkButBlocked:
            return u":/filesharing/status_av_blocked";
        case FileStatus::AntivirusNoCheck:
            return u":/filesharing/status_av_nocheck";
        default:
            break;
        }

        return {};
    }

    const QPixmap& getFileStatusIcon(FileStatus _status, Styling::StyleVariable _color);
    QString getFileStatusText(FileStatus _status);

    void showFileStatusToast(FileStatus _status);
}
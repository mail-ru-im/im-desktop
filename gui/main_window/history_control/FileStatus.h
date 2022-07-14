#pragma once

#include "../../../common.shared/antivirus/antivirus_types.h"
#include "../../styles/StyleVariable.h"

namespace Ui
{
    enum class FileStatusType
    {
        NoStatus,
        TrustRequired,
        AntivirusInAsyncProgress,
        AntivirusInSyncProgress,
        VirusInfected,
        AntivirusOk,
        AntivirusUnknown
    };

    class FileStatus
    {
        using Type = FileStatusType;

    public:
        FileStatus() = default;
        explicit FileStatus(core::antivirus::check _check, bool _trustRequired, bool _accountAntivirus);
        Type type() const noexcept { return type_; }
        void updateData(core::antivirus::check _check, bool _trustRequired, bool _accountAntivirus);
        void setTrustRequired(bool _trustRequired);
        void setAntivirusCheckResult(core::antivirus::check _check);
        void setAntivirusCheckResult(core::antivirus::check::result  _result);
        void setAccountAntivirus(bool _accountAntivirus);

        bool isAntivirusInProgress() const noexcept;
        bool isAntivirusChecked() const noexcept;
        bool isTrustRequired() const noexcept;

        bool isVirusInfected() const noexcept;

        bool isNoStatusType() const noexcept;
        bool isTrustRequiredType() const noexcept;
        bool isVirusInfectedType() const noexcept;
        bool isAntivirusInProgressType() const noexcept;
        bool isAntivirusCheckedType() const noexcept;
        bool isAntivirusAllowedType() const noexcept;

    private:
        void updateType();

    private:
        core::antivirus::check antivirusCheck_;
        Type type_ = Type::NoStatus;
        bool trustRequired_ = false;
        bool accountAntivirus_ = false;
    };

    QSize getFileStatusIconSize() noexcept;
    QSize getFileStatusIconSizeWithCircle() noexcept;


    constexpr QStringView getFileStatusIconPath(FileStatusType _type) noexcept
    {
        switch (_type)
        {
        case FileStatusType::TrustRequired:
            return u":/filesharing/status_blocked";
        case FileStatusType::AntivirusInSyncProgress:
        case FileStatusType::AntivirusInAsyncProgress:
            return u":/filesharing/status_av_inprogress";
        case FileStatusType::VirusInfected:
            return u":/filesharing/status_av_infected";
        case FileStatusType::AntivirusOk:
            return u":/filesharing/status_av_ok";
        case FileStatusType::AntivirusUnknown:
            return u":/filesharing/status_av_nocheck";
        default:
            break;
        }

        return {};
    }

    const QPixmap& getFileStatusIcon(FileStatusType _type, Styling::StyleVariable _color);
    QString getFileStatusText(FileStatusType _type);

    void showFileStatusToast(FileStatusType _type);

    bool needShowStatus(const FileStatus& _status);
    bool needShowAntivirusStatus(const FileStatus& _status);
} // namespace Ui

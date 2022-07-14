#include "stdafx.h"
#include "FileStatus.h"

#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "previewer/toast.h"
#include "../../utils/features.h"

namespace
{
    Ui::FileStatusType evaluateFileStatus(core::antivirus::check _check, bool _trustRequired, bool _accountAntivirus)
    {
        using Type = Ui::FileStatusType;
        if (!_accountAntivirus)
            return _trustRequired ? Type::TrustRequired : Type::NoStatus;

        using result = core::antivirus::check::result;
        const bool isSyncCheck = core::antivirus::check::mode::sync == _check.mode_;
        switch (_check.result_)
        {
        case result::safe:
            return _trustRequired ? Type::TrustRequired : Type::AntivirusOk;
        case result::unsafe:
            return Type::VirusInfected;
        case result::unchecked:
            if (isSyncCheck)
                return Type::AntivirusInSyncProgress;
            return _trustRequired ? Type::TrustRequired : Type::AntivirusInAsyncProgress;
        case result::unknown:
            return _trustRequired ? Type::TrustRequired : Type::AntivirusUnknown;
        }
        im_assert(!"Wrong file status");
        return Type::NoStatus;
    }
}

namespace Ui
{
    FileStatus::FileStatus(core::antivirus::check _check, bool _trustRequired, bool _accountAntivirus)
        : antivirusCheck_{ _check }
        , type_{ evaluateFileStatus(_check, _trustRequired, _accountAntivirus) }
        , trustRequired_{ _trustRequired }
        , accountAntivirus_{ _accountAntivirus }
    {
    }

    void Ui::FileStatus::updateData(core::antivirus::check _check, bool _trustRequired, bool _accountAntivirus)
    {
        bool updated = std::exchange(trustRequired_, _trustRequired) != _trustRequired;
        updated |= std::exchange(antivirusCheck_, _check) != _check;
        updated |= std::exchange(accountAntivirus_, _accountAntivirus) != _accountAntivirus;
        if (updated)
            updateType();
    }

    void FileStatus::setTrustRequired(bool _trustRequired)
    {
        if (std::exchange(trustRequired_, _trustRequired) != _trustRequired)
            updateType();
    }

    void FileStatus::setAntivirusCheckResult(core::antivirus::check _check)
    {
        if (std::exchange(antivirusCheck_, _check) != _check)
            updateType();
    }

    void Ui::FileStatus::setAntivirusCheckResult(core::antivirus::check::result _result)
    {
        auto& result = antivirusCheck_.result_;
        if (std::exchange(result, _result) != _result)
            updateType();
    }

    void FileStatus::setAccountAntivirus(bool _accountAntivirus)
    {
        if (std::exchange(accountAntivirus_, _accountAntivirus) != _accountAntivirus)
            updateType();
    }

    bool FileStatus::isAntivirusInProgress() const noexcept
    {
        return core::antivirus::check::result::unchecked == antivirusCheck_.result_;
    }

    bool FileStatus::isAntivirusChecked() const noexcept
    {
        using result = core::antivirus::check::result;
        return (result::safe == antivirusCheck_.result_) || (result::unsafe == antivirusCheck_.result_);
    }

    bool FileStatus::isTrustRequired() const noexcept
    {
        return trustRequired_;
    }

    bool Ui::FileStatus::isVirusInfected() const noexcept
    {
        return core::antivirus::check::result::unsafe == antivirusCheck_.result_;
    }

    bool FileStatus::isNoStatusType() const noexcept
    {
        return Type::NoStatus == type_;
    }

    bool FileStatus::isTrustRequiredType() const noexcept
    {
        return Type::TrustRequired == type_;
    }

    bool FileStatus::isVirusInfectedType() const noexcept
    {
        return Type::VirusInfected == type_;
    }

    bool FileStatus::isAntivirusInProgressType() const noexcept
    {
        return Type::AntivirusInAsyncProgress == type_ || Type::AntivirusInSyncProgress == type_;
    }

    bool FileStatus::isAntivirusCheckedType() const noexcept
    {
        return Type::AntivirusOk == type_ || Type::VirusInfected == type_;
    }

    bool FileStatus::isAntivirusAllowedType() const noexcept
    {
        return (Type::VirusInfected != type()) && (Type::AntivirusInSyncProgress != type());
    }

    void FileStatus::updateType()
    {
        type_ = evaluateFileStatus(antivirusCheck_, trustRequired_, accountAntivirus_);
    }

    QSize getFileStatusIconSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QSize getFileStatusIconSizeWithCircle() noexcept
    {
        return Utils::scale_value(QSize(28, 28));
    }

    const QPixmap& getFileStatusIcon(FileStatusType _type, Styling::StyleVariable _color)
    {
        using IconColors = std::unordered_map<Styling::StyleVariable, QPixmap>;
        using IconCache = std::unordered_map<FileStatusType, IconColors>;

        static IconCache cache;

        const auto path = getFileStatusIconPath(_type);
        if (path.empty())
        {
            im_assert(!"invalid file status path");
            static const QPixmap empty;
            return empty;
        }

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            cache.clear();

        auto& pm = cache[_type][_color];
        if (pm.isNull())
            pm = Utils::renderSvg(path.toString(), getFileStatusIconSize(), Styling::getParameters().getColor(_color));
        return pm;
    }

    QString getFileStatusText(FileStatusType _type)
    {
        switch (_type)
        {
        case FileStatusType::NoStatus:
            return {};
        case FileStatusType::TrustRequired:
            return QT_TRANSLATE_NOOP("toast", "Administrator has set the restrictions on access to files");
        case FileStatusType::AntivirusInSyncProgress:
        case FileStatusType::AntivirusInAsyncProgress:
            return QT_TRANSLATE_NOOP("toast", "No access. Antivirus check in progress");
        case FileStatusType::VirusInfected:
            return QT_TRANSLATE_NOOP("toast", "No access. File is blocked by the antivirus");
        case FileStatusType::AntivirusOk:
            return QT_TRANSLATE_NOOP("toast", "The file is available. No viruses found");
        case FileStatusType::AntivirusUnknown:
            return QT_TRANSLATE_NOOP("toast", "File not checked (and never will be). Available to download");
        default:
            break;
        }

        im_assert(false);
        return {};
    }

    void showFileStatusToast(FileStatusType _type)
    {
        if (_type == FileStatusType::NoStatus)
            return;

        if (const auto text = getFileStatusText(_type); !text.isEmpty())
            Utils::showToastOverMainWindow(text, Utils::defaultToastVerOffset());
    }

    bool needShowStatus(const FileStatus& _status)
    {
        return _status.isTrustRequired() || needShowAntivirusStatus(_status);
    }

    bool needShowAntivirusStatus(const FileStatus& _status)
    {
        if (!Features::isAntivirusCheckEnabled())
            return false;
        const bool antivirusChecked = _status.isAntivirusCheckedType();
        const bool antivirusInProgress = _status.isAntivirusInProgressType() && Features::isAntivirusCheckProgressVisible();
        const bool antivirusUnchecked = FileStatusType::AntivirusUnknown == _status.type() && Features::isAntivirusCheckProgressVisible();
        return antivirusChecked || antivirusInProgress || antivirusUnchecked;
    }
} // namespace Ui

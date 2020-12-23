#include "stdafx.h"
#include "SendLogsToServerHandler.h"

#include "SendLogsToServerRequest.h"
#include "core_dispatcher.h"
#include "../gui_coll_helper.h"
#include "../InterConnector.h"
#include "main_window/ContactDialog.h"
#include "main_window/MainWindow.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/contact_profile.h"
#include "my_info.h"
#include "../common.shared/config/config.h"

#include "../../../common.shared/version_info_constants.h"
#include "../../../common.shared/config/config.h"
#include "../../memory_stats/gui_memory_monitor.h"

namespace Utils
{

SendLogsToServerHandler::SendLogsToServerHandler(std::shared_ptr<SendLogsToServerRequest> _request)
    : request_(std::move(_request))
{

}

SendLogsToServerHandler::~SendLogsToServerHandler() = default;

void SendLogsToServerHandler::start()
{
    // 0. get user consent
    if (!getUserConsent())
    {
        Result res;
        res.status_ = Result::Status::Failed;

        Q_EMIT finished(request_->getId(), res);
        return;
    }

    GuiMemoryMonitor::instance().writeLogMemoryReport([this]
    {
        Ui::GetDispatcher()->post_message_to_core("get_logs_path", nullptr, this,
            [this](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);
            onHaveLogsPath(QString::fromUtf8(coll.get_value_as_string("path")));
        });
    });
}

void SendLogsToServerHandler::onHaveLogsPath(const QString &_logsPath)
{
    Result res;

    QDir logsDir(QDir::cleanPath(_logsPath));
    if (!logsDir.exists())
    {
        res.status_ = Result::Status::Failed;

        Q_EMIT finished(request_->getId(), res);
        return;
    }

    const auto aimId = Ui::MyInfo()->aimId();
    Ui::gui_coll_helper col(Ui::GetDispatcher()->create_collection(), true);

    col.set_value_as_string("url", "https://" + std::string(config::get().url(config::urls::feedback)));
    const auto productNameShort = config::get().string(config::values::product_name_short);
    // fb.screen_resolution
    col.set_value_as_qstring("screen_resolution", (qsl("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())));
    // fb.referrer
    col.set_value_as_string("referrer", productNameShort);
    {
        const auto product = config::get().string(config::values::product_name);
        const QString icqVer = QString::fromUtf8(product.data(), product.size()) % ql1c(' ') % QString::fromUtf8(VERSION_INFO_STR);
        auto osv = QSysInfo::prettyProductName();
        if (!osv.length() || osv == u"unknown")
            osv = ql1s("%1 %2 (%3 %4)").arg(QSysInfo::productType(), QSysInfo::productVersion(), QSysInfo::kernelType(), QSysInfo::kernelVersion());

        const auto concat = ql1s("%1 %2 %3:%4").arg(osv, icqVer, QString::fromUtf8(productNameShort.data(), productNameShort.size()), aimId);
        // fb.question.3004
        col.set_value_as_qstring("version", concat);
        // fb.question.159
        col.set_value_as_qstring("os_version", osv);
        // fb.question.178
        col.set_value_as_string("build_type", build::is_debug() ? "beta" : "live");
        // fb.question.3005
        if constexpr (platform::is_apple())
            col.set_value_as_string("platform", "OSx");
        else if constexpr (platform::is_windows())
            col.set_value_as_string("platform", "Windows");
        else if constexpr (platform::is_linux())
            col.set_value_as_string("platform", "Linux");
        else
            col.set_value_as_string("platform", "Unknown");
    }

    // fb.user_name
    col.set_value_as_qstring("user_name", Logic::GetFriendlyContainer()->getFriendly(aimId));

    // fb.message
    col.set_value_as_string("message", "Logs");

    // communication_email ==> UIN, no user interaction
    col.set_value_as_qstring("communication_email", Ui::MyInfo()->aimId());
    // Lang
    col.set_value_as_qstring("language", QLocale::system().name());
    // attachements_count

    // 2. get all file paths
    const auto fileEntries = logsDir.entryList(QDir::Filter::Files, QDir::SortFlag::Time);

    int logsCount = (request_->getInfo().logsInfo_.haveNumLogsFilesLimit() ? std::min(request_->getInfo().logsInfo_.getNumLogsFilesLimit(), fileEntries.count())
                                                                        : fileEntries.count());

    col.set_value_as_string("attachements_count", std::to_string(logsCount + 1)); // +1 'coz we're sending log txt
    if (!fileEntries.empty())
    {
        core::ifptr<core::iarray> farray(col->create_array());
        farray->reserve(logsCount);

        for (QStringList::size_type i = 0;
                i < fileEntries.count() && (!request_->getInfo().logsInfo_.haveNumLogsFilesLimit() || i < request_->getInfo().logsInfo_.getNumLogsFilesLimit());
                ++i)
        {
            farray->push_back(col.create_qstring_value(fileEntries[i]).get());
        }

        // fb.attachement
        col.set_value_as_array("attachement", farray.get());
    }

    Ui::GetDispatcher()->post_message_to_core("feedback/send", col.get());


    res.status_ = Result::Status::Success;
    Q_EMIT finished(request_->getId(), res);
}

void SendLogsToServerHandler::stop()
{

}

bool SendLogsToServerHandler::getUserConsent() const
{
    return Utils::GetConfirmationWithTwoButtons(
        QT_TRANSLATE_NOOP("popup_window", "Cancel"),
        QT_TRANSLATE_NOOP("popup_window", "Yes"),
        QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to send the app logs?"),
        QT_TRANSLATE_NOOP("popup_window", "Send logs"),
        nullptr
    );
}

}

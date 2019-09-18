#include "stdafx.h"
#include "SendLogsToServerHandler.h"

#include "SendLogsToServerRequest.h"
#include "core_dispatcher.h"
#include "../gui_coll_helper.h"
#include "../InterConnector.h"
#include "main_window/ContactDialog.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/contact_profile.h"
#include "my_info.h"

#include "../../../common.shared/version_info_constants.h"
#include "../../memory_stats/gui_memory_monitor.h"

namespace Utils
{

SendLogsToServerHandler::SendLogsToServerHandler(SendLogsToServerRequest *_request)
    : request_(_request)
{

}

SendLogsToServerHandler::~SendLogsToServerHandler()
{

}

void SendLogsToServerHandler::start()
{
    // 0. get user consent
    if (!getUserConsent())
    {
        Result res;
        res.status_ = Result::Status::Failed;

        emit finished(request_->getId(), res);
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

        emit finished(request_->getId(), res);
        return;
    }

    Logic::getContactListModel()->getContactProfile(QString(), [=](Logic::profile_ptr _profile, int32_t /*error*/)
    {
        if (!_profile)
        {
            emit Ui::GetDispatcher()->feedbackSent(false);
            return;
        }

        Ui::gui_coll_helper col(Ui::GetDispatcher()->create_collection(), true);

        col.set_value_as_string("url", build::GetProductVariant(icq_feedback_url, agent_feedback_url, biz_feedback_url, dit_feedback_url));

        // fb.screen_resolution
        col.set_value_as_qstring("screen_resolution", (qsl("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())));
        // fb.referrer
        col.set_value_as_qstring("referrer", build::ProductNameShort());
        {
            const auto appName = build::ProductName() % ql1s(" ");
            const QString icqVer = appName + QString::fromUtf8(VERSION_INFO_STR);
            auto osv = QSysInfo::prettyProductName();
            if (!osv.length() || osv == ql1s("unknown"))
                osv = qsl("%1 %2 (%3 %4)").arg(QSysInfo::productType(), QSysInfo::productVersion(), QSysInfo::kernelType(), QSysInfo::kernelVersion());

            const auto concat = (build::is_icq() ? qsl("%1 %2 icq:%3") : qsl("%1 %2 agent:%3")).arg(osv, icqVer, _profile ? _profile->get_aimid() : QString());
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

        auto fn = qsl("%1%2%3").arg(_profile->get_first_name(), _profile->get_first_name().length() ? qsl(" ") : QString(), _profile->get_last_name());
        if (!fn.length())
        {
            if (!_profile->get_contact_name().isEmpty())
                fn = _profile->get_contact_name();
            else if (_profile->has_nick())
                fn = _profile->get_nick_pretty();
            else if (!_profile->get_friendly().isEmpty())
                fn = _profile->get_friendly();
        }
        // fb.user_name
        col.set_value_as_qstring("user_name", fn);

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
    });


    res.status_ = Result::Status::Success;
    emit finished(request_->getId(), res);
}

void SendLogsToServerHandler::stop()
{

}

bool SendLogsToServerHandler::getUserConsent() const
{
    return Utils::GetConfirmationWithTwoButtons(
        QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
        QT_TRANSLATE_NOOP("popup_window", "YES"),
        QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to send the app logs?"),
        QT_TRANSLATE_NOOP("popup_window", "Send logs"),
        nullptr
    );
}

}

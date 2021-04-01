#include "stdafx.h"

#include "SendLogsToUinHandler.h"
#include "SendLogsToUinRequest.h"
#include "core_dispatcher.h"
#include "../gui_coll_helper.h"
#include "../InterConnector.h"
#include "main_window/ContactDialog.h"
#include "main_window/MainWindow.h"
#include "../../memory_stats/gui_memory_monitor.h"
#include "../common.shared/config/config.h"

namespace Utils
{

SendLogsToUinHandler::SendLogsToUinHandler(std::shared_ptr<SendLogsToUinRequest> _request)
    : request_(std::move(_request))
{
    im_assert(request_);
}

void SendLogsToUinHandler::start()
{
    // 0. check if UIN is in the list of allowed
    // 1. get user consent
    if (!uinIsAllowed(request_->getInfo().uin_) || !getUserConsent())
    {
        Result res;
        res.status_ = Result::Status::Failed;

        Q_EMIT finished(request_->getId(), res);
        return;
    }

    GuiMemoryMonitor::instance().writeLogMemoryReport([this]
    {
        // 2. get logs path
        Ui::GetDispatcher()->post_message_to_core("get_logs_path", nullptr, this, [this](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);

            onHaveLogsPath(QString::fromUtf8(coll.get_value_as_string("path")));
        });
    });
}

void SendLogsToUinHandler::stop()
{

}

void SendLogsToUinHandler::sendFileToContact(const QString &_filePath, const QString &_contact)
{
    QFileInfo info(_filePath);
    im_assert(info.exists());
    if (!info.exists() || info.size() == 0)
        return;

    Ui::GetDispatcher()->uploadSharedFile(_contact, _filePath);
}

void SendLogsToUinHandler::onHaveLogsPath(const QString &_logsPath)
{
    Result res;

    QDir logsDir(QDir::cleanPath(_logsPath));
    if (!logsDir.exists())
    {
        res.status_ = Result::Status::Failed;

        Q_EMIT finished(request_->getId(), res);
        return;
    }

    // 2. get all file paths
    const auto logFileEntries = logsDir.entryList(QDir::Filter::Files, QDir::SortFlag::Time);

    for (QStringList::size_type i = 0;
         i < logFileEntries.count() && (!request_->getInfo().logsInfo_.haveNumLogsFilesLimit() || i < request_->getInfo().logsInfo_.getNumLogsFilesLimit());
         ++i)
        sendFileToContact(logsDir.filePath(logFileEntries[i]), request_->getInfo().uin_);

    // 3. get rtp dumps
    if (request_->getInfo().logsInfo_.haveNumRTPFilesLimit())
    {
        QDir rtpDir = logsDir;
        if (rtpDir.cdUp() && rtpDir.exists())
        {
            rtpDir.setNameFilters({ QString::fromLatin1("*.pcapng") });
            const auto rtpFileEntries = rtpDir.entryList(QDir::Filter::Files, QDir::SortFlag::Time);

            for (QStringList::size_type i = 0;
                i < rtpFileEntries.count() && (!request_->getInfo().logsInfo_.haveNumRTPFilesLimit() || i < request_->getInfo().logsInfo_.getNumRTPFilesLimit());
                ++i)
            {
                sendFileToContact(rtpDir.filePath(rtpFileEntries[i]), request_->getInfo().uin_);
            }
        }
    }

    Utils::openDialogWithContact(request_->getInfo().uin_);

    res.status_ = Result::Status::Success;
    Q_EMIT finished(request_->getId(), res);
}

bool SendLogsToUinHandler::getUserConsent() const
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (mainWindow)
        mainWindow->showMessengerPage();
    return Utils::GetConfirmationWithTwoButtons(
        QT_TRANSLATE_NOOP("popup_window", "Cancel"),
        QT_TRANSLATE_NOOP("popup_window", "Yes"),
        QT_TRANSLATE_NOOP("popup_window", request_->getInfo().logsInfo_.haveNumRTPFilesLimit() ?
            "Are you sure you want to send app logs and call dumps?" : "Are you sure you want to send the app logs?"),
        QT_TRANSLATE_NOOP("popup_window", "Send logs"),
        nullptr);
}

bool SendLogsToUinHandler::uinIsAllowed(const QString &_contactUin) const
{
    const static auto uins = []() {
        const auto uins_csv = config::get().string(config::values::uins_for_send_log_csv);
        const auto splitted = QString::fromUtf8(uins_csv.data(), uins_csv.size()).split(ql1c(','), QString::SkipEmptyParts);
        std::vector<QString> res;
        res.reserve(splitted.size());
        for (const auto& x : splitted)
            res.push_back(x);
        return res;
    }();
    return std::any_of(uins.begin(), uins.end(), [&_contactUin](const QString& _uin) {
        return _uin == _contactUin;
    });
}

}

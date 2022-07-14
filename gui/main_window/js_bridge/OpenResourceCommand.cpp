#include "stdafx.h"
#include "OpenResourceCommand.h"
#include "../../core_dispatcher.h"
#include "../WebAppPage.h"

namespace
{
    constexpr auto timeForResponseFromCore = std::chrono::milliseconds(3000);
}

namespace JsBridge
{
    OpenResourceCommand::OpenResourceCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
        , timer_ { new QTimer(this) }
        , chatId_ { getParamAsString(qsl("chatId")) }
    {
        if (chatId_.startsWith(u'@'))
            chatId_ = chatId_.mid(1);
    }

    const QString& OpenResourceCommand::chatId() const { return chatId_; }

    void OpenResourceCommand::execute()
    {
        if (chatId_.isEmpty())
        {
            setNotFoundErrorCode();
            return;
        }

        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this, &OpenResourceCommand::onTimeout);

        seq_ = Ui::GetDispatcher()->getIdInfo(chatId_);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::idInfo, this, &OpenResourceCommand::onGetIdInfo);
        timer_->start(timeForResponseFromCore);
    }

    void OpenResourceCommand::onTimeout() { setNotFoundErrorCode(); }

    void OpenResourceCommand::onGetIdInfo(const qint64 _seq, const Data::IdInfo& _idInfo)
    {
        if (seq_ != _seq)
            return;

        timer_->stop();
        if (const auto& sn = _idInfo.sn_; !sn.isEmpty())
            onIdInfoSuccessResponse(sn);
        else
            setNotFoundErrorCode();
    }

    OpenProfileCommand::OpenProfileCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : OpenResourceCommand(_reqId, _requestParams, _webPage)
        , webPage_ { _webPage }
    {}

    void OpenProfileCommand::setNotFoundErrorCode() { setErrorCode(qsl("PROFILE_NOT_FOUND"), qsl("Profile \"%1\" not found").arg(chatId())); }

    void OpenProfileCommand::onIdInfoSuccessResponse(const QString& _aimId)
    {
        if (webPage_ && webPage_->isVisible())
        {
            webPage_->showProfile(_aimId);
            addSuccessParameter({});
        }
        else
        {
            setErrorCode(qsl("INTERNAL_ERROR"), qsl("Web page hidden"));
        }
    }

    OpenDialogCommand::OpenDialogCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : OpenResourceCommand(_reqId, _requestParams, _webPage)
    {}

    void OpenDialogCommand::setNotFoundErrorCode() { setErrorCode(qsl("DIALOG_NOT_FOUND"), qsl("Dialog \"%1\" not found").arg(chatId())); }

    void OpenDialogCommand::onIdInfoSuccessResponse(const QString& _aimId)
    {
        bool ok;
        qint64 messageId = getParamAsString(qsl("messageId")).toLongLong(&ok, 10);
        messageId = (!ok || messageId == 0) ? -1 : messageId;
        Utils::InterConnector::instance().openDialog(_aimId, messageId);
        addSuccessParameter({});
    }
} // namespace JsBridge

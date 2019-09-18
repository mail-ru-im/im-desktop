#include "CallQualityStatsMgr.h"

#include "../gui/core_dispatcher.h"
#include "gui_settings.h"
#include "utils/gui_coll_helper.h"

#include "RateCallQualityWidget.h"
#include "RatingIssuesWidget.h"
#include "controls/GeneralDialog.h"
#include "main_window/MainWindow.h"
#include "utils/InterConnector.h"
#include "my_info.h"

namespace
{
constexpr int RETRY_GET_CONFIG_TIMEOUT_MS = 120 * 1000; // 2 mins
}

namespace Ui
{

CallQualityStatsMgr::CallQualityStatsMgr(QObject *_parent)
    : QObject(_parent),
      qualityReasonsPopupConfig_(ShowQualityReasonsPopupConfig::defaultConfig()),
      qualityStarsPopupConfig_(ShowQualityStarsPopupConfig::defaultConfig()),
      callsCount_(0)
{
    initConfigs();

    connect(this, &CallQualityStatsMgr::showCallQualityStarsPopup,
            this, &CallQualityStatsMgr::onShowCallQualityStarsPopup, Qt::QueuedConnection /* intentionally queued */);

    reconnectToMyInfo();
}

void CallQualityStatsMgr::onVoipCallEndedStat(const voip_manager::CallEndStat& _stat)
{
    if (_stat.callTimeInSecs() > qualityStarsPopupConfig_.showOnDurationMin())
        setCallsCount(callsCount() + 1);

    setLastStat(_stat);

    if (shouldShowStarsPopup(_stat))
    {
        emit showCallQualityStarsPopup();
    }
}

void CallQualityStatsMgr::onShowQualityReasonsConfig(const ShowQualityReasonsPopupConfig &_reasonsConfig)
{
    qualityReasonsPopupConfig_ = _reasonsConfig;
}

void CallQualityStatsMgr::onShowQualityStarsConfig(const ShowQualityStarsPopupConfig &_starsConfig)
{
    qualityStarsPopupConfig_ = _starsConfig;
}

int CallQualityStatsMgr::callsCount() const
{
    return callsCount_;
}

void CallQualityStatsMgr::onShowCallQualityStarsPopup()
{
    auto w = new RateCallQualityWidget(qualityStarsPopupConfig_.dialogTitle(), nullptr);
    connect(w, &RateCallQualityWidget::ratingConfirmed,
            this, [this](int _starsCount){
        onRatingConfirmed(_starsCount);
    });
    connect(w, &RateCallQualityWidget::ratingCancelled,
            this, [this](){
        sendStats(0, QString(), QString::fromStdString(lastStat_.contact()), {});
    });

    GeneralDialog::Options options;
    options.preferredSize_ = QSize(Utils::scale_value(380), -1);

    auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow(),
                                                         false, true, true, true, options);
    auto okCancelButtons = generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "OK"), true);
    w->setOkCancelButton(okCancelButtons.first, okCancelButtons.second);

    const auto res = generalDialog->showInCenter();
    Q_UNUSED(res);
}

void CallQualityStatsMgr::onRatingConfirmed(int _starsCount)
{
    auto it = std::find(qualityStarsPopupConfig_.secondPopupStars().cbegin(),
                        qualityStarsPopupConfig_.secondPopupStars().cend(),
                        _starsCount);

    if (it == qualityStarsPopupConfig_.secondPopupStars().cend())
    {
        sendStats(_starsCount, QString(), QString::fromStdString(lastStat_.contact()), {});
        return;
    }

    auto w = new RatingIssuesWidget(qualityReasonsPopupConfig_, nullptr);

    GeneralDialog::Options options;
    options.preferredSize_ = QSize(Utils::scale_value(380), 0);

    auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow(),
                                                         false, true, true, true,
                                                         options);
    auto okCancelButtons = generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                                                         QT_TRANSLATE_NOOP("popup_window", "OK"), true);
    Q_UNUSED(okCancelButtons);

    auto res = generalDialog->showInCenter();
    Q_UNUSED(res);

    sendStats(_starsCount, qualityReasonsPopupConfig_.surveyId(),
              QString::fromStdString(lastStat_.contact()), w->getSelectedReasons());
}

void CallQualityStatsMgr::sendStats(int _starsCount, const QString &_surveyId,
                                    const QString&_contact, const std::vector<QString> &_reasonIds)
{
    core::coll_helper helper(Ui::GetDispatcher()->create_collection(), true);

    helper.set<QString>("survey_id", _surveyId);
    helper.set<QString>("aimid", _contact);
    helper.set_value_as_int("starsCount", _starsCount);

    core::ifptr<core::iarray> reasons_array(helper->create_array());
    reasons_array->reserve(static_cast<int>(_reasonIds.size()));
    for (const auto& reason : _reasonIds)
    {
        core::ifptr<core::ivalue> val(helper->create_value());
        auto reasonStr = reason.toStdString();
        val->set_as_string(reasonStr.c_str(), reasonStr.length());
        reasons_array->push_back(val.get());
    }
    helper.set_value_as_array("reasons", reasons_array.get());

    GetDispatcher()->post_message_to_core("send_voip_calls_quality_report",
                                          helper.get());
}

void CallQualityStatsMgr::reconnectToMyInfo()
{
    connect(Ui::MyInfo(), &Ui::my_info::received, this, &CallQualityStatsMgr::initCallsCount,
            Qt::UniqueConnection);
}

void CallQualityStatsMgr::initConfigs()
{
    core::coll_helper helper(Ui::GetDispatcher()->create_collection(), true);

    auto lang = Utils::GetTranslator()->getLang();
    QLocale locale(lang);
    helper.set<QString>("locale", locale.name());
    helper.set<QString>("lang", lang);

    GetDispatcher()->post_message_to_core("get_voip_calls_quality_popup_conf",
                                          helper.get(),
                                          this,
                                          [this](core::icollection* _coll)
    {
        Ui::gui_coll_helper coll(_coll, false);

        if (coll.is_value_exist("error") && coll.get_value_as_int("error"))
        {
            QTimer::singleShot(RETRY_GET_CONFIG_TIMEOUT_MS, this, &CallQualityStatsMgr::initConfigs);
            return;
        }

        qualityStarsPopupConfig_.unserialize(coll);
        qualityReasonsPopupConfig_.unserialize(coll);
    });
}

void CallQualityStatsMgr::initCallsCount()
{
    auto aimId = Ui::MyInfo()->aimId();

    if (currentAimId_ != aimId)
        currentAimId_ = aimId;
    else
        return;

    auto calls = get_gui_settings()->get_value<qt_gui_settings::VoipCallsCountMap>(qsl(settings_voip_calls_count_map), {});

    callsCount_ = (calls.find(aimId) != calls.end()) ? calls[aimId]
                                                     : 0;

}

void CallQualityStatsMgr::setCallsCount(int _callsCount)
{
    callsCount_ = _callsCount;

    auto calls = get_gui_settings()->get_value<qt_gui_settings::VoipCallsCountMap>(qsl(settings_voip_calls_count_map), {});
    calls[Ui::MyInfo()->aimId()] = callsCount_;
    get_gui_settings()->set_value(qsl(settings_voip_calls_count_map), calls);
}

bool CallQualityStatsMgr::shouldShowStarsPopup(const voip_manager::CallEndStat& _stat) const
{
    if (!qualityStarsPopupConfig_.isValid())
        return false;

    bool showOnCurrentCallNumber = std::find(qualityStarsPopupConfig_.showOnCallNumber().begin(),
                                             qualityStarsPopupConfig_.showOnCallNumber().end(),
                                             callsCount()) != qualityStarsPopupConfig_.showOnCallNumber().end();

    return showOnCurrentCallNumber &&
           _stat.connectionEstablished() &&
            _stat.callTimeInSecs() > qualityStarsPopupConfig_.showOnDurationMin();
}

void CallQualityStatsMgr::setLastStat(const voip_manager::CallEndStat &_lastStat)
{
    lastStat_ = _lastStat;
}

}

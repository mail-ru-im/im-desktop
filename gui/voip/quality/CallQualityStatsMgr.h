#pragma once

#include <QObject>

#include "ShowQualityReasonsPopupConfig.h"
#include "ShowQualityStarsPopupConfig.h"

#include "../../../core/Voip/VoipManagerDefines.h"

namespace Ui
{

class CallQualityStatsMgr: public QObject
{
    Q_OBJECT

public:
    CallQualityStatsMgr(QObject *_parent = nullptr);

    using StarCountType = int;

public Q_SLOTS:
    void onVoipCallEndedStat(const voip_manager::CallEndStat& _stat);

    void onShowQualityReasonsConfig(const ShowQualityReasonsPopupConfig& _reasonsConfig);
    void onShowQualityStarsConfig(const ShowQualityStarsPopupConfig& _starsConfig);

    int callsCount() const;

    void onShowCallQualityStarsPopup();
    void onRatingConfirmed(int _starsCount);
    void sendStats(int _starsCount, const QString& _surveyId, const QString &_contact, const std::vector<QString> &_reasonIds);
    void reconnectToMyInfo();
    void initCallsCount();

Q_SIGNALS:
    void showCallQualityStarsPopup();

private:
    void initConfigs();
    void setCallsCount(int _callsCount);
    bool shouldShowStarsPopup(const voip_manager::CallEndStat& _stat) const;
    void setLastStat(const voip_manager::CallEndStat& _lastStat);

private:
    ShowQualityReasonsPopupConfig qualityReasonsPopupConfig_;
    ShowQualityStarsPopupConfig   qualityStarsPopupConfig_;

    int                           callsCount_;
    voip_manager::CallEndStat lastStat_;
    QString currentAimId_;
};

}

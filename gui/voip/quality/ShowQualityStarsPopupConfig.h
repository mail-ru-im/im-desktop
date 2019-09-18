#pragma once

#include <QString>
#include <vector>
#include "../corelib/collection_helper.h"

namespace Ui
{

class ShowQualityStarsPopupConfig
{
public:
    ShowQualityStarsPopupConfig(const core::coll_helper& _coll);
    ShowQualityStarsPopupConfig();

    const std::vector<int>& secondPopupStars() const;
    const std::vector<int>& showOnCallNumber() const;
    unsigned int showOnDurationMin() const;
    bool showPopup() const;
    const QString& dialogTitle() const;

    bool unserialize(const core::coll_helper& _coll);

    bool isValid() const;

    static ShowQualityStarsPopupConfig defaultConfig();

private:
    void clear();
    void setValid(bool _valid);

private:
    std::vector<int32_t> secondPopupStars_;
    std::vector<int32_t> showOnCallNumber_;
    int32_t              showOnDurationMin_;
    bool                 showPopup_;

    QString              title_;

    bool isValid_ = false;
};

}

Q_DECLARE_METATYPE(Ui::ShowQualityStarsPopupConfig)

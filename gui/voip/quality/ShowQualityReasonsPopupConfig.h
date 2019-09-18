#pragma once

#include <QString>
#include <vector>
#include "utils/gui_coll_helper.h"

namespace Ui
{

class ShowQualityReasonsPopupConfig
{
public:
    struct Reason
    {
        QString reason_id_;
        QString display_text_;

        void unserialize(const gui_coll_helper& _coll);
    };

    struct ReasonCategory
    {
        QString category_id_;
        std::vector<Reason> reasons_;

        void unserialize(const gui_coll_helper &_coll);
    };

public:
    ShowQualityReasonsPopupConfig(const core::coll_helper& _coll); // coll_helper?
    ShowQualityReasonsPopupConfig();

    bool unserialize(const core::coll_helper& _coll);
    bool isValid() const;

    static ShowQualityReasonsPopupConfig defaultConfig();
    const std::vector<ReasonCategory>& categories() const;

    const QString& surveyId() const;
    const QString& surveyTitle() const;

private:
    void clear();
    void setValid(bool _valid);

private:
    std::vector<ReasonCategory> categories_;
    QString surveyId_;
    QString surveyTitle_;

    bool isValid_ = false;
};

}

Q_DECLARE_METATYPE(Ui::ShowQualityReasonsPopupConfig)

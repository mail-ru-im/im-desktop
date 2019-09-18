#pragma once

#include <QWidget>
#include "ShowQualityReasonsPopupConfig.h"

class QVBoxLayout;

namespace Ui
{

class RCheckboxList;
class TextBrowserEx;
class ScrollAreaWithTrScrollBar;

class RatingIssuesWidget: public QWidget
{
    Q_OBJECT

public:
    explicit RatingIssuesWidget(const ShowQualityReasonsPopupConfig& _config, QWidget *_parent = nullptr);

    std::vector<QString> getSelectedReasons() const;

protected:
    void paintEvent(QPaintEvent *_event) override;

private:
    ShowQualityReasonsPopupConfig config_;

    QVBoxLayout* globalLayout_ = nullptr;
    RCheckboxList* reasonsList_ = nullptr;
    TextBrowserEx* title_ = nullptr;

    ScrollAreaWithTrScrollBar* scrollArea_ = nullptr;
};

}

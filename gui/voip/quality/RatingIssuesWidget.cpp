#include "RatingIssuesWidget.h"

#include <QVBoxLayout>
#include "utils/utils.h"
#include "utils/Text.h"
#include "controls/CheckboxList.h"
#include "controls/TransparentScrollBar.h"

#include "controls/LabelEx.h"
#include "controls/TextBrowserEx.h"
#include "fonts.h"

namespace
{
constexpr auto REASON_HEIGHT = 44;
constexpr auto HOR_OFFSET = 16;
constexpr auto SEPARATOR_HEIGHT = 1;
constexpr auto GRADIENT_HEIGHT = 40;

constexpr auto DIALOG_WIDTH = 380;
}

namespace Ui
{

RatingIssuesWidget::RatingIssuesWidget(const ShowQualityReasonsPopupConfig& _config, QWidget *_parent)
    : QWidget(_parent),
      config_(_config)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedWidth(Utils::scale_value(DIALOG_WIDTH));

    globalLayout_ = Utils::emptyVLayout(this);

    TextBrowserEx::Options br_options;
    br_options.font_ = Fonts::appFontScaled(23);
    br_options.bodyMargins_ = QMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16), Utils::scale_value(20));

    title_ = new TextBrowserEx(br_options, this);
    title_->setHtml(ql1s("<body>") % _config.surveyTitle() % ql1s("</body>"));
    title_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH));

    const auto& metrics = Utils::evaluateTextHeightMetrics(_config.surveyTitle(),
                                                           Utils::scale_value(DIALOG_WIDTH),
                                                           QFontMetrics(br_options.font_));

    title_->setMaximumHeight(metrics.getHeightPx() + Utils::scale_value(12) /* margin top */);
    title_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    title_->setPlaceholderText(QString());
    title_->setAutoFillBackground(false);
    title_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setAlignment(Qt::AlignLeft);
    title_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

    Testing::setAccessibleName(title_, qsl("AS title_"));

    scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea_->setContentsMargins(0, 0, 0, 0);
    scrollArea_->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea_->viewport(), true);

    CheckboxList::Options options;
    options.separatorHeight_ = SEPARATOR_HEIGHT;
    options.separatorColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    options.overlayGradientBottom_ = true;
    options.overlayWhenHeightDoesNotFit_ = true;
    options.overlayHeight_ = Utils::scale_value(GRADIENT_HEIGHT);
    options.removeBottomOverlayThreshold_ = Utils::scale_value(5);
    options.checkboxToTextOffset_ = Utils::scale_value(16);

    reasonsList_ = new RCheckboxList(scrollArea_,
                                     Fonts::appFontScaled(16),
                                     Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                                     Utils::scale_value(HOR_OFFSET),
                                     Utils::scale_value(REASON_HEIGHT),
                                     options);

    std::vector<int> separatorIndexes;
    separatorIndexes.reserve(config_.categories().size());

    int curSep = 0;
    for (const auto& category: config_.categories())
    {
        for (const auto& reason : category.reasons_)
            reasonsList_->addItem(reason.reason_id_, reason.display_text_);

        curSep += category.reasons_.size();
        separatorIndexes.push_back(curSep);
    }

    reasonsList_->setSeparators(separatorIndexes);
    reasonsList_->setScrollAreaHeight(scrollArea_->size().height());

    scrollArea_->setStyleSheet(ql1s("background-color: ") % Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE) % ql1s("; border: none;"));
    scrollArea_->setWidget(reasonsList_);

    globalLayout_->addWidget(title_);
    globalLayout_->addWidget(scrollArea_);
}

std::vector<QString> RatingIssuesWidget::getSelectedReasons() const
{
    return reasonsList_->getSelectedItems();
}

void RatingIssuesWidget::paintEvent(QPaintEvent *_event)
{
    QWidget::paintEvent(_event);
    reasonsList_->setScrollAreaHeight(scrollArea_->size().height());
}

}




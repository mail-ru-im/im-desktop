#include "RateCallQualityWidget.h"

#include <QVBoxLayout>
#include <QPushButton>

#include "utils/utils.h"
#include "utils/Text.h"
#include "controls/HorizontalImgList.h"
#include "controls/PictureWidget.h"
#include "controls/DialogButton.h"
#include "controls/TextBrowserEx.h"
#include "fonts.h"

namespace
{
constexpr int STAR_WIDTH_HEIGHT = 34;
constexpr int STAR_HOR_OFFSET = 4;
constexpr int MARGIN = 0;

const auto CAT_IMAGE_BAD = qsl(":/voip_quality/cat_image_bad.svg");
const auto CAT_IMAGE_NEUTRAL = qsl(":/voip_quality/cat_image.svg");
const auto CAT_IMAGE_GOOD = qsl(":/voip_quality/cat_image_good.svg");

constexpr auto HOR_TITLE_OFFSET = 16;
constexpr auto WIDGET_WIDTH = 380;
}

namespace Ui
{

RateCallQualityWidget::RateCallQualityWidget(const QString &_wTitle, QWidget *_parent)
    : QWidget(_parent),
      okDialogButton_(nullptr),
      cancelDialogButton_(nullptr)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedWidth(Utils::scale_value(WIDGET_WIDTH));

    globalLayout_ = Utils::emptyVLayout(this);
    globalLayout_->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    globalLayout_->setSpacing(Utils::scale_value(0));

    HorizontalImgList::Options options;
    options.highlightOnHover_ = true;
    options.hlOnHoverHigherThan_ = 0;
    options.rememberClickedHighlight_ = true;

    starsWidget_ = new HorizontalImgList(options, this);

    QSize starSize(Utils::scale_value(STAR_WIDTH_HEIGHT), Utils::scale_value(STAR_WIDTH_HEIGHT));
    starsWidget_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    starsWidget_->setFixedHeight(Utils::scale_value(STAR_WIDTH_HEIGHT));
    starsWidget_->setContentsMargins(0, 0, 0, 0);
    starsWidget_->setItemOffsets(QMargins(Utils::scale_value(STAR_HOR_OFFSET), 0,
                                          Utils::scale_value(STAR_HOR_OFFSET), 0));

    HorizontalImgList::ImageInfo info(starSize,
                                      Utils::renderSvg(qsl(":/voip_quality/empty_star_icon.svg"), starSize),
                                      Utils::renderSvg(qsl(":/voip_quality/fill_star_icon.svg"), starSize));
    starsWidget_->setIdenticalItems(info, 5);
    starsWidget_->setVisible(true);

    catSize_ = Utils::scale_value(QSize(148, 178));

    catWidget_ = new PictureWidget(this);
    catWidget_->setContentsMargins(0, 0, 0, 0);
    catWidget_->setFixedSize(catSize_);
    catWidget_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    catWidget_->setIconAlignment(Qt::AlignLeft | Qt::AlignTop);
    catWidget_->setImage(Utils::renderSvg(CAT_IMAGE_NEUTRAL, catSize_), -1);
    Testing::setAccessibleName(catWidget_, qsl("AS rate call quality widget logo catWidget_"));

    auto evalAndSetHeight = [](TextBrowserEx& _textBrowser, const QFont& _font) {
        auto textHeightMetrics = Utils::evaluateTextHeightMetrics(_textBrowser.toPlainText(), _textBrowser.width(), QFontMetrics(_font));
        auto height_alleged = textHeightMetrics.getHeightPx();

        _textBrowser.setFixedHeight(height_alleged);
    };

    TextBrowserEx::Options titleOptions;
    titleOptions.font_ = Fonts::appFontScaled(23);

    title_ = new TextBrowserEx(titleOptions, this);
    title_->setText(_wTitle);
    title_->setFixedWidth(Utils::scale_value(WIDGET_WIDTH) - 2 * Utils::scale_value(HOR_TITLE_OFFSET));
    title_->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
    title_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    title_->setPlaceholderText(QString());
    title_->setAutoFillBackground(false);
    title_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setAlignment(Qt::AlignCenter);
    title_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    title_->document()->setDocumentMargin(0);
    evalAndSetHeight(*title_, titleOptions.font_);

    globalLayout_->addSpacing(Utils::scale_value(12));
    globalLayout_->addWidget(title_);
    globalLayout_->addSpacing(Utils::scale_value(28));
    globalLayout_->addWidget(catWidget_);
    globalLayout_->addSpacing(Utils::scale_value(21));
    globalLayout_->addWidget(starsWidget_);
    globalLayout_->addSpacing(Utils::scale_value(44 - 16));

    globalLayout_->setAlignment(catWidget_, Qt::AlignCenter);
    globalLayout_->setAlignment(starsWidget_, Qt::AlignCenter);
    globalLayout_->setAlignment(title_, Qt::AlignCenter);

    connect(starsWidget_, &HorizontalImgList::itemClicked,
            this, &RateCallQualityWidget::onCallRated);

    connect(starsWidget_, &HorizontalImgList::itemHovered,
            this, &RateCallQualityWidget::onCallRateHovered);

    connect(starsWidget_, &HorizontalImgList::mouseLeft,
            this, [this](){
        updateCatImageFor(currentStarsCount_);
    });
}

void RateCallQualityWidget::setOkCancelButton(DialogButton* _okButton, DialogButton* _cancelButton)
{
    okDialogButton_ = _okButton;
    cancelDialogButton_ = _cancelButton;

    if (okDialogButton_)
    {
        okDialogButton_->setEnabled(false);

        connect(okDialogButton_, &QPushButton::clicked,
                this, [this](){
            onConfirmRating(true, currentStarsCount_);
        });
    }

    if (cancelDialogButton_)
    {
        cancelDialogButton_->setEnabled(true);

        connect(cancelDialogButton_, &QPushButton::clicked,
                this, [this](){
            onConfirmRating(false, 0);
        });
    }
}

void RateCallQualityWidget::onCallRated(int _starIndex)
{
    currentStarsCount_ = _starIndex + 1;

    // Update cat widget
    updateCatImageFor(currentStarsCount_);

    // enable ok button
    if (okDialogButton_)
    {
        okDialogButton_->setEnabled(true);
    }
}

void RateCallQualityWidget::onCallRateHovered(int _starIndex)
{
    auto starsCount = _starIndex + 1;
    if (starsCount <= currentStarsCount_)
        return;

    updateCatImageFor(starsCount);
}

void RateCallQualityWidget::updateCatImageFor(int _starsCount)
{
    if (_starsCount < 0)
        catWidget_->setImage(Utils::renderSvg(CAT_IMAGE_NEUTRAL, catSize_), -1);
    else if (_starsCount < 4)
        catWidget_->setImage(Utils::renderSvg(CAT_IMAGE_BAD, catSize_), -1);
    else
        catWidget_->setImage(Utils::renderSvg(CAT_IMAGE_GOOD, catSize_), -1);
}

void RateCallQualityWidget::onConfirmRating(bool _doConfirm, int _starsCount)
{
    if (!_doConfirm)
    {
        emit ratingCancelled();
        return;
    }

    emit ratingConfirmed(_starsCount);
}

void RateCallQualityWidget::paintEvent(QPaintEvent* _ev)
{
    QWidget::paintEvent(_ev);
    //!!!
//    QPainter p(this);
//    p.setPen(Qt::blue);
//    p.drawRect(catWidget_->x(), catWidget_->y(), catWidget_->width(), catWidget_->height());
//    p.drawRect(title_->x(), title_->y(), title_->width(), title_->height());
}

}

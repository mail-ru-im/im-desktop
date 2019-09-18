#include "stdafx.h"

#include "../main_window/history_control/ActionButtonWidget.h"
#include "../themes/ResourceIds.h"
#include "../utils/utils.h"
#include "styles/ThemeParameters.h"

#include "DownloadWidget.h"

namespace
{
    const int DownloadWidgetWidth = 320;
    const int DownloadWidgetHeight = 200;

    static const Ui::ActionButtonResource::ResourceSet DownloadButtonResources(
        Themes::PixmapResourceId::PreviewerReload,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);
}

Previewer::DownloadWidget::DownloadWidget(QWidget* _parent)
    : QFrame(_parent)
    , isLayoutSetted_(false)
{
    const auto style = qsl(
        "QLabel[ErrorMessage=\"true\"]{"
        "color: %1;"
        "font-size: 13dip;}"
    ).arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT));

    holder_ = new QFrame();
    holder_->setProperty("DownloadWidget", true);
    Utils::ApplyStyle(holder_, style);
    holder_->setFixedSize(Utils::scale_value(DownloadWidgetWidth), Utils::scale_value(DownloadWidgetHeight));

    topSpacer_ = new QSpacerItem(0, 0);

    button_ = new Ui::ActionButtonWidget(DownloadButtonResources);
    connect(button_, &Ui::ActionButtonWidget::startClickedSignal, this,
        [this]()
        {
            startLoading();
            emit tryDownloadAgain();
        });

    connect(button_, &Ui::ActionButtonWidget::stopClickedSignal, this,
        [this]()
        {
            stopLoading();
            emit cancelDownloading();
        });

    auto buttonLayout = Utils::emptyHLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    Testing::setAccessibleName(button_, qsl("AS dw button_"));
    buttonLayout->addWidget(button_);

    auto buttonHolder = new QFrame();
    buttonHolder->setLayout(buttonLayout);

    textSpacer_ = new QSpacerItem(0, 0);

    errorMessage_ = new QLabel(QT_TRANSLATE_NOOP("previewer", "Unable to download the image"));
    Utils::ApplyStyle(errorMessage_, style);
    errorMessage_->setProperty("ErrorMessage", true);
    errorMessage_->setAlignment(Qt::AlignHCenter);
    errorMessage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QVBoxLayout* layout = Utils::emptyVLayout();
    layout->addSpacerItem(topSpacer_);
    Testing::setAccessibleName(buttonHolder, qsl("AS dw buttonHolder"));
    layout->addWidget(buttonHolder);
    layout->addSpacerItem(textSpacer_);
    Testing::setAccessibleName(errorMessage_, qsl("AS dw errorMessage_"));
    layout->addWidget(errorMessage_);
    layout->addStretch();

    holder_->setLayout(layout);

    QVBoxLayout* mainLayout = Utils::emptyVLayout();
    Testing::setAccessibleName(holder_, qsl("AS dw holder_"));
    mainLayout->addWidget(holder_);
    mainLayout->setAlignment(Qt::AlignCenter);

    setLayout(mainLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Previewer::DownloadWidget::setupLayout()
{
    if (isLayoutSetted_)
        return;

    isLayoutSetted_ = true;

    const auto buttonHeight = button_->height();

    const auto topSpacing = (Utils::scale_value(DownloadWidgetHeight) - buttonHeight) / 2;

    const auto metrics = QFontMetrics(errorMessage_->font());
    const auto overline = metrics.overlinePos();
    const auto textHeight = errorMessage_->height();

    const auto textSpacing = (topSpacing - textHeight) / 2 - overline;

    topSpacer_->changeSize(100, topSpacing);
    textSpacer_->changeSize(100, textSpacing);
}

void Previewer::DownloadWidget::startLoading()
{
    setupLayout();
    button_->startAnimation();
    errorMessage_->hide();
}

void Previewer::DownloadWidget::stopLoading()
{
    setupLayout();
    button_->stopAnimation();
    errorMessage_->show();
}

void Previewer::DownloadWidget::onDownloadingError()
{
    stopLoading();
}

void Previewer::DownloadWidget::mousePressEvent(QMouseEvent* _event)
{
    const auto pos = holder_->mapFromParent(_event->pos());
    const auto rect = holder_->frameRect();
    if (rect.contains(pos))
    {
        emit clicked();
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

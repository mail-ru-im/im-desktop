
#include "stdafx.h"
#include "MaskPanel.h"
#include "DetachedVideoWnd.h"
#include "../utils/utils.h"
#include "../core_dispatcher.h"
#include "MaskManager.h"
#include "VoipTools.h"


// Items view settings.
enum {
    /*NORMAL_ITEM_BORDER = 1,*/ SELECTED_ITEM_BORDER = 1,
    BORDER_SHADOW = 1,
    AVATAR_SIZE        = 33,
    NORMAL_ITEM_SIZE   = 37,// + NORMAL_ITEM_BORDER * 2,
    SELECTED_ITEM_SIZE = NORMAL_ITEM_SIZE,// + SELECTED_ITEM_BORDER * 2,
    PANEL_SIZE = 116, HIDE_PRIVIEW_PRIMARY_INTERVAL = 7000,
    SCROLLING_SPEED = 10, MASK_SPACING = 7,
    MASKS_ANIMATION_MOVE_SPEED = 25,
    MASKS_ANIMATION_RESIZE_SPEED = 32,
    MASKS_ANIMATION_RESIZE_DISTANCE = 32,
    MASKS_ANIMATION_DURATION = 300, // In ms.
};

Ui::MaskWidget::MaskWidget(const voip_masks::Mask* mask) : mask_(mask), loadingProgress_(0.0),
    maskEngineReady_(false), applyWhenEnebled_(false),
    closeIcon_(Utils::parse_image_name(qsl(":/voip/close_mask_100")))
{
    setObjectName(qsl("MaskWidget"));
    setCursor(Qt::PointingHandCursor);

    updatePreview();
    updateJsonPath();

    if (mask_)
    {
        connect(mask_, &voip_masks::Mask::previewLoaded, this, &MaskWidget::updatePreview);
        connect(mask_, &voip_masks::Mask::loaded, this, &MaskWidget::updateJsonPath);
        connect(mask_, &voip_masks::Mask::loaded, this, &MaskWidget::loaded);
        connect(mask_, &voip_masks::Mask::loadingProgress, this, &MaskWidget::updateLoadingProgress);
    }

    //auto margin = Utils::scale_value(1);
    //setContentsMargins(0, 0, 0, 0);
    setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE /*+ margin*/, NORMAL_ITEM_SIZE /*+ margin*/)));

    resizeAnimationMin_ = new QPropertyAnimation(this, QByteArrayLiteral("minimumSize"), this);
    resizeAnimationMin_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimationMin_->setDuration(100);

    resizeAnimationMax_ = new QPropertyAnimation(this, QByteArrayLiteral("maximumSize"), this);
    resizeAnimationMax_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimationMax_->setDuration(100);

    setContentsMargins(0, 0, 0, 0);
}

void Ui::MaskWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::NoBrush);

    painter.fillRect(contentsRect(), Qt::transparent);

    int border = Utils::scale_value(SELECTED_ITEM_BORDER) * 2;
    {
        Utils::PainterSaver ps(painter);
        QColor maskPenColor(Qt::transparent);
        QColor maskBrushColor("#000000");
        maskBrushColor.setAlphaF(0.5);
        painter.setPen(QPen(maskPenColor));
        painter.setBrush(QBrush(maskBrushColor));

        // Draw outer border.
        QRectF contentRectF = contentsRect().marginsRemoved(QMargins(border, border, border, border));

        // Fix for retina and non-retina displays.
        // Fo retina we will have roundBorder 0.5.
        float roundBorder = 1.0f / Utils::scale_bitmap_ratio();
        contentRectF.adjust(roundBorder, roundBorder,  - roundBorder, - roundBorder);
        painter.drawEllipse(contentRectF);
    }

    // Draw image.
    if (!image_.isNull())
    {
        Utils::PainterSaver ps(painter);
        auto imageRect = contentsRect().marginsRemoved(QMargins(border, border, border, border));

        painter.setPen(QPen(Qt::transparent));
        painter.setBrush(QBrush(Qt::transparent));

        QPainterPath path;
        path.addEllipse(imageRect);
        painter.setClipPath(path);

        auto resizedImage = image_.scaled(Utils::scale_bitmap(imageRect.size()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(imageRect, resizedImage, resizedImage.rect());
    }

    // Draw disabled state or selected.
    if (!isLoaded() || !maskEngineReady_ || isChecked())
    {
        Utils::PainterSaver ps(painter);
        painter.setPen(QPen(Qt::transparent));
        QColor maskDisabledColor("#000000");
        maskDisabledColor.setAlphaF(0.5);
        painter.setBrush(QBrush(maskDisabledColor));

        auto borderRect = contentsRect().marginsRemoved(QMargins(border, border, border, border));
        // Draw outer border.
        int startAngel  = 90 * 16; // 90 degree.
        int spanAngel = (1.0 - loadingProgress_) * 360 * 16; // 90 degree - progress * 360
        painter.drawPie(borderRect, startAngel, (!maskEngineReady_ || (isChecked() && isLoaded())) ? 360 * 16 : spanAngel);
    }

    // Selected border.
    if (isChecked() && isLoaded())
    {
        Utils::PainterSaver ps(painter);

        QPen pen(QColor(ql1s("#ffffff")));
        pen.setWidth(Utils::scale_value(SELECTED_ITEM_BORDER) * 2);

        painter.setPen(pen);

        int borderSelect = Utils::scale_value(SELECTED_ITEM_BORDER + BORDER_SHADOW);
        auto borderRect = contentsRect().marginsRemoved(QMargins(borderSelect, borderSelect, borderSelect, borderSelect));

        // Draw outer border.
        painter.drawEllipse(borderRect);

        // Draw border shadow
        int borderShadow = Utils::scale_value(BORDER_SHADOW);
        auto borderShadowRect = contentsRect().marginsRemoved(QMargins(borderShadow, borderShadow, borderShadow, borderShadow));
        QColor shadowColor("#000000");
        shadowColor.setAlphaF(0.2);
        QPen shadowPen(shadowColor);
        shadowPen.setWidth(Utils::scale_value(BORDER_SHADOW));
        painter.setPen(shadowPen);
        painter.drawEllipse(borderShadowRect);
    }

    // Draw close icon
    if (isChecked() && isLoaded())
    {
        auto imageRect = contentsRect().marginsRemoved(QMargins(border, border, border, border));
        auto iconSize = Utils::scale_value(20) * width() / Utils::scale_value(SELECTED_ITEM_SIZE);
        QRect closeIconRect(imageRect.left() + imageRect.width() / 2 - iconSize / 2,
                            imageRect.top()  + imageRect.height() / 2 - iconSize / 2,
                            iconSize,
                            iconSize);
        painter.drawPixmap(closeIconRect, closeIcon_, closeIcon_.rect());
    }
}

QString Ui::MaskWidget::maskPath()
{
    return maskPath_;
}

void Ui::MaskWidget::setPixmap(const QPixmap& image)
{
    image_ = image;
    update();
}

QPixmap Ui::MaskWidget::pixmap()
{
    return image_;
}

void Ui::MaskWidget::setChecked(bool check)
{
    QPushButton::setChecked(check);

    QSize targetSize = Utils::scale_value(check ? QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE) :
        QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE));

    if (targetSize != size())
    {
        resizeAnimationMax_->stop();
        resizeAnimationMin_->stop();

        resizeAnimationMax_->setEndValue(targetSize);
        resizeAnimationMin_->setEndValue(targetSize);

        resizeAnimationMax_->start();
        resizeAnimationMin_->start();
    }

    if (check)
    {
        if (isEnabled())
        {
            Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        }
        else
        {
            applyWhenEnebled_ = true;
        }
    }
    else
    {
        applyWhenEnebled_ = false;
    }
}

void Ui::MaskWidget::updatePreview()
{
    if (mask_)
    {
        image_ = mask_->preview();
        update();
    }
}

void Ui::MaskWidget::updateJsonPath()
{
    if (mask_)
    {
        loading_ = false;
        maskPath_ = mask_->jsonPath();
        name_     = mask_->id();
        updateLoadingProgress(!maskPath_.isEmpty() ? 100 : 0);
        update();
    }
}

void Ui::MaskWidget::updateLoadingProgress(unsigned progress)
{
    loadingProgress_ = progress / 100.0f;
    update();
}

void Ui::MaskWidget::setMaskEngineReady(bool ready)
{
    maskEngineReady_ = ready;
    updateJsonPath();
    update();
}

void Ui::MaskWidget::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::EnabledChange && isEnabled() && applyWhenEnebled_)
    {
        Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        applyWhenEnebled_ = false;
    }

    QPushButton::changeEvent(event);
}

bool Ui::MaskWidget::isEmptyMask()
{
    return mask_ == nullptr;
}

bool Ui::MaskWidget::isLoaded()
{
    return mask_ == nullptr || !maskPath_.isEmpty();
}

const QString& Ui::MaskWidget::name()
{
    return name_;
}

void Ui::MaskWidget::setXPos(int x)
{
    move(x, y());
}

void Ui::MaskWidget::setYCenter(int y)
{
    move(x(), y - height() / 2);
}

int  Ui::MaskWidget::yCenter()
{
    return y() + height() / 2;
}

void Ui::MaskWidget::setYPos(int y)
{
    move(x(), y);
}

void Ui::MaskWidget::setXCenter(int x)
{
    move(x - width() / 2, y());
}

int  Ui::MaskWidget::xCenter()
{
    return x() + width() / 2;
}

void Ui::MaskWidget::updateSize()
{
    setFixedSize(Utils::scale_value(isChecked() ? QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE) :
                                                  QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
}

void Ui::MaskWidget::load()
{
    if (!std::exchange(loading_, true))
        Ui::GetDispatcher()->getVoipController().getMaskManager()->loadMask(name_);
}

void Ui::MaskWidget::moveEvent(QMoveEvent *event)
{
    // Made position even, because during animation we have twitching without this code.
    if (x() % 2 == 1)
    {
        setXPos(x() - 1);
    }
    if (y() % 2 == 1)
    {
        setYPos(y() - 1);
    }
}


Ui::MaskPanel::MaskPanel(QWidget* _parent, QWidget* _container, int topOffset, int bottomOffset) :
#ifdef __APPLE__
    // We use Qt::WindowDoesNotAcceptFocus for mac, bcause it fix ghost window after call stop on Sierra
    BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    BaseBottomVideoPanel(_parent, Qt::Widget)
#else
    BaseBottomVideoPanel(_parent)
#endif
    , container_(_container)
    , rootWidget_(nullptr)
    , scrollWidget_(nullptr)
    , layoutMaskListWidget_(nullptr)
    , topOffset_(topOffset)
    , bottomOffset_(bottomOffset)
    , maskListWidget_(nullptr)
    , direction_(QBoxLayout::TopToBottom)
    , hasLocalVideo_(false)
    , hidePreviewPrimaryTimer_(new QTimer(this))
    , backgroundWidget_(nullptr)
    , isFadedVisible_(false)
    //, animationRunning_(false)
    //, hideMaskListOnAnimationFinish_(false)
{
    // Layout
    // rootLayout -> rootWidget_ -> layoutTarget -> maskListWidget_ -> layoutMaskListWidget -> maskList_ -> Masks
    //                                                                          |-------->upButton_/downButton_
    //

    setContentsMargins(0, 0, 0, 0);

    setObjectName(qsl("MaskPanel"));

#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/mask_panel")));
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/mask_panel_linux")));
#endif


#ifndef __APPLE__
    backgroundWidget_ = new PanelBackground(this);
    backgroundWidget_->updateSizeFromParent();
#endif

    QVBoxLayout* mainLayout = Utils::emptyVLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);

    rootWidget_ = new QWidget(this);
    rootWidget_->setObjectName(qsl("MaskPanelRoot"));
    rootWidget_->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* layoutTarget = Utils::emptyVLayout();
    //layoutTarget->setAlignment(Qt::AlignCenter);
    rootWidget_->setLayout(layoutTarget);
    mainLayout->addWidget(rootWidget_);

    // This widget contains up/down buttons, mask list.
    maskListWidget_ = createMaskListWidget();

    layoutTarget->addWidget(maskListWidget_, 0, Qt::AlignCenter);

    auto closePanel = new QPushButton();
    closePanel->setProperty("CloseMaskArror", true);
    closePanel->setCursor(QCursor(Qt::PointingHandCursor));
    closePanel->setFlat(true);

    layoutTarget->addWidget(closePanel, 0, Qt::AlignCenter);

    hidePreviewPrimaryTimer_->setInterval(HIDE_PRIVIEW_PRIMARY_INTERVAL);
    hidePreviewPrimaryTimer_->setSingleShot(true);

    connect(upButton_, SIGNAL(clicked()), this, SLOT(scrollListUp()));
    connect(downButton_, SIGNAL(clicked()), this, SLOT(scrollListDown()));

    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)),
        this, SLOT(setVideoStatus(bool)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)),
        this, SLOT(updateMaskList(const voip_manager::ContactEx&)), Qt::DirectConnection);

    connect(hidePreviewPrimaryTimer_, SIGNAL(timeout()), this, SLOT(autoHidePreviewPrimary()), Qt::DirectConnection);

    //connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);

    connect(closePanel, &QPushButton::clicked, this, &Ui::MaskPanel::onHideMaskPanel, Qt::DirectConnection);

    rootEffect_ = std::make_unique<UIEffects>(*rootWidget_, true, false);

    updateMaskList();

    //rootWidget_->setFixedHeight(1);
    //rootWidget_->move(0, height());

    scrollWidget_->show();
}

Ui::MaskPanel::~MaskPanel()
{
}


void Ui::MaskPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);

    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
        {
            if (container_)
            {
                container_->raise();
                raise();
            }
        }
    }
}

void Ui::MaskPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    hidePreviewPrimaryTimer_->stop();
    emit onMouseEnter();
}

void Ui::MaskPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    hidePreviewPrimaryTimer_->start();
    emit onMouseLeave();
}

void Ui::MaskPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

void Ui::MaskPanel::setMaskList(const voip_masks::MaskList& maskList)
{
    //hideMaskListWithOutAnimation();
    clearMaskList();

    for (auto mask : maskList)
    {
        maskLayout_->addSpacing(Utils::scale_value(MASK_SPACING));
        appendItem(mask);
    }

    updateUpDownButtons();

    selectMask(nullptr);

    //if (!isFadedVisible_)
    //{
    //    isFadedVisible_ = true;
    //    fadeOut(0);
    //}

    //layout()->setEnabled(false);
}

void Ui::MaskPanel::showMaskList()
{
    //sendOpenStatistic();

    //scrollWidget_->show();

    //Ui::GetDispatcher()->getVoipController().initMaskEngine();
    //if (!animationRunning_)
    //{
    //    scrollWidget_->animationShow();
    //    animationRunning_ = true;
    //}
}


Ui::MaskWidget* Ui::MaskPanel::appendItem(const voip_masks::Mask* mask)
{
    MaskWidget* icon = new MaskWidget(mask);

    icon->setCheckable(true);
    icon->setFlat(true);
    icon->setChecked(false);
    icon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    icon->setContentsMargins(0, 0, 0, 0);

    // Disable change layout if widget was hide.
    QSizePolicy sp_retain = icon->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    icon->setSizePolicy(sp_retain);

    if (mask)
    {
        icon->setMaskEngineReady(Ui::GetDispatcher()->getVoipController().isMaskEngineEnabled());

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMaskEngineEnable(bool)),
            icon, SLOT(setMaskEngineReady(bool)), Qt::DirectConnection);
    }
    else
    {
        icon->setMaskEngineReady(true);
    }

    connect(icon, &MaskWidget::clicked, this, &MaskPanel::changedMask);
    connect(icon, &MaskWidget::loaded, this, &MaskPanel::onMaskLoaded);

    maskLayout_->addWidget(icon, 0, Qt::AlignTop);

    return icon;
}

void Ui::MaskPanel::changedMask()
{
    MaskWidget* mask = qobject_cast<MaskWidget*>(sender());

    if (mask)
    {
        if (mask->isChecked())
        {
            sendSelectMaskStatistic(mask);
            selectMask(mask);
        }
        else
        {
            selectMask(nullptr);
        }

        hidePreviewPrimaryTimer_->start();
    }
}

void Ui::MaskPanel::selectMask(MaskWidget* mask)
{
    // Reset buttons state.
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    for (auto button : maskButtons)
    {
        button->setChecked(false);
    }

    current_ = mask;

    if (mask)
    {
        if (!mask->isLoaded())
        {
            mask->load();
            return;
        }

        if (!hasLocalVideo_ && !mask->isEmptyMask())
        {
            Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
            hasLocalVideo_ = true;
        }

        {
            const QSignalBlocker blocker(mask);
            mask->setChecked(true);
        }

        QMetaObject::invokeMethod(this, "scrollToWidget", Qt::QueuedConnection,
            Q_ARG(QString, mask->name()));

        // TODO: Pass mask to main panel.
        //if (!mask->isEmptyMask())
        //{
        //    currentMaskButton_->setPixmap(mask->pixmap());
        //    emit makePreviewPrimary();
        //}
        //else
        //{
        //    // Set first mask pixmap for current mask button.
        //    auto mask = getFirstMask();
        //    auto pixmap = mask ? mask->pixmap() : QPixmap();
        //    currentMaskButton_->setPixmap(pixmap);
        //}
    }
    else
    {
        Ui::GetDispatcher()->getVoipController().loadMask("");

        //if (currentMaskButton_->pixmap().isNull())
        //{
        //    auto mask = getFirstMask();
        //    auto pixmap = mask ? mask->pixmap() : QPixmap();
        //    currentMaskButton_->setPixmap(pixmap);
        //}
    }
}

QWidget* Ui::MaskPanel::createMaskListWidget()
{
    layoutMaskListWidget_ = Utils::emptyHLayout();
    layoutMaskListWidget_->setAlignment(Qt::AlignTop);

    maskLayout_ = Utils::emptyHLayout();
    maskLayout_->setAlignment(Qt::AlignTop);
    //maskLayout_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    scrollWidget_ = new voipTools::BoundBox<QWidget>();
    scrollWidget_->setObjectName(qsl("MaskList"));
    scrollWidget_->setContentsMargins(0, 0, 0, 0);

    QWidget* res = new voipTools::BoundBox<QWidget>(QColor(255, 0, 0));
    res->setLayout(layoutMaskListWidget_);
    res->setContentsMargins(0, 0, 0, 0);

    // Scroll area with all masks.
    maskList_ = new QScrollArea(res);
    maskList_->setObjectName(qsl("MaskListScroll"));
    maskList_->setProperty("orintation", qsl("horizontal"));
    maskList_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    maskList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    maskList_->setWidgetResizable(true);
    maskList_->setWidget(scrollWidget_);
    maskList_->setContentsMargins(0, 0, 0, 0);
    maskList_->setAlignment(Qt::AlignTop);
    maskList_->setLineWidth(0);
    maskList_->setFrameShape(QFrame::NoFrame);

    scrollWidget_->setLayout(maskLayout_);

    // Scroll up/down buttons.
    upButton_ = new QPushButton();
    upButton_->setProperty("MaskArrow", qsl("left"));
    //upButton_->setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
    upButton_->setCursor(Qt::PointingHandCursor);

    downButton_ = new QPushButton();
    downButton_->setProperty("MaskArrow", qsl("right"));
    //downButton_->setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
    downButton_->setCursor(Qt::PointingHandCursor);

    // Disable change layout if widget was hide.
    QSizePolicy retainUp = upButton_->sizePolicy();
    retainUp.setRetainSizeWhenHidden(true);
    upButton_->setSizePolicy(retainUp);

    // Disable change layout if widget was hide.
    QSizePolicy retainDown = downButton_->sizePolicy();
    retainDown.setRetainSizeWhenHidden(true);
    downButton_->setSizePolicy(retainDown);

    layoutMaskListWidget_->addSpacing(Utils::scale_value(MASK_SPACING));

    // TODO: This hack, because under mac, ScrollArea has top content marging.
    constexpr auto hackLayout = platform::is_apple() ? Qt::AlignCenter : Qt::AlignTop;
    layoutMaskListWidget_->addWidget(upButton_,   0, hackLayout);
    layoutMaskListWidget_->addSpacing(-Utils::scale_value(18));
    layoutMaskListWidget_->addWidget(maskList_,   1, Qt::AlignTop);
    layoutMaskListWidget_->addSpacing(-Utils::scale_value(18));
    layoutMaskListWidget_->addWidget(downButton_, 0, hackLayout);
    layoutMaskListWidget_->addSpacing(Utils::scale_value(MASK_SPACING));

    maskList_->horizontalScrollBar()->setSingleStep(Utils::scale_value(SCROLLING_SPEED));
    maskList_->verticalScrollBar()->setSingleStep(Utils::scale_value(SCROLLING_SPEED));

    connect(maskList_->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateUpDownButtons()));
    connect(maskList_->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateUpDownButtons()));
    connect(maskList_->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateUpDownButtons()));
    connect(maskList_->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateUpDownButtons()));

    //connect(scrollWidget_, SIGNAL(animationFinished(bool)), this, SLOT(animationFinished(bool)), Qt::DirectConnection);
    //connect(scrollWidget_, SIGNAL(animationRunned(bool)), this, SLOT(animationRunned(bool)), Qt::DirectConnection);

    return res;
}

void Ui::MaskPanel::scrollListUp()
{
    maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    hidePreviewPrimaryTimer_->start();
}

void Ui::MaskPanel::scrollListDown()
{
    maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    hidePreviewPrimaryTimer_->start();
}

void Ui::MaskPanel::updateUpDownButtons()
{
    auto scroll = maskList_->horizontalScrollBar();
    upButton_->setVisible(scroll->value()   != scroll->minimum());
    downButton_->setVisible(scroll->value() != scroll->maximum());
}

void Ui::MaskPanel::setVideoStatus(bool bEnabled)
{
    hasLocalVideo_ = bEnabled;
}

void Ui::MaskPanel::updateMaskList(const voip_manager::ContactEx&  contactEx)
{
    if (contactEx.connection_count == 1)
    {
        updateMaskList();
    }
}

void Ui::MaskPanel::updateMaskList()
{
    setMaskList(Ui::GetDispatcher()->getVoipController().getMaskManager()->getAvailableMasks());
}

void Ui::MaskPanel::setTopOffset(int topOffset)
{
    topOffset_ = topOffset;
    updatePosition(*parentWidget());
}

void Ui::MaskPanel::chooseFirstMask()
{
    auto firstMaskButton = getFirstMask();
    if (firstMaskButton && !firstMaskButton->isChecked() && getSelectedWidget() == nullptr)
    {
        selectMask(firstMaskButton);
        sendSelectMaskStatistic(firstMaskButton);
    }
}

void Ui::MaskPanel::autoHidePreviewPrimary()
{
    //hideMaskList();
}

void Ui::MaskPanel::onMaskLoaded()
{
    MaskWidget* mask = qobject_cast<MaskWidget*>(sender());
    if (current_ == mask)
        selectMask(mask);
}

Ui::MaskWidget* Ui::MaskPanel::getSelectedWidget()
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (mask->isChecked())
        {
            selectedMask = mask;
        }
    });

    return selectedMask;
}

Ui::MaskWidget* Ui::MaskPanel::getFirstMask()
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    if (maskButtons.size() > 0)
    {
        return maskButtons[0];
    }

    return nullptr;
}


void Ui::MaskPanel::callDestroyed()
{
    onHideMaskPanel();
    //hideMaskListWithOutAnimation();
    clearMaskList();
}

void Ui::MaskPanel::clearMaskList()
{
    // Remove all current masks.
    QLayoutItem* item = NULL;
    while ((item = maskLayout_->takeAt(0)) != NULL)
    {
        delete item->widget();
        delete item;
    }
}

void Ui::MaskPanel::resizeEvent(QResizeEvent * event)
{
    if (backgroundWidget_)
    {
        backgroundWidget_->updateSizeFromParent();
    }
}

void Ui::MaskPanel::wheelEvent(QWheelEvent * event)
{
    if (scrollWidget_->isVisible())
    {
        // Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
        // But we made the same scroll as default.
        int mouseWheelValue = event->delta() / 30;

        while (mouseWheelValue != 0)
        {
            if (mouseWheelValue < 0)
            {
                maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
                maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
            else
            {
                maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
                maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }

            (mouseWheelValue > 0 ? mouseWheelValue-- : mouseWheelValue++);
        }

        hidePreviewPrimaryTimer_->start();
    }
}

void Ui::MaskPanel::sendOpenStatistic()
{
    core::stats::event_props_type props;

    bool bLoaded = true;

    enumerateMask([&bLoaded](MaskWidget* mask)
        {
            bLoaded = bLoaded && mask->isLoaded();
        });

    props.emplace_back("Masks_Ready", bLoaded ? "yes" : "no");
    props.emplace_back("Model_Ready", Ui::GetDispatcher()->getVoipController().isMaskEngineEnabled() ? "yes" : "no");

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::masks_open, props);
}

void Ui::MaskPanel::enumerateMask(std::function<void(MaskWidget* mask)> func)
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();

    std::for_each(maskButtons.begin(), maskButtons.end(), func);
}

void Ui::MaskPanel::sendSelectMaskStatistic(MaskWidget* mask)
{
    core::stats::event_props_type props;

    if (mask && !mask->name().isEmpty())
    {
        bool isAccepted = false;
        getCallStatus(isAccepted);

        props.emplace_back("name", mask->name().toUtf8().data());
        props.emplace_back("when", isAccepted ? "call_started" : "call_outgoing");

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::masks_select, props);
    }
}

void Ui::MaskPanel::scrollToWidget(QString maskName)
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (maskName == mask->name())
        {
            selectedMask = mask;
        }
    });

    if (selectedMask)
    {
        maskList_->ensureWidgetVisible(selectedMask, 0, 0);
    }
}

bool Ui::MaskPanel::isOpened()
{
    return isVisible();
}

void Ui::MaskPanel::showEvent(QShowEvent* event)
{
    //showMaskList();
    sendOpenStatistic();

    Ui::GetDispatcher()->getVoipController().initMaskEngine();
}

void Ui::MaskPanel::hideEvent(QHideEvent*)
{
    isFadedVisible_ = false;
}

void Ui::MaskPanel::fadeIn(unsigned int duration)
{
#ifndef __linux__
    if (!isFadedVisible_ /*&& !rootEffect_->isGeometryRunning()*/)
    {
        isFadedVisible_ = true;
        //BaseBottomVideoPanel::fadeIn(duration);
        //rootEffect_->geometryTo(QRect(0, 0, width(), height()), duration);
        rootEffect_->fadeIn(duration);
    }
#endif
}

void Ui::MaskPanel::fadeOut(unsigned int duration)
{
#ifndef __linux__
    if (isFadedVisible_ /*&& !rootEffect_->isGeometryRunning()*/)
    {
        isFadedVisible_ = false;
        //BaseBottomVideoPanel::fadeOut(duration);
        //rootEffect_->geometryTo(QRect(0, height(), width(), height()), duration);
        rootEffect_->fadeOut(duration);
    }
#endif
}

void Ui::MaskPanel::updatePosition(const QWidget& parent)
{
    BaseBottomVideoPanel::updatePosition(parent);

    rootWidget_->setFixedWidth(width());

    //if (!rootEffect_->isGeometryRunning())
    //{
    //    if (isFadedVisible_)
    //    {
    //        rootWidget_->move(0, 0);
    //        rootWidget_->setFixedSize(size());
    //    }
    //    else
    //    {
    //        rootWidget_->move(0, height());
    //        rootWidget_->setFixedSize(size());
    //    }
    //}
}

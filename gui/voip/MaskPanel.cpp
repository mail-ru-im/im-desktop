#include "stdafx.h"
#include "MaskPanel.h"
#include "DetachedVideoWnd.h"
#include "../utils/utils.h"
#include "../core_dispatcher.h"
#include "MaskManager.h"

namespace
{
    auto getMaskPanelRootMargins() noexcept
    {
        if constexpr (platform::is_linux())
            return QMargins(0, Utils::scale_value(12), 0, 0);
        else
            return QMargins();
    }
}

// Items view settings.
enum
{
    SELECTED_ITEM_BORDER = 1,
    BORDER_SHADOW = 1,
    AVATAR_SIZE = 33,
    NORMAL_ITEM_SIZE = 37,
    SELECTED_ITEM_SIZE = NORMAL_ITEM_SIZE,
    PANEL_SIZE = 116,
    SCROLLING_SPEED = 10,
    MASK_SPACING = 7,
    MASKS_ANIMATION_MOVE_SPEED = 25,
    MASKS_ANIMATION_RESIZE_SPEED = 32,
};

Ui::MaskWidget::MaskWidget(voip_masks::Mask* _mask)
    : mask_(_mask)
    , closeIcon_(Utils::parse_image_name(qsl(":/voip/close_mask_100")))
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

    setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));

    resizeAnimationMin_ = new QPropertyAnimation(this, QByteArrayLiteral("minimumSize"), this);
    resizeAnimationMin_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimationMin_->setDuration(100);

    resizeAnimationMax_ = new QPropertyAnimation(this, QByteArrayLiteral("maximumSize"), this);
    resizeAnimationMax_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimationMax_->setDuration(100);

    setContentsMargins(0, 0, 0, 0);
}

void Ui::MaskWidget::paintEvent(QPaintEvent*)
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
        QColor maskBrushColor(u"#000000");
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
        QColor maskDisabledColor(u"#000000");
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

        QPen pen(QColor(u"#ffffff"));
        pen.setWidth(Utils::scale_value(SELECTED_ITEM_BORDER) * 2);

        painter.setPen(pen);

        int borderSelect = Utils::scale_value(SELECTED_ITEM_BORDER + BORDER_SHADOW);
        auto borderRect = contentsRect().marginsRemoved(QMargins(borderSelect, borderSelect, borderSelect, borderSelect));

        // Draw outer border.
        painter.drawEllipse(borderRect);

        // Draw border shadow
        int borderShadow = Utils::scale_value(BORDER_SHADOW);
        auto borderShadowRect = contentsRect().marginsRemoved(QMargins(borderShadow, borderShadow, borderShadow, borderShadow));
        QColor shadowColor(u"#000000");
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

const QString& Ui::MaskWidget::maskPath() const
{
    return maskPath_;
}

void Ui::MaskWidget::setPixmap(const QPixmap& _image)
{
    image_ = _image;
    update();
}

const QPixmap& Ui::MaskWidget::pixmap() const
{
    return image_;
}

void Ui::MaskWidget::setChecked(bool _check)
{
    QPushButton::setChecked(_check);

    QSize targetSize = Utils::scale_value(_check ? QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE) :
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

    if (_check)
    {
        if (isEnabled())
            Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        else
            applyWhenEnabled_ = true;
    }
    else
    {
        applyWhenEnabled_ = false;
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
        name_ = mask_->id();
        updateLoadingProgress(!maskPath_.isEmpty() ? 100 : 0);
        update();
    }
}

void Ui::MaskWidget::updateLoadingProgress(unsigned _progress)
{
    loadingProgress_ = _progress / 100.0f;
    update();
}

void Ui::MaskWidget::setMaskEngineReady(bool _ready)
{
    maskEngineReady_ = _ready;
    updateJsonPath();
    update();
}

void Ui::MaskWidget::changeEvent(QEvent* _event)
{
    if (_event->type() == QEvent::EnabledChange && isEnabled() && applyWhenEnabled_)
    {
        Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        applyWhenEnabled_ = false;
    }
    QPushButton::changeEvent(_event);
}

bool Ui::MaskWidget::isEmptyMask() const
{
    return mask_ == nullptr;
}

bool Ui::MaskWidget::isLoaded() const
{
    return mask_ == nullptr || !maskPath_.isEmpty();
}

const QString& Ui::MaskWidget::name() const
{
    return name_;
}

void Ui::MaskWidget::setXPos(int _x)
{
    move(_x, y());
}

void Ui::MaskWidget::setYCenter(int _y)
{
    move(x(), _y - height() / 2);
}

int  Ui::MaskWidget::yCenter() const
{
    return y() + height() / 2;
}

void Ui::MaskWidget::setYPos(int _y)
{
    move(x(), _y);
}

void Ui::MaskWidget::setXCenter(int _x)
{
    move(_x - width() / 2, y());
}

int  Ui::MaskWidget::xCenter() const
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

Ui::MaskPanel::MaskPanel(QWidget* _parent, QWidget* _container, int _topOffset, int _bottomOffset) :
#ifdef __APPLE__
    // We use Qt::WindowDoesNotAcceptFocus for mac, bcause it fix ghost window after call stop on Sierra
    BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    BaseBottomVideoPanel(_parent, Qt::Widget)
#else
    BaseBottomVideoPanel(_parent)
#endif
    , container_(_container)
    , rootWidget_(nullptr)
    , scrollWidget_(nullptr)
    , layoutMaskListWidget_(nullptr)
    , topOffset_(_topOffset)
    , bottomOffset_(_bottomOffset)
    , maskListWidget_(nullptr)
    , direction_(QBoxLayout::TopToBottom)
    , hasLocalVideo_(false)
    , backgroundWidget_(nullptr)
    , isFadedVisible_(false)
{
    // Layout
    // rootLayout -> rootWidget_ -> layoutTarget -> maskListWidget_ -> layoutMaskListWidget -> maskList_ -> Masks
    //                                                                          |-------->upButton_/downButton_

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

    auto mainLayout = Utils::emptyVLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);

    rootWidget_ = new QWidget(this);
    rootWidget_->setObjectName(qsl("MaskPanelRoot"));
    rootWidget_->setContentsMargins(getMaskPanelRootMargins());

    auto layoutTarget = Utils::emptyVLayout();
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

    connect(upButton_, &QPushButton::clicked, this, &MaskPanel::scrollListUp);
    connect(downButton_, &QPushButton::clicked, this, &MaskPanel::scrollListDown);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &MaskPanel::setVideoStatus);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallCreated, this, &MaskPanel::onVoipCallCreated);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncomingAccepted, this, &MaskPanel::onVoipCallIncomingAccepted);

    connect(closePanel, &QPushButton::clicked, this, &Ui::MaskPanel::onHideMaskPanel);

    rootEffect_ = std::make_unique<UIEffects>(*rootWidget_, true, false);

    updateMaskList();

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
    Q_EMIT onMouseEnter();
}

void Ui::MaskPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    Q_EMIT onMouseLeave();
}

void Ui::MaskPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
        Q_EMIT onkeyEscPressed();
}

void Ui::MaskPanel::setMaskList(const voip_masks::MaskList& _maskList)
{
    clearMaskList();

    for (auto& mask : _maskList)
    {
        maskLayout_->addSpacing(Utils::scale_value(MASK_SPACING));
        appendItem(mask);
    }

    updateUpDownButtons();

    selectMask(nullptr);
}

void Ui::MaskPanel::showMaskList()
{
}

Ui::MaskWidget* Ui::MaskPanel::appendItem(voip_masks::Mask* _mask)
{
    auto icon = new MaskWidget(_mask);

    icon->setCheckable(true);
    icon->setFlat(true);
    icon->setChecked(false);
    icon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    icon->setContentsMargins(0, 0, 0, 0);

    // Disable change layout if widget was hide.
    QSizePolicy sp_retain = icon->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    icon->setSizePolicy(sp_retain);

    if (_mask)
    {
        icon->setMaskEngineReady(Ui::GetDispatcher()->getVoipController().isMaskEngineEnabled());
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMaskEngineEnable, icon, &MaskWidget::setMaskEngineReady);
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
    if (auto mask = qobject_cast<MaskWidget*>(sender()))
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
    }
}

void Ui::MaskPanel::selectMask(MaskWidget* _mask)
{
    // Reset buttons state.
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    for (auto button : maskButtons)
        button->setChecked(false);

    current_ = _mask;

    if (_mask)
    {
        if (!_mask->isLoaded())
        {
            _mask->load();
            return;
        }

        if (!hasLocalVideo_ && !_mask->isEmptyMask())
        {
            Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
            hasLocalVideo_ = true;
        }

        {
            const QSignalBlocker blocker(_mask);
            _mask->setChecked(true);
        }

        QMetaObject::invokeMethod(this, [this, name = _mask->name()]() { scrollToWidget(name); }, Qt::QueuedConnection);
    }
    else
    {
        Ui::GetDispatcher()->getVoipController().loadMask("");
    }
}

QWidget* Ui::MaskPanel::createMaskListWidget()
{
    layoutMaskListWidget_ = Utils::emptyHLayout();
    layoutMaskListWidget_->setAlignment(Qt::AlignTop);

    maskLayout_ = Utils::emptyHLayout();
    maskLayout_->setAlignment(Qt::AlignTop);

    scrollWidget_ = new QWidget();
    scrollWidget_->setObjectName(qsl("MaskList"));
    scrollWidget_->setContentsMargins(0, 0, 0, 0);

    auto res = new QWidget();
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
    upButton_->setCursor(Qt::PointingHandCursor);

    downButton_ = new QPushButton();
    downButton_->setProperty("MaskArrow", qsl("right"));
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

    connect(maskList_->verticalScrollBar(), &QScrollBar::valueChanged, this, &MaskPanel::updateUpDownButtons);
    connect(maskList_->verticalScrollBar(), &QScrollBar::rangeChanged, this, &MaskPanel::updateUpDownButtons);
    connect(maskList_->horizontalScrollBar(), &QScrollBar::valueChanged, this, &MaskPanel::updateUpDownButtons);
    connect(maskList_->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &MaskPanel::updateUpDownButtons);

    return res;
}

void Ui::MaskPanel::scrollListUp()
{
    maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
}

void Ui::MaskPanel::scrollListDown()
{
    maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
}

void Ui::MaskPanel::updateUpDownButtons()
{
    auto scroll = maskList_->horizontalScrollBar();
    upButton_->setVisible(scroll->value() != scroll->minimum());
    downButton_->setVisible(scroll->value() != scroll->maximum());
}

void Ui::MaskPanel::setVideoStatus(bool bEnabled)
{
    hasLocalVideo_ = bEnabled;
}

void Ui::MaskPanel::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
{
    if (!_contactEx.incoming)
        updateMaskList();
}

void Ui::MaskPanel::onVoipCallIncomingAccepted(const std::string& _callId)
{
    updateMaskList();
}

void Ui::MaskPanel::updateMaskList()
{
    Ui::GetDispatcher()->getVoipController().getMaskManager()->loadPreviews();
    setMaskList(Ui::GetDispatcher()->getVoipController().getMaskManager()->getAvailableMasks());
}

void Ui::MaskPanel::setTopOffset(int _topOffset)
{
    topOffset_ = _topOffset;
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

void Ui::MaskPanel::onMaskLoaded()
{
    auto mask = qobject_cast<MaskWidget*>(sender());
    if (current_ == mask)
        selectMask(mask);
}

Ui::MaskWidget* Ui::MaskPanel::getSelectedWidget()
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (mask->isChecked())
            selectedMask = mask;
    });

    return selectedMask;
}

Ui::MaskWidget* Ui::MaskPanel::getFirstMask()
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    if (maskButtons.size() > 0)
        return maskButtons[0];

    return nullptr;
}


void Ui::MaskPanel::callDestroyed()
{
    onHideMaskPanel();
    clearMaskList();

    Ui::GetDispatcher()->getVoipController().getMaskManager()->resetMasks();
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

void Ui::MaskPanel::resizeEvent(QResizeEvent*)
{
    if (backgroundWidget_)
        backgroundWidget_->updateSizeFromParent();
}

void Ui::MaskPanel::wheelEvent(QWheelEvent* _event)
{
    if (scrollWidget_->isVisible())
    {
        // Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
        // But we made the same scroll as default.
        int mouseWheelValue = _event->delta() / 30;

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

void Ui::MaskPanel::enumerateMask(std::function<void(MaskWidget* mask)> _func)
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    std::for_each(maskButtons.begin(), maskButtons.end(), std::move(_func));
}

void Ui::MaskPanel::sendSelectMaskStatistic(MaskWidget* _mask)
{
    if (_mask && !_mask->name().isEmpty())
    {
        bool isAccepted = false;
        getCallStatus(isAccepted);

        core::stats::event_props_type props;
        props.emplace_back("name", _mask->name().toUtf8().data());
        props.emplace_back("when", isAccepted ? "call_started" : "call_outgoing");

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::masks_select, props);
    }
}

void Ui::MaskPanel::scrollToWidget(const QString& _maskName)
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (_maskName == mask->name())
            selectedMask = mask;
    });
    if (selectedMask)
        maskList_->ensureWidgetVisible(selectedMask, 0, 0);
}

bool Ui::MaskPanel::isOpened()
{
    return isVisible();
}

void Ui::MaskPanel::showEvent(QShowEvent*)
{
    sendOpenStatistic();
    Ui::GetDispatcher()->getVoipController().initMaskEngine();
}

void Ui::MaskPanel::hideEvent(QHideEvent*)
{
    isFadedVisible_ = false;
}

void Ui::MaskPanel::fadeIn(unsigned int _duration)
{
    if constexpr (!platform::is_linux())
    {
        if (!isFadedVisible_)
        {
            isFadedVisible_ = true;
            rootEffect_->fadeIn(_duration);
        }
    }
}

void Ui::MaskPanel::fadeOut(unsigned int _duration)
{
    if constexpr (!platform::is_linux())
    {
        if (isFadedVisible_)
        {
            isFadedVisible_ = false;
            rootEffect_->fadeOut(_duration);
        }
    }
}

void Ui::MaskPanel::updatePosition(const QWidget& _parent)
{
    BaseBottomVideoPanel::updatePosition(_parent);
    rootWidget_->setFixedWidth(width());
}

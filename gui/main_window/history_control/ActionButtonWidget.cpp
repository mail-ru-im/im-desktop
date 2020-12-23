#include "stdafx.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "../../utils/utils.h"

#include "ActionButtonWidgetLayout.h"
#include "MessageStyle.h"

#include "ActionButtonWidget.h"

UI_NS_BEGIN

namespace
{
    constexpr auto PROGRESS_BAR_ANGLE_MAX = 360;

    constexpr auto PROGRESS_BAR_MIN_PERCENTAGE = 0.03;

    int32_t getMaxIconDimension();

    constexpr double animationDuration() noexcept
    {
        return 700;
    }
}

namespace ActionButtonResource
{
    ResourceSet::ResourceSet(
        const ResId startButtonImage,
        const ResId startButtonImageActive,
        const ResId startButtonImageHover,
        const ResId stopButtonImage,
        const ResId stopButtonImageActive,
        const ResId stopButtonImageHover)
        : StartButtonImage_(startButtonImage)
        , StartButtonImageActive_(startButtonImageActive)
        , StartButtonImageHover_(startButtonImageHover)
        , StopButtonImage_(stopButtonImage)
        , StopButtonImageActive_(stopButtonImageActive)
        , StopButtonImageHover_(stopButtonImageHover)
    {
        assert(StartButtonImage_ > ResId::Min);
        assert(StartButtonImage_ < ResId::Max);
        assert(StartButtonImageHover_ > ResId::Min);
        assert(StartButtonImageHover_ < ResId::Max);
        assert(StartButtonImageActive_ > ResId::Min);
        assert(StartButtonImageActive_ < ResId::Max);
        assert(StopButtonImage_ > ResId::Min);
        assert(StopButtonImage_ < ResId::Max);
        assert(StopButtonImageHover_ > ResId::Min);
        assert(StopButtonImageHover_ < ResId::Max);
        assert(StopButtonImageActive_ > ResId::Min);
        assert(StopButtonImageActive_ < ResId::Max);
    }

    const ResourceSet ResourceSet::Play_(
        Themes::PixmapResourceId::FileSharingMediaPlay,
        Themes::PixmapResourceId::FileSharingMediaPlayHover,
        Themes::PixmapResourceId::FileSharingMediaPlayHover,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);

    const ResourceSet ResourceSet::DownloadMediaFile_(
        Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
        Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
        Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);

    const ResourceSet ResourceSet::DownloadVideo_(
        Themes::PixmapResourceId::FileSharingVideoDownload,
        Themes::PixmapResourceId::FileSharingVideoDownloadActive,
        Themes::PixmapResourceId::FileSharingVideoDownloadHover,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);
}

QSize ActionButtonWidget::getMaxIconSize()
{
    const QSize maxSize(
        getMaxIconDimension(),
        getMaxIconDimension());

    return Utils::scale_value(maxSize);
}

ActionButtonWidget::ActionButtonWidget(const ActionButtonResource::ResourceSet &resourceIds, QWidget *parent)
    : QWidget(parent)
    , AnimationStartTimer_(nullptr)
    , ProgressBaseAngleAnimation_(nullptr)
    , ResourceIds_(resourceIds)
    , Progress_(0)
    , ProgressBarAngle_(0)
    , ResourcesInitialized_(false)
    , IsHovered_(false)
    , IsPressed_(false)
    , IsAnimating_(false)
    , Layout_(nullptr)
    , animation_(nullptr)
{
    initResources();

    ProgressTextFont_ = MessageStyle::getRotatingProgressBarTextFont();

    ProgressPen_ = MessageStyle::getRotatingProgressBarPen();

    selectCurrentIcon();

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    Layout_ = new ActionButtonWidgetLayout(this);
    setLayout(Layout_);

    Layout_->setGeometry(QRect());

    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void ActionButtonWidget::ensureAnimationInitialized()
{
    if (animation_)
        return;

    animation_ = new QVariantAnimation(this);
    animation_->setStartValue(0.0);
    animation_->setEndValue(360.0);
    animation_->setDuration(animationDuration());
    animation_->setLoopCount(-1);
    connect(animation_, &QVariantAnimation::valueChanged, this, qOverload<>(&ActionButtonWidget::update));
}

void ActionButtonWidget::createAnimationStartTimer()
{
    if (AnimationStartTimer_)
        return;

    AnimationStartTimer_ = new QTimer(this);
    AnimationStartTimer_->setSingleShot(true);

    QObject::connect(
        AnimationStartTimer_,
        &QTimer::timeout,
        this,
        &ActionButtonWidget::onAnimationStartTimeout);
}

void ActionButtonWidget::drawIcon(QPainter &p)
{
    assert(Layout_);
    assert(CurrentIcon_);

    CurrentIcon_->Draw(p, Layout_->getIconRect());
}

void ActionButtonWidget::drawProgress(QPainter &p)
{
    assert(IsAnimating_);

    const auto QT_ANGLE_MULT = 16;

    const auto baseAngle = animation_ ? animation_->currentValue().toDouble() * QT_ANGLE_MULT : 0.0;

    const auto progress = std::max(Progress_, PROGRESS_BAR_MIN_PERCENTAGE);

    const auto progressAngle = (int)std::ceil(progress * PROGRESS_BAR_ANGLE_MAX * QT_ANGLE_MULT);

    Utils::PainterSaver ps(p);
    p.setPen(ProgressPen_);
    p.drawArc(Layout_->getProgressRect(), -baseAngle, -progressAngle);
}

void ActionButtonWidget::drawProgressText(QPainter &_p)
{
    const auto &textRect = Layout_->getProgressTextRect();

    Utils::PainterSaver p(_p);

    auto backgroundRect = textRect.adjusted(-MessageStyle::getProgressTextRectHMargin(), -MessageStyle::getProgressTextRectVMargin(),
                                             MessageStyle::getProgressTextRectHMargin(), MessageStyle::getProgressTextRectVMargin());

    auto backgroundRectRadius = backgroundRect.height() / 2;

    QPainterPath path;
    path.addRoundedRect(backgroundRect, backgroundRectRadius, backgroundRectRadius);

    _p.fillPath(path, QColor(0, 0, 0, 255 * 0.5));

    _p.setFont(ProgressTextFont_);
    _p.setPen(MessageStyle::getRotatingProgressBarTextPen());
    _p.drawText(textRect, Qt::AlignHCenter, IsAnimating_ ? ProgressText_ : DefaultProgressText_);
}

int ActionButtonWidget::getProgressBarBaseAngle() const
{
    assert(ProgressBarAngle_ >= 0);
    assert(ProgressBarAngle_ <= PROGRESS_BAR_ANGLE_MAX);

    return ProgressBarAngle_;
}

void ActionButtonWidget::setProgressBarBaseAngle(const int32_t _val)
{
    assert(_val >= 0);
    assert(_val <= PROGRESS_BAR_ANGLE_MAX);

    const auto isAngleChanged = (ProgressBarAngle_ != _val);

    ProgressBarAngle_ = _val;

    if (isAngleChanged)
        update();
}

QPoint ActionButtonWidget::getCenterBias() const
{
    assert(Layout_);

    const QRect geometry(QPoint(), sizeHint());
    const auto geometryCenter = geometry.center();

    const auto iconCenter = Layout_->getIconRect().center();

    const auto direction = (iconCenter - geometryCenter);
    return direction;
}

QSize ActionButtonWidget::getIconSize() const
{
    assert(CurrentIcon_);
    return CurrentIcon_->GetSize();
}

QPoint ActionButtonWidget::getLogicalCenter() const
{
    assert(Layout_);

    return Layout_->getIconRect().center();
}

const QString& ActionButtonWidget::getProgressText() const
{
    if (!IsAnimating_ && !DefaultProgressText_.isEmpty())
        return DefaultProgressText_;
    else
        return ProgressText_;
}

const QFont& ActionButtonWidget::getProgressTextFont() const
{
    return ProgressTextFont_;
}

const ActionButtonResource::ResourceSet& ActionButtonWidget::getResourceSet() const
{
    return ResourceIds_;
}

void ActionButtonWidget::setProgress(const double progress)
{
    assert(progress >= 0);
    assert(progress <= 1.01);

    const auto isProgressChanged = (std::abs(Progress_ - progress) >= 0.01);
    if (!isProgressChanged)
        return;

    Progress_ = progress;

    if (IsAnimating_)
        update();
}

void ActionButtonWidget::setProgressPen(const QColor color, const double a, const double width)
{
    assert(a >= 0);
    assert(a <= 1);
    assert(width > 0);
    assert(width < 5);

    QColor progressColor(color);
    progressColor.setAlphaF(a);
    QBrush progressBrush(progressColor);

    ProgressPen_ = QPen(progressBrush, width);
}

void ActionButtonWidget::setProgressText(const QString &progressText)
{
    assert(!progressText.isEmpty());

    if (ProgressText_ == progressText)
        return;

    ProgressText_ = progressText;

    Layout_->setGeometry(QRect());

    updateGeometry();

    update();
}

void ActionButtonWidget::setDefaultProgressText(const QString &progressText)
{
    assert(!progressText.isEmpty());

    if (DefaultProgressText_ == progressText)
        return;

    DefaultProgressText_ = progressText;

    Layout_->setGeometry(QRect());

    updateGeometry();

    update();
}

void ActionButtonWidget::setResourceSet(const ActionButtonResource::ResourceSet &resourceIds)
{
    ResourceIds_ = resourceIds;

    ResourcesInitialized_ = false;

    initResources();

    selectCurrentIcon();

    resetLayoutGeometry();
}

QSize ActionButtonWidget::sizeHint() const
{
    assert(Layout_);

    return Layout_->sizeHint();
}

void ActionButtonWidget::startAnimation(const int32_t _delay)
{
    assert(_delay >= 0);

    if (IsAnimating_)
        return;

    if (_delay > 0)
    {
        createAnimationStartTimer();

        AnimationStartTimer_->stop();
        AnimationStartTimer_->setInterval(_delay);
        AnimationStartTimer_->start();
    }
    else
    {
        onAnimationStartTimeout();
    }
}

void ActionButtonWidget::stopAnimation()
{
    if (AnimationStartTimer_)
        AnimationStartTimer_->stop();

    if (!IsAnimating_)
        return;

    stopProgressBarAnimation();

    IsAnimating_ = false;

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::onVisibilityChanged(const bool isVisible)
{
    if (isVisible)
        resumeAnimation();
    else
        pauseAnimation();
}

bool ActionButtonWidget::isWaitingForDelay()
{
    return AnimationStartTimer_ && AnimationStartTimer_->isActive();
}

void ActionButtonWidget::pauseAnimation()
{
    ensureAnimationInitialized();
    if (animation_->state() == QAbstractAnimation::Running)
        animation_->pause();
}

void ActionButtonWidget::resumeAnimation()
{
    ensureAnimationInitialized();
    if (animation_->state() == QAbstractAnimation::Paused)
        animation_->resume();
}

void ActionButtonWidget::enterEvent(QEvent *)
{
    IsHovered_ = true;

    if (selectCurrentIcon())
        resetLayoutGeometry();
}

void ActionButtonWidget::hideEvent(QHideEvent* e)
{
    QWidget::hideEvent(e);
    IsHovered_ = false;
    IsPressed_ = false;

    if (selectCurrentIcon())
        resetLayoutGeometry();

    pauseAnimation();
}

void ActionButtonWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    resumeAnimation();
}

void ActionButtonWidget::leaveEvent(QEvent *)
{
    IsHovered_ = false;
    IsPressed_ = false;

    if (selectCurrentIcon())
        resetLayoutGeometry();
}

#ifdef __APPLE__
/*
https://jira.mail.ru/browse/IMDESKTOP-3109
normally clicking on a widget causes events like: press -> release. but on sierra (b5-b6) we got: press -> move -> release.
i hope it's a temporary case because before b5 it was ok.
*/
namespace { static auto mousePressStartPoint = QPoint(); }
#endif

void ActionButtonWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (IsPressed_)
    {
#ifdef __APPLE__
        if (!((QCursor::pos() - mousePressStartPoint).manhattanLength() <= 2))
#endif
        {
            Q_EMIT dragSignal();
            IsPressed_ = false;
        }
    }
    QWidget::mouseMoveEvent(e);
}

void ActionButtonWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    IsPressed_ = false;

    if (event->button() == Qt::LeftButton)
    {
#ifdef __APPLE__
        mousePressStartPoint = QCursor::pos();
#endif
        event->accept();
        IsPressed_ = true;
    }

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    if ((event->button() == Qt::LeftButton) && IsPressed_)
    {
        event->accept();

        if (IsAnimating_)
        {
            Q_EMIT stopClickedSignal(event->globalPos());
        }
        else
        {
            Q_EMIT startClickedSignal(event->globalPos());
        }
    }

    IsPressed_ = false;

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawIcon(p);

    if (IsAnimating_ && !ProgressText_.isEmpty() || !DefaultProgressText_.isEmpty())
        drawProgressText(p);

    if (IsAnimating_)
        drawProgress(p);
}

void ActionButtonWidget::initResources()
{
    if (ResourcesInitialized_)
        return;

    ResourcesInitialized_ = true;

    using namespace Themes;

    StartButtonImage_ = GetPixmap(ResourceIds_.StartButtonImage_);
    StartButtonImageHover_ = GetPixmap(ResourceIds_.StartButtonImageHover_);
    StartButtonImageActive_ = GetPixmap(ResourceIds_.StartButtonImageActive_);

    StopButtonImage_ = GetPixmap(ResourceIds_.StopButtonImage_);
    StopButtonImageHover_ = GetPixmap(ResourceIds_.StopButtonImageHover_);
    StopButtonImageActive_ = GetPixmap(ResourceIds_.StopButtonImageActive_);
}

void ActionButtonWidget::resetLayoutGeometry()
{
    Layout_->setGeometry(geometry());
    resize(sizeHint());
    updateGeometry();

    update();
}

bool ActionButtonWidget::selectCurrentIcon()
{
    auto icon = (IsAnimating_ ? StopButtonImage_ : StartButtonImage_);

    if (IsAnimating_)
    {
        if (IsHovered_)
            icon = StopButtonImageHover_;

        if (IsPressed_)
            icon = StopButtonImageActive_;
    }
    else
    {
        if (IsHovered_)
            icon = StartButtonImageHover_;

        if (IsPressed_)
            icon = StartButtonImageActive_;
    }

    const auto iconChanged = (icon != CurrentIcon_);

    CurrentIcon_ = icon;
    assert(CurrentIcon_);
    assert(CurrentIcon_->GetWidth() <= getMaxIconDimension());
    assert(CurrentIcon_->GetHeight() <= getMaxIconDimension());

    return iconChanged;
}

void ActionButtonWidget::startProgressBarAnimation()
{
    stopProgressBarAnimation();

    ensureAnimationInitialized();
    IsAnimating_ = true;
    animation_->start();
}

void ActionButtonWidget::stopProgressBarAnimation()
{
    if (animation_)
        animation_->stop();
}

void ActionButtonWidget::onAnimationStartTimeout()
{
    if (IsAnimating_)
        return;

    startProgressBarAnimation();

    selectCurrentIcon();

    resetLayoutGeometry();

    Q_EMIT internallyChangedSignal();
}

namespace
{
    int32_t getMaxIconDimension()
    {
        return Utils::scale_value(72);
    }
}

UI_NS_END

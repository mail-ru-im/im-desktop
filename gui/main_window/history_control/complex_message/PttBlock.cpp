#include "stdafx.h"

#include "../common.shared/config/config.h"
#include "../../../../corelib/enumerations.h"

#include "../../../controls/TextUnit.h"
#include "../../../controls/TooltipWidget.h"

#include "../../../core_dispatcher.h"
#include "../../../fonts.h"
#include "../../../gui_settings.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/features.h"
#include "../../../styles/ThemeParameters.h"
#include "../../../cache/ColoredCache.h"

#include "../../input_widget/InputWidgetUtils.h"

#ifndef STRIP_AV_MEDIA
#include "../../../media/ptt/AudioUtils.h"
#include "../../sounds/SoundsManager.h"
#endif // !STRIP_AV_MEDIA

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"
#include "../MessageStatusWidget.h"
#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "PttBlockLayout.h"


#include "PttBlock.h"

namespace
{
    QString formatDuration(const int32_t _mSeconds)
    {
#ifndef STRIP_AV_MEDIA
        return ptt::formatDuration(std::chrono::milliseconds(_mSeconds));
#else
        return qsl("00:00");
#endif
    }

    QSize getButtonSize()
    {
        return Utils::scale_value(QSize(40, 40));
    }

    QSize getPlayIconSize()
    {
        return QSize(18, 18);
    }

    int getBulletClickableRadius()
    {
        return Utils::scale_value(32);
    }

    int getBulletSize(bool _hovered)
    {
        return _hovered ? Utils::scale_value(10) : Utils::scale_value(8);
    }

    constexpr std::chrono::milliseconds getMaxPttLength() noexcept
    {
#ifndef STRIP_AV_MEDIA
        return ptt::maxDuration();
#else
        return std::chrono::seconds::zero();
#endif
    }

    constexpr std::chrono::milliseconds animDuration = std::chrono::seconds(2);
    constexpr std::chrono::milliseconds animDownloadDelay = std::chrono::milliseconds(300);
    constexpr auto QT_ANGLE_MULT = 16;
    constexpr double idleProgressValue = 0.75;
}

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace PttDetails
{
    struct PlayButtonIcons
    {
        QPixmap play_;
        QPixmap pause_;

        void fill(const QColor& _color)
        {
            const auto makeIcon = [](const QString& _path, const QColor& _clr)
            {
                return Utils::renderSvgScaled(_path, getPlayIconSize(), _clr);
            };

            play_ = makeIcon(qsl(":/videoplayer/video_play"), _color);
            pause_ = makeIcon(qsl(":/videoplayer/video_pause"), _color);
        }
    };

    PlayButton::PlayButton(QWidget* _parent, const QString& _aimId)
        : ClickableWidget(_parent)
        , aimId_(_aimId)
        , isPressed_(false)
        , state_(ButtonState::play)
    {
        updateStyle();
        setFixedSize(getButtonSize());
        connect(this, &PlayButton::hoverChanged, this, qOverload<>(&PlayButton::update));
        connect(this, &PlayButton::pressed, this, [this]() { setPressed(true); });
        connect(this, &PlayButton::released, this, [this]() { setPressed(false); });
    }

    void PlayButton::setPressed(const bool _isPressed)
    {
        if (isPressed_ != _isPressed)
        {
            isPressed_ = _isPressed;
            update();
        }

        Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimId_);
    }

    void PlayButton::setState(const ButtonState _state)
    {
        if (state_ != _state)
        {
            state_ = _state;
            update();
        }
    }

    void PlayButton::updateStyle()
    {
        im_assert(!aimId_.isEmpty());

        const auto params = Styling::getParameters(aimId_);
        normal_  = params.getColor(Styling::StyleVariable::PRIMARY);
        hovered_ = params.getColor(Styling::StyleVariable::PRIMARY_HOVER);
        pressed_ = params.getColor(Styling::StyleVariable::PRIMARY_ACTIVE);

        update();
    }

    void PlayButton::paintEvent(QPaintEvent* _e)
    {
        static Utils::ColoredCache<PlayButtonIcons> iconsNormal(Styling::StyleVariable::BASE_GLOBALWHITE);

        QColor bgColor;
        if (isHovered())
        {
            if (isPressed_)
                bgColor = pressed_;
            else
                bgColor = hovered_;
        }
        else
        {
            bgColor = normal_;
        }

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.setBrush(bgColor);
        p.drawEllipse(rect());

        const auto& cache = iconsNormal;
        const auto& icons = cache.get(aimId_);
        const auto icon = state_ == ButtonState::play ? icons.play_ : icons.pause_;

        const auto ratio = Utils::scale_bitmap_ratio();
        p.drawPixmap((width() - icon.width() / ratio) / 2, (height() - icon.height() / ratio) / 2, icon);
    }

    ButtonWithBackground::ButtonWithBackground(QWidget* _parent, const QString& _icon, const QString& _aimId, bool _isOutgoing)
        : CustomButton(_parent, _icon)
        , aimId_(_aimId)
        , isOutgoing_(_isOutgoing)
    {
        updateStyle();
        setFixedSize(getButtonSize());
    }

    void ButtonWithBackground::updateStyle()
    {
        setDefaultColor(Styling::getParameters(aimId_).getColor(Styling::StyleVariable::PRIMARY));
        setActiveColor(Styling::getParameters(aimId_).getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
    }

    void ButtonWithBackground::paintEvent(QPaintEvent * _e)
    {
        {
            auto bgVar = isOutgoing() ? Styling::StyleVariable::PRIMARY_BRIGHT : Styling::StyleVariable::BASE_BRIGHT;
            if (isActive())
            {
                bgVar = Styling::StyleVariable::GHOST_SECONDARY;
            }
            else if (isHovered())
            {
                if (isPressed())
                    bgVar = isOutgoing() ? Styling::StyleVariable::PRIMARY_BRIGHT_ACTIVE :Styling::StyleVariable::BASE_BRIGHT_ACTIVE;
                else
                    bgVar = isOutgoing() ? Styling::StyleVariable::PRIMARY_BRIGHT_HOVER :Styling::StyleVariable::BASE_BRIGHT_HOVER;
            }

            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters(aimId_).getColor(bgVar));
            p.drawEllipse(rect());
        }

        CustomButton::paintEvent(_e);
    }

    bool ButtonWithBackground::isOutgoing() const
    {
        return isOutgoing_;
    }

    ProgressWidget::ProgressWidget(QWidget* _parent, const ButtonType _type, bool _isOutgoing)
        : QWidget(_parent)
        , type_(_type)
        , progress_(0.)
        , isOutgoing_(_isOutgoing)
        , anim_(new QVariantAnimation(this))
    {
        setFixedSize(getButtonSize());

        anim_->setStartValue(0.0);
        anim_->setEndValue(360.0);
        anim_->setDuration(animDuration.count());
        anim_->setLoopCount(-1);
        connect(anim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ProgressWidget::update));
        anim_->start();
    }

    ProgressWidget::~ProgressWidget()
    {
        anim_->stop();
    }

    void ProgressWidget::setProgress(const double _progress)
    {
        progress_ = _progress;
        update();
    }

    void ProgressWidget::updateStyle()
    {
    }

    void ProgressWidget::paintEvent(QPaintEvent* _e)
    {
        QColor bgColor;
        if (type_ == ButtonType::play)
        {
            bgColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        }
        else
        {
            bgColor = isOutgoing_
                    ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT)
                    : Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        }

        QColor lineColor;
        if (type_ == ButtonType::play)
        {
            lineColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
        }
        else
        {
            lineColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE);
        }

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.setBrush(bgColor);
        p.drawEllipse(rect());

        const auto animAngle = anim_->state() == QAbstractAnimation::Running ? anim_->currentValue().toDouble() : 0.0;
        const auto baseAngle = (animAngle * QT_ANGLE_MULT);
        const auto progressAngle = (int)std::ceil(progress_ * 360 * QT_ANGLE_MULT);

        const QPen pen(lineColor, Utils::scale_value(2));
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        static const auto r2 = rect().adjusted(Utils::scale_value(8), Utils::scale_value(8), -Utils::scale_value(8), -Utils::scale_value(8));
        p.drawArc(r2, -baseAngle, -progressAngle);
    }
}

PttBlock::PttBlock(
    ComplexMessageItem *_parent,
    const QString &_link,
    const int32_t _durationSec,
    int64_t _id,
    int64_t _prevId)
    : FileSharingBlockBase(_parent, _link, core::file_sharing_content_type(core::file_sharing_base_content_type::ptt))
    , durationMSec_(_durationSec * 1000)
    , durationText_(formatDuration(durationMSec_))
    , isDecodedTextCollapsed_(true)
    , isPlayed_(false)
    , isPlaybackScheduled_(false)
    , playbackState_(PlaybackState::Stopped)
    , playbackProgressMsec_(0)
    , playbackProgressAnimation_(nullptr)
    , playingId_(-1)
    , pttLayout_(nullptr)
    , textRequestId_(-1)
    , id_(_id)
    , prevId_(_prevId)
    , isOutgoing_(_parent->isOutgoing())
    , buttonPlay_(nullptr)
    , buttonText_(nullptr)
    , buttonCollapse_(nullptr)
    , progressPlay_(nullptr)
    , progressText_(nullptr)
    , downloadAnimDelay_(nullptr)
{
    im_assert(_durationSec >= 0);

    if (!getParentComplexMessage()->isHeadless())
    {
        buttonPlay_ = new PttDetails::PlayButton(this, getChatAimid());
        Testing::setAccessibleName(buttonPlay_, qsl("AS PttBlock buttonPlay"));
        Utils::InterConnector::instance().disableInMultiselect(buttonPlay_, getChatAimid());
    }

    pttLayout_ = new PttBlockLayout();
    setBlockLayout(pttLayout_);
    setLayout(pttLayout_);
    setMouseTracking(true);
}

PttBlock::PttBlock(ComplexMessageItem *_parent,
    const HistoryControl::FileSharingInfoSptr& _fsInfo,
    const int32_t _durationSec,
    int64_t _id,
    int64_t _prevId)
    : FileSharingBlockBase(_parent, _fsInfo->GetUri(), core::file_sharing_content_type(core::file_sharing_base_content_type::ptt))
    , durationMSec_(_durationSec * 1000)
    , durationText_(formatDuration(durationMSec_))
    , isDecodedTextCollapsed_(true)
    , isPlayed_(false)
    , isPlaybackScheduled_(false)
    , playbackState_(PlaybackState::Stopped)
    , playbackProgressMsec_(0)
    , playbackProgressAnimation_(nullptr)
    , playingId_(-1)
    , pttLayout_(nullptr)
    , textRequestId_(-1)
    , id_(_id)
    , prevId_(_prevId)
    , isOutgoing_(_parent->isOutgoing())
    , buttonPlay_(nullptr)
    , buttonText_(nullptr)
    , buttonCollapse_(nullptr)
    , progressPlay_(nullptr)
    , progressText_(nullptr)
    , downloadAnimDelay_(nullptr)
{
    im_assert(_durationSec >= 0);

    if (!getParentComplexMessage()->isHeadless())
    {
        buttonPlay_ = new PttDetails::PlayButton(this, getChatAimid());
        Testing::setAccessibleName(buttonPlay_, qsl("AS PttBlock buttonPlay"));
        Utils::InterConnector::instance().disableInMultiselect(buttonPlay_, getChatAimid());
    }

    pttLayout_ = new PttBlockLayout();
    setBlockLayout(pttLayout_);
    setLayout(pttLayout_);
    setMouseTracking(true);

    if (_fsInfo->IsOutgoing())
    {
        setLocalPath(_fsInfo->GetLocalPath());
        const auto &uploadingProcessId = _fsInfo->GetUploadingProcessId();
        if (!uploadingProcessId.isEmpty())
        {
            setLoadedFromLocal(true);
            setUploadId(uploadingProcessId);
            onDataTransferStarted();
            QObject::connect(this, &PttBlock::uploaded, this, [this]()
            {
                requestMetainfo(false);
            });
        }
    }
}

PttBlock::~PttBlock() = default;

void PttBlock::clearSelection()
{
    FileSharingBlockBase::clearSelection();

    if (decodedTextCtrl_)
    {
        decodedTextCtrl_->clearSelection();
        update();
    }
}

QSize PttBlock::getCtrlButtonSize() const
{
    return getButtonSize();
}

Data::FString PttBlock::getSelectedText(const bool, const TextDestination) const
{
    QString result;
    result.reserve(512);

    if (FileSharingBlockBase::isSelected())
    {
        result += FileSharingBlockBase::getSelectedText().string();
        result += QChar::LineFeed;
    }

    if (decodedTextCtrl_)
        result += decodedTextCtrl_->getSelectedText().string();

    return result;
}

QSize PttBlock::getTextButtonSize() const
{
    return getButtonSize();
}

bool PttBlock::hasDecodedText() const
{
    return !decodedText_.isEmpty();
}

bool PttBlock::isDecodedTextCollapsed() const
{
    return isDecodedTextCollapsed_;
}

void PttBlock::selectByPos(const QPoint& from, const QPoint& to, bool topToBottom)
{
    im_assert(to.y() >= from.y());

    if (!isDecodedTextVisible())
    {
        FileSharingBlockBase::selectByPos(from, to, topToBottom);

        if (decodedTextCtrl_)
            decodedTextCtrl_->clearSelection();
    }
    else
    {
        const auto localFrom = mapFromGlobal(from);
        const auto localTo = mapFromGlobal(to);

        const auto isHeaderSelected = (localFrom.y() < MessageStyle::Ptt::getPttBlockHeight());
        setSelected(isHeaderSelected);

        if (decodedTextCtrl_)
            decodedTextCtrl_->select(localFrom, localTo);
    }

    update();
}

void PttBlock::selectAll()
{
    if (!isDecodedTextVisible())
    {
        FileSharingBlockBase::selectAll();
        if (decodedTextCtrl_)
            decodedTextCtrl_->clearSelection();
    }
    else
    {
        setSelected(true);
        if (decodedTextCtrl_)
            decodedTextCtrl_->selectAll();
    }

    update();
}

void PttBlock::setCtrlButtonGeometry(const QRect &_rect)
{
    im_assert(!_rect.isEmpty());
    im_assert(!getParentComplexMessage()->isHeadless());

    buttonPlay_->move(_rect.topLeft());
    buttonPlay_->show();

    if (progressPlay_)
        progressPlay_->move(_rect.topLeft());
}

int32_t PttBlock::setDecodedTextWidth(const int32_t _width)
{
    return decodedTextCtrl_ ? decodedTextCtrl_->getHeight(_width) : 0;
}

void PttBlock::setDecodedTextOffsets(int _x, int _y)
{
    if (decodedTextCtrl_)
        decodedTextCtrl_->setOffsets(_x, _y);
}

void PttBlock::setTextButtonGeometry(const QPoint& _pos)
{
    if (buttonText_)
        buttonText_->move(_pos);

    if (buttonCollapse_)
        buttonCollapse_->move(_pos);

    if (progressText_)
        progressText_->move(_pos);
}

void PttBlock::onMenuOpenFolder()
{
    showFileInDir(Utils::OpenAt::Folder);
}

void PttBlock::mouseMoveEvent(QMouseEvent* _e)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
    {
        Tooltip::forceShow(false);
        bulletPressed_ = false;
        bulletHovered_ = false;
        GenericBlock::mouseMoveEvent(_e);
        return;
    }
    const auto curPos = _e->pos();
    if (const auto needHover = getBulletRect().contains(curPos) && !checkButtonsHovered(); needHover != bulletHovered_)
    {
        bulletHovered_ = needHover;
        update();
    }
    continuePlaying_ &= bulletPressed_;
    if (bulletPressed_)
        updatePlaybackProgress(curPos);

    const auto needShowTooltip = getPlaybackRect().contains(curPos) || bulletPressed_;
    Tooltip::forceShow(needShowTooltip);
    if (needShowTooltip)
        showBulletTooltip(curPos);
    else
        Tooltip::hide();
    setCursor((bulletHovered_ || needShowTooltip) ? Qt::PointingHandCursor : Qt::ArrowCursor);

    GenericBlock::mouseMoveEvent(_e);
}

void PttBlock::mousePressEvent(QMouseEvent* _e)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
    {
        Tooltip::forceShow(false);
        bulletPressed_ = false;
        bulletHovered_ = false;
        GenericBlock::mousePressEvent(_e);
        return;
    }
    if (_e->button() == Qt::LeftButton)
    {
        const auto pos = _e->pos();
        bulletPressed_ = (getBulletRect().contains(pos) || getPlaybackRect().contains(pos)) && !buttonPlay_->rect().contains(pos);
        bulletHovered_ = bulletPressed_;
        continuePlaying_ = bulletPressed_ && playbackProgressAnimation_ && isPlaying();
        updatePlaybackProgress(pos);
    }
    if (!bulletPressed_)
        GenericBlock::mousePressEvent(_e);
}

void PttBlock::mouseReleaseEvent(QMouseEvent* _e)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
    {
        Tooltip::forceShow(false);
        bulletPressed_ = false;
        bulletHovered_ = false;
        GenericBlock::mouseReleaseEvent(_e);
        return;
    }
    bulletPressed_ = false;
    const auto curPos = _e->pos();
    bulletHovered_ = getBulletRect().contains(curPos) && !buttonPlay_->rect().contains(curPos);
    if (continuePlaying_)
    {
        const auto progress = getPlaybackPercentProgress(curPos);
        if (progress < 1.0)
            startPlayback();
        else
            continuePlaying_ = false;
    }
    updateButtonsStates();
    GenericBlock::mouseReleaseEvent(_e);
}

void PttBlock::leaveEvent(QEvent* _e)
{
    bulletHovered_ = false;
    Tooltip::forceShow(false);
    update();
}

void PttBlock::enterEvent(QEvent* _e)
{
    const auto curPos = mapFromGlobal(QCursor::pos());
    const auto needShowTootip = getPlaybackRect().contains(curPos) || bulletPressed_;
    Tooltip::forceShow(needShowTootip);
    if (needShowTootip)
        showBulletTooltip(curPos);
    else
        Tooltip::hide();
    setCursor(bulletHovered_ || needShowTootip ? Qt::PointingHandCursor : Qt::ArrowCursor);
    GenericBlock::enterEvent(_e);
}

void PttBlock::wheelEvent(QWheelEvent* _e)
{
    if constexpr (platform::is_apple())
        Tooltip::hide();

    GenericBlock::wheelEvent(_e);
}

void PttBlock::drawBlock(QPainter &_p, const QRect& _rect, const QColor& quote_color)
{
    if (!isStandalone() && FileSharingBlockBase::isSelected())
    {
        const auto &bubbleRect = pttLayout_->getContentRect();
        renderClipPaths(bubbleRect);
        drawBubble(_p, bubbleRect);
    }

    drawDuration(_p);
    drawPlaybackProgress(_p, playbackProgressMsec_, durationMSec_);

    if (decodedTextCtrl_ && !isDecodedTextCollapsed_)
        decodedTextCtrl_->draw(_p);
}

void PttBlock::initializeFileSharingBlock()
{
    connectSignals();

    requestMetainfo(false);
}

void PttBlock::onDataTransferStarted()
{
    if (isFileDownloaded())
        return;

    if (!downloadAnimDelay_)
    {
        downloadAnimDelay_ = new QTimer(this);
        downloadAnimDelay_->setSingleShot(true);
        downloadAnimDelay_->setInterval(animDownloadDelay);
        connect(downloadAnimDelay_, &QTimer::timeout, this, &PttBlock::showDownloadAnimation);
    }

    downloadAnimDelay_->start();
}

void PttBlock::showDownloadAnimation()
{
    im_assert((!getParentComplexMessage()->isHeadless()));

    if (isFileDownloaded())
        return;

    if (!progressPlay_)
    {
        progressPlay_ = new PttDetails::ProgressWidget(this, PttDetails::ProgressWidget::ButtonType::play, isOutgoing());
        progressPlay_->setProgress(idleProgressValue);
        progressPlay_->move(buttonPlay_->pos());
        progressPlay_->show();
    }

    buttonPlay_->hide();
}

void PttBlock::onDataTransferStopped()
{
}

void PttBlock::onDownloaded()
{
    im_assert(!getParentComplexMessage()->isHeadless());

    buttonPlay_->show();

    if (progressPlay_)
    {
        progressPlay_->hide();
        progressPlay_->deleteLater();
        progressPlay_ = nullptr;
    }

    if (isPlaybackScheduled_)
    {
        isPlaybackScheduled_ = false;

        startPlayback();
    }
}

void PttBlock::onDownloadedAction()
{
    if (isPlaybackScheduled_)
    {
        isPlaybackScheduled_ = false;

        startPlayback();
    }
}

void PttBlock::onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal, bool /*_showBytes*/)
{
    if (progressPlay_)
    {
        const auto progress = ((double)_bytesTransferred / (double)_bytesTotal);
        progressPlay_->setProgress(progress);
    }
}

void PttBlock::onDownloadingFailed(const int64_t _seq)
{
    // replacing the block with the source text is already
    // present in FileSharingBlockBase::onFileSharingError
}

void PttBlock::onLocalCopyInfoReady(const bool isCopyExists)
{
    const auto isPlayed = (isPlayed_ || isCopyExists);

    const auto isPlayedStatusChanged = (isPlayed != isPlayed_);
    if (!isPlayedStatusChanged)
        return;

    isPlayed_ = isPlayed;

    updateButtonsStates();
}

void PttBlock::onMetainfoDownloaded()
{
    if (buttonText_)
    {
        buttonText_->setVisible(canShowButtonText());
    }
    else if (canShowButtonText())
    {
        buttonText_ = new PttDetails::ButtonWithBackground(this, qsl(":/ptt/text_icon"), getChatAimid(), isOutgoing());
        buttonText_->setFixedSize(getButtonSize());
        buttonText_->move(pttLayout_->getTextButtonRect().topLeft());
        buttonText_->show();

        Utils::InterConnector::instance().disableInMultiselect(buttonText_, getChatAimid());
        connect(buttonText_, &PttDetails::ButtonWithBackground::clicked, this, &PttBlock::onTextButtonClicked);
    }
}

void PttBlock::onPreviewMetainfoDownloaded()
{
    im_assert(!"you're not expected to be here");
}

void PttBlock::connectSignals()
{
    im_assert(!getParentComplexMessage()->isHeadless());

    connect(buttonPlay_, &PttDetails::PlayButton::clicked, this, &PttBlock::onPlayButtonClicked);
    connect(buttonPlay_, &PttDetails::PlayButton::hoverChanged, this, &PttBlock::updateBulletHoveredState);

    connect(GetDispatcher(), &core_dispatcher::speechToText, this, &PttBlock::onPttText);
#ifndef STRIP_AV_MEDIA
    connect(GetSoundsManager(), &SoundsManager::pttPaused, this, &PttBlock::onPttPaused);
    connect(GetSoundsManager(), &SoundsManager::pttFinished, this, &PttBlock::onPttFinished);
#endif // !STRIP_AV_MEDIA
}

void PttBlock::drawBubble(QPainter &_p, const QRect &_bubbleRect)
{
    im_assert(!_bubbleRect.isEmpty());
    im_assert(!isStandalone());

    Utils::PainterSaver ps(_p);
    _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const auto &bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), getChatAimid());
    _p.setBrush(bodyBrush);
    _p.setPen(Qt::NoPen);

    _p.drawRoundedRect(_bubbleRect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
}

void PttBlock::drawDuration(QPainter &_p)
{
    im_assert(!durationText_.isEmpty());
    im_assert(!getParentComplexMessage()->isHeadless());

    const auto durationLeftMargin = Utils::scale_value(12);
    const auto durationBaseline = Utils::scale_value(34);

    const auto textX = buttonPlay_->pos().x() + getButtonSize().width() + MessageStyle::getBubbleHorPadding();
    const auto textY = durationBaseline + (isStandalone() ? pttLayout_->getTopMargin() : MessageStyle::getBubbleVerPadding());

    Utils::PainterSaver ps(_p);

    _p.setFont(MessageStyle::Ptt::getDurationFont());
    _p.setPen(Styling::getParameters(getChatAimid()).getColor(Styling::StyleVariable::TEXT_PRIMARY));

    const auto secondsLeft = (durationMSec_ - playbackProgressMsec_);
    const QString text = (isPlaying() || isPaused()) ? formatDuration(secondsLeft) : (isPlayed_ ? durationText_ : durationText_ % ql1c(' ') % QChar(0x2022));

    im_assert(!text.isEmpty());
    _p.drawText(textX, textY, text);
}

void PttBlock::drawPlaybackProgress(QPainter &_p, const int32_t _progressMsec, const int32_t _durationMsec)
{
    im_assert(_progressMsec >= 0);
    im_assert(_durationMsec > 0);
    im_assert(!getParentComplexMessage()->isHeadless());

    const auto leftMargin  = buttonPlay_->pos().x() + getButtonSize().width() + MessageStyle::getBubbleHorPadding();
    const auto rightMargin = (canShowButtonText() ? width() - pttLayout_->getTextButtonRect().left() : 0) + MessageStyle::getBubbleHorPadding();

    const auto totalWidth = width() - leftMargin - rightMargin;
    const auto progress = _durationMsec ? (double)_progressMsec / _durationMsec : 0.;
    const auto playedWidth = std::min(int(progress * totalWidth), totalWidth);

    const auto x = leftMargin;
    const auto y = Utils::scale_value(17) + (isStandalone() ? pttLayout_->getTopMargin() : MessageStyle::getBubbleVerPadding());
    const auto halfPen = MessageStyle::Ptt::getPttProgressWidth();

    const auto drawLine = [&_p, x, y, halfPen](const auto& _color, const auto& _width)
    {
        _p.setPen(QPen(_color, MessageStyle::Ptt::getPttProgressWidth(), Qt::SolidLine, Qt::RoundCap));

        const auto top = y + halfPen;
        const auto retinaShift = Utils::is_mac_retina() ? 1 : 0;
        const auto left = x + halfPen - retinaShift;
        const auto right = std::max(x + _width - halfPen + retinaShift, left);
        _p.drawLine(left, top, right, top);
    };

    Utils::PainterSaver ps(_p);
    if (!isPlayed_)
    {
        drawLine(getPlaybackColor(), totalWidth);
    }
    else
    {
        drawLine(getProgressColor(), totalWidth);

        if (playedWidth > 0)
            drawLine(getPlaybackColor(), playedWidth);
    }

    const auto bulletR = getBulletSize(bulletHovered_ || bulletPressed_);
    bulletPos_.setX(x + playedWidth);
    bulletPos_.setY(y - halfPen + getBulletSize(false) / 2);

    QRect bullet(bulletPos_, QSize(bulletR, bulletR));
    _p.setPen(Qt::NoPen);
    _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
    _p.drawEllipse(bullet.translated(-bulletR / 2, -bulletR / 2));
}

int32_t PttBlock::getPlaybackProgress() const
{
    im_assert(playbackProgressMsec_ >= 0);
    im_assert(playbackProgressMsec_ < getMaxPttLength().count());

    return playbackProgressMsec_;
}

void PttBlock::setPlaybackProgress(const int32_t _value)
{
    im_assert(_value >= 0);
    im_assert(_value < getMaxPttLength().count());

    playbackProgressMsec_ = _value;

    Q_EMIT Utils::InterConnector::instance().pttProgressChanged(getId(), getFileSharingId(), playbackProgressMsec_ == durationMSec_ ? 0 : playbackProgressMsec_);

    update();
}

void PttBlock::initializeDecodedTextCtrl()
{
    if (!decodedTextCtrl_)
    {
        decodedTextCtrl_ = TextRendering::MakeTextUnit(decodedText_);
        updateDecodedTextStyle();
    }
    else
    {
        decodedTextCtrl_->setText(decodedText_);
    }
}

bool PttBlock::isDecodedTextVisible() const
{
    return (hasDecodedText() && !isDecodedTextCollapsed());
}

bool PttBlock::isPaused() const
{
    return (playbackState_ == PlaybackState::Paused);
}

bool PttBlock::isPlaying() const
{
    return (playbackState_ == PlaybackState::Playing);
}

bool PttBlock::isStopped() const
{
    return (playbackState_ == PlaybackState::Stopped);
}

bool PttBlock::isTextRequested() const
{
    return (textRequestId_ != -1);
}

void PttBlock::renderClipPaths(const QRect &_bubbleRect)
{
    im_assert(!_bubbleRect.isEmpty());
    if (_bubbleRect.isEmpty())
        return;

    const auto isBubbleRectChanged = (_bubbleRect != bubbleClipRect_);
    if (isBubbleRectChanged)
    {
        bubbleClipPath_ = Utils::renderMessageBubble(_bubbleRect, MessageStyle::getBorderRadius());
        bubbleClipRect_ = _bubbleRect;
    }
}

void PttBlock::requestText()
{
    im_assert(textRequestId_ == -1);

    textRequestId_ = GetDispatcher()->pttToText(getLink(), Utils::GetTranslator()->getLang());

    isPlayed_ = true;

    updateButtonsStates();

    startTextRequestProgressAnimation();
}

void PttBlock::startTextRequestProgressAnimation()
{
    if (buttonText_ && buttonText_->isVisible())
    {
        buttonText_->hide();

        if (!progressText_)
        {
            progressText_ = new PttDetails::ProgressWidget(this, PttDetails::ProgressWidget::ButtonType::text, isOutgoing());
            progressText_->move(buttonText_->pos());
            progressText_->setProgress(idleProgressValue);
            progressText_->show();
        }
    }
}

void PttBlock::stopTextRequestProgressAnimation()
{
    if (buttonText_)
    {
        buttonText_->show();

        if (progressText_)
        {
            progressText_->hide();
            progressText_->deleteLater();
            progressText_ = nullptr;
        }
    }
}

void PttBlock::startPlayback()
{
#ifndef STRIP_AV_MEDIA
    isPlayed_ = true;

    im_assert(!isPlaying());
    if (isPlaying())
        return;

    playbackState_ = PlaybackState::Playing;

    initPlaybackAnimation(AnimationState::Play);

    playbackProgressAnimation_->stop();
    playingId_ = GetSoundsManager()->playPtt(getFileLocalPath(), playingId_, durationMSec_);

    auto updateProgress = [this]()
    {
        const auto progress = playbackProgressMsec_ == durationMSec_ ? 0 : getPlaybackPercentProgress(bulletPos_);
        GetSoundsManager()->setProgressOffset(playingId_, progress);
        updatePlaybackAnimation(progress);
        playbackProgressAnimation_->start();
        updateButtonsStates();
    };

#ifdef USE_SYSTEM_OPENAL
    QTimer::singleShot(20, this, updateProgress);
#else
    updateProgress();
#endif

#endif // !STRIP_AV_MEDIA
}

void PttBlock::pausePlayback()
{
#ifndef STRIP_AV_MEDIA
    if (!isPlaying())
        return;

    playbackState_ = PlaybackState::Paused;

    im_assert(playingId_ > 0);
    GetSoundsManager()->pausePtt(playingId_);

    if (playbackProgressAnimation_)
        playbackProgressAnimation_->pause();

    if (!continuePlaying_)
        updateButtonsStates();
#endif // !STRIP_AV_MEDIA
}

void PttBlock::initPlaybackAnimation(AnimationState _state)
{
#ifndef STRIP_AV_MEDIA
    playingId_ = GetSoundsManager()->playPtt(getFileLocalPath(), playingId_, durationMSec_, getPlaybackPercentProgress(bulletPos_));
    GetSoundsManager()->pausePtt(playingId_);

    im_assert(durationMSec_ > 0);

    if (!playbackProgressAnimation_)
    {
        playbackProgressAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("PlaybackProgress"), this);
        playbackProgressAnimation_->setLoopCount(1);
    }

    playbackProgressAnimation_->stop();
    playbackProgressAnimation_->setEndValue(durationMSec_);

    if (_state == AnimationState::Play)
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_playptt_action, { { "from_gallery", "no" },  { "chat_type", Ui::getStatsChatType(getChatAimid()) } });

#endif // !STRIP_AV_MEDIA
}

void PttBlock::updatePlaybackAnimation(float _progress)
{
    const auto currentTime = _progress * durationMSec_;
    setPlaybackProgress(currentTime);
    playbackProgressAnimation_->setStartValue(currentTime);
    playbackProgressAnimation_->setDuration(durationMSec_ - currentTime);
    durationText_ = formatDuration(durationMSec_ - currentTime);
}

void PttBlock::updateButtonsStates()
{
    updatePlayButtonState();
}

void PttBlock::updatePlayButtonState()
{
    im_assert(!getParentComplexMessage()->isHeadless());

    if (isPlaying())
        buttonPlay_->setState(PttDetails::PlayButton::ButtonState::pause);
    else
        buttonPlay_->setState(PttDetails::PlayButton::ButtonState::play);
}

void PttBlock::updateCollapseState()
{
    if (!isDecodedTextCollapsed_ && !buttonCollapse_)
    {
        buttonCollapse_ = new PttDetails::ButtonWithBackground(this, qsl(":/controls/top_icon"), getChatAimid());
        buttonCollapse_->setFixedSize(getButtonSize());
        buttonCollapse_->move(pttLayout_->getTextButtonRect().topLeft());

        Utils::InterConnector::instance().disableInMultiselect(buttonCollapse_, getChatAimid());
        connect(buttonCollapse_, &PttDetails::ButtonWithBackground::clicked, this, &PttBlock::onTextButtonClicked);
    }

    buttonCollapse_->setVisible(!isDecodedTextCollapsed_);
    buttonText_->setVisible(isDecodedTextCollapsed_);

    notifyBlockContentsChanged();
}

void PttBlock::updateBulletHoveredState()
{
    if (buttonPlay_->isHovered() && bulletHovered_)
    {
        bulletHovered_ = false;
        update();
    }
}

void PttBlock::updateStyle()
{
    if (buttonPlay_)
        buttonPlay_->updateStyle();

    if (buttonText_)
        buttonText_->updateStyle();

    if (buttonCollapse_)
        buttonCollapse_->updateStyle();

    if (progressText_)
        progressText_->updateStyle();

    if (progressPlay_)
        progressPlay_->updateStyle();

    updateDecodedTextStyle();

    update();
}

void PttBlock::updateFonts()
{
    updateDecodedTextStyle();
}

void PttBlock::updateDecodedTextStyle()
{
    if (decodedTextCtrl_)
    {
        decodedTextCtrl_->init(MessageStyle::getTextFont(), getDecodedTextColor(), MessageStyle::getLinkColor(), MessageStyle::getTextSelectionColor(getChatAimid()), MessageStyle::getHighlightColor());
        notifyBlockContentsChanged();
    }
}

QColor PttBlock::getDecodedTextColor() const
{
    return Styling::getParameters(getChatAimid()).getColor(Styling::StyleVariable::TEXT_SOLID);
}

QColor PttBlock::getProgressColor() const
{
    return Styling::getParameters(getChatAimid()).getColor(Styling::StyleVariable::GHOST_SECONDARY);
}

QColor PttBlock::getPlaybackColor() const
{
    return Styling::getParameters(getChatAimid()).getColor(Styling::StyleVariable::PRIMARY);
}

bool PttBlock::isOutgoing() const
{
    return isOutgoing_;
}

bool PttBlock::canShowButtonText() const
{
    return Meta_.recognize_ && config::get().is_on(config::features::ptt_recognition);
}

bool PttBlock::checkButtonsHovered() const
{
    // only buttonText_ & buttonCollapse_ ignore mouse move events & propagate moseMoveEvent to parent (PttBlock)
    return (buttonText_ && buttonText_->isHovered()) || (buttonCollapse_ && buttonCollapse_->isHovered());
}

QRect PttBlock::getBulletRect() const
{
    const auto r = getBulletClickableRadius();
    QRect bulletRect(bulletPos_, QSize(r, r));
    bulletRect.translate(-r / 2, -r / 2);
    return bulletRect;
}

QRect PttBlock::getPlaybackRect() const
{
    const auto leftMargin = buttonPlay_->pos().x() + getButtonSize().width() + MessageStyle::getBubbleHorPadding();
    const auto rightMargin = (canShowButtonText() ? width() - pttLayout_->getTextButtonRect().left() : 0) + MessageStyle::getBubbleHorPadding();
    const auto totalWidth = width() - leftMargin - rightMargin;

    QRect playbackRect(leftMargin, getBulletRect().top(), totalWidth, getBulletRect().height());
    return playbackRect;
}

void PttBlock::updatePlaybackProgress(const QPoint& _curPos)
{
#ifndef STRIP_AV_MEDIA
    if (bulletPressed_)
    {
        initPlaybackAnimation(AnimationState::Pause);
        const auto progress = getPlaybackPercentProgress(_curPos);
        updatePlaybackAnimation(progress);
    }
    update();
#endif
}

double PttBlock::getPlaybackPercentProgress(const QPoint& _curPos) const
{
    const auto playbackRect = getPlaybackRect();
    const auto leftMargin = playbackRect.left();
    const auto pos = _curPos.x() - leftMargin;
    const auto totalWidth = playbackRect.width();
    const auto progress = std::clamp(double(pos) / totalWidth, 0.0, 1.0);

    return progress;
}

void PttBlock::showBulletTooltip(QPoint _curPos) const
{
    const auto playbackRect = getPlaybackRect();
    const auto left = playbackRect.x();
    const auto right = playbackRect.right();
    const auto progress = getPlaybackPercentProgress(_curPos);
    const int32_t currentTime = progress * durationMSec_;
    _curPos.setX(std::clamp(_curPos.x(), left, right));

    auto pos = mapToGlobal(_curPos);
    auto bulletPos = mapToGlobal(bulletPos_);
    const auto r = getBulletClickableRadius();
    pos.setY(bulletPos.y());

    QRect targetRect(pos, QSize(r, r));
    targetRect.translate(-r / 2, -r / 2);
    Tooltip::show(Utils::getFormattedTime(std::chrono::milliseconds(currentTime)), targetRect);
}

void PttBlock::onPlayButtonClicked()
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
        return;

    Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    if (isPlaying())
    {
        pausePlayback();
        return;
    }

    isPlaybackScheduled_ = true;

    if (isFileDownloading())
        return;

    startDownloading(true);
}

void PttBlock::onTextButtonClicked()
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
        return;

    if (hasDecodedText())
    {
        isDecodedTextCollapsed_ = !isDecodedTextCollapsed_;

        updateCollapseState();
    }
    else
    {
        requestText();
    }

    Q_EMIT Utils::InterConnector::instance().setFocusOnInput(getChatAimid());
}

void PttBlock::onPttFinished(int _id, bool _byPlay)
{
    if (_id < 0 || _id != playingId_)
        return;

    playingId_ = -1;

    if (playbackProgressAnimation_)
        playbackProgressAnimation_->stop();

    playbackProgressMsec_ = 0;
    durationText_ = formatDuration(durationMSec_);

    playbackState_ = PlaybackState::Stopped;

    updateButtonsStates();

    if (_byPlay)
        pttPlayed(id_);

    update();
}

void PttBlock::onPttPaused(int _id)
{
    if (_id < 0)
        return;

    if (_id == playingId_ && isPlaying())
        pausePlayback();
}

void PttBlock::onPttText(qint64 _seq, int _error, QString _text, int _comeback)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
        return;

    im_assert(_seq > 0);

    if (textRequestId_ != _seq)
        return;

    textRequestId_  = -1;

    const auto isError = (_error != 0);
    const auto isComeback = (_comeback > 0);

    if (isComeback)
    {
        const auto retryTimeoutMsec = ((_comeback + 1) * 1000);

        QTimer::singleShot(
            retryTimeoutMsec,
            Qt::VeryCoarseTimer,
            this,
            &PttBlock::requestText);

        return;
    }

    decodedText_ = isError ? QT_TRANSLATE_NOOP("ptt_widget", "unclear message") : _text;

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_recognptt_action, {
        { "from_gallery", "no" },
        { "chat_type", Ui::getStatsChatType(getChatAimid()) },
        { "result", isError ? "fail" : "success" } });

    stopTextRequestProgressAnimation();

    initializeDecodedTextCtrl();

    isDecodedTextCollapsed_ = false;
    updateCollapseState();
}

void PttBlock::pttPlayed(qint64 id)
{
    if (prevId_ == -1 || id != prevId_ || !Utils::InterConnector::instance().isHistoryPageVisible(getChatAimid()))
        return;

    if (!isPlayed_ && !isPlaying())
    {
        isPlaybackScheduled_ = true;

        if (isFileDownloading())
        {
            return;
        }

        startDownloading(true);
    }
}

int PttBlock::desiredWidth(int) const
{
    return MessageStyle::Ptt::getPttBlockMaxWidth() + (isStandalone() ? 0 : 2 * MessageStyle::getBubbleHorPadding());
}

bool PttBlock::isNeedCheckTimeShift() const
{
    if (isStandalone() && !isDecodedTextCollapsed() && decodedTextCtrl_)
    {
        if (const auto cm = getParentComplexMessage())
        {
            if (const auto timeWidget = cm->getTimeWidget())
            {
                const auto timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();
                const auto lastLineW = decodedTextCtrl_->getLastLineWidth();

                return width() - lastLineW < timeWidth;
            }
        }
    }

    return false;
}

bool PttBlock::isSelected() const
{
    return FileSharingBlockBase::isSelected() || (decodedTextCtrl_ && !isDecodedTextCollapsed_ && decodedTextCtrl_->isSelected());
}

bool PttBlock::clicked(const QPoint& _p)
{
    if (decodedTextCtrl_ && !isDecodedTextCollapsed_)
    {
        QPoint mappedPoint = mapFromParent(_p, pttLayout_->getBlockGeometry());
        decodedTextCtrl_->clicked(mappedPoint);
    }

    return true;
}

bool PttBlock::pressed(const QPoint & _p)
{
    if (decodedTextCtrl_ && !isDecodedTextCollapsed_ && tripleClickTimer_ && tripleClickTimer_->isActive())
    {
        tripleClickTimer_->stop();
        decodedTextCtrl_->selectAll();
        decodedTextCtrl_->fixSelection();
        update();

        return true;
    }

    return false;
}

void PttBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
{
    if (decodedTextCtrl_ && !isDecodedTextCollapsed_)
    {
        QPoint mappedPoint = mapFromParent(_p, pttLayout_->getBlockGeometry());
        const auto textArea = QRect(decodedTextCtrl_->offsets(), rect().bottomRight());
        if (textArea.contains(mappedPoint) && !decodedTextCtrl_->isAllSelected())
        {
            if (!tripleClickTimer_)
            {
                tripleClickTimer_ = new QTimer(this);
                tripleClickTimer_->setInterval(QApplication::doubleClickInterval());
                tripleClickTimer_->setSingleShot(true);
            }
            auto tripleClick = [this, callback = std::move(_callback)](bool result)
            {
                if (callback)
                    callback(result);
                if (result)
                    tripleClickTimer_->start();
            };
            decodedTextCtrl_->doubleClicked(mappedPoint, true, std::move(tripleClick));
            update();
            return;
        }
    }

    if (_callback)
        _callback(true);
}

void PttBlock::releaseSelection()
{
    if (decodedTextCtrl_)
    {
        decodedTextCtrl_->releaseSelection();
    }
    update();
}

void PttBlock::onVisibleRectChanged(const QRect& _visibleRect)
{
    if (_visibleRect.isEmpty())
        if (!Features::isBackgroundPttPlayEnabled())
            pausePlayback();
}

IItemBlock::MenuFlags PttBlock::getMenuFlags(QPoint p) const
{
    if (isSelected())
        return MenuFlags::MenuFlagCopyable;
    return FileSharingBlockBase::getMenuFlags(p);
}

bool PttBlock::setProgress(const Utils::FileSharingId& _fsId, const int32_t _value)
{
    if (_fsId == getFileSharingId())
    {
        setPlaybackProgress(_value);
        return true;
    }
    return false;
}

UI_COMPLEX_MESSAGE_NS_END

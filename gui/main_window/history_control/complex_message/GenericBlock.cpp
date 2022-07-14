#include "stdafx.h"

#include "../../../utils/utils.h"

#include "utils/InterConnector.h"

#include "main_window/MainWindow.h"

#include "previewer/toast.h"

#include "ComplexMessageItem.h"
#include "IItemBlockLayout.h"


#include "GenericBlock.h"
#include "QuoteBlock.h"
#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"
#include "../../mediatype.h"

#include "../../../controls/TooltipWidget.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    constexpr std::chrono::milliseconds resourcesUnloadDelay = std::chrono::seconds(build::is_debug() ? 1 : 5);

    constexpr auto tooltipMode = Tooltip::TooltipMode::Multiline;

    QSize mentionTooltipMaxSize()
    {
        return {Utils::scale_value(416), Utils::scale_value(100)};
    }
}

GenericBlock::GenericBlock(
    ComplexMessageItem* _parent,
    const Data::FString& _sourceText,
    const MenuFlags _menuFlags,
    const bool _isResourcesUnloadingEnabled)
    : GenericBlock(_parent, Data::FStringView{ _sourceText }, _menuFlags, _isResourcesUnloadingEnabled)
{
}

GenericBlock::GenericBlock(
    ComplexMessageItem* _parent,
    Data::FStringView _sourceText,
    const MenuFlags _menuFlags,
    const bool _isResourcesUnloadingEnabled)
    : QWidget(_parent)
    , QuoteAnimation_(new QuoteColorAnimation(this))
    , MenuFlags_(_menuFlags)
    , Parent_(_parent)
    , personTooltip_(new Utils::PersonTooltip(this))
    , SourceText_(_sourceText.toFString())
    , IsResourcesUnloadingEnabled_(_isResourcesUnloadingEnabled)
{
    im_assert(Parent_);
    QuoteAnimation_->setAimId(getChatAimid());
    connect(this, &GenericBlock::setQuoteAnimation, _parent, &ComplexMessageItem::setQuoteAnimation);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::stopTooltipTimer, this, [this]()
    {
            personTooltip_->hide();
    }, Qt::DirectConnection);
}

GenericBlock::GenericBlock()
    : QuoteAnimation_(new QuoteColorAnimation(this))
{
    im_assert(!"program isn't supposed to reach this point");
}

GenericBlock::~GenericBlock() = default;

void GenericBlock::setParentMessage(ComplexMessageItem* _parent)
{
    im_assert(_parent != nullptr);
    if (_parent == nullptr)
        return;
    Parent_ = _parent;
    QWidget::setParent(_parent);
}

QSize GenericBlock::blockSizeForMaxWidth(const int32_t maxWidth)
{
    if (const auto layout = getBlockLayout())
        return layout->blockSizeForMaxWidth(maxWidth);

    return QSize();
}


void GenericBlock::deleteLater()
{
    QWidget::deleteLater();
}

QString GenericBlock::formatRecentsText() const
{
    return getSourceText().string();
}

Ui::MediaType GenericBlock::getMediaType() const
{
    return Ui::MediaType::noMedia;
}

ComplexMessageItem* GenericBlock::getParentComplexMessage() const
{
    im_assert(Parent_);
    return Parent_;
}

qint64 GenericBlock::getId() const
{
    return getParentComplexMessage()->getId();
}

QString GenericBlock::getSenderAimid() const
{
    return getParentComplexMessage()->getSenderAimid();
}

QString GenericBlock::getSenderFriendly() const
{
    return getParentComplexMessage()->getSenderFriendly();
}

void GenericBlock::setSourceText(const Data::FString& _text)
{
    SourceText_ = _text;
}

const QString& GenericBlock::getChatAimid() const
{
    return getParentComplexMessage()->getChatAimid();
}

Data::FString GenericBlock::getSourceText() const
{
    im_assert(!SourceText_.isEmpty());
    return SourceText_;
}

Data::FString GenericBlock::getTextInstantEdit() const
{
    return getSourceText();
}

QString GenericBlock::getPlaceholderText() const
{
    return getSourceText().string();
}

const QString& GenericBlock::getLink() const
{
    static QString emptyLink;
    return emptyLink;
}

QStringList GenericBlock::messageLinks() const
{
    return QStringList();
}

QString GenericBlock::getTextForCopy() const
{
    return getSourceText().string();
}

bool GenericBlock::isBubbleRequired() const
{
    return IsBubbleRequired_;
}

bool GenericBlock::isMarginRequired() const
{
    return IsBubbleRequired_; // by default request margin for blocks with bubble
}

void GenericBlock::setBubbleRequired(bool required)
{
    IsBubbleRequired_ = required;
}

bool GenericBlock::isOutgoing() const
{
    return getParentComplexMessage()->isOutgoingPosition();
}

bool GenericBlock::isDraggable() const
{
    return true;
}

bool GenericBlock::isSharingEnabled() const
{
    return true;
}

bool GenericBlock::isStandalone() const
{
    return getParentComplexMessage()->isSimple();
}

bool GenericBlock::isSenderVisible() const
{
    return getParentComplexMessage()->isSenderVisible();
}

IItemBlock::MenuFlags GenericBlock::getMenuFlags(QPoint) const
{
    return MenuFlags_;
}

bool GenericBlock::onMenuItemTriggered(const QVariantMap &params)
{
    const auto command = params[qsl("command")].toString();

    if (command == u"copy_link")
    {
        if (const auto link = params[qsl("arg")].toString(); !link.isEmpty())
        {
            QApplication::clipboard()->setText(link);
            Utils::showCopiedToast();
        }
        else
        {
            onMenuCopyLink();
        }

        return true;
    }
    else if (command == u"copy_email")
    {
        if (const auto email = params[qsl("arg")].toString(); !email.isEmpty())
        {
            QApplication::clipboard()->setText(email);
            showToast(QT_TRANSLATE_NOOP("toast", "Email copied"));
        }

        return true;
    }
    else if (command == u"copy_file")
    {
        onMenuCopyFile();
        return true;
    }
    else if (command == u"save_as")
    {
        onMenuSaveFileAs();
        return true;
    }
    else if (command == u"open_in_browser")
    {
        onMenuOpenInBrowser();
    }
    else if (command == u"open_folder")
    {
        onMenuOpenFolder();
    }
    else if (command == u"open_profile")
    {
        if (const auto email = params[qsl("arg")].toString(); !email.isEmpty())
            Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(email);
    }

    return false;
}

void GenericBlock::onActivityChanged(const bool isActive)
{
    if (!isActive)
    {
        startResourcesUnloadTimer();
        return;
    }

    stopResourcesUnloadTimer();

    onRestoreResources();

    if (!Initialized_)
    {
        Initialized_ = true;
        initialize();
    }
}

void GenericBlock::onVisibleRectChanged(const QRect&)
{
}

void GenericBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{}

QRect GenericBlock::setBlockGeometry(const QRect &ltr)
{
    auto blockLayout = getBlockLayout();
    if (!blockLayout)
        return ltr;

    const auto blockGeometry = blockLayout->setBlockGeometry(ltr);

    setGeometry(blockGeometry);

    return blockGeometry;
}

QSize GenericBlock::sizeHint() const
{
    if (const auto blockLayout = getBlockLayout())
        return blockLayout->blockSizeHint();

    return QSize(-1, 0);
}

void GenericBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    if (MessageStyle::isBlocksGridEnabled())
    {
        p.setPen(Qt::red);
        p.drawRect(rect());
    }
}

void GenericBlock::initialize()
{
    setMouseTracking(true);
}

void GenericBlock::shiftHorizontally(const int _shift)
{
    move(x() + _shift, y());

    if (auto blockLayout = getBlockLayout())
        blockLayout->shiftHorizontally(_shift);
}

void GenericBlock::setGalleryId(qint64 _msgId)
{
    galleryId_ = _msgId;
}

void GenericBlock::setInsideQuote(bool _inside)
{
    IsInsideQuote_ = _inside;
}

void GenericBlock::setInsideForward(bool _inside)
{
    IsInsideForward_ = _inside;
}

bool GenericBlock::isInsideQuote() const
{
    return IsInsideQuote_;
}

bool GenericBlock::isInsideForward() const
{
    return IsInsideForward_;
}

void GenericBlock::selectByPos(const QPoint &from, const QPoint &to, bool topToBottom)
{
    const QRect globalWidgetRect(
        mapToGlobal(rect().topLeft()),
        mapToGlobal(rect().bottomRight()));

    auto selectionArea(globalWidgetRect);
    selectionArea.setTop(from.y());
    selectionArea.setBottom(to.y());
    selectionArea = selectionArea.normalized();

    const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);
    im_assert(selectionOverlap.height() >= 0);

    const auto widgetHeight = std::max(globalWidgetRect.height(), 1);
    const auto overlappedHeight = selectionOverlap.height();
    const auto overlapRatePercents = ((overlappedHeight * 100) / widgetHeight);
    im_assert(overlapRatePercents >= 0);

    const auto isSelected = (overlapRatePercents > 50);
    if (isSelected)
    {
        Utils::InterConnector::instance().setMultiselect(true, getChatAimid());
        Parent_->setSelectionCenter(topToBottom ? (Parent_->mapFromGlobal(to).y() - 2) : (Parent_->mapFromGlobal(from).y() + 2));
    }
    setSelected(isSelected);
}

void GenericBlock::selectAll()
{
    setSelected(true);
}

bool GenericBlock::isSelected() const
{
    return IsSelected_;
}

void GenericBlock::setSelected(bool _selected)
{
    if (std::exchange(IsSelected_, _selected) != _selected)
        update();
}

void GenericBlock::clearSelection()
{
    setSelected(false);
}

Data::FilesPlaceholderMap GenericBlock::getFilePlaceholders()
{
    Data::FilesPlaceholderMap emptyFiles;
    return emptyFiles;
}

void GenericBlock::updateWith(IItemBlock* _other)
{
    const auto otherText = _other->getSourceText();
    if (getSourceText() != otherText)
        setText(otherText);
}

bool GenericBlock::needStretchToOthers() const
{
    return false;
}

void GenericBlock::stretchToWidth(const int _width)
{
    auto blockGeometry = getBlockGeometry();
    if (needStretchByShift())
    {
        shiftHorizontally((_width - blockGeometry.width()) / 2);
    }
    else
    {
        blockGeometry.setSize(QSize(_width, blockGeometry.height()));
        resizeBlock(blockGeometry.size());
        setGeometry(blockGeometry);
    }
}

QRect GenericBlock::getBlockGeometry() const
{
    auto layout = getBlockLayout();

    if (!layout)
        return QRect();

    return layout->getBlockGeometry();
}

bool GenericBlock::drag()
{
    return false;
}

void GenericBlock::enterEvent(QEvent *)
{
    getParentComplexMessage()->onHoveredBlockChanged(this);
}

bool GenericBlock::isInitialized() const
{
    return Initialized_;
}

void GenericBlock::notifyBlockContentsChanged()
{
    if (auto blockLayout = getBlockLayout())
        blockLayout->onBlockContentsChanged();

    getParentComplexMessage()->updateSize();

    updateGeometry();

    update();
}

bool GenericBlock::hasLeadLines() const
{
    return false;
}

void GenericBlock::markDirty()
{
    if (auto blockLayout = getBlockLayout())
        blockLayout->markDirty();
}

void GenericBlock::onMenuCopyLink()
{
    im_assert(!"should be overriden in child class");
}

void GenericBlock::onMenuCopyFile()
{
    im_assert(!"should be overriden in child class");
}

void GenericBlock::onMenuSaveFileAs()
{
    im_assert(!"should be overriden in child class");
}

void GenericBlock::onMenuOpenInBrowser()
{

}

void GenericBlock::onMenuOpenFolder()
{

}

void GenericBlock::showFileInDir(const Utils::OpenAt)
{
}

void GenericBlock::onRestoreResources()
{

}

void GenericBlock::onUnloadResources()
{

}

void GenericBlock::paintEvent(QPaintEvent* _e)
{
    if (!isInitialized())
        return;

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);

    drawBlock(p, _e->rect(), QuoteAnimation_->quoteColor());

    if (MessageStyle::isBlocksGridEnabled())
    {
        p.setPen(Qt::red);
        p.drawRect(rect());
    }

    QWidget::paintEvent(_e);
}

void GenericBlock::mouseMoveEvent(QMouseEvent* _e)
{
    const auto isLeftButtonPressed = ((_e->buttons() & Qt::LeftButton) != 0);
    const auto beginDrag = (isLeftButtonPressed && isDraggable() && !Utils::InterConnector::instance().isMultiselect(getChatAimid()) && !Parent_->isSelected());
    if (beginDrag)
    {
        auto pos = _e->pos();
        if (mousePos_.isNull())
        {
            mousePos_ = pos;
        }
        else if (!mousePos_.isNull() && (abs(mousePos_.x() - pos.x()) > MessageStyle::getDragDistance() || abs(mousePos_.y() - pos.y()) > MessageStyle::getDragDistance()))
        {
            mousePos_ = QPoint();
            drag();
            return;
        }
    }
    else
    {
        mousePos_ = QPoint();
    }
    return QWidget::mouseMoveEvent(_e);
}

void GenericBlock::mousePressEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::LeftButton)
        mouseClickPos_ = _e->pos();

    return QWidget::mousePressEvent(_e);
}

void GenericBlock::mouseReleaseEvent(QMouseEvent* _e)
{
    if (mouseClickPos_.x() == _e->pos().x() && mouseClickPos_.y() == _e->pos().y())
        Q_EMIT clicked();

    return QWidget::mouseReleaseEvent(_e);
}

void GenericBlock::wheelEvent(QWheelEvent* _e)
{
    if constexpr (platform::is_apple())
    {
        if (tooltipActivated_)
            hideTooltip();
    }
    QWidget::wheelEvent(_e);
}

void GenericBlock::startResourcesUnloadTimer()
{
    if (!IsResourcesUnloadingEnabled_)
    {
        return;
    }

    if (ResourcesUnloadingTimer_)
    {
        ResourcesUnloadingTimer_->start();
        return;
    }

    ResourcesUnloadingTimer_ = new QTimer(this);
    ResourcesUnloadingTimer_->setSingleShot(true);
    ResourcesUnloadingTimer_->setInterval(resourcesUnloadDelay);

    QObject::connect(ResourcesUnloadingTimer_, &QTimer::timeout, this, &GenericBlock::onResourceUnloadingTimeout);

    ResourcesUnloadingTimer_->start();
}

void GenericBlock::stopResourcesUnloadTimer()
{
    if (!ResourcesUnloadingTimer_)
        return;

    ResourcesUnloadingTimer_->stop();
}

void GenericBlock::onResourceUnloadingTimeout()
{
    onUnloadResources();
}

void GenericBlock::setQuoteSelection()
{
    QuoteAnimation_->startQuoteAnimation();
}

void GenericBlock::hideBlock()
{
    hide();
}

bool GenericBlock::isHasLinkInMessage() const
{
    return false;
}

QPoint GenericBlock::getShareButtonPos(const bool _isBubbleRequired, const QRect& _bubbleRect) const
{
    const auto blockGeometry = getBlockGeometry();

    auto buttonX = 0;

    const auto rect = _isBubbleRequired ? _bubbleRect : blockGeometry;

    if (isOutgoing())
        buttonX = rect.left() - getParentComplexMessage()->getSharingAdditionalWidth();
    else
        buttonX = rect.right() + MessageStyle::getSharingButtonMargin();

    auto buttonY = blockGeometry.top();
    if (_isBubbleRequired)
        buttonY += MessageStyle::getSharingButtonTopMargin();

    return QPoint(buttonX, buttonY);
}

QPoint GenericBlock::mapFromParent(const QPoint& _point, const QRect& _blockGeometry) const
{
    return QPoint(_point.x() - _blockGeometry.x(), _point.y() - _blockGeometry.y());
}

void GenericBlock::resizeBlock(const QSize& _size)
{
    if (auto layout = getBlockLayout())
    {
        layout->resizeBlock(_size);
        onBlockSizeChanged(_size);
    }
}

void GenericBlock::onBlockSizeChanged(const QSize& _size)
{
    if (auto layout = getBlockLayout())
        layout->onBlockSizeChanged(_size);
}

void GenericBlock::showFileSavedToast(const QString& _path)
{
    Utils::showToastOverContactDialog(new SavedPathToast(_path, Utils::InterConnector::instance().getMainWindow()));
}

void GenericBlock::showErrorToast()
{
    showToast(QT_TRANSLATE_NOOP("previewer", "Save error"));
}

void GenericBlock::showToast(const QString& _text)
{
    Utils::showTextToastOverContactDialog(_text);
}

bool GenericBlock::isEdited() const
{
    return getParentComplexMessage()->isEdited();
}

qint64 GenericBlock::getGalleryId() const
{
    return galleryId_ == -1 ? getId() : galleryId_;
}

bool GenericBlock::isTooltipActivated() const
{
    auto tooltip = Tooltip::getDefaultMultilineTooltip();
    if (tooltip)
        return tooltipActivated_ && tooltip->isTooltipVisible();
    return tooltipActivated_;
}

void GenericBlock::showTooltip(QString _text, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos)
{
    hideTooltip();
    personTooltip_->show(Utils::PersonTooltipType::Text, _text, _rect, _arrowDir, _arrowPos);
    tooltipActivated_ = true;
}

void GenericBlock::showMentionTooltip(QString _text, QString, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos)
{
    personTooltip_->show(Utils::PersonTooltipType::Person, _text, _rect, _arrowDir, _arrowPos);
    tooltipActivated_ = true;
}

void GenericBlock::hideTooltip()
{
    personTooltip_->hide();
    tooltipActivated_ = false;
}

UI_COMPLEX_MESSAGE_NS_END

#include "stdafx.h"

#include "../../../utils/utils.h"

#include "utils/InterConnector.h"

#include "main_window/ContactDialog.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/history_control/HistoryControl.h"

#include "previewer/toast.h"

#include "ComplexMessageItem.h"
#include "IItemBlockLayout.h"


#include "GenericBlock.h"
#include "QuoteBlock.h"
#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"
#include "../../mediatype.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    constexpr std::chrono::milliseconds resourcesUnloadDelay = std::chrono::seconds(build::is_debug() ? 1 : 5);
}

GenericBlock::GenericBlock(
    ComplexMessageItem *parent,
    const QString &sourceText,
    const MenuFlags menuFlags,
    const bool isResourcesUnloadingEnabled)
    : QWidget(parent)
    , QuoteAnimation_(parent)
    , Initialized_(false)
    , IsResourcesUnloadingEnabled_(isResourcesUnloadingEnabled)
    , MenuFlags_(menuFlags)
    , Parent_(parent)
    , ResourcesUnloadingTimer_(nullptr)
    , SourceText_(sourceText)
    , IsBubbleRequired_(true)
    , galleryId_(-1)
    , IsInsideQuote_(false)
    , IsInsideForward_(false)
    , IsSelected_(false)
{
    assert(Parent_);

    QuoteAnimation_.setAimId(getChatAimid());
    connect(this, &GenericBlock::setQuoteAnimation, parent, &ComplexMessageItem::setQuoteAnimation);
}

GenericBlock::GenericBlock()
    : QuoteAnimation_(this)
{
    assert(!"program isn't supposed to reach this point");
}

GenericBlock::~GenericBlock()
{
}

QSize GenericBlock::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth > 0);

    return getBlockLayout()->blockSizeForMaxWidth(maxWidth);
}


void GenericBlock::deleteLater()
{
    QWidget::deleteLater();
}

QString GenericBlock::formatRecentsText() const
{
    return getSourceText();
}

Ui::MediaType GenericBlock::getMediaType() const
{
    return Ui::MediaType::noMedia;
}

ComplexMessageItem* GenericBlock::getParentComplexMessage() const
{
    assert(Parent_);
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

void GenericBlock::setSourceText(const QString& _text)
{
    SourceText_ = _text;
}

const QString& GenericBlock::getChatAimid() const
{
    return getParentComplexMessage()->getChatAimid();
}

QString GenericBlock::getSourceText() const
{
    assert(!SourceText_.isEmpty());
    return SourceText_;
}

QString GenericBlock::getTextForCopy() const
{
    return getSourceText();
}

bool GenericBlock::isBubbleRequired() const
{
    return IsBubbleRequired_;
}

void GenericBlock::setBubbleRequired(bool required)
{
    IsBubbleRequired_ = required;
}

bool GenericBlock::isOutgoing() const
{
    return getParentComplexMessage()->isOutgoing();
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

IItemBlock::MenuFlags GenericBlock::getMenuFlags() const
{
    return MenuFlags_;
}

bool GenericBlock::onMenuItemTriggered(const QVariantMap &params)
{
    const auto command = params[qsl("command")].toString();

    if (command == ql1s("copy_link"))
    {
        if (const auto link = params[qsl("arg")].toString(); !link.isEmpty())
            QApplication::clipboard()->setText(link);
        else
            onMenuCopyLink();
        return true;
    }
    else if (command == ql1s("copy_email"))
    {
        if (const auto email = params[qsl("arg")].toString(); !email.isEmpty())
        {
            QApplication::clipboard()->setText(email);
            showToast(email % QChar::LineFeed % QT_TRANSLATE_NOOP("toast", "Email copied"));
        }

        return true;
    }
    else if (command == ql1s("copy_file"))
    {
        onMenuCopyFile();
        return true;
    }
    else if (command == ql1s("save_as"))
    {
        onMenuSaveFileAs();
        return true;
    }
    else if (command == ql1s("open_in_browser"))
    {
        onMenuOpenInBrowser();
    }
    else if (command == ql1s("open_folder"))
    {
        onMenuOpenFolder();
    }
    else if (command == ql1s("open_profile"))
    {
        if (const auto email = params[qsl("arg")].toString(); !email.isEmpty())
            emit Utils::InterConnector::instance().openDialogOrProfileById(email);
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

    if (Initialized_)
    {
        return;
    }

    Initialized_ = true;

    initialize();
}

void GenericBlock::onVisibilityChanged(const bool /*isVisible*/)
{
}

void GenericBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{}

QRect GenericBlock::setBlockGeometry(const QRect &ltr)
{
    auto blockLayout = getBlockLayout();
    if (!blockLayout)
    {
        return ltr;
    }

    const auto blockGeometry = blockLayout->setBlockGeometry(ltr);

    setGeometry(blockGeometry);

    return blockGeometry;
}

QSize GenericBlock::sizeHint() const
{
    const auto blockLayout = getBlockLayout();

    if (blockLayout)
    {
        return blockLayout->blockSizeHint();
    }

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

void GenericBlock::selectByPos(const QPoint &from, const QPoint &to, const BlockSelectionType selection)
{
    const QRect globalWidgetRect(
        mapToGlobal(rect().topLeft()),
        mapToGlobal(rect().bottomRight()));

    auto selectionArea(globalWidgetRect);
    selectionArea.setTop(from.y());
    selectionArea.setBottom(to.y());
    selectionArea = selectionArea.normalized();

    const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);
    assert(selectionOverlap.height() >= 0);

    const auto widgetHeight = std::max(globalWidgetRect.height(), 1);
    const auto overlappedHeight = selectionOverlap.height();
    const auto overlapRatePercents = ((overlappedHeight * 100) / widgetHeight);
    assert(overlapRatePercents >= 0);

    const auto isSelected = (overlapRatePercents > 45);

    setSelected(isSelected);
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
    auto blockLayout = getBlockLayout();

    if (!blockLayout)
    {
        assert(!"block layout is missing");
        return;
    }

    blockLayout->onBlockContentsChanged();

    getParentComplexMessage()->updateSize();

    updateGeometry();

    update();
}

void GenericBlock::onMenuCopyLink()
{
    assert(!"should be overriden in child class");
}

void GenericBlock::onMenuCopyFile()
{
    assert(!"should be overriden in child class");
}

void GenericBlock::onMenuSaveFileAs()
{
    assert(!"should be overriden in child class");
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

void GenericBlock::paintEvent(QPaintEvent *e)
{
    if (!isInitialized())
        return;

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);

    drawBlock(p, e->rect(), QuoteAnimation_.quoteColor());

    if (MessageStyle::isBlocksGridEnabled())
    {
        p.setPen(Qt::red);
        p.drawRect(rect());
    }

    QWidget::paintEvent(e);
}

void GenericBlock::mouseMoveEvent(QMouseEvent *e)
{
    const auto isLeftButtonPressed = ((e->buttons() & Qt::LeftButton) != 0);
    const auto beginDrag = (isLeftButtonPressed && isDraggable());
    if (beginDrag)
    {
        auto pos = e->pos();
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
    return QWidget::mouseMoveEvent(e);
}

void GenericBlock::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        mouseClickPos_ = e->pos();

    return QWidget::mousePressEvent(e);
}

void GenericBlock::mouseReleaseEvent(QMouseEvent *e)
{
    if (mouseClickPos_.x() == e->pos().x() && mouseClickPos_.y() == e->pos().y())
        emit clicked();

    return QWidget::mouseReleaseEvent(e);
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
    ResourcesUnloadingTimer_->setInterval(resourcesUnloadDelay.count());

    QObject::connect(ResourcesUnloadingTimer_, &QTimer::timeout, this, &GenericBlock::onResourceUnloadingTimeout);

    ResourcesUnloadingTimer_->start();
}

void GenericBlock::stopResourcesUnloadTimer()
{
    if (!ResourcesUnloadingTimer_)
        return;

    ResourcesUnloadingTimer_->stop();
}

QPoint GenericBlock::getToastPos(const QSize& _dialogSize)
{
    return QPoint(_dialogSize.width() / 2, _dialogSize.height() - Utils::scale_value(36));
}

void GenericBlock::onResourceUnloadingTimeout()
{
    onUnloadResources();
}

void GenericBlock::setQuoteSelection()
{
    QuoteAnimation_.startQuoteAnimation();
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
    const auto blockLayout = getBlockLayout();
    assert(blockLayout);

    const auto blockGeometry = blockLayout->getBlockGeometry();

    auto buttonX = 0;

    const auto rect = _isBubbleRequired ? _bubbleRect : blockGeometry;

    if (isOutgoing())
        buttonX = rect.left() - getParentComplexMessage()->getSharingAdditionalWidth();
    else
        buttonX = rect.right() + MessageStyle::getSharingButtonMargin();

    const auto buttonY = blockGeometry.top();

    return QPoint(buttonX, buttonY);
}

QPoint GenericBlock::mapFromParent(const QPoint& _point, const QRect& _blockGeometry) const
{
    return QPoint(_point.x() - _blockGeometry.x(), _point.y() - _blockGeometry.y());
}

void GenericBlock::showFileSavedToast(const QString& _path)
{
    if (auto dialog = Utils::InterConnector::instance().getContactDialog())
    {
        if (auto page = dialog->getHistoryPage(dialog->currentAimId()))
            ToastManager::instance()->showToast(new SavedPathToast(_path, page), getToastPos(page->size()));
    }
}

void GenericBlock::showErrorToast()
{
    showToast(QT_TRANSLATE_NOOP("previewer", "Save error"));
}

void GenericBlock::showFileCopiedToast()
{
    showToast(QT_TRANSLATE_NOOP("previewer", "Copied to clipboard"));
}

void GenericBlock::showToast(const QString& _text)
{
    if (auto dialog = Utils::InterConnector::instance().getContactDialog())
    {
        if (auto page = dialog->getHistoryPage(dialog->currentAimId()))
            Ui::ToastManager::instance()->showToast(new Ui::Toast(_text, page), getToastPos(page->size()));
    }
}

qint64 GenericBlock::getGalleryId() const
{
    return galleryId_ == -1 ? getId() : galleryId_;
}

UI_COMPLEX_MESSAGE_NS_END

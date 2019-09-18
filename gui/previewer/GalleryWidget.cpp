#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../main_window/ContactDialog.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "../main_window/history_control/ActionButtonWidget.h"
#include "../types/images.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/stat_utils.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/memory_utils.h"

#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/GroupChatOperations.h"
#include "../themes/ResourceIds.h"
#include "../my_info.h"

#include "boost/range/adaptor/reversed.hpp"

#include "Drawable.h"
#include "DownloadWidget.h"
#include "GalleryFrame.h"
#include "ImageLoader.h"
#include "ImageViewerWidget.h"

#include "toast.h"

#include "galleryloader.h"
#include "avatarloader.h"

#include "GalleryWidget.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

namespace
{
    const qreal backgroundOpacity = 0.92;
    const QBrush backgroundColor(QColor(ql1s("#1E1E1E")));
    const int scrollTimeoutMsec = 500;
    const int delayTimeoutMsec = 500;
    const int navButtonWidth = 56;
    const QSize navButtonIconSize = {20, 20};

    enum
    {
        DownloadIndex = 0,
        ImageIndex
    };

    auto getControlsWidgetHeight()
    {
        return Utils::scale_value(162);
    }

    static const Ui::ActionButtonResource::ResourceSet DownloadButtonResources(
        Themes::PixmapResourceId::PreviewerReload,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);
}

using namespace Previewer;

GalleryWidget::GalleryWidget(QWidget* _parent)
    : QWidget(_parent)
    , controlWidgetHeight_(getControlsWidgetHeight())
    , isClosing_(false)
{
    setMouseTracking(true);

    controlsWidget_ = new GalleryFrame(this);
    connect(controlsWidget_, &GalleryFrame::needHeight, this, &GalleryWidget::updateControlsHeight);

    connect(controlsWidget_, &GalleryFrame::prev, this, &GalleryWidget::onPrevClicked);
    connect(controlsWidget_, &GalleryFrame::next, this, &GalleryWidget::onNextClicked);
    connect(controlsWidget_, &GalleryFrame::zoomIn, this, &GalleryWidget::onZoomIn);
    connect(controlsWidget_, &GalleryFrame::zoomOut, this, &GalleryWidget::onZoomOut);
    connect(controlsWidget_, &GalleryFrame::forward, this, &GalleryWidget::onForward);
    connect(controlsWidget_, &GalleryFrame::goToMessage, this, &GalleryWidget::onGoToMessage);
    connect(controlsWidget_, &GalleryFrame::saveAs, this, &GalleryWidget::onSaveAs);
    connect(controlsWidget_, &GalleryFrame::close, this, &GalleryWidget::onEscapeClick);
    connect(controlsWidget_, &GalleryFrame::save, this, &GalleryWidget::onSave);
    connect(controlsWidget_, &GalleryFrame::copy, this, &GalleryWidget::onCopy);
    connect(controlsWidget_, &GalleryFrame::openContact, this, &GalleryWidget::onOpenContact);

    connect(new QShortcut(Qt::CTRL + Qt::Key_Equal, this), &QShortcut::activated, this, &GalleryWidget::onZoomIn);
    connect(new QShortcut(Qt::CTRL + Qt::Key_Minus, this), &QShortcut::activated, this, &GalleryWidget::onZoomOut);
    connect(new QShortcut(Qt::CTRL + Qt::Key_0, this), &QShortcut::activated, this, &GalleryWidget::onResetZoom);

    if constexpr (!platform::is_apple())
        connect(new QShortcut(Qt::CTRL + Qt::Key_W, this), &QShortcut::activated, this, &GalleryWidget::escape);

    nextButton_ = new NavigationButton(this);
    prevButton_ = new NavigationButton(this);

    const auto iconSize = Utils::scale_value(navButtonIconSize);
    auto prevPixmap = Utils::renderSvgScaled(qsl(":/controls/back_icon"), iconSize, QColor(255, 255, 255, 0.2 * 255));
    auto nextPixmap = prevPixmap.transformed(QTransform().rotate(180));

    auto prevPixmapHovered = Utils::renderSvgScaled(qsl(":/controls/back_icon"), iconSize, Qt::white);
    auto nextPixmapHovered = prevPixmapHovered.transformed(QTransform().rotate(180));

    nextButton_->setDefaultPixmap(nextPixmap);
    prevButton_->setDefaultPixmap(prevPixmap);

    nextButton_->setHoveredPixmap(nextPixmapHovered);
    prevButton_->setHoveredPixmap(prevPixmapHovered);

    connect(nextButton_, &NavigationButton::clicked, this, &GalleryWidget::onNextClicked);
    connect(prevButton_, &NavigationButton::clicked, this, &GalleryWidget::onPrevClicked);

    imageViewer_ = new ImageViewerWidget(this);
    imageViewer_->setVideoWidthOffset(Utils::scale_value(2 * navButtonWidth));
    connect(imageViewer_, &ImageViewerWidget::zoomChanged, this, &GalleryWidget::updateZoomButtons);
    connect(imageViewer_, &ImageViewerWidget::closeRequested, this, &GalleryWidget::escape);
    connect(imageViewer_, &ImageViewerWidget::clicked, this, &GalleryWidget::clicked);
    connect(imageViewer_, &ImageViewerWidget::fullscreenToggled, this, &GalleryWidget::onFullscreenToggled);
    connect(imageViewer_, &ImageViewerWidget::playClicked, this, &GalleryWidget::onPlayClicked);

    QVBoxLayout* layout = Utils::emptyVLayout();
    Testing::setAccessibleName(imageViewer_, qsl("AS gw imageViewer_"));
    layout->addWidget(imageViewer_);

    setLayout(layout);

    setAttribute(Qt::WA_TranslucentBackground);

    if constexpr (platform::is_linux())
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);
    else if constexpr (platform::is_apple())
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    else
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    scrollTimer_ = new QTimer(this);
    connect(scrollTimer_, &QTimer::timeout, scrollTimer_, &QTimer::stop);

    delayTimer_ = new QTimer(this);
    delayTimer_->setSingleShot(true);
    connect(delayTimer_, &QTimer::timeout, this, &GalleryWidget::onDelayTimeout);

    progressButton_ = new Ui::ActionButtonWidget(Ui::ActionButtonResource::ResourceSet::Play_, this);
    progressButton_->hide();

    connect(progressButton_, &Ui::ActionButtonWidget::stopClickedSignal, this, [this]()
    {
        progressButton_->stopAnimation();
        contentLoader_->cancelCurrentItemLoading();
    });

    connect(progressButton_, &Ui::ActionButtonWidget::startClickedSignal, this, [this]()
    {
        progressButton_->startAnimation();
        contentLoader_->startCurrentItemLoading();
    });

}

GalleryWidget::~GalleryWidget()
{
}

void GalleryWidget::openGallery(const QString &_aimId, const QString &_link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer)
{
    aimId_ = _aimId;
    contentLoader_.reset(createGalleryLoader(_aimId, _link, _msgId, _attachedPlayer));

    init();

    if (_attachedPlayer)
        onMediaLoaded();
}

void GalleryWidget::openAvatar(const QString &_aimId)
{
    aimId_ = _aimId;
    contentLoader_.reset(createAvatarLoader(_aimId));

    init();
}

bool GalleryWidget::isClosing() const
{
    return isClosing_;
}

void GalleryWidget::closeGallery()
{
    isClosing_ = true;

    Ui::MainPage::instance()->getContactDialog()->setFocusOnInputWidget();

    imageViewer_->reset();
    showNormal();
    close();
    releaseKeyboard();

    Ui::ToastManager::instance()->hideToast();

    // it fixes https://jira.mail.ru/browse/IMDESKTOP-8547, which is most propably caused by https://bugreports.qt.io/browse/QTBUG-49238
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
}

void GalleryWidget::showMinimized()
{
    imageViewer_->hide();

    // todo: fix gallery minimize in macos
    if constexpr (platform::is_apple())
        QWidget::hide();
    else
        QWidget::showMinimized();
}

void GalleryWidget::showFullScreen()
{
#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
#else
    QWidget::showFullScreen();
#endif

    imageViewer_->show();
}

void GalleryWidget::escape()
{
    if (!imageViewer_->closeFullscreen())
    {
        closeGallery();
        emit closed();
    }
}

void GalleryWidget::moveToScreen()
{
    const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
    const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);
    setGeometry(screenGeometry);
}

QString GalleryWidget::getDefaultText() const
{
    return QT_TRANSLATE_NOOP("previewer", "Loading...");
}

QString GalleryWidget::getInfoText(int _n, int _total) const
{
    return QT_TRANSLATE_NOOP("previewer", "%1 of %2").arg(_n).arg(_total);
}

void GalleryWidget::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton && !controlsWidget_->isCaptionExpanded())
    {
        closeGallery();
    }
}

void GalleryWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        clicked();
    }
}

void GalleryWidget::keyPressEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (key == Qt::Key_Escape)
        escape();
    else if (key == Qt::Key_Left && hasPrev())
        prev();
    else if (key == Qt::Key_Right && hasNext())
        next();

    if constexpr (platform::is_linux() || platform::is_windows())
    {
        if (_event->modifiers() == Qt::ControlModifier && key == Qt::Key_Q)
        {
            closeGallery();
            emit finished();
        }
    }
}

void GalleryWidget::keyReleaseEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (key == Qt::Key_Plus || key == Qt::Key_Equal)
        onZoomIn();
    else if (key == Qt::Key_Minus)
        onZoomOut();
}

void GalleryWidget::wheelEvent(QWheelEvent* _event)
{
    onWheelEvent(_event->angleDelta());
}

void GalleryWidget::paintEvent(QPaintEvent* /*_event*/)
{
    QPainter painter(this);
    painter.setOpacity(backgroundOpacity);
    painter.fillRect(0, 0, width(), height() - controlWidgetHeight_, backgroundColor);
}

bool GalleryWidget::event(QEvent* _event)
{
    if (_event->type() == QEvent::Gesture)
    {
        auto ge = static_cast<QGestureEvent*>(_event);
        if (QGesture *pinch = ge->gesture(Qt::PinchGesture))
        {
            auto pg = static_cast<QPinchGesture *>(pinch);
            imageViewer_->scaleBy(pg->scaleFactor(), QCursor::pos());
            return true;
        }
    }

    return QWidget::event(_event);
}

void GalleryWidget::updateControls()
{
    updateNavButtons();

    auto currentItem = contentLoader_->currentItem();

    if (currentItem)
    {
        GalleryFrame::Actions menuActions;
        if (currentItem->msg())
        {
            auto sender = currentItem->sender();
            if (!sender.isEmpty())
                controlsWidget_->setAuthorText(Logic::GetFriendlyContainer()->getFriendly(sender));

            auto t = QDateTime::fromTime_t(currentItem->time());
            auto dateStr = t.date().toString(qsl("dd.MM.yyyy"));
            auto timeStr = t.toString(qsl("hh:mm"));

            controlsWidget_->setDateText(QT_TRANSLATE_NOOP("previewer", "%1 at %2").arg(dateStr, timeStr));
            controlsWidget_->setCaption(currentItem->caption());

            menuActions = {GalleryFrame::Copy, GalleryFrame::SaveAs, GalleryFrame::Forward, GalleryFrame::GoTo};
            controlsWidget_->setAuthorAndDateVisible(true);
        }
        else
        {
            menuActions = {GalleryFrame::Copy, GalleryFrame::SaveAs};
            controlsWidget_->setAuthorAndDateVisible(false);
        }

        auto index = contentLoader_->currentIndex();
        auto total = contentLoader_->totalCount();
        if (index > 0 && total > 0)
        {
            controlsWidget_->setCounterText(getInfoText(index, total));
        }
        else
        {
            controlsWidget_->setCounterText(QString());
        }

        controlsWidget_->setMenuActions(menuActions);
    }
}

void GalleryWidget::updateControlsHeight(int _add)
{
    controlWidgetHeight_ = getControlsWidgetHeight() + _add;
    updateControlsFrame();
    update();
}

void GalleryWidget::onPrevClicked()
{
    prev();
}

void GalleryWidget::onNextClicked()
{
    next();
}

void GalleryWidget::onSaveAs()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    if constexpr (platform::is_linux())
    {
        releaseKeyboard();
        setWindowFlags(windowFlags() & ~Qt::BypassWindowManagerHint);
        imageViewer_->hide();
    }

    if constexpr (platform::is_apple())
        bringToBack();

    QString savePath;

    const auto guard = QPointer(this);

    Utils::saveAs(item->fileName(), [this, item, guard, &savePath](const QString& name, const QString& dir)
    {
        if (!guard)
            return;

        if constexpr (platform::is_linux())
            grabKeyboard();

        const auto destinationFile = QFileInfo(dir, name).absoluteFilePath();

        item->save(destinationFile);

        savePath = destinationFile;

    }, std::function<void ()>(), false);

    if (!guard)
        return;

    if constexpr (platform::is_linux())
    {
        setWindowFlags(windowFlags() | Qt::BypassWindowManagerHint);
        show();
        imageViewer_->show();
    }

    if constexpr (platform::is_apple())
        showOverAll();

    activateWindow();

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "save" } });
}

void GalleryWidget::onMediaLoaded()
{
    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    delayTimer_->stop();
    progressButton_->hide();
    controlsWidget_->setSaveEnabled(true);
    controlsWidget_->setMenuEnabled(true);

    Ui::ToastManager::instance()->hideToast();
    imageViewer_->show();

    item->showMedia(imageViewer_);

    updateControls();

    controlsWidget_->setZoomVisible(imageViewer_->isZoomSupport());

    controlsWidget_->setZoomInEnabled(true);
    controlsWidget_->setZoomOutEnabled(true);

    controlsWidget_->show();

    connectContainerEvents();
}

void GalleryWidget::onPreviewLoaded()
{
    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    imageViewer_->show();
    Ui::ToastManager::instance()->hideToast();

    item->showPreview(imageViewer_);

    updateControls();

    controlsWidget_->setZoomInEnabled(true);
    controlsWidget_->setZoomOutEnabled(true);

    controlsWidget_->show();

    showProgress();

    connectContainerEvents();
}

void GalleryWidget::onImageLoadingError()
{
    delayTimer_->stop();
    progressButton_->hide();

    controlsWidget_->setSaveEnabled(false);
    controlsWidget_->setMenuEnabled(false);

    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    imageViewer_->hide();
    Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("previewer", "Loading error")), getToastPos());
}

void GalleryWidget::updateNavButtons()
{
    auto navSupported = contentLoader_->navigationSupported();
    controlsWidget_->setNavigationButtonsVisible(navSupported);
    controlsWidget_->setPrevEnabled(hasPrev());
    controlsWidget_->setNextEnabled(hasNext());

    prevButton_->setVisible(hasPrev() && navSupported);
    nextButton_->setVisible(hasNext() && navSupported);
}

void GalleryWidget::onZoomIn()
{
    imageViewer_->zoomIn();
}

void GalleryWidget::onZoomOut()
{
    imageViewer_->zoomOut();
}

void GalleryWidget::onResetZoom()
{
    imageViewer_->resetZoom();
    controlsWidget_->setZoomInEnabled(true);
    controlsWidget_->setZoomOutEnabled(true);
}

void GalleryWidget::onWheelEvent(const QPoint& _delta)
{
    static const auto minDelta = 30;

    const auto delta = _delta.x();

    if (std::abs(delta) < minDelta)
        return;

    if (scrollTimer_->isActive())
        return;

    scrollTimer_->start(scrollTimeoutMsec);

    if (delta < 0 && hasNext())
        next();
    else if (delta > 0 && hasPrev())
        prev();
}

void GalleryWidget::onEscapeClick()
{
    if (!imageViewer_->closeFullscreen())
    {
        closeGallery();
    }
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "cross_btn" } });
}

void GalleryWidget::onSave()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    QDir downloadDir(Ui::getDownloadPath());
    if (!downloadDir.exists())
        downloadDir.mkpath(qsl("."));

    item->save(downloadDir.filePath(item->fileName()));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "save" } });
}

void GalleryWidget::onForward()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    Data::Quote q;
    q.chatId_ = item->aimId();
    q.senderId_ = item->sender();
    q.text_ = item->link();
    q.msgId_ = item->msg();
    q.time_ = item->time();
    q.senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(q.senderId_);

    escape();

    QTimer::singleShot(0, [quote = std::move(q)]() { Ui::forwardMessage({ std::move(quote) }, false, QString(), QString(), true); });

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "forward" } });
}

void GalleryWidget::onGoToMessage()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    const auto msgId = item->msg();
    if (msgId > 0)
    {
        Logic::getContactListModel()->setCurrent(aimId_, msgId, true, nullptr, msgId);
        escape();
    }

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "go_to_message" } });
}

void GalleryWidget::onCopy()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    item->copyToClipboard();

    Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("previewer", "Copied to clipboard")), getToastPos());
}

void GalleryWidget::onOpenContact()
{
    const auto item = contentLoader_->currentItem();
    if (!item || !QDateTime::fromTime_t(item->time()).date().isValid())
        return;

    Utils::openDialogOrProfile(item->sender());
    escape();
}

void GalleryWidget::onDelayTimeout()
{
    delayTimer_->stop();
    controlsWidget_->setSaveEnabled(false);
    controlsWidget_->setMenuEnabled(false);
    controlsWidget_->setZoomVisible(imageViewer_->isZoomSupport());

    auto item = contentLoader_->currentItem();
    if (item && item->isVideo())
    {
        auto buttonSize = progressButton_->sizeHint();
        auto localPos = QPoint((width() - buttonSize.width()) / 2, (height() - controlsWidget_->height() - buttonSize.height()) / 2);

        progressButton_->move(mapToGlobal(localPos));
        progressButton_->startAnimation();
        progressButton_->show();
    }
}

void GalleryWidget::updateZoomButtons()
{
    controlsWidget_->setZoomInEnabled(imageViewer_->canZoomIn());
    controlsWidget_->setZoomOutEnabled(imageViewer_->canZoomOut());
}

void GalleryWidget::onItemSaved(const QString& _path)
{
    Ui::ToastManager::instance()->showToast(new Ui::SavedPathToast(_path), getToastPos());
}

void GalleryWidget::onItemSaveError()
{
    Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("previewer", "Save error")), getToastPos());
}

void GalleryWidget::clicked()
{
    controlsWidget_->collapseCaption();
}

void GalleryWidget::onFullscreenToggled(bool _enabled)
{
#ifdef __APPLE__
    if (!_enabled)
    {
        raise();
        MacSupport::showNSPopUpMenuWindowLevel(this);
    }
#endif
}

void GalleryWidget::onPlayClicked(bool _paused)
{
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", _paused ? "stop" : "play" } });
}

void GalleryWidget::showProgress()
{
    delayTimer_->start(delayTimeoutMsec);
}

bool GalleryWidget::hasPrev() const
{
    return contentLoader_->hasPrev();
}

void GalleryWidget::prev()
{
    assert(hasPrev());

    showProgress();

    contentLoader_->prev();

    updateControls();
    controlsWidget_->onPrev();
}

bool GalleryWidget::hasNext() const
{
    return contentLoader_->hasNext();
}

void GalleryWidget::next()
{
    assert(hasNext());

    showProgress();

    contentLoader_->next();

    updateControls();
    controlsWidget_->onNext();
}

void GalleryWidget::updateControlsFrame()
{
    controlsWidget_->setFixedSize(QSize(width(), controlWidgetHeight_));

    auto y = height() - controlsWidget_->height();

    controlsWidget_->move(0, y);
    controlsWidget_->raise();

    static const auto bwidth = Utils::scale_value(navButtonWidth);

    nextButton_->setFixedSize(bwidth, height());
    prevButton_->setFixedSize(bwidth, height());

    nextButton_->move(width() - bwidth, 0);
    prevButton_->move(0, 0);

    nextButton_->raise();
    prevButton_->raise();
}

QSize GalleryWidget::getViewSize()
{
    updateControlsFrame();

    return QSize(width(), height() - controlWidgetHeight_);
}

QPoint GalleryWidget::getToastPos()
{
    auto localPos = QPoint(width() / 2, height() - Utils::scale_value(180));
    return mapToGlobal(localPos);
}

ContentLoader* GalleryWidget::createGalleryLoader(const QString& _aimId, const QString& _link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer)
{
    auto galleryLoader = new GalleryLoader(_aimId, _link, _msgId, _attachedPlayer);

    connect(galleryLoader, &GalleryLoader::previewLoaded, this, &GalleryWidget::onPreviewLoaded);
    connect(galleryLoader, &GalleryLoader::mediaLoaded, this, &GalleryWidget::onMediaLoaded);
    connect(galleryLoader, &GalleryLoader::itemUpdated, this, &GalleryWidget::updateControls);
    connect(galleryLoader, &GalleryLoader::contentUpdated, this, &GalleryWidget::updateNavButtons);
    connect(galleryLoader, &GalleryLoader::itemSaved, this, &GalleryWidget::onItemSaved);
    connect(galleryLoader, &GalleryLoader::itemSaveError, this, &GalleryWidget::onItemSaveError);
    connect(galleryLoader, &GalleryLoader::error, this, &GalleryWidget::onImageLoadingError);

    return galleryLoader;
}

ContentLoader* GalleryWidget::createAvatarLoader(const QString& _aimId)
{
    auto avatarLoader = new AvatarLoader(_aimId);

    connect(avatarLoader, &AvatarLoader::mediaLoaded, this, &GalleryWidget::onMediaLoaded);
    connect(avatarLoader, &AvatarLoader::itemSaved, this, &GalleryWidget::onItemSaved);
    connect(avatarLoader, &AvatarLoader::itemSaveError, this, &GalleryWidget::onItemSaveError);

    return avatarLoader;
}

void GalleryWidget::init()
{
    moveToScreen();

    updateControlsFrame();

#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
#else
    showFullScreen();
#endif

    showProgress();

    grabKeyboard();

    grabGesture(Qt::PinchGesture);

    updateControls();

    imageViewer_->setViewSize(getViewSize());
}

void GalleryWidget::bringToBack()
{
#ifdef __APPLE__
    MacSupport::showNSModalPanelWindowLevel(this);
    imageViewer_->bringToBack();
#endif
}

void GalleryWidget::showOverAll()
{
#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
    imageViewer_->showOverAll();
#endif
}

void GalleryWidget::connectContainerEvents()
{
    imageViewer_->connectExternalWheelEvent([this](const QPoint& _delta)
    {
        onWheelEvent(_delta);
    });
}

NavigationButton::NavigationButton(QWidget *_parent)
    : QWidget(_parent)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

NavigationButton::~NavigationButton() = default;

void NavigationButton::setHoveredPixmap(const QPixmap& _pixmap)
{
    hoveredPixmap_ = _pixmap;
}

void NavigationButton::setDefaultPixmap(const QPixmap &_pixmap)
{
    defaultPixmap_ = _pixmap;

    if (activePixmap_.isNull())
        activePixmap_ = defaultPixmap_;
}

void NavigationButton::setFixedSize(int _width, int _height)
{
    QWidget::setFixedSize(_width, _height);
}

void NavigationButton::setVisible(bool _visible)
{
    QWidget::setVisible(_visible);

    activePixmap_ = defaultPixmap_;
}

void NavigationButton::mousePressEvent(QMouseEvent *_event)
{
    _event->accept();
}

void NavigationButton::mouseReleaseEvent(QMouseEvent *_event)
{
    const auto mouseOver = rect().contains(_event->pos());
    if (mouseOver)
        emit clicked();

    activePixmap_ = mouseOver ? hoveredPixmap_ : defaultPixmap_;

    update();
}

void NavigationButton::mouseMoveEvent(QMouseEvent *_event)
{
    QWidget::enterEvent(_event);

    activePixmap_ = hoveredPixmap_;

    update();
}

void NavigationButton::leaveEvent(QEvent *_event)
{
    QWidget::leaveEvent(_event);

    activePixmap_ = defaultPixmap_;

    update();
}

void NavigationButton::paintEvent(QPaintEvent *_event)
{
    QPainter p(this);

    static const auto iconSize = Utils::scale_value(navButtonIconSize);

    const auto thisRect = rect();

    const QRect rcButton((thisRect.width() - iconSize.width()) / 2, (thisRect.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());

    p.drawPixmap(rcButton, activePixmap_);
}

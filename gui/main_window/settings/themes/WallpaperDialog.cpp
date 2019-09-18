#include "stdafx.h"

#include "WallpaperDialog.h"
#include "WallpaperWidget.h"
#include "WallpaperPreviewWidget.h"

#include "gui_settings.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromFileTask.h"
#include "utils/ResizePixmapTask.h"
#include "controls/GeneralDialog.h"
#include "controls/FlowLayout.h"
#include "controls/DialogButton.h"
#include "controls/TransparentScrollBar.h"
#include "controls/CheckboxList.h"
#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"
#include "main_window/MainWindow.h"
#include "main_window/friendly/FriendlyContainer.h"
#include "../../../core_dispatcher.h"

namespace
{
    int captionHeightAllChats()
    {
        return Utils::scale_value(52);
    }

    int captionHeightSingleChat()
    {
        return Utils::scale_value(68);
    }

    int getThumbSpacing()
    {
        return Utils::scale_value(4);
    }

    int getBgHeight()
    {
        return Utils::scale_value(136);
    }

    int getLeftMargin()
    {
        return Utils::scale_value(16);
    }

    int getCaptionLeftMargin()
    {
        return Utils::scale_value(20);
    }

    int checkboxHeight()
    {
        return Utils::scale_value(24);
    }

    QFont getCaptionFont()
    {
        return Fonts::appFontScaled(14);
    }

    int getCornerRadius()
    {
        return Utils::scale_value(4);
    }

    int getDragOverlayHorMargin()
    {
        return Utils::scale_value(16);
    }

    int thumbsAreaMinHeight()
    {
        return Utils::scale_value(132);
    }

    int thumbsAreaMaxHeight()
    {
        return Utils::scale_value(480);
    }

    constexpr auto dragActivateDelay() noexcept { return std::chrono::milliseconds(500); }

    QString globalUserWpId() { return qsl("0"); }

    QString contactId(const QString& _contact)
    {
        return _contact.isEmpty() ? globalUserWpId() : _contact;
    }

    int getDialogWidth()
    {
        return Utils::scale_value(380);
    }
}

namespace Ui
{
    DragOverlay::DragOverlay(QWidget* _parent)
        : QWidget(_parent)
        , dragMouseoverTimer_(new QTimer(this))
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAcceptDrops(true);

        dragMouseoverTimer_->setInterval(dragActivateDelay().count());
        dragMouseoverTimer_->setSingleShot(true);
        connect(dragMouseoverTimer_, &QTimer::timeout, this, &DragOverlay::onTimer);
    }

    void DragOverlay::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        const auto innerRect = rect().adjusted(getDragOverlayHorMargin(), 0, -getDragOverlayHorMargin(), 0);

        QPainterPath path;
        path.addRoundedRect(innerRect, getCornerRadius(), getCornerRadius());
        p.setClipPath(path);
        p.fillRect(innerRect, Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT));

        QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Utils::scale_value(2));
        static const auto dashes = []()
        {
            QVector<qreal> dashes;
            dashes << Utils::scale_value(2) << Utils::scale_value(4);
            return dashes;
        }();
        pen.setDashPattern(dashes);

        p.setPen(pen);
        p.drawRoundedRect(innerRect, getCornerRadius(), getCornerRadius());

        static auto fnt = getCaptionFont();
        p.setFont(fnt);
        p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        p.drawText(innerRect, QT_TRANSLATE_NOOP("wallpaper_select", "From a file"), QTextOption(Qt::AlignCenter));
    }

    void DragOverlay::dragEnterEvent(QDragEnterEvent* _e)
    {
        dragMouseoverTimer_->start();

        _e->acceptProposedAction();
    }

    void DragOverlay::dragLeaveEvent(QDragLeaveEvent* _e)
    {
        dragMouseoverTimer_->stop();

        hide();
        _e->accept();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlay::dragMoveEvent(QDragMoveEvent* _e)
    {
        _e->acceptProposedAction();
        update();
    }

    void DragOverlay::dropEvent(QDropEvent* _e)
    {
        dragMouseoverTimer_->stop();
        onTimer();

        const QMimeData* mimeData = _e->mimeData();
        const auto mimeDataWithImage = Utils::isMimeDataWithImage(mimeData);
        if (mimeData->hasUrls() || mimeDataWithImage)
        {
            if (mimeDataWithImage)
            {
                auto image = Utils::getImageFromMimeData(mimeData);
                if (!image.isNull())
                {
                    auto task = new Utils::ResizePixmapTask(QPixmap::fromImage(std::move(image)), Utils::getMaxImageSize());

                    connect(task, &Utils::ResizePixmapTask::resizedSignal, this, &DragOverlay::onResized);
                    QThreadPool::globalInstance()->start(task);
                }
            }
            else
            {
                const QList<QUrl> urlList = mimeData->urls();
                if (!urlList.isEmpty())
                {
                    if (const auto url = urlList.first(); url.isLocalFile())
                    {
                        QFileInfo info(url.toLocalFile());
                        const bool canDrop =
                            !(info.isBundle() || info.isDir()) &&
                            info.size() > 0;

                        if (canDrop)
                            emit fileDropped(info.absoluteFilePath(), QPrivateSignal());
                    }
                }
            }
        }

        _e->acceptProposedAction();
        hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlay::onTimer()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void DragOverlay::onResized(const QPixmap& _pixmap)
    {
        assert(!_pixmap.isNull());
        if (!_pixmap.isNull())
            emit imageDropped(_pixmap, QPrivateSignal());
    }


    WallpaperSelectWidget::WallpaperSelectWidget(QWidget* _parent, const QString& _aimId, bool _fromSettings)
        : QWidget(_parent)
        , thumbLayout_(new FlowLayout(0, getThumbSpacing(), getThumbSpacing()))
        , preview_(nullptr)
        , setToAll_(nullptr)
        , dragOverlay_(new DragOverlay(this))
        , scrollArea_(CreateScrollAreaAndSetTrScrollBarV(this))
        , targetContact_(_aimId)
        , userWallpaper_(Styling::getThemesContainer().createUserWallpaper(contactId(targetContact_)))
        , fromSettings_(_fromSettings)
    {
        QWidget* innerArea = new QWidget(scrollArea_);
        innerArea->setLayout(thumbLayout_);

        Utils::grabTouchWidget(scrollArea_->viewport(), true);
        Utils::grabTouchWidget(scrollArea_);
        Utils::grabTouchWidget(innerArea);
        connect(QScroller::scroller(scrollArea_->viewport()), &QScroller::stateChanged, this, &WallpaperSelectWidget::touchScrollStateChanged);

        thumbLayout_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        scrollArea_->setStyleSheet(qsl("background: transparent; border: none;"));
        scrollArea_->setWidget(innerArea);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setMinimumHeight(thumbsAreaMinHeight());
        scrollArea_->setMaximumHeight(thumbsAreaMaxHeight());

        const PreviewMessagesVector previewMsgs =
        {
            { QT_TRANSLATE_NOOP("wallpaper_select", "I miss you!"),     QTime(12, 44), QString() },
            { QT_TRANSLATE_NOOP("wallpaper_select", "I miss you too :)"),  QTime(16, 44), QT_TRANSLATE_NOOP("wallpaper_select", "Alice") },
        };
        preview_ = new WallpaperPreviewWidget(this, previewMsgs);
        preview_->setFixedHeight(getBgHeight());

        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->setAlignment(Qt::AlignTop);
        rootLayout->addWidget(preview_);
        rootLayout->addSpacing(Utils::scale_value(16));

        auto areaLayout = Utils::emptyHLayout();
        areaLayout->addSpacerItem(new QSpacerItem(0, thumbsAreaMaxHeight(), QSizePolicy::Fixed, QSizePolicy::Expanding));
        areaLayout->addWidget(scrollArea_);

        rootLayout->addLayout(areaLayout);

        if (targetContact_.isEmpty())
        {
            setToAll_ = new CheckBox(this);
            setToAll_->setText(QT_TRANSLATE_NOOP("wallpaper_select", "Reset all previous"));
            setToAll_->setFixedHeight(checkboxHeight());

            auto cbLayout = Utils::emptyHLayout();
            cbLayout->addSpacing(getLeftMargin());
            cbLayout->addWidget(setToAll_);
            rootLayout->addSpacing(Utils::scale_value(16));
            rootLayout->addLayout(cbLayout);
        }
        else
        {
            contactName_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(targetContact_), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            contactName_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            contactName_->evaluateDesiredSize();
            contactName_->setOffsets(getLeftMargin(), Utils::scale_value(2));
        }

        fillWallpapersList();

        preview_->updateFor(targetContact_);

        dragOverlay_->hide();
        connect(dragOverlay_, &DragOverlay::fileDropped, this, &WallpaperSelectWidget::onFileSelected);
        connect(dragOverlay_, &DragOverlay::imageDropped, this, [this](const QPixmap& _pixmap)
        {
            updateSelection(userWallpaper_->getId());
            onImageLoaded(_pixmap);
        });
        setAcceptDrops(true);
    }

    WallpaperSelectWidget::~WallpaperSelectWidget()
    {
        Styling::getThemesContainer().resetTryOnWallpaper();
        Styling::getThemesContainer().unloadUnusedWallpaperImages();
    }

    void WallpaperSelectWidget::paintEvent(QPaintEvent *)
    {
        if (contactName_)
        {
            QPainter p(this);
            contactName_->draw(p);
        }
    }

    void WallpaperSelectWidget::resizeEvent(QResizeEvent *)
    {
        if (contactName_)
            contactName_->getHeight(width() - contactName_->horOffset() * 2);

        dragOverlay_->setGeometry(scrollArea_->geometry());
    }

    void WallpaperSelectWidget::dragEnterEvent(QDragEnterEvent* _e)
    {
        if (!dragOverlay_->isVisible())
            showDragOverlay();

        _e->acceptProposedAction();
    }

    void WallpaperSelectWidget::dragLeaveEvent(QDragLeaveEvent* _e)
    {
        _e->accept();
    }

    void WallpaperSelectWidget::dragMoveEvent(QDragMoveEvent* _e)
    {
        if (!scrollArea_->geometry().contains(_e->pos()))
        {
            if (dragOverlay_->isVisible())
                hideDragOverlay();
        }
        else if (!dragOverlay_->isVisible())
        {
            showDragOverlay();
        }

        _e->acceptProposedAction();
    }

    void WallpaperSelectWidget::onOkClicked()
    {
        if (isSetToAllChecked())
            Styling::getThemesContainer().resetAllContactWallpapers();

        if (selectedId_.isValid())
        {
            const auto globalId = Styling::getThemesContainer().getGlobalWallpaperId();
            const auto selectedGlobal = globalId == selectedId_;
            const auto selectedUserWallpaper = selectedId_ == userWallpaper_->getId();

            if (selectedUserWallpaper)
                Styling::getThemesContainer().addUserWallpaper(userWallpaper_);

            if (targetContact_.isEmpty())
            {
                if (!selectedGlobal || selectedUserWallpaper)
                    Styling::getThemesContainer().setGlobalWallpaper(selectedId_);
            }
            else
            {
                const auto curContId = Styling::getThemesContainer().getContactWallpaperId(targetContact_);
                if (curContId.isValid() && selectedGlobal) // reset contact wallpaper to selected global
                    Styling::getThemesContainer().resetContactWallpaper(targetContact_);
                else
                    Styling::getThemesContainer().setContactWallpaper(targetContact_, selectedId_);
            }

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::wallpaperscr_action, { {"use" , isSetToAllChecked() ? "all" : "this"}, {"from", fromSettings_ ? "settings" : "profile" }, {"do", "native"} });
        }
    }

    void WallpaperSelectWidget::onUserWallpaperClicked(const Styling::WallpaperId& _id)
    {
        QFileDialog fileDialog(platform::is_linux() ? nullptr : this);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setViewMode(QFileDialog::Detail);
        fileDialog.setDirectory(get_gui_settings()->get_value<QString>(settings_appearance_last_directory, QDir::homePath()));

        fileDialog.setNameFilter(QT_TRANSLATE_NOOP("avatar_upload", "Images (*.jpg *.jpeg *.png *.bmp)"));

        if (fileDialog.exec())
        {
            get_gui_settings()->set_value<QString>(settings_appearance_last_directory, fileDialog.directory().absolutePath());

            onFileSelected(fileDialog.selectedFiles().constFirst());
        }
    }

    void WallpaperSelectWidget::onWallpaperClicked(const Styling::WallpaperId& _id)
    {
        Styling::getThemesContainer().setTryOnWallpaper(_id, Styling::getTryOnAccount());
        preview_->updateFor(Styling::getTryOnAccount());
        updateSelection(_id);
    }

    void WallpaperSelectWidget::onImageLoaded(const QPixmap& pixmap)
    {
        if (pixmap.isNull())
            return;

        userWallpaper_->setWallpaperImage(Utils::tintImage(pixmap, userWallpaper_->getTint()));

        if (selectedId_ == userWallpaper_->getId())
        {
            Styling::getThemesContainer().setTryOnWallpaper(userWallpaper_, Styling::getTryOnAccount());
            preview_->updateFor(Styling::getTryOnAccount());
            updateSelection(userWallpaper_->getId());
        }
    }

    void WallpaperSelectWidget::onUserWallpaperTileChanged(const bool _tile)
    {
        userWallpaper_->setTiled(_tile);
    }

    void WallpaperSelectWidget::onFileSelected(const QString& _path)
    {
        assert(!_path.isEmpty());
        if (_path.isEmpty())
            return;

        auto task = new Utils::LoadPixmapFromFileTask(_path, Utils::getMaxImageSize());

        QObject::connect(task, &Utils::LoadPixmapFromFileTask::loadedSignal, this, &WallpaperSelectWidget::onImageLoaded);
        QThreadPool::globalInstance()->start(task);

        updateSelection(userWallpaper_->getId());
    }

    void WallpaperSelectWidget::fillWallpapersList()
    {
        const auto wallpapers = Styling::getThemesContainer().getAvailableWallpapers(Styling::getThemesContainer().getCurrentThemeId());
        const auto defaultWp = Styling::getThemesContainer().getCurrentTheme()->getDefaultWallpaper();

        const auto addWallpaper = [this, defaultWpId = defaultWp->getId()](const auto& _wpWidget, const auto& _slot)
        {
            const auto wpId = _wpWidget->getId();

            if (wpId == defaultWpId)
                _wpWidget->setCaption(QT_TRANSLATE_NOOP("wallpaper_select", "Default"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

            Utils::grabTouchWidget(_wpWidget);
            Testing::setAccessibleName(_wpWidget, qsl("AS wallpaper ") % wpId.id_);
            thumbLayout_->addWidget(_wpWidget);

            connect(_wpWidget, &WallpaperWidget::wallpaperClicked, this, _slot);
        };

        {
            auto selFileWidget = new UserWallpaperWidget(this, userWallpaper_);
            selFileWidget->setCaption(QT_TRANSLATE_NOOP("wallpaper_select", "From a file"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            addWallpaper(selFileWidget, &WallpaperSelectWidget::onUserWallpaperClicked);
        }

        for (const auto& wp : wallpapers)
        {
            if (!wp->getId().isUser())
                addWallpaper(new WallpaperWidget(this, wp), &WallpaperSelectWidget::onWallpaperClicked);
        }

        auto curWpId = targetContact_.isEmpty()
            ? Styling::getThemesContainer().getGlobalWallpaperId()
            : Styling::getThemesContainer().getContactWallpaperId(targetContact_);

        if (!targetContact_.isEmpty() && curWpId.isUser() && curWpId != userWallpaper_->getId())
            curWpId = userWallpaper_->getId();

        updateSelection(curWpId);
    }

    void WallpaperSelectWidget::touchScrollStateChanged(QScroller::State _st)
    {
        for (int i = 0; i < thumbLayout_->count(); ++i)
        {
            QLayoutItem* layoutItem = thumbLayout_->itemAt(i);
            if (QWidget* w = layoutItem->widget())
                w->setAttribute(Qt::WA_TransparentForMouseEvents, _st != QScroller::Inactive);
        }
    }

    bool WallpaperSelectWidget::isSetToAllChecked() const
    {
        return setToAll_ && setToAll_->isChecked();
    }

    void WallpaperSelectWidget::updateSelection(const Styling::WallpaperId& _id)
    {
        const auto widgets = findChildren<WallpaperWidget*>();
        for (auto wpWidget : widgets)
            wpWidget->setSelected(_id == wpWidget->getId());

        selectedId_ = _id;
    }

    void WallpaperSelectWidget::showDragOverlay()
    {
        dragOverlay_->raise();
        dragOverlay_->show();
        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void WallpaperSelectWidget::hideDragOverlay()
    {
        dragOverlay_->hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void showWallpaperSelectionDialog(const QString& _aimId, bool _fromSettings)
    {
        auto ws = new WallpaperSelectWidget(nullptr, _aimId);

        GeneralDialog dialog(ws, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, QSize(getDialogWidth(), 0));
        dialog.addLabel(_aimId.isEmpty() ? QT_TRANSLATE_NOOP("wallpaper_select", "Chats wallpaper") : QT_TRANSLATE_NOOP("wallpaper_select", "Chat wallpaper"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(ws->layout()))
        {
            const auto neededHeight = _aimId.isEmpty() ? captionHeightAllChats() : captionHeightSingleChat();
            const auto curHeight = dialog.getHeaderHeight();
            vLayout->insertSpacing(0, neededHeight - curHeight);
        }

        auto [okBtn, _] = dialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "APPLY"), true);
        QObject::connect(okBtn, &DialogButton::clicked, ws, &WallpaperSelectWidget::onOkClicked);

        dialog.showInCenter();
    }
}

#include "stdafx.h"

#include "FilesWidget.h"
#include "MainWindow.h"

#include "controls/TextEditEx.h"
#include "controls/FileSharingIcon.h"
#include "controls/CustomButton.h"
#include "controls/EmojiPickerDialog.h"

#include "fonts.h"
#include "gui_settings.h"

#include "utils/utils.h"
#include "utils/MimeDataUtils.h"
#include "utils/features.h"
#include "utils/translator.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromFileTask.h"
#include "utils/LoadFirstFrameTask.h"
#include "input_widget/InputWidgetUtils.h"

#include "history_control/FileSizeFormatter.h"
#include "history_control/ThreadPlate.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto DIALOG_WIDTH = 340;
    constexpr auto DIALOG_HEIGHT = 178;
    constexpr auto HEADER_HEIGHT = 56;
    constexpr auto INPUT_AREA = 56;
    constexpr auto INPUT_AREA_MAX = 86;
    constexpr auto HOR_OFFSET = 16;
    constexpr auto HOR_OFFSET_MY = 16;
    constexpr auto HOR_OFFSET_SMALL = 8;
    constexpr auto VER_OFFSET = 10;
    constexpr auto INPUT_TOP_OFFSET = 20;
    constexpr auto INPUT_BOTTOM_OFFSET = 8;
    constexpr auto BOTTOM_OFFSET = 12;
    constexpr auto DEFAULT_INPUT_HEIGHT = 24;
    constexpr auto MIN_PREVIEW_HEIGHT = 80;
    constexpr auto MAX_PREVIEW_HEIGHT = 268;
    constexpr auto PREVIEW_TOP_OFFSET = 16;
    constexpr auto FILE_PREVIEW_SIZE = 44;
    constexpr auto FILE_PREVIEW_HOR_OFFSET = 12;
    constexpr auto FILE_PREVIEW_VER_OFFSET = 16;
    constexpr auto FILE_NAME_OFFSET = 2;
    constexpr auto FILE_SIZE_OFFSET = 4;
    constexpr auto REMOVE_BUTTON_SIZE = 20;
    constexpr auto REMOVE_BUTTON_OFFSET = 4;
    constexpr auto PLACEHOLDER_HEIGHT = 124;
    constexpr auto DURATION_WIDTH = 36;
    constexpr auto DURATION_HEIGHT = 20;
    constexpr auto VIDEO_ICON_SIZE = 16;
    constexpr auto RIGHT_OFFSET = 6;
    constexpr auto durationFontSize = platform::is_apple() ? 11 : 13;

    const auto roundRadius() noexcept { return Utils::scale_value(5); }
    const auto threadPlateMargin() noexcept { return Utils::scale_value(16); }
    constexpr auto emojiWidgetWidthNonScaled() { return 32; }
    const auto emojiWidgetSize() noexcept { return Utils::scale_value(QSize(emojiWidgetWidthNonScaled(), emojiWidgetWidthNonScaled())); }
    constexpr auto emojiWidgetRightMarginNonScaled() { return 8; }
    const auto emojiWidgetRightMargin() noexcept { return Utils::scale_value(emojiWidgetRightMarginNonScaled()); }
    const auto descriptionInputSize() noexcept
    {
        return Utils::scale_value(QSize(
            DIALOG_WIDTH - HOR_OFFSET * 2 - emojiWidgetWidthNonScaled() - emojiWidgetRightMarginNonScaled(),
            DEFAULT_INPUT_HEIGHT));
    }
    const auto descriptionMinimumHeight() noexcept { return Utils::scale_value(32); }
    const auto descriptionDefaultHeight() noexcept { return Utils::scale_value(48); }

    QSize addEmojiIconSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }

    auto getRemoveIcon(Styling::StyleVariable _color)
    {
        return Utils::StyledPixmap::scaled(qsl(":/history_icon"), QSize(REMOVE_BUTTON_SIZE, REMOVE_BUTTON_SIZE), Styling::ThemeColorKey{ _color });
    }

    Utils::StyledPixmap getRemoveIcon()
    {
        static auto remove_normal = getRemoveIcon(Styling::StyleVariable::BASE_SECONDARY);
        return remove_normal;
    }

    Utils::StyledPixmap getRemoveIconHover()
    {
        static auto remove_hover = getRemoveIcon(Styling::StyleVariable::BASE_SECONDARY_HOVER);
        return remove_hover;
    }

    Utils::StyledPixmap getRemoveIconActive()
    {
        static auto remove_active = getRemoveIcon(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
        return remove_active;
    }

    QPixmap getDurationIcon()
    {
        static auto video_icon = Utils::StyledPixmap::scaled(qsl(":/videoplayer/duration_time_icon"), QSize(VIDEO_ICON_SIZE, VIDEO_ICON_SIZE), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });
        return video_icon.actualPixmap();
    }

    QString formatDuration(const qint64 _seconds)
    {
        if (_seconds < 0)
            return qsl("00:00");

        const auto minutes = (_seconds / 60);
        const auto seconds = (_seconds % 60);

        return qsl("%1:%2")
            .arg(minutes, 2, 10, ql1c('0'))
            .arg(seconds, 2, 10, ql1c('0'));
    }

    bool isDragDisallowed(const QMimeData* _mimeData)
    {
        bool oneExternalUrl = _mimeData->hasUrls() && _mimeData->urls().count() == 1 && !_mimeData->urls().constFirst().isLocalFile();
        return oneExternalUrl || MimeData::isMimeDataWithImage(_mimeData);
    }
}

namespace Ui
{
    FilesAreaItem::FilesAreaItem(const FileToSend& _file, int _width)
        : file_(_file)
    {
        if (file_.isClipboardImage())
        {
            initText(QT_TRANSLATE_NOOP("files_widget", "Copied from clipboard"), -1, _width);
            localPreviewLoaded(file_.getPixmap(), file_.getPixmap().size());
        }
        else
        {
            const auto& fi = file_.getFileInfo();
            initText(fi.fileName(), file_.getSize(), _width);

            if (Utils::is_image_extension(fi.suffix()))
            {
                auto task = new Utils::LoadPixmapFromFileTask(fi.absoluteFilePath());
                QObject::connect(task, &Utils::LoadPixmapFromFileTask::loadedSignal, this, &FilesAreaItem::localPreviewLoaded);
                QThreadPool::globalInstance()->start(task);
            }
            else if (Utils::is_video_extension(fi.suffix()))
            {
                auto task = new Utils::LoadFirstFrameTask(fi.absoluteFilePath());
                QObject::connect(task, &Utils::LoadFirstFrameTask::loadedSignal, this, &FilesAreaItem::localPreviewLoaded);
                QThreadPool::globalInstance()->start(task);
            }
            else
            {
                preview_ = FileSharingIcon::getIcon(fi.absoluteFilePath());
                iconPreview_ = true;
            }
        }
    }


    bool FilesAreaItem::operator==(const FilesAreaItem& _other) const
    {
        return file_ == _other.file_;
    }

    void FilesAreaItem::draw(QPainter& _p, const QPoint& _at)
    {
        Utils::PainterSaver ps(_p);
        if (iconPreview_)
        {
            _p.drawPixmap(_at, preview_);
        }
        else
        {
            auto b = QBrush(preview_);
            QMatrix m;
            m.translate(_at.x(), _at.y());
            b.setMatrix(m);
            _p.setBrush(b);
            _p.setPen(Qt::NoPen);
            _p.drawRoundedRect(QRect(_at, preview_.size()), roundRadius(), roundRadius());
        }

        const auto x = _at.x() + Utils::scale_value(FILE_PREVIEW_SIZE + FILE_PREVIEW_HOR_OFFSET);
        const auto y = _at.y() + Utils::scale_value(FILE_NAME_OFFSET);

        name_->setOffsets(x, y);
        name_->draw(_p);

        if (size_)
        {
            size_->setOffsets(x, y + name_->cachedSize().height());
            size_->draw(_p);
        }
    }

    void FilesAreaItem::initText(const QString& _name, int64_t _size, int _width)
    {
        name_ = TextRendering::MakeTextUnit(_name, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        name_->init({ Fonts::appFontScaled(16), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        name_->evaluateDesiredSize();
        name_->elide(_width - Utils::scale_value(FILE_PREVIEW_SIZE + FILE_SIZE_OFFSET + REMOVE_BUTTON_SIZE + FILE_NAME_OFFSET + REMOVE_BUTTON_OFFSET));

        if (_size >= 0)
        {
            size_ = TextRendering::MakeTextUnit(HistoryControl::formatFileSize(_size));
            size_->init({ Fonts::appFontScaled(13), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });
            size_->evaluateDesiredSize();
        }
    }

    void FilesAreaItem::localPreviewLoaded(QPixmap pixmap, const QSize _originalSize)
    {
        const auto minSize = std::min(_originalSize.width(), _originalSize.height());
        preview_ = pixmap.copy((pixmap.width() - minSize) / 2, (pixmap.height() - minSize) / 2, minSize, minSize);
        preview_ = preview_.scaledToHeight(Utils::scale_value(FILE_PREVIEW_SIZE), Qt::SmoothTransformation);
        Q_EMIT needUpdate();
    }

    FilesScroll::FilesScroll(QWidget* _parent, FilesArea* _area, bool _acceptDrops)
        : QScrollArea(_parent)
        , dnd_(false)
        , placeholder_(false)
        , area_(_area)
    {
        area_->setParent(this);
        setWidget(area_);
        setAcceptDrops(_acceptDrops);
    }

    void FilesScroll::paintEvent(QPaintEvent* _event)
    {
        if (dnd_ || placeholder_)
        {
            QPainter painter(viewport());

            painter.setPen(Qt::NoPen);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.95));

            const auto dashed = placeholder_ && !dnd_;
            QPen pen(Styling::getParameters().getColor(dashed ? Styling::StyleVariable::BASE_PRIMARY : Styling::StyleVariable::PRIMARY), Utils::scale_value(1), dashed ? Qt::CustomDashLine : Qt::SolidLine, Qt::RoundCap);
            if (dashed)
                pen.setDashPattern({ 2, 4 });

            painter.setPen(pen);

            static const auto padding = Utils::scale_value(2);
            static const auto padding_right = Utils::scale_value(10);

            if (dnd_)
                painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.02));

            painter.drawRoundedRect(
                padding,
                padding,
                rect().width() - padding * 2 - padding_right,
                rect().height() - padding * 2,
                roundRadius(),
                roundRadius()
            );

            painter.setFont(Fonts::appFontScaled(24));
            Utils::drawText(
                painter,
                QPointF(rect().width() / 2, rect().height() / 2),
                Qt::AlignCenter,
                QT_TRANSLATE_NOOP("files_widget", "Add files")
            );
        }
        else
        {
            QScrollArea::paintEvent(_event);
        }
    }

    void FilesScroll::setPlaceholder(bool _placeholder)
    {
        placeholder_ = _placeholder;
        update();
    }

    void FilesScroll::dragEnterEvent(QDragEnterEvent* _event)
    {
        if (isDragDisallowed(_event->mimeData()))
        {
            _event->ignore();
            return;
        }

        dnd_ = true;
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area_->hide();
        update();
        _event->acceptProposedAction();
    }

    void FilesScroll::dragLeaveEvent(QDragLeaveEvent* _event)
    {
        dnd_ = false;
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area_->show();
        update();
        _event->accept();
    }

    void FilesScroll::dropEvent(QDropEvent* _event)
    {
        const QMimeData* mimeData = _event->mimeData();

        if (isDragDisallowed(mimeData))
        {
            _event->ignore();
            return;
        }

        if (mimeData->hasUrls())
        {
            const QList<QUrl> urlList = mimeData->urls();
            for (const QUrl& url : urlList)
            {
                if (url.isLocalFile())
                    area_->addItem(new FilesAreaItem(url.toLocalFile(), width()));
            }
        }
        _event->acceptProposedAction();
        dnd_ = false;
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area_->show();
        update();
    }



    FilesArea::FilesArea(QWidget* _parent)
        : QWidget(_parent)
        , removeIndex_(-1)
        , hovered_(false)
        , dnd_(false)
    {
        setMouseTracking(true);
        setFixedHeight(0);
    }

    void FilesArea::addItem(FilesAreaItem* _item)
    {
        if (items_.empty())
            setFixedHeight(0);

        connect(_item, &FilesAreaItem::needUpdate, this, &FilesArea::needUpdate);

        items_.emplace_back(_item);

        auto h = height() + Utils::scale_value(FILE_PREVIEW_SIZE);
        if (items_.size() > 1)
            h += Utils::scale_value(FILE_PREVIEW_VER_OFFSET);

        setFixedHeight(h);
        Q_EMIT sizeChanged();
    }

    FilesToSend FilesArea::getItems() const
    {
        FilesToSend result;
        result.reserve(items_.size());
        for (const auto& i : items_)
            result.push_back(i->getFile());

        return result;
    }

    int FilesArea::count() const
    {
        return items_.size();
    }

    void FilesArea::clear()
    {
        items_.clear();
        update();
        setFixedHeight(0);
    }

    void FilesArea::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        QPoint at(0, 0);
        auto j = 0;
        for (const auto& i : items_)
        {
            if (j == removeIndex_)
                p.drawPixmap(width() - Utils::scale_value(REMOVE_BUTTON_SIZE + REMOVE_BUTTON_OFFSET), at.y() + Utils::scale_value(FILE_PREVIEW_SIZE) / 2 - Utils::scale_value(REMOVE_BUTTON_SIZE) / 2, removeButton_.actualPixmap());

            i->draw(p, at);
            at.setY(at.y() + Utils::scale_value(FILE_PREVIEW_SIZE + FILE_PREVIEW_VER_OFFSET));
            ++j;
        }
    }

    void FilesArea::mouseMoveEvent(QMouseEvent* _event)
    {
        removeIndex_ = -1;
        auto r = QRect(0, 0, width(), Utils::scale_value(FILE_PREVIEW_SIZE));
        for (size_t j = 0; j < items_.size(); ++j)
        {
            if (r.contains(_event->pos()))
            {
                removeIndex_ = j;

                auto removeRect = QRect(width() - Utils::scale_value(REMOVE_BUTTON_SIZE), r.y() + Utils::scale_value(FILE_PREVIEW_SIZE) / 2 - Utils::scale_value(REMOVE_BUTTON_SIZE) / 2,
                                        Utils::scale_value(REMOVE_BUTTON_SIZE), Utils::scale_value(REMOVE_BUTTON_SIZE));

                hovered_ = removeRect.contains(_event->pos());
                removeButton_ = hovered_ ? getRemoveIconHover() : getRemoveIcon();
            }

            r.setY(r.y() + Utils::scale_value(FILE_PREVIEW_SIZE + FILE_PREVIEW_VER_OFFSET));
            r.setHeight(Utils::scale_value(FILE_PREVIEW_SIZE));
        }
        setCursor(hovered_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
        return QWidget::mouseMoveEvent(_event);
    }

    void FilesArea::leaveEvent(QEvent* _event)
    {
        removeIndex_ = -1;
        hovered_ = false;
        update();
        return QWidget::leaveEvent(_event);
    }

    void FilesArea::mousePressEvent(QMouseEvent* _event)
    {
        if (hovered_)
        {
            removeButton_ = getRemoveIconActive();
            update();
        }

        return QWidget::mousePressEvent(_event);
    }

    void FilesArea::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (hovered_)
        {
            removeButton_ = getRemoveIconHover();
            auto i = 0;
            for (auto iter = items_.begin(); iter != items_.end(); ++iter)
            {
                if (i == removeIndex_)
                {
                    items_.erase(iter);
                    auto h = height();
                    h -= Utils::scale_value(FILE_PREVIEW_SIZE);
                    if (!items_.empty())
                        h -= Utils::scale_value(FILE_PREVIEW_VER_OFFSET);
                    setFixedHeight(items_.empty() ? Utils::scale_value(PLACEHOLDER_HEIGHT) : h);
                    Q_EMIT sizeChanged();
                    break;
                }
                ++i;
            }
            update();
        }

        return QWidget::mouseReleaseEvent(_event);
    }

    void FilesArea::needUpdate()
    {
        update();
    }

    FilesWidget::FilesWidget(const FilesToSend& _files, Target _target, QWidget* _parent)
        : QWidget(_parent)
        , currentDocumentHeight_(-1)
        , neededHeight_(-1)
        , drawDuration_(false)
        , isGif_(false)
        , prevMax_(-1)
    {
        title_ = TextRendering::MakeTextUnit(QString());
        title_->init({ Fonts::appFontScaled(23), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        title_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET));
        title_->evaluateDesiredSize();

        if (_target == Target::Thread)
        {
            threadPlate_ = ThreadPlate::plateForPopup(this);
            threadPlate_->updateGeometry();
            threadPlate_->show();
        }

        descriptionBox_ = new TextEditBox(this, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, false, addEmojiIconSize());
        descriptionBox_->initUi();
        descriptionBox_->layout()->setSpacing(emojiWidgetRightMargin());
        descriptionBox_->layout()->setContentsMargins({ 0, 0, 0, Utils::scale_value(4) });
        descriptionBox_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - 2*HOR_OFFSET));
        Utils::ApplyStyle(descriptionBox_, Styling::getParameters().getTextEditBoxCommonQss());

        description_ = descriptionBox_->textEdit<TextEditEx>();
        description_->document()->documentLayout()->setPaintDevice(nullptr);//avoid unnecessary scaling performed by its own paintDevice
        description_->setFormatEnabled(Features::isFormattingInInputEnabled());
        description_->setTabChangesFocus(true);
        description_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        description_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        description_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        description_->setFrameStyle(QFrame::NoFrame);
        description_->setDocumentMargin(0);
        description_->setHeightSupplement(Utils::scale_value(4));
        description_->setContentsMargins(0, 0, 0, 0);
        description_->setPlaceholderText(QT_TRANSLATE_NOOP("files_widget", "Add caption"));
        description_->setAcceptDrops(false);
        description_->setUndoRedoEnabled(true);
        description_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
        description_->setFixedWidth(descriptionInputSize().width());
        description_->setStyleSheet(qsl("background-color: transparent; border-style: none;"));
        connect(description_, &TextEditEx::enter, this, &FilesWidget::enter);
        Testing::setAccessibleName(description_, qsl("AS FilesWidget description"));

        emojiButton_ = descriptionBox_->emojiButton<CustomButton>();
        emojiButton_->setDefaultImage(qsl(":/smile"));
        emojiButton_->setFixedSize(addEmojiIconSize());
        emojiButton_->setCheckable(true);
        emojiButton_->setChecked(false);
        emojiButton_->setFocusPolicy(Qt::TabFocus);
        emojiButton_->setFocusColor(focusColorPrimaryKey());
        updateButtonColors(emojiButton_, InputStyleMode::Default);
        Testing::setAccessibleName(emojiButton_, qsl("AS FilesWidget emojiWidget"));
        connect(emojiButton_, &CustomButton::clicked, this, &FilesWidget::popupEmojiPicker);

        filesArea_ = new FilesArea(this);
        connect(filesArea_, &FilesArea::sizeChanged, this, &FilesWidget::updateFilesArea);
        area_ = new FilesScroll(this, filesArea_, true);
        area_->setStyleSheet(qsl("background: transparent;"));
        area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area_->setWidgetResizable(true);
        area_->setFocusPolicy(Qt::NoFocus);
        area_->setFrameShape(QFrame::NoFrame);
        Utils::ApplyStyle(area_->verticalScrollBar(), Styling::getParameters().getVerticalScrollBarSimpleQss());//ok

        filesArea_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET - RIGHT_OFFSET));
        area_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET - RIGHT_OFFSET));

#ifdef _WIN32
        DragAcceptFiles((HWND)Utils::InterConnector::instance().getMainWindow()->winId(), TRUE);
#endif //_WIN32

        connect(area_->verticalScrollBar(), &QScrollBar::rangeChanged, this, &FilesWidget::scrollRangeChanged);
        connect(description_, &TextEditEx::textChanged, this, &FilesWidget::descriptionChanged);

        initPreview(_files);
        updateDescriptionHeight();
    }

    FilesWidget::~FilesWidget() = default;

    FilesToSend FilesWidget::getFiles() const
    {
        if (filesArea_->count() == 0)
            return { singleFile_ };
        else
            return filesArea_->getItems();
    }

    Data::FString FilesWidget::getDescription() const
    {
        return description_->getText();
    }

    int FilesWidget::getCursorPos() const
    {
        return description_->textCursor().position();
    }

    const Data::MentionMap& FilesWidget::getMentions() const
    {
        return description_->getMentions();
    }

    void FilesWidget::setFocusOnInput()
    {
        if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
        {
            if (!mainWindow->isActive())
                mainWindow->activate();
        }
        description_->setFocus();
    }

    void FilesWidget::setDescription(const Data::FString& _text, const Data::MentionMap& _mentions)
    {
        description_->setMentions(_mentions);
        description_->document()->setTextWidth(description_->width());
        description_->setFormattedText(_text);
    }

    void FilesWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        p.save();
        title_->draw(p);
        p.restore();

        if (!singlePreview_.isNull())
        {
            p.setPen(Qt::NoPen);

            QRect pixmapRect = singlePreview_.rect();

            auto y = Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height();
            if (singlePreview_.height() < Utils::scale_value(MIN_PREVIEW_HEIGHT) || singlePreview_.width() < Utils::scale_value(MAX_PREVIEW_HEIGHT))
            {
                const QRect r(
                    Utils::scale_value(HOR_OFFSET),
                    y,
                    width() - Utils::scale_value(2*HOR_OFFSET),
                    std::max(singlePreview_.height(), Utils::scale_value(MIN_PREVIEW_HEIGHT))
                );

                p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
                p.drawRoundedRect(r, roundRadius(), roundRadius());

                pixmapRect.moveCenter(r.center());
            }
            else
            {
                const QRect r(
                    Utils::scale_value(HOR_OFFSET),
                    y,
                    width() - Utils::scale_value(2*HOR_OFFSET),
                    singlePreview_.height()
                );

                pixmapRect.moveCenter(r.center());

                QPainterPath clipPath;
                clipPath.addRoundedRect(pixmapRect, roundRadius(), roundRadius());
                p.setClipPath(clipPath);
            }
            p.drawPixmap(pixmapRect, singlePreview_);

            if (drawDuration_)
            {
                p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.5));
                p.drawRoundedRect(pixmapRect.x() + Utils::scale_value(HOR_OFFSET_SMALL), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL), Utils::scale_value(DURATION_WIDTH) + (isGif_ ? 0 : Utils::scale_value(VIDEO_ICON_SIZE)), Utils::scale_value(DURATION_HEIGHT), Utils::scale_value(8), Utils::scale_value(8));

                if (!isGif_)
                    p.drawPixmap(Utils::scale_value(HOR_OFFSET + HOR_OFFSET_SMALL) + Utils::scale_value(1), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL) + Utils::scale_value(2), getDurationIcon());

                if (durationLabel_)
                {
                    durationLabel_->setOffsets(pixmapRect.x() + Utils::scale_value(HOR_OFFSET_SMALL) + (Utils::scale_value(DURATION_WIDTH) - durationLabel_->desiredWidth()) / 2 + (isGif_ ? 0 : Utils::scale_value(VIDEO_ICON_SIZE)), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL) + Utils::scale_value(DURATION_HEIGHT) / 2);
                    durationLabel_->draw(p, TextRendering::VerPosition::MIDDLE);
                }
            }
        }

        QWidget::paintEvent(_event);
    }

    void FilesWidget::resizeEvent(QResizeEvent* _event)
    {
        if (threadPlate_)
        {
            threadPlate_->move(width() - threadPlate_->width() - Utils::scale_value(HOR_OFFSET), title_->verOffset() + (title_->cachedSize().height() - threadPlate_->height()) / 2);
            title_->elide(width() - threadPlate_->width() - threadPlateMargin());
        }
    }

    void FilesWidget::keyPressEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Escape && emojiPicker_)
            emojiPicker_->close();
        else
            QWidget::keyPressEvent(_event);
    }

    void FilesWidget::descriptionChanged()
    {
        updateDescriptionHeight();
    }

    void FilesWidget::updateFilesArea()
    {
        auto items = filesArea_->getItems();

        if (!singlePreview_.isNull())
        {
            QSignalBlocker sb(filesArea_);
            filesArea_->clear();
            filesArea_->addItem(new FilesAreaItem(singleFile_, Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2)));
            filesArea_->addItem(new FilesAreaItem(*items.begin(), Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2)));
            singlePreview_ = QPixmap();
            items = filesArea_->getItems();
        }

        if (items.size() == 1)
            initPreview(items);

        area_->setPlaceholder(items.empty());
        updateTitle();
        Q_EMIT setButtonActive(!items.empty());
        updateSize();
    }

    void FilesWidget::localPreviewLoaded(QPixmap pixmap, const QSize _originalSize)
    {
        auto items = filesArea_->getItems();
        if(items.size() == 1)
        {
            filesArea_->clear();
            if (!pixmap.isNull())
            {
                if (pixmap.width() > pixmap.height())
                {
                    if (pixmap.width() > Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2))
                        singlePreview_ = pixmap.scaledToWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2), Qt::SmoothTransformation);
                    else
                        singlePreview_ = pixmap;
                }
                else
                {
                    if (pixmap.height() > Utils::scale_value(MAX_PREVIEW_HEIGHT))
                        singlePreview_ = pixmap.scaledToHeight(Utils::scale_value(MAX_PREVIEW_HEIGHT), Qt::SmoothTransformation);
                    else
                        singlePreview_ = pixmap;
                }
            }

            updateSize();
            update();
            singleFile_ = items.at(0);
        }
    }

    void FilesWidget::enter()
    {
        if (filesArea_->count() > 0 || !singlePreview_.isNull())
            Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
    }

    void FilesWidget::duration(qint64 _duration)
    {
        setDuration(formatDuration(_duration));
    }

    void FilesWidget::scrollRangeChanged(int, int max)
    {
        if (max > prevMax_ && prevMax_ != -1)
            area_->verticalScrollBar()->setValue(max);

        prevMax_ = max;
    }

    void FilesWidget::popupEmojiPicker(bool _on)
    {
        if (emojiPicker_ && !_on)
        {
            emojiPicker_->close();
        }
        else
        {
            if (!emojiPicker_)
                emojiPicker_ = new EmojiPickerDialog(description_, emojiButton_, this);
            connect(emojiPicker_, &EmojiPickerDialog::emojiSelected, this, &FilesWidget::onEmojiSelected);
            connect(emojiPicker_, &EmojiPickerDialog::destroyed, this, [this]()
            {
                emojiButton_->setChecked(false);
            });
            emojiPicker_->popup();
            description_->setFocus();
        }
    }

    void FilesWidget::onEmojiSelected(const Emoji::EmojiCode& _code)
    {
        description_->insertEmoji(_code);
        description_->setFocus();
    }

    void FilesWidget::updateSize()
    {
        auto descrHeight = std::max(descriptionBox_->height(), descriptionDefaultHeight());
        auto h = Utils::scale_value(HEADER_HEIGHT + PREVIEW_TOP_OFFSET + BOTTOM_OFFSET) + descrHeight;
        if (!singlePreview_.isNull())
        {
            const auto needHeight = std::max(singlePreview_.height(), Utils::scale_value(MIN_PREVIEW_HEIGHT));
            h += needHeight;
            area_->setFixedHeight(needHeight);
        }
        else
        {
            area_->setFixedHeight(std::min(Utils::scale_value(MAX_PREVIEW_HEIGHT), filesArea_->height()));
            h += area_->height();
        }

        setFixedSize(Utils::scale_value(DIALOG_WIDTH), std::max(h, Utils::scale_value(DIALOG_HEIGHT)));

        area_->move(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height());

        const auto yOffset = height() - BOTTOM_OFFSET - descriptionBox_->height();
        descriptionBox_->move(Utils::scale_value(HOR_OFFSET), yOffset);

        update();
    }

    void FilesWidget::setDuration(const QString& _duration)
    {
        if (!durationLabel_)
        {
            durationLabel_ = TextRendering::MakeTextUnit(_duration);

            TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaledFixed(durationFontSize, Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT } };
            params.maxLinesCount_ = 1;
            durationLabel_->init(params);
        }
        else
        {
            durationLabel_->setText(_duration);
        }
        durationLabel_->evaluateDesiredSize();
    }

    void FilesWidget::initPreview(const FilesToSend& _files)
    {
        singlePreview_ = QPixmap();
        singleFile_ = FileToSend();
        drawDuration_ = false;

        if (_files.empty())
            return;

        const auto& file = _files.front();

        const auto addFilesToArea = [this, &file, &_files]()
        {
            QSignalBlocker sb(filesArea_);
            for (const auto& f : _files)
                filesArea_->addItem(new FilesAreaItem(f, Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2)));
            singleFile_ = file;
        };

        if (_files.size() == 1)
        {
            if (file.isFile())
            {
                const auto& fi = file.getFileInfo();
                const auto ext = fi.suffix();

                if (ext.compare(u"gif", Qt::CaseInsensitive) == 0)
                {
                    drawDuration_ = true;
                    isGif_ = true;
                    setDuration(QT_TRANSLATE_NOOP("files_widget", "GIF"));
                }
                else
                {
                    drawDuration_ = Utils::is_video_extension(ext);
                }

                if (Utils::is_image_extension(ext))
                {
                    auto task = new Utils::LoadPixmapFromFileTask(fi.absoluteFilePath());
                    connect(task, &Utils::LoadPixmapFromFileTask::loadedSignal, this, &FilesWidget::localPreviewLoaded);
                    QThreadPool::globalInstance()->start(task);
                }
                else if (Utils::is_video_extension(ext))
                {
                    auto task = new Utils::LoadFirstFrameTask(fi.absoluteFilePath());
                    connect(task, &Utils::LoadFirstFrameTask::loadedSignal, this, &FilesWidget::localPreviewLoaded);
                    connect(task, &Utils::LoadFirstFrameTask::duration, this, &FilesWidget::duration);
                    QThreadPool::globalInstance()->start(task);
                }

                if (filesArea_->getItems() != _files)
                    addFilesToArea();
            }
            else
            {
                addFilesToArea();
                localPreviewLoaded(file.getPixmap(), file.getPixmap().size());
            }
        }
        else
        {
            addFilesToArea();
        }
        updateTitle();
    }

    void FilesWidget::updateDescriptionHeight()
    {
        const auto newDocHeight = description_->getTextHeight();

        if ((currentDocumentHeight_ != -1 && newDocHeight == currentDocumentHeight_) || newDocHeight == 0)
            return;

        static const auto minBoxHeight = Utils::scale_value(platform::is_apple() ? 37 : 36); // add 1px for apple retina

        static const auto marginDiff = Utils::scale_value(INPUT_TOP_OFFSET + INPUT_BOTTOM_OFFSET);
        static const auto minHeight = Utils::scale_value(INPUT_AREA) - marginDiff;
        static const auto maxHeight = Utils::scale_value(INPUT_AREA_MAX)  - marginDiff / 2;

        if (currentDocumentHeight_ == -1)
        {
            neededHeight_ = std::max(newDocHeight, Utils::scale_value(DEFAULT_INPUT_HEIGHT));
            currentDocumentHeight_ = newDocHeight;
        }

        const auto diff = newDocHeight - currentDocumentHeight_;
        neededHeight_ += diff;

        currentDocumentHeight_ = newDocHeight;

        auto newEditHeight = neededHeight_;
        auto sbPolicy = Qt::ScrollBarAlwaysOff;

        if (newEditHeight > maxHeight)
        {
            newEditHeight = maxHeight;
            sbPolicy = Qt::ScrollBarAsNeeded;
        }
        description_->setVerticalScrollBarPolicy(sbPolicy);
        description_->setFixedHeight(newEditHeight);
        descriptionBox_->setFixedHeight(std::max(newEditHeight + (newEditHeight > minHeight) * Utils::scale_value(2), minBoxHeight));

        updateSize();
    }

    void FilesWidget::updateTitle()
    {
        if (!title_)
            return;

        const auto amount = filesArea_->count();
        auto amountString = [amount]() -> QString
        {
            return QT_TRANSLATE_NOOP("files_widget", "Send ") % QString::number(amount) % ql1c(' ') %
                Utils::GetTranslator()->getNumberString(
                    amount,
                    QT_TRANSLATE_NOOP3("files_widget", "file", "1"),
                    QT_TRANSLATE_NOOP3("files_widget", "files", "2"),
                    QT_TRANSLATE_NOOP3("files_widget", "files", "5"),
                    QT_TRANSLATE_NOOP3("files_widget", "files", "21")
                );
        };

        title_->setText(amount > 0 ? amountString() : QT_TRANSLATE_NOOP("files_widget", "Send files"));
        update();
    }
}

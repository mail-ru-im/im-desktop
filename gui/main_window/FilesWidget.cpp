#include "stdafx.h"

#include "FilesWidget.h"
#include "MainWindow.h"
#include "history_control/FileSizeFormatter.h"


#include "../controls/FileSharingIcon.h"
#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/translator.h"
#include "../utils/InterConnector.h"
#include "../utils/LoadPixmapFromFileTask.h"
#include "../utils/LoadFirstFrameTask.h"
#include "../gui_settings.h"
#include "../styles/ThemeParameters.h"

namespace
{
    const auto DIALOG_WIDTH = 340;
    const auto HEADER_HEIGHT = 56;
    const auto INPUT_OFFSET = 20;
    const auto INPUT_AREA = 56;
    const auto INPUT_AREA_MAX = 86;
    const auto HOR_OFFSET = 16;
    const auto HOR_OFFSET_SMALL = 8;
    const auto VER_OFFSET = 10;
    const auto INPUT_TOP_OFFSET = 16;
    const auto INPUT_BOTTOM_OFFSET = 8;
    const auto BOTTOM_OFFSET = 12;
    const auto DEFAULT_INPUT_HEIGHT = 24;
    const auto MIN_PREVIEW_HEIGHT = 80;
    const auto MAX_PREVIEW_HEIGHT = 308;
    const auto PREVIEW_TOP_OFFSET = 16;
    const auto PREVIEW_BOTTOM_OFFSET = 20;
    const auto FILE_PREVIEW_SIZE = 44;
    const auto FILE_PREVIEW_HOR_OFFSET = 12;
    const auto FILE_PREVIEW_VER_OFFSET = 16;
    const auto FILE_NAME_OFFSET = 2;
    const auto FILE_SIZE_OFFSET = 4;
    const auto REMOVE_BUTTON_SIZE = 20;
    const auto REMOVE_BUTTON_OFFSET = 4;
    const auto PLACEHOLDER_HEIGHT = 124;
    const auto DURATION_WIDTH = 36;
    const auto DURATION_HEIGHT = 20;
    const auto VIDEO_ICON_SIZE = 16;
    const auto RIGHT_OFFSET = 6;

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
}

namespace Ui
{
    InputEdit::InputEdit(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText)
        : TextEditEx(_parent, _font, _color, _input, _isFitToText)
    {

    }

    bool InputEdit::catchEnter(int _modifiers)
    {
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        switch (key1_to_send)
        {
        case Ui::KeyToSendMessage::Enter:
            return _modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier;
        case Ui::KeyToSendMessage::Shift_Enter:
            return _modifiers & Qt::ShiftModifier;
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return _modifiers & Qt::ControlModifier;
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return _modifiers & Qt::MetaModifier;
        default:
            break;
        }
        return false;
    }

    bool InputEdit::catchNewLine(const int _modifiers)
    {
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        const auto ctrl = _modifiers & Qt::ControlModifier;
        const auto shift = _modifiers & Qt::ShiftModifier;
        const auto meta = _modifiers & Qt::MetaModifier;
        const auto enter = (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);

        switch (key1_to_send)
        {
        case Ui::KeyToSendMessage::Enter:
            return shift || ctrl || meta;
        case Ui::KeyToSendMessage::Shift_Enter:
            return  ctrl || meta || enter;
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return shift || meta || enter;
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return shift || ctrl || enter;
        default:
            break;
        }
        return false;
    }

    FilesAreaItem::FilesAreaItem(const QString& _filepath, int _width)
        : path_(_filepath)
        , iconPreview_(false)
    {
        QFileInfo f(_filepath);

        name_ = TextRendering::MakeTextUnit(f.fileName(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        name_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        name_->evaluateDesiredSize();
        name_->elide(_width - Utils::scale_value(FILE_PREVIEW_SIZE + FILE_SIZE_OFFSET + REMOVE_BUTTON_SIZE + FILE_NAME_OFFSET + REMOVE_BUTTON_OFFSET));

        size_ = TextRendering::MakeTextUnit(HistoryControl::formatFileSize(f.size()));
        size_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        size_->evaluateDesiredSize();

        if (Utils::is_image_extension(f.suffix()))
        {
            auto task = new Utils::LoadPixmapFromFileTask(_filepath);

            QObject::connect(
                task,
                &Utils::LoadPixmapFromFileTask::loadedSignal,
                this,
                &FilesAreaItem::localPreviewLoaded);

            QThreadPool::globalInstance()->start(task);
        }
        else if (Utils::is_video_extension(f.suffix()))
        {
            auto task = new Utils::LoadFirstFrameTask(_filepath);

            QObject::connect(
                task,
                &Utils::LoadFirstFrameTask::loadedSignal,
                this,
                &FilesAreaItem::localPreviewLoaded);

            QThreadPool::globalInstance()->start(task);
        }
        else
        {
            preview_ = FileSharingIcon::getIcon(_filepath);
            iconPreview_ = true;
        }
    }

    bool FilesAreaItem::operator==(const FilesAreaItem& _other) const
    {
        return path_ == _other.path_;
    }

    void FilesAreaItem::draw(QPainter& _p, const QPoint& _at)
    {
        _p.save();
        if (iconPreview_)
        {
            _p.drawPixmap(_at, preview_);
        }
        else
        {
            _p.setRenderHint(QPainter::Antialiasing, true);
            auto b = QBrush(preview_);
            QMatrix m;
            m.translate(_at.x(), _at.y());
            b.setMatrix(m);
            _p.setBrush(b);
            _p.setPen(Qt::NoPen);
            _p.drawRoundedRect(QRect(_at.x(), _at.y(), preview_.width(), preview_.height()), Utils::scale_value(5), Utils::scale_value(5));
        }

        name_->setOffsets(_at.x() + Utils::scale_value(FILE_PREVIEW_SIZE + FILE_PREVIEW_HOR_OFFSET), _at.y() + Utils::scale_value(FILE_NAME_OFFSET));
        name_->draw(_p);
        size_->setOffsets(_at.x() + Utils::scale_value(FILE_PREVIEW_SIZE + FILE_PREVIEW_HOR_OFFSET), _at.y() + Utils::scale_value(FILE_NAME_OFFSET) + name_->cachedSize().height());
        size_->draw(_p);
        _p.restore();
    }

    QString FilesAreaItem::path() const
    {
        return path_;
    }

    void FilesAreaItem::localPreviewLoaded(QPixmap pixmap, const QSize _originalSize)
    {
        auto minSize = std::min(_originalSize.width(), _originalSize.height());
        preview_ = pixmap.copy(pixmap.width() / 2 - minSize / 2, pixmap.height() / 2 - minSize / 2, minSize, minSize);
        preview_ = preview_.scaledToHeight(Utils::scale_value(FILE_PREVIEW_SIZE), Qt::SmoothTransformation);
        emit needUpdate();
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

            static const QColor overlayColor = []() {
                auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
                c.setAlphaF(0.95);
                return c;
            }();

            painter.fillRect(rect(), overlayColor);

            QPen pen((placeholder_ && !dnd_) ? Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY) : Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Utils::scale_value(1), (placeholder_ && !dnd_) ? Qt::CustomDashLine : Qt::SolidLine, Qt::RoundCap);
            if (placeholder_ && !dnd_)
                pen.setDashPattern({ 2, 4 });

            painter.setPen(pen);

            static const auto padding = Utils::scale_value(2);
            static const auto padding_right = Utils::scale_value(10);
            static const auto radius = Utils::scale_value(5);

            if (dnd_)
            {
                auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
                color.setAlphaF(0.02);
                painter.setBrush(color);
            }

            painter.drawRoundedRect(
                padding,
                padding,
                rect().width() - padding * 2 - padding_right,
                rect().height() - padding * 2,
                radius,
                radius
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
                {
                    area_->addItem(new FilesAreaItem(url.toLocalFile(), width()));
                }
            }
        }
        _event->acceptProposedAction();
        dnd_ = false;
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area_->show();
        update();
    }

    bool FilesScroll::isDragDisallowed(const QMimeData* _mimeData) const
    {
        bool oneExternalUrl = _mimeData->hasUrls() && _mimeData->urls().count() == 1 && !_mimeData->urls().constFirst().isLocalFile();
        return oneExternalUrl || Utils::isMimeDataWithImage(_mimeData);
    }

    FilesArea::FilesArea(QWidget* _parent)
        : QWidget(_parent)
        , removeIndex_(-1)
        , hovered_(false)
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
        auto h = height();
        h += Utils::scale_value(FILE_PREVIEW_SIZE);
        if (items_.size() > 1)
            h += Utils::scale_value(FILE_PREVIEW_VER_OFFSET);

        setFixedHeight(h);
        emit sizeChanged();
    }

    QStringList FilesArea::getItems() const
    {
        QStringList result;
        for (const auto& i : items_)
            result.push_back(i->path());

        return result;
    }

    int FilesArea::count() const
    {
        return items_.size();
    }

    void FilesArea::clear()
    {
        items_.clear(); update();
        setFixedHeight(0);
    }

    void FilesArea::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        QPoint at(0, 0);
        auto j = 0;
        for (const auto& i : items_)
        {
            if (j == removeIndex_)
                p.drawPixmap(width() - Utils::scale_value(REMOVE_BUTTON_SIZE + REMOVE_BUTTON_OFFSET), at.y() + Utils::scale_value(FILE_PREVIEW_SIZE) / 2 - Utils::scale_value(REMOVE_BUTTON_SIZE) / 2, removeButton_);

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

                auto s = QSize(REMOVE_BUTTON_SIZE, REMOVE_BUTTON_SIZE);
                static auto remove_normal = Utils::renderSvgScaled(qsl(":/history_icon"), s, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
                static auto remove_hover = Utils::renderSvgScaled(qsl(":/history_icon"), s, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
                static auto remove_active = Utils::renderSvgScaled(qsl(":/history_icon"), s, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));

                auto removeRect = QRect(width() - Utils::scale_value(REMOVE_BUTTON_SIZE), r.y() + Utils::scale_value(FILE_PREVIEW_SIZE) / 2 - Utils::scale_value(REMOVE_BUTTON_SIZE) / 2,
                                        Utils::scale_value(REMOVE_BUTTON_SIZE), Utils::scale_value(REMOVE_BUTTON_SIZE));

                hovered_ = removeRect.contains(_event->pos());
                removeButton_ = hovered_ ? remove_hover : remove_normal;
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
        static auto remove_active = Utils::renderSvgScaled(qsl(":/history_icon"), QSize(REMOVE_BUTTON_SIZE, REMOVE_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
        if (hovered_)
        {
            removeButton_ = remove_active;
            update();
        }

        return QWidget::mousePressEvent(_event);
    }

    void FilesArea::mouseReleaseEvent(QMouseEvent* _event)
    {
        static auto remove_hover = Utils::renderSvgScaled(qsl(":/history_icon"), QSize(REMOVE_BUTTON_SIZE, REMOVE_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        if (hovered_)
        {
            removeButton_ = remove_hover;
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
                    emit sizeChanged();
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

    FilesWidget::FilesWidget(QWidget* _parent, const QStringList& _files, const QPixmap& _imageBuffer)
        : QWidget(_parent)
        , currentDocumentHeight_(-1)
        , neededHeight_(-1)
        , drawDuration_(false)
        , isGif_(false)
        , prevMax_(-1)
    {
        title_ = TextRendering::MakeTextUnit(QString());
        title_->init(Fonts::appFontScaled(23), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        title_->evaluateDesiredSize();

        description_ = new InputEdit(this, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, false);
        Utils::ApplyStyle(description_, Styling::getParameters().getTextEditCommonQss(true));
        description_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        description_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        description_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        description_->setFrameStyle(QFrame::NoFrame);
        description_->document()->setDocumentMargin(0);
        description_->setContentsMargins(0, 0, 0, 0);
        description_->setPlaceholderText(QT_TRANSLATE_NOOP("files_widget", "Add caption"));
        description_->setAcceptDrops(false);

        description_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2));
        description_->setFixedHeight(Utils::scale_value(DEFAULT_INPUT_HEIGHT));

        connect(description_, &TextEditEx::enter, this, &FilesWidget::enter);

        filesArea_ = new FilesArea(this);
        connect(filesArea_, &FilesArea::sizeChanged, this, &FilesWidget::updateFilesArea);
        area_ = new FilesScroll(this, filesArea_, _imageBuffer.isNull());
        area_->setStyleSheet(qsl("background: transparent;"));
        area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area_->setWidgetResizable(true);
        area_->setFocusPolicy(Qt::NoFocus);
        area_->setFrameShape(QFrame::NoFrame);
        Utils::ApplyStyle(area_->verticalScrollBar(), qsl("\
            QScrollBar:vertical\
            {\
                width: 4dip;\
                margin-top: 0px;\
                margin-bottom: 0px;\
                margin-right: 0px;\
            }"));

        filesArea_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET - RIGHT_OFFSET));
        area_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET - RIGHT_OFFSET));

#ifdef _WIN32
        DragAcceptFiles((HWND)Utils::InterConnector::instance().getMainWindow()->winId(), _imageBuffer.isNull());
#endif //_WIN32

        connect(area_->verticalScrollBar(), &QScrollBar::rangeChanged, this, &FilesWidget::scrollRangeChanged);
        connect(description_, &TextEditEx::textChanged, this, &FilesWidget::descriptionChanged);

        initPreview(_files, _imageBuffer);
        updateDescriptionHeight();
    }

    FilesWidget::~FilesWidget()
    {
#ifdef _WIN32
        DragAcceptFiles((HWND)Utils::InterConnector::instance().getMainWindow()->winId(), TRUE);
#endif //_WIN32
    }

    QStringList FilesWidget::getFiles() const
    {
        if (filesArea_->count() == 0)
            return { previewPath_ };
        else
            return filesArea_->getItems();
    }

    QString FilesWidget::getDescription() const
    {
        return description_->getPlainText();
    }

    const Data::MentionMap& FilesWidget::getMentions() const
    {
        return description_->getMentions();
    }

    void FilesWidget::setFocusOnInput()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        description_->setFocus();
    }

    void FilesWidget::setDescription(const QString& _text, const Data::MentionMap& _mentions)
    {
        description_->setMentions(_mentions);
        description_->setPlainText(_text, false);
    }

    void FilesWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        title_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET));
        title_->draw(p);

        if (!preview_.isNull())
        {
            auto x = width() / 2 - preview_.width() / 2;
            auto y = Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height();
            if (preview_.height() < Utils::scale_value(MIN_PREVIEW_HEIGHT) || preview_.width() < Utils::scale_value(MAX_PREVIEW_HEIGHT))
            {
                p.setPen(Qt::NoPen);
                p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
                p.drawRoundedRect(QRect(Utils::scale_value(HOR_OFFSET), y, Utils::scale_value(MAX_PREVIEW_HEIGHT), preview_.height() < Utils::scale_value(MIN_PREVIEW_HEIGHT) ? Utils::scale_value(MIN_PREVIEW_HEIGHT) : preview_.height()), Utils::scale_value(5), Utils::scale_value(5));
                if (preview_.height() < Utils::scale_value(MIN_PREVIEW_HEIGHT))
                    y += Utils::scale_value(MIN_PREVIEW_HEIGHT) / 2 - preview_.height() / 2;

                p.drawPixmap(x, y, preview_);
            }
            else
            {
                auto b = QBrush(preview_);
                QMatrix m;
                m.translate(x, y);
                b.setMatrix(m);
                p.setBrush(b);
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(QRect(x, y, preview_.width(), preview_.height()), Utils::scale_value(5), Utils::scale_value(5));
            }

            if (drawDuration_)
            {
                static const QColor durationColor = []() {
                    auto c = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
                    c.setAlphaF(0.5);
                    return c;
                }();

                p.setPen(Qt::NoPen);
                p.setBrush(durationColor);
                p.drawRoundedRect(Utils::scale_value(HOR_OFFSET + HOR_OFFSET_SMALL), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL), Utils::scale_value(DURATION_WIDTH) + (isGif_ ? 0 : Utils::scale_value(VIDEO_ICON_SIZE)), Utils::scale_value(DURATION_HEIGHT), Utils::scale_value(8), Utils::scale_value(8));

                if (!isGif_)
                {
                    static auto video_icon = Utils::renderSvgScaled(qsl(":/videoplayer/duration_time_icon"), QSize(VIDEO_ICON_SIZE, VIDEO_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
                    p.drawPixmap(Utils::scale_value(HOR_OFFSET + HOR_OFFSET_SMALL) + Utils::scale_value(1), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL) + Utils::scale_value(2), video_icon);
                }

                if (durationLabel_)
                {
                    durationLabel_->setOffsets(Utils::scale_value(HOR_OFFSET + HOR_OFFSET_SMALL) + Utils::scale_value(DURATION_WIDTH) / 2 - durationLabel_->desiredWidth() / 2 + (isGif_ ? 0 : Utils::scale_value(VIDEO_ICON_SIZE)), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height() + Utils::scale_value(HOR_OFFSET_SMALL) + Utils::scale_value(DURATION_HEIGHT) / 2);
                    durationLabel_->draw(p, TextRendering::VerPosition::MIDDLE);
                }
            }
        }

        QWidget::paintEvent(_event);
    }

    void FilesWidget::descriptionChanged()
    {
        updateDescriptionHeight();
    }

    void FilesWidget::updateFilesArea()
    {
        auto items = filesArea_->getItems();
        if (!preview_.isNull() || filesArea_->count() == 1)
        {
            if (!preview_.isNull())
                items.push_front(previewPath_);

            initPreview(items);
        }

        const QString text = QT_TRANSLATE_NOOP("files_widget", "Send ") % QString::number(items.size()) % ql1c(' ') %
            Utils::GetTranslator()->getNumberString(
                items.size(),
                QT_TRANSLATE_NOOP3("files_widget", "file", "1"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "2"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "5"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "21")
            );

        title_->setText(items.empty() ? QT_TRANSLATE_NOOP("files_widget", "Send files") : text);
        update();

        emit setButtonActive(!items.empty());
        area_->setPlaceholder(items.empty());

        updateSize();
    }

    void FilesWidget::localPreviewLoaded(QPixmap pixmap, const QSize _originalSize)
    {
        if (!pixmap.isNull())
        {
            if (pixmap.width() > pixmap.height())
            {
                if (pixmap.width() > Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2))
                    preview_ = pixmap.scaledToWidth(Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2), Qt::SmoothTransformation);
                else
                    preview_ = pixmap;
            }
            else
            {
                if (pixmap.height() > Utils::scale_value(MAX_PREVIEW_HEIGHT))
                    preview_ = pixmap.scaledToHeight(Utils::scale_value(MAX_PREVIEW_HEIGHT), Qt::SmoothTransformation);
                else
                    preview_ = pixmap;
            }
        }

        updateSize();
        update();
    }

    void FilesWidget::enter()
    {
        if (filesArea_->count() != 0 || !preview_.isNull())
            emit Utils::InterConnector::instance().acceptGeneralDialog();
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

    void FilesWidget::updateSize()
    {
        setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
        auto h = Utils::scale_value(HEADER_HEIGHT);
        if (!preview_.isNull())
        {
            auto needHeight = std::max(preview_.height(), Utils::scale_value(MIN_PREVIEW_HEIGHT));
            h += needHeight;
            area_->setFixedHeight(std::max(preview_.height(), Utils::scale_value(MIN_PREVIEW_HEIGHT)));
        }
        else
        {
            area_->setFixedHeight(std::min(Utils::scale_value(MAX_PREVIEW_HEIGHT), filesArea_->height()));
            h += area_->height();
        }
        h += Utils::scale_value(PREVIEW_TOP_OFFSET + BOTTOM_OFFSET);
        h += description_->height();

        setFixedHeight(h);

        area_->move(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET + PREVIEW_TOP_OFFSET) + title_->cachedSize().height());
        description_->move(QPoint(width() / 2 - description_->width() / 2, rect().height() - Utils::scale_value(BOTTOM_OFFSET + VER_OFFSET/2 - (platform::is_apple() ? Utils::scale_value(2) : 0) ) - description_->height()));
        update();
    }

    void FilesWidget::setDuration(const QString& _duration)
    {
        if (!durationLabel_)
        {
            durationLabel_ = TextRendering::MakeTextUnit(QString());
            static const auto fontSize = platform::is_apple() ? 11 : 13;
            durationLabel_->init(Fonts::appFontScaled(fontSize, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }

        durationLabel_->setText(_duration);
        durationLabel_->evaluateDesiredSize();
    }

    void FilesWidget::initPreview(const QStringList& _files, const QPixmap& _imageBuffer)
    {
        filesArea_->clear();

        if (!_imageBuffer.isNull())
        {
            localPreviewLoaded(_imageBuffer, _imageBuffer.size());
        }
        else
        {
            preview_ = QPixmap();
            drawDuration_ = false;
            if (_files.size() == 1)
            {
                bool handled = false;

                QFileInfo f(_files.front());
                const auto ext = f.suffix();

                if (ext.compare(ql1s("gif"), Qt::CaseInsensitive) == 0)
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
                    auto task = new Utils::LoadPixmapFromFileTask(_files.front());

                    QObject::connect(
                        task,
                        &Utils::LoadPixmapFromFileTask::loadedSignal,
                        this,
                        &FilesWidget::localPreviewLoaded);

                    QThreadPool::globalInstance()->start(task);

                    handled = true;
                }
                else if (Utils::is_video_extension(ext))
                {
                    auto task = new Utils::LoadFirstFrameTask(_files.front());

                    QObject::connect(
                        task,
                        &Utils::LoadFirstFrameTask::loadedSignal,
                        this,
                        &FilesWidget::localPreviewLoaded);

                    QObject::connect(
                        task,
                        &Utils::LoadFirstFrameTask::duration,
                        this,
                        &FilesWidget::duration);

                    QThreadPool::globalInstance()->start(task);

                    handled = true;
                }

                if (handled)
                {
                    previewPath_ = _files.front();
                    const QString text = QT_TRANSLATE_NOOP("files_widget", "Send ") % QString::number(_files.size()) % ql1c(' ') %
                        Utils::GetTranslator()->getNumberString(
                            _files.size(),
                            QT_TRANSLATE_NOOP3("files_widget", "file", "1"),
                            QT_TRANSLATE_NOOP3("files_widget", "files", "2"),
                            QT_TRANSLATE_NOOP3("files_widget", "files", "5"),
                            QT_TRANSLATE_NOOP3("files_widget", "files", "21")
                        );
                    title_->setText(text);
                    return;
                }
            }

            for (const auto& f : _files)
            {
                filesArea_->blockSignals(true);
                filesArea_->addItem(new FilesAreaItem(f, Utils::scale_value(DIALOG_WIDTH - HOR_OFFSET * 2)));
                filesArea_->blockSignals(false);
            }
        }

        const auto amount = _imageBuffer.isNull() ? _files.size() : 1;
        const QString text = QT_TRANSLATE_NOOP("files_widget", "Send ") % QString::number(amount) % ql1c(' ') %
            Utils::GetTranslator()->getNumberString(
                amount,
                QT_TRANSLATE_NOOP3("files_widget", "file", "1"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "2"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "5"),
                QT_TRANSLATE_NOOP3("files_widget", "files", "21")
            );
        title_->setText(text);
    }

    void FilesWidget::updateDescriptionHeight()
    {
        const auto newDocHeight = description_->getTextHeight();

        if ((currentDocumentHeight_ != -1 && newDocHeight == currentDocumentHeight_) || newDocHeight == 0)
            return;

        static const auto marginDiff = Utils::scale_value(INPUT_TOP_OFFSET + INPUT_BOTTOM_OFFSET);
        static const auto minHeight = Utils::scale_value(INPUT_AREA) - marginDiff;
        static const auto maxHeight = Utils::scale_value(INPUT_AREA_MAX) - marginDiff;

        if (currentDocumentHeight_ == -1)
        {
            neededHeight_ = minHeight;

            static auto defaultDocumentHeight = newDocHeight;
            currentDocumentHeight_ = defaultDocumentHeight;
        }

        const auto diff = newDocHeight - currentDocumentHeight_;
        neededHeight_ += diff;

        currentDocumentHeight_ = newDocHeight;

        auto newEditHeight = neededHeight_;
        auto sbPolicy = Qt::ScrollBarAlwaysOff;

        if (newEditHeight < minHeight)
        {
            newEditHeight = minHeight;
        }
        else if (newEditHeight > maxHeight)
        {
            newEditHeight = maxHeight;
            sbPolicy = Qt::ScrollBarAsNeeded;
        }

        description_->setVerticalScrollBarPolicy(sbPolicy);
        description_->setFixedHeight(newEditHeight);
        updateSize();
    }
}

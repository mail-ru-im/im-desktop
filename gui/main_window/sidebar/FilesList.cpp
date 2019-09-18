#include "stdafx.h"

#include "FilesList.h"
#include "SidebarUtils.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/FileSharingIcon.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../GroupChatOperations.h"
#include "../friendly/FriendlyContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../history_control/FileSizeFormatter.h"
#include "utils/stat_utils.h"
#include "../../styles/ThemeParameters.h"

#ifdef __APPLE__
#   include "../../utils/macos/mac_support.h"
#endif

namespace
{
    const int VER_OFFSET = 8;
    const int ICON_VER_OFFSET = 12;
    const int HOR_OFFSET = 16;
    const int RIGHT_OFFSET = 40;
    const int MORE_BUTTON_SIZE = 20;
    const int PREVIEW_SIZE = 44;
    const int PREVIEW_RIGHT_OFFSET = 12;
    const int NAME_OFFSET = 0;
    const int DATE_HEIGHT = 20;
    const int DATE_OFFSET = 0;
    const int SHOW_IN_FOLDER_OFFSET = 8;
    const int BUTTON_SIZE = 20;
    const int DATE_TOP_OFFSET = 12;
    const int DATE_BOTTOM_OFFSET = 8;
    const int DATE_LEFT_OFFSET = 4;

    const std::string MediaTypeName("File");
}

namespace Ui
{
    DateFileItem::DateFileItem(time_t _time, qint64 _msg, qint64 _seq)
        : time_(_time)
        , msg_(_msg)
        , seq_(_seq)
    {
        auto dt = QDateTime::fromTime_t(_time);
        auto date = QLocale().standaloneMonthName(dt.date().month());

        date_ = Ui::TextRendering::MakeTextUnit(date.toUpper());
        date_->init(Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        date_->evaluateDesiredSize();
    }

    void DateFileItem::draw(QPainter& _p, const QRect& _rect, double _progress)
    {
        date_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(DATE_TOP_OFFSET) + _rect.y());
        date_->draw(_p);
    }

    int DateFileItem::getHeight() const
    {
        return Utils::scale_value(DATE_TOP_OFFSET + DATE_BOTTOM_OFFSET) + date_->cachedSize().height();
    }

    qint64 DateFileItem::getMsg() const
    {
        return msg_;
    }

    qint64 DateFileItem::getSeq() const
    {
        return seq_;
    }

    time_t DateFileItem::time() const
    {
        return time_;
    }

    FileItem::FileItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width)
        : link_(_link)
        , height_(0)
        , width_(_width)
        , msg_(_msg)
        , seq_(_seq)
        , downloading_(false)
        , transferred_(0)
        , total_(0)
        , outgoing_(_outgoing)
        , reqId_(-1)
        , sender_(_sender)
        , time_(_time)
        , sizet_(0)
        , lastModified_(0)
    {
        height_ = QFontMetrics(Fonts::appFontScaled(16)).height();
        height_ += QFontMetrics(Fonts::appFontScaled(12)).height();

        date_ = Ui::TextRendering::MakeTextUnit(_date, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        date_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        date_->evaluateDesiredSize();

        friedly_ = Ui::TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(sender_), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        friedly_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        friedly_->evaluateDesiredSize();

        showInFolder_ = Ui::TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("chat_page", "Show in folder"), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        showInFolder_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE));
        showInFolder_->evaluateDesiredSize();

        height_ += date_->cachedSize().height();
        height_ += Utils::scale_value(DATE_OFFSET);
        height_ += Utils::scale_value(VER_OFFSET) * 2;

        setMoreButtonState(ButtonState::HIDDEN);
    }

    void FileItem::draw(QPainter& _p, const QRect& _rect, double _progress)
    {
        _p.setRenderHints(QPainter::Antialiasing);
        _p.setRenderHints(QPainter::HighQualityAntialiasing);
        if (!more_.isNull())
            _p.drawPixmap(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, height_ / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2 + _rect.y(), more_);

        if (localPath_.isEmpty())
        {
            const auto QT_ANGLE_MULT = 16;
            const auto baseAngle = (_progress * QT_ANGLE_MULT);
            const auto progress = total_ == 0 ? 0 : std::max((double)transferred_ / (double)total_, 0.03);
            const auto progressAngle = (int)std::ceil(progress * 360 * QT_ANGLE_MULT);

            auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(ICON_VER_OFFSET) + _rect.y(), Utils::scale_value(PREVIEW_SIZE), Utils::scale_value(PREVIEW_SIZE));
            auto r2 = QRect(Utils::scale_value(HOR_OFFSET) + Utils::scale_value(4), Utils::scale_value(ICON_VER_OFFSET) + _rect.y() + Utils::scale_value(4), Utils::scale_value(PREVIEW_SIZE) - Utils::scale_value(8), Utils::scale_value(PREVIEW_SIZE) - Utils::scale_value(8));

            static auto download_icon = Utils::renderSvgScaled(qsl(":/filesharing/download_icon"), QSize(BUTTON_SIZE, BUTTON_SIZE), QColor(qsl("#ffffff")));
            static auto cancel_icon = Utils::renderSvgScaled(qsl(":/filesharing/cancel_icon"), QSize(BUTTON_SIZE, BUTTON_SIZE), QColor(qsl("#ffffff")));

            _p.save();
            _p.setBrush(FileSharingIcon::getFillColor(filename_));
            _p.setPen(Qt::transparent);
            _p.drawEllipse(r);
            if (downloading_)
            {
                auto p = QPen(QColor(qsl("#ffffff")), Utils::scale_value(2));
                _p.setPen(p);
                _p.drawArc(r2, -baseAngle, -progressAngle);
                _p.drawPixmap(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE / 2) - cancel_icon.width() / 2 / Utils::scale_bitmap_ratio(), Utils::scale_value(ICON_VER_OFFSET + PREVIEW_SIZE / 2) - cancel_icon.height() / 2 / Utils::scale_bitmap_ratio() + _rect.y(), cancel_icon);
            }
            else
            {
                _p.drawPixmap(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE / 2) - download_icon.width() / 2 / Utils::scale_bitmap_ratio(), Utils::scale_value(ICON_VER_OFFSET + PREVIEW_SIZE / 2) - download_icon.height() / 2 / Utils::scale_bitmap_ratio() + _rect.y(), download_icon);
            }
            _p.restore();
        }
        else
        {
            if (!fileType_.isNull())
                _p.drawPixmap(Utils::scale_value(HOR_OFFSET), Utils::scale_value(ICON_VER_OFFSET) + _rect.y(), fileType_);
        }

        auto offset = 0;
        if (name_)
        {
            name_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET) + _rect.y());
            name_->draw(_p);
            offset += name_->cachedSize().height();
            offset += Utils::scale_value(NAME_OFFSET);
        }

        if (size_)
        {
            size_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET + NAME_OFFSET) + name_->cachedSize().height() + _rect.y());
            size_->draw(_p);

            if (!localPath_.isEmpty())
            {
                showInFolder_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET + SHOW_IN_FOLDER_OFFSET) + size_->cachedSize().width(), Utils::scale_value(VER_OFFSET + NAME_OFFSET) + name_->cachedSize().height() + _rect.y());
                showInFolder_->draw(_p);
            }

            offset += size_->cachedSize().height();
            offset += Utils::scale_value(DATE_OFFSET);
        }

        friedly_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET) + offset + _rect.y());
        friedly_->draw(_p);

        date_->setOffsets(width_ - Utils::scale_value(RIGHT_OFFSET) - date_->cachedSize().width(), Utils::scale_value(VER_OFFSET) + offset + _rect.y());
        date_->draw(_p);
    }

    int FileItem::getHeight() const
    {
        return height_;
    }

    void FileItem::setWidth(int _width)
    {
        width_ = _width;
        if (name_)
            name_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET));
        if (size_)
            size_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET));
        if (friedly_)
            friedly_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET + DATE_LEFT_OFFSET) - date_->cachedSize().width());
    }

    void FileItem::setReqId(qint64 _id)
    {
        reqId_ = _id;
    }
    qint64 FileItem::reqId() const
    {
        return reqId_;
    }

    qint64 FileItem::getMsg() const
    {
        return msg_;
    }

    qint64 FileItem::getSeq() const
    {
        return seq_;
    }

    QString FileItem::getLink() const
    {
        return link_;
    }

    QString FileItem::sender() const
    {
        return sender_;
    }

    time_t FileItem::time() const
    {
        return time_;
    }

    qint64 FileItem::size() const
    {
        return sizet_;
    }

    qint64 FileItem::lastModified() const
    {
        return lastModified_;
    }

    void FileItem::setFilename(const QString& _name)
    {
        name_ = Ui::TextRendering::MakeTextUnit(_name, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        name_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        name_->evaluateDesiredSize();
        name_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET));\

        filename_ = _name;

        fileType_ = FileSharingIcon::getIcon(_name);
    }

    void FileItem::setFilesize(qint64 _size)
    {
        sizet_ = _size;
        size_ = Ui::TextRendering::MakeTextUnit(HistoryControl::formatFileSize(_size), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        size_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        size_->evaluateDesiredSize();
        size_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET));
    }

    void FileItem::setLastModified(qint64 _lastModified)
    {
        lastModified_ = _lastModified;
    }

    bool FileItem::isOverControl(const QPoint& _pos)
    {
        static auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(PREVIEW_SIZE), Utils::scale_value(PREVIEW_SIZE));
        return localPath_.isEmpty() && r.contains(_pos);
    }

    bool FileItem::isOverLink(const QPoint& _pos, const QPoint& _pos2)
    {
        static auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(PREVIEW_SIZE), Utils::scale_value(PREVIEW_SIZE));
        return !localPath_.isEmpty() && (showInFolder_->contains(_pos) || r.contains(_pos2) || name_->contains(_pos));
    }

    bool FileItem::isOverIcon(const QPoint & _pos)
    {
        static auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(PREVIEW_SIZE), Utils::scale_value(PREVIEW_SIZE));
        return !localPath_.isEmpty() && r.contains(_pos);
    }

    void FileItem::setDownloading(bool _downloading)
    {
        downloading_ = _downloading;
    }

    bool FileItem::isDownloading() const
    {
        return downloading_;
    }

    void FileItem::setProgress(qint64 _transferred, qint64 _total)
    {
        transferred_ = _transferred;
        total_ = _total;
    }

    void FileItem::setLocalPath(const QString& _path)
    {
        downloading_ = false;
        localPath_ = _path;
    }

    QString FileItem::getLocalPath() const
    {
        return localPath_;
    }

    bool FileItem::isOverDate(const QPoint& _pos) const
    {
        return date_->contains(_pos);
    }

    void FileItem::setDateState(bool _hover, bool _active)
    {
        if (_hover)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
        else if (_active)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
        else
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    }

    bool FileItem::isOutgoing() const
    {
        return outgoing_;
    }

    void FileItem::setMoreButtonState(const ButtonState& _state)
    {
        moreState_ = _state;

        if (_state == ButtonState::HIDDEN)
        {
            more_ = QPixmap();
            return;
        }

        QColor color;
        switch (_state)
        {
        case ButtonState::NORMAL:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
            break;
        case ButtonState::HOVERED:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            break;
        case ButtonState::PRESSED:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;
        default:
            break;
        }

        more_ = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), QSize(MORE_BUTTON_SIZE, MORE_BUTTON_SIZE), color);
    }

    bool FileItem::isOverMoreButton(const QPoint& _pos, int _h) const
    {
        auto r = QRect(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, _h + height_ / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, Utils::scale_value(MORE_BUTTON_SIZE), Utils::scale_value(MORE_BUTTON_SIZE));
        return r.contains(_pos);
    }

    ButtonState FileItem::moreButtonState() const
    {
        return moreState_;
    }

    FilesList::FilesList(QWidget* _parent)
        : MediaContentWidget(Type::Files, _parent)
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileMetainfoDownloaded, this, &FilesList::fileSharingFileMetainfoDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloading, this, &FilesList::fileDownloading);
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &FilesList::fileDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingError, this, &FilesList::fileError);
        setMouseTracking(true);
    }

    void FilesList::initFor(const QString& _aimId)
    {
        MediaContentWidget::initFor(_aimId);
        Items_.clear();
        RequestIds_.clear();
        setFixedHeight(0);
    }

    void FilesList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            Utils::UrlParser parser;
            parser.process(QStringRef(&e.url_));

            const auto isFilesharing = parser.hasUrl() && parser.getUrl().is_filesharing();
            if (!isFilesharing)
                continue;

            auto reqId = GetDispatcher()->downloadFileSharingMetainfo(e.url_);

            auto time = QDateTime::fromTime_t(e.time_);
            auto item = std::make_unique<FileItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width());
            h += item->getHeight();

            RequestIds_.push_back(reqId);
            item->setReqId(reqId);

            Items_.push_back(std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void FilesList::processUpdates(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            if (e.action_ == qsl("del"))
            {
                Items_.erase(std::remove_if(Items_.begin(), Items_.end(), [e](const auto& i) { return i->getMsg() == e.msg_id_; }), Items_.end());
                h = 0;
                for (const auto& i : Items_)
                    h += i->getHeight();

                continue;
            }

            Utils::UrlParser parser;
            parser.process(QStringRef(&e.url_));

            const auto isFilesharing = parser.hasUrl() && parser.getUrl().is_filesharing();
            if (!isFilesharing || (e.type_ != qsl("file") && e.type_ != qsl("audio")))
                continue;

            auto time = QDateTime::fromTime_t(e.time_);

            auto iter = Items_.cbegin();
            for (; iter != Items_.cend(); ++iter)
            {
                if (iter->get()->isDateItem())
                {
                    auto dt = QDateTime::fromTime_t(iter->get()->time());
                    if (dt.date().month() == time.date().month() && dt.date().year() == time.date().year())
                        continue;
                }

                auto curMsg = iter->get()->getMsg();
                auto curSeq = iter->get()->getSeq();
                if (curMsg > e.msg_id_ || (curMsg == e.msg_id_ && curSeq > e.seq_))
                    continue;

                break;
            }

            auto reqId = GetDispatcher()->downloadFileSharingMetainfo(e.url_);
            auto item = std::make_unique<FileItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width());
            h += item->getHeight();

            RequestIds_.push_back(reqId);
            item->setReqId(reqId);

            Items_.insert(iter, std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void FilesList::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        auto h = 0;

        const auto visibleRect = visibleRegion().boundingRect();
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (visibleRect.intersects(r))
                i->draw(p, r, anim_.isRunning() ? anim_.current() : 0.0);

            h += i->getHeight();
        }
    }

    void FilesList::resizeEvent(QResizeEvent* _event)
    {
        for (auto& i : Items_)
            i->setWidth(_event->size().width());

        MediaContentWidget::resizeEvent(_event);
    }

    void FilesList::mousePressEvent(QMouseEvent* _event)
    {
        auto h = 0;
        for (auto& i : Items_)
        {
            if (i->isOverDate(_event->pos()))
            {
                i->setDateState(false, true);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(_event->pos(), h))
            {
                i->setMoreButtonState(ButtonState::PRESSED);
            }

            h += i->getHeight();
        }

        update();
        pos_ = _event->pos();
        MediaContentWidget::mousePressEvent(_event);
    }

    void FilesList::mouseReleaseEvent(QMouseEvent* _event)
    {
        Utils::GalleryMediaActionCont cont(MediaTypeName, aimId_);

        if (_event->button() == Qt::RightButton)
        {
            cont.happened();
            return MediaContentWidget::mouseReleaseEvent(_event);
        }

        if (Utils::clicked(pos_, _event->pos()))
        {
            auto h = 0, j = 0;
            for (auto& i : Items_)
            {
                auto r = QRect(0, h, width(), i->getHeight());
                if (r.contains(_event->pos()))
                {
                    cont.happened();

                    auto p = _event->pos();
                    p.setY(p.y() - h);
                    if (i->isOverControl(p))
                    {
                        if (i->isDownloading())
                        {
                            Ui::GetDispatcher()->abortSharedFileDownloading(i->getLink());
                            i->setDownloading(false);
                            Downloading_.erase(std::remove(Downloading_.begin(), Downloading_.end(), i->reqId()), Downloading_.end());
                            if (Downloading_.empty())
                                anim_.finish();

                            update();
                        }
                        else
                        {
                            auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                            Downloading_.push_back(reqId);
                            i->setReqId(reqId);
                        }
                    }
                    else if (i->isOverLink(_event->pos(), p))
                    {
#ifdef _WIN32
                        QFileInfo f(i->getLocalPath());
                        if (!f.exists() || f.size() != i->size() || f.lastModified().toTime_t() != i->lastModified())
                        {
                            auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                            Downloading_.push_back(reqId);
                            i->setLocalPath(QString());
                            i->setReqId(reqId);
                            h += i->getHeight();
                            continue;
                        }
#endif //_WIN32
                        const auto path = i->getLocalPath();
                        const auto openAt = i->isOverIcon(p) ? Utils::OpenAt::Launcher : Utils::OpenAt::Folder;
                        Utils::openFileOrFolder(QStringRef(&path), openAt);
                    }
                }
                if (i->isOverDate(_event->pos()))
                {
                    emit Logic::getContactListModel()->select(aimId_, i->getMsg(), i->getMsg(), Logic::UpdateChatSelection::No);
                    i->setDateState(true, false);
                }
                else
                {
                    i->setDateState(false, false);
                }

                if (i->moreButtonState() == ButtonState::PRESSED)
                {
                    cont.happened();

                    if (i->isOverMoreButton(_event->pos(), h))
                        i->setMoreButtonState(ButtonState::HOVERED);
                    else
                        i->setMoreButtonState(ButtonState::NORMAL);

                    if (Utils::clicked(pos_, _event->pos()))
                        showContextMenu(ItemData(i->getLink(), i->getMsg(), i->sender(), i->time()), _event->pos(), true);
                }

                h += i->getHeight();
                ++j;
            }
        }
    }

    void FilesList::mouseMoveEvent(QMouseEvent* _event)
    {
        auto h = 0;
        auto point = false;
        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_event->pos()))
            {
                auto p = _event->pos();
                p.setY(p.y() - h);
                if (i->isOverControl(p) || i->isOverLink(_event->pos(), p))
                    point = true;
            }
            if (i->isOverDate(_event->pos()))
            {
                point = true;
                i->setDateState(true, false);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(_event->pos(), h))
            {
                i->setMoreButtonState(ButtonState::HOVERED);
                point = true;
            }
            else
            {
                if (r.contains(_event->pos()))
                    i->setMoreButtonState(ButtonState::NORMAL);
                else
                    i->setMoreButtonState(ButtonState::HIDDEN);
            }

            h += i->getHeight();
        }

        setCursor(point ? Qt::PointingHandCursor : Qt::ArrowCursor);

        update();
        MediaContentWidget::mouseMoveEvent(_event);
    }

    void FilesList::leaveEvent(QEvent* _event)
    {
        for (auto& i : Items_)
            i->setMoreButtonState(ButtonState::HIDDEN);

        update();
        MediaContentWidget::leaveEvent(_event);
    }

    MediaContentWidget::ItemData FilesList::itemAt(const QPoint& _pos)
    {
        auto h = 0;
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_pos))
            {
                return ItemData(i->getLink(), i->getMsg(), i->sender(), i->time());
            }
            h += i->getHeight();
        }

        return ItemData();
    }

    void FilesList::fileSharingFileMetainfoDownloaded(qint64 _seq, const Data::FileSharingMeta& _meta)
    {
        if (std::find(RequestIds_.begin(), RequestIds_.end(),_seq) == RequestIds_.end())
            return;

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setFilename(_meta.filenameShort_);
                i->setFilesize(_meta.size_);
                i->setLocalPath(_meta.localPath_);
                i->setLastModified(_meta.lastModified_);
                break;
            }
        }

        update();
    }

    void FilesList::fileDownloading(qint64 _seq, QString _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal)
    {
        if (std::find(Downloading_.begin(), Downloading_.end(), _seq) == Downloading_.end() || Items_.empty())
            return;

        if (!anim_.isRunning())
            anim_.start([this]() { update(); }, 0.0, 360.0, 700, anim::linear, -1);

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setDownloading(true);
                i->setProgress(_bytesTransferred, _bytesTotal);
                break;
            }
        }

        update();
    }

    void FilesList::fileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
    {
        if (Items_.empty())
            return;

        if (std::find(Downloading_.begin(), Downloading_.end(), _seq) != Downloading_.end())
        {
            for (auto& i : Items_)
            {
                if (i->reqId() == _seq)
                {
                    i->setLocalPath(_result.filename_);
                    QFileInfo f(_result.filename_);
                    i->setLastModified(f.lastModified().toTime_t());
                    Downloading_.erase(std::remove(Downloading_.begin(), Downloading_.end(), i->reqId()), Downloading_.end());
                    if (Downloading_.empty())
                        anim_.finish();
                    Utils::showDownloadToast(_result);
                    break;
                }
            }

            update();
        }
        else
        {
            auto needUpdate = false;
            for (auto& i : Items_)
            {
                if (i->getLink() == _result.requestedUrl_)
                {
                    i->setLocalPath(_result.filename_);
                    QFileInfo f(_result.filename_);
                    i->setLastModified(f.lastModified().toTime_t());
                    needUpdate = true;
                    Utils::showDownloadToast(_result);
                }
            }

            if (needUpdate)
                update();
        }

    }

    void FilesList::fileError(qint64 _seq, const QString& _rawUri, qint32 _errorCode)
    {
        if (std::find(Downloading_.begin(), Downloading_.end(), _seq) == Downloading_.end() || Items_.empty())
            return;

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                Downloading_.erase(std::remove(Downloading_.begin(), Downloading_.end(), i->reqId()), Downloading_.end());
                if (Downloading_.empty())
                    anim_.finish();

                auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                Downloading_.push_back(reqId);
                return;
            }
        }
    }

    void FilesList::validateDates()
    {
        if (Items_.empty())
            return;

        auto h = height();

        if (!Items_.front()->isDateItem())
        {
            Items_.emplace(Items_.begin(), new DateFileItem(Items_.front()->time(), Items_.front()->getMsg(), Items_.front()->getSeq()));
            h += Items_.front()->getHeight();
        }

        auto iter = Items_.begin();
        auto prevIsDate = iter->get()->isDateItem();
        auto prevDt = QDateTime::fromTime_t(iter->get()->time()).date();
        ++iter;

        while (iter != Items_.end())
        {
            auto isDate = iter->get()->isDateItem();
            auto dt = QDateTime::fromTime_t(iter->get()->time()).date();
            if ((dt.month() == prevDt.month() && dt.year() == prevDt.year()) || prevIsDate != isDate)
            {
                prevIsDate = isDate;
                prevDt = dt;
                ++iter;
                continue;
            }

            if (!prevIsDate && !isDate)
            {
                auto dateItem = std::make_unique<DateFileItem>(iter->get()->time(), iter->get()->getMsg(), iter->get()->getSeq());
                h += dateItem->getHeight();

                iter = Items_.insert(iter, std::move(dateItem));
                prevIsDate = true;
                prevDt = dt;
                ++iter;
                continue;
            }

            auto prev = std::prev(iter);
            h -= prev->get()->getHeight();
            iter = Items_.erase(prev);
            prevIsDate = true;
            prevDt = dt;
            ++iter;
        }

        setFixedHeight(h);
    }
}

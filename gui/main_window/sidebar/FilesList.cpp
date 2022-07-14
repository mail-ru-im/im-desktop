#include "stdafx.h"

#include "FilesList.h"
#include "SidebarUtils.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/FileSharingIcon.h"
#include "../../controls/TooltipWidget.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/UrlParser.h"
#include "../../utils/InterConnector.h"
#include "../../my_info.h"
#include "../GroupChatOperations.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/FileSharingMetaContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../history_control/FileSizeFormatter.h"
#include "../history_control/MessageStyle.h"
#include "utils/stat_utils.h"
#include "../../styles/ThemeParameters.h"
#include "previewer/toast.h"
#include "../../../common.shared/antivirus/antivirus_types.h"

#ifdef __APPLE__
#   include "../../utils/macos/mac_support.h"
#endif

namespace
{
    constexpr int VER_OFFSET = 8;
    constexpr int ICON_VER_OFFSET = 12;
    constexpr int HOR_OFFSET = 16;
    constexpr int RIGHT_OFFSET = 40;
    constexpr int MORE_BUTTON_SIZE = 20;
    constexpr int PREVIEW_SIZE = 44;
    constexpr int PREVIEW_RIGHT_OFFSET = 12;
    constexpr int NAME_OFFSET = 0;
    constexpr int DATE_OFFSET = 0;
    constexpr int SHOW_IN_FOLDER_OFFSET = 8;
    constexpr int BUTTON_SIZE = 20;
    constexpr int DATE_TOP_OFFSET = 12;
    constexpr int DATE_BOTTOM_OFFSET = 8;
    constexpr int DATE_LEFT_OFFSET = 4;

    constexpr std::string_view MediaTypeName("File");

    constexpr std::chrono::milliseconds getDataTransferTimeout() noexcept
    {
        return std::chrono::milliseconds(300);
    }

    QRect iconRect() noexcept
    {
        return Utils::scale_value(QRect(HOR_OFFSET, ICON_VER_OFFSET, PREVIEW_SIZE, PREVIEW_SIZE));
    }

    const QPixmap& getMoreIcon(Ui::ButtonState _state)
    {
        if (_state == Ui::ButtonState::HIDDEN)
        {
            static const QPixmap empty;
            return empty;
        }

        static std::unordered_map<Ui::ButtonState, QPixmap> cache;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            cache.clear();

        auto& icon = cache[_state];
        if (icon.isNull())
        {
            QColor color;
            switch (_state)
            {
            case Ui::ButtonState::NORMAL:
                color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
                break;
            case Ui::ButtonState::HOVERED:
                color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
                break;
            case Ui::ButtonState::PRESSED:
                color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
                break;
            default:
                break;
            }
            icon = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), { MORE_BUTTON_SIZE, MORE_BUTTON_SIZE }, color);
        }

        return icon;
    }

    const Utils::FileSharingId& emptyFileSharingId()
    {
        static Utils::FileSharingId empty;
        return empty;
    }
} // namespace

namespace Ui
{
    const Utils::FileSharingId& BaseFileItem::getFileSharingId() const
    {
        return emptyFileSharingId();
    }

    DateFileItem::DateFileItem(time_t _time, qint64 _msg, qint64 _seq)
        : time_(_time)
        , msg_(_msg)
        , seq_(_seq)
    {
        auto dt = QDateTime::fromSecsSinceEpoch(_time);
        auto date = QLocale().standaloneMonthName(dt.date().month());

        date_ = Ui::TextRendering::MakeTextUnit(date.toUpper());
        date_->init({ Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });
        date_->evaluateDesiredSize();
    }

    void DateFileItem::draw(QPainter& _p, const QRect& _rect, double _progress)
    {
        date_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(DATE_TOP_OFFSET) + _rect.y());
        date_->draw(_p);

        markDrew();
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

    FileItem::FileItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width, QWidget* _parent)
        : BaseFileItem(_parent)
        , link_(_link)
        , fileSharingId_ { Ui::ComplexMessage::extractIdFromFileSharingUri(link_) }
        , width_(_width)
        , msg_(_msg)
        , seq_(_seq)
        , outgoing_(_outgoing)
        , sender_(_sender)
        , time_(_time)
        , parent_{ _parent }
    {
        height_ = QFontMetrics(Fonts::appFontScaled(16)).height();
        height_ += QFontMetrics(Fonts::appFontScaled(12)).height();

        showInFolder_ = Ui::TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("chat_page", "Show in folder"), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        showInFolder_->init({ Fonts::appFontScaled(12), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_INVERSE } });
        showInFolder_->evaluateDesiredSize();

        date_ = Ui::TextRendering::MakeTextUnit(_date, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        TextRendering::TextUnit::InitializeParameters params(Fonts::appFontScaled(13), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
        date_->init(params);
        date_->evaluateDesiredSize();

        friendly_ = Ui::TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(sender_), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        params.maxLinesCount_ = 1;
        friendly_->init(params);
        friendly_->evaluateDesiredSize();
        elide(friendly_, Utils::scale_value(DATE_LEFT_OFFSET) - date_->getLastLineWidth());

        height_ += date_->cachedSize().height();
        height_ += Utils::scale_value(DATE_OFFSET);
        height_ += Utils::scale_value(VER_OFFSET) * 2;

        setMoreButtonState(ButtonState::HIDDEN);
    }

    void FileItem::draw(QPainter& _p, const QRect& _rect, double _progress)
    {
        Utils::PainterSaver ps(_p);
        _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        _p.translate(_rect.topLeft());

        if (localPath_.isEmpty() && !status_.isVirusInfected())
        {
            const auto QT_ANGLE_MULT = 16;
            const auto baseAngle = (_progress * QT_ANGLE_MULT);
            const auto progress = total_ == 0 ? 0 : std::max((double)transferred_ / (double)total_, 0.03);
            const auto progressAngle = (int)std::ceil(progress * 360 * QT_ANGLE_MULT);

            const auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(ICON_VER_OFFSET), Utils::scale_value(PREVIEW_SIZE), Utils::scale_value(PREVIEW_SIZE));
            const auto r2 = QRect(Utils::scale_value(HOR_OFFSET) + Utils::scale_value(4), Utils::scale_value(ICON_VER_OFFSET) + Utils::scale_value(4), Utils::scale_value(PREVIEW_SIZE) - Utils::scale_value(8), Utils::scale_value(PREVIEW_SIZE) - Utils::scale_value(8));

            Utils::PainterSaver ps2(_p);
            _p.setBrush(FileSharingIcon::getFillColor(filename_));
            _p.setPen(Qt::transparent);
            _p.drawEllipse(r);

            if (downloading_)
            {
                auto p = QPen(QColor(u"#ffffff"), Utils::scale_value(2));
                _p.setPen(p);
                _p.drawArc(r2, -baseAngle, -progressAngle);
            }

            static const auto downloadIcon = Utils::renderSvgScaled(qsl(":/filesharing/download_icon"), QSize(BUTTON_SIZE, BUTTON_SIZE), QColor(u"#ffffff"));
            static const auto cancelIcon = Utils::renderSvgScaled(qsl(":/filesharing/cancel_icon"), QSize(BUTTON_SIZE, BUTTON_SIZE), QColor(u"#ffffff"));
            const auto& icon = downloading_ ? cancelIcon : downloadIcon;
            _p.drawPixmap(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE / 2) - icon.width() / 2 / Utils::scale_bitmap_ratio(), Utils::scale_value(ICON_VER_OFFSET + PREVIEW_SIZE / 2) - icon.height() / 2 / Utils::scale_bitmap_ratio(), icon);
        }
        else
        {
            if (!fileType_.isNull())
                _p.drawPixmap(Utils::scale_value(HOR_OFFSET), Utils::scale_value(ICON_VER_OFFSET), fileType_);
        }

        if (Ui::needShowStatus(status_))
            _p.drawPixmap(getStatusRect(), getFileStatusIcon(status_.type(), MessageStyle::getFileStatusIconColor(status_.type(), false)));

        if (moreState_ != ButtonState::HIDDEN)
            _p.drawPixmap(getMoreButtonRect(), getMoreIcon(moreState_));

        auto offset = 0;
        if (name_)
        {
            name_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET));
            name_->draw(_p);
            offset += name_->cachedSize().height();
            offset += Utils::scale_value(NAME_OFFSET);
        }

        if (size_)
        {
            size_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET + NAME_OFFSET) + name_->cachedSize().height());
            size_->draw(_p);

            if (!localPath_.isEmpty() && isAntivirusAllowed())
            {
                showInFolder_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET + SHOW_IN_FOLDER_OFFSET) + size_->getLastLineWidth(), Utils::scale_value(VER_OFFSET + NAME_OFFSET) + name_->cachedSize().height());
                showInFolder_->draw(_p);
            }

            offset += size_->cachedSize().height();
            offset += Utils::scale_value(DATE_OFFSET);
        }

        friendly_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET) + offset);
        friendly_->draw(_p);

        date_->setOffsets(width_ - Utils::scale_value(RIGHT_OFFSET) - date_->getLastLineWidth(), Utils::scale_value(VER_OFFSET) + offset);
        date_->draw(_p);

        markDrew();
    }

    int FileItem::getHeight() const
    {
        return height_;
    }

    void FileItem::setWidth(int _width)
    {
        if (width_ == _width)
            return;

        width_ = _width;
        elide(name_);
        elide(size_);
        elide(friendly_, Utils::scale_value(DATE_LEFT_OFFSET) - date_->getLastLineWidth());
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

    QString FileItem::getFilename() const
    {
        return filename_;
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

    const Utils::FileSharingId& FileItem::getFileSharingId() const
    {
        return fileSharingId_;
    }

    bool FileItem::isAntivirusAllowed() const
    {
        return status_.isAntivirusAllowedType();
    }

    void FileItem::setFilename(const QString& _name)
    {
        name_ = Ui::TextRendering::MakeTextUnit(_name, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(16), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.maxLinesCount_ = 1;
        name_->init(params);
        name_->evaluateDesiredSize();
        elide(name_);

        filename_ = _name;

        fileType_ = FileSharingIcon::getIcon(_name);
    }

    void FileItem::setFilesize(qint64 _size)
    {
        sizet_ = _size;
        size_ = Ui::TextRendering::MakeTextUnit(HistoryControl::formatFileSize(_size), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(12), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.maxLinesCount_ = 1;
        size_->init(params);
        size_->evaluateDesiredSize();
        elide(size_);
    }

    void FileItem::setLastModified(qint64 _lastModified)
    {
        lastModified_ = _lastModified;
    }

    void FileItem::setMetaInfo(const Data::FileSharingMeta& _meta)
    {
        FileItem::setFilename(_meta.filenameShort_);
        FileItem::setFilesize(_meta.size_);
        FileItem::setLocalPath(_meta.localPath_);
        FileItem::setLastModified(_meta.lastModified_);

        status_.updateData(_meta.antivirusCheck_, _meta.trustRequired_, Features::isAntivirusCheckEnabled());
    }

    bool FileItem::isOverControl(const QPoint& _pos)
    {
        if (!isDrew())
            return false;
        static const QRegion r(iconRect(), QRegion::RegionType::Ellipse);
        return localPath_.isEmpty() && r.contains(_pos);
    }

    bool FileItem::isOverLink(const QPoint& _pos)
    {
        if (!isDrew())
            return false;
        static const QRegion r(iconRect(), QRegion::RegionType::Ellipse);
        return !localPath_.isEmpty() && (showInFolder_->contains(_pos) || r.contains(_pos) || (name_ && name_->contains(_pos)));
    }

    bool FileItem::isOverIcon(const QPoint& _pos)
    {
        if (!isDrew())
            return false;
        static const QRegion r(iconRect(), QRegion::RegionType::Ellipse);
        return !localPath_.isEmpty() && r.contains(_pos);
    }

    bool FileItem::isOverFilename(const QPoint& _pos) const
    {
        return name_ && name_->contains(_pos);
    }

    bool FileItem::isOverStatus(const QPoint& _pos) const
    {
        return getStatusRect().contains(_pos);
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
        if (!isDrew())
            return false;
        return date_->contains(_pos);
    }

    void FileItem::setDateState(bool _hover, bool _active)
    {
        if (_hover)
            date_->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER });
        else if (_active)
            date_->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_ACTIVE });
        else
            date_->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
    }

    bool FileItem::isOutgoing() const
    {
        return outgoing_;
    }

    void FileItem::setMoreButtonState(const ButtonState& _state)
    {
        moreState_ = _state;
    }

    bool FileItem::isOverMoreButton(const QPoint& _pos, int _h) const
    {
        if (!isDrew())
            return false;

        return getMoreButtonRect().translated(0, _h).contains(_pos);
    }

    ButtonState FileItem::moreButtonState() const
    {
        return moreState_;
    }

    bool FileItem::needsTooltip() const
    {
        return name_ && name_->isElided();
    }

    QRect FileItem::getTooltipRect() const
    {
        return QRect(0, name_->offsets().y(), width_, name_->cachedSize().height());
    }

    void FileItem::elide(const std::unique_ptr<TextRendering::TextUnit>& _unit, int _addedWidth)
    {
        if (_unit)
            _unit->getHeight(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_SIZE + PREVIEW_RIGHT_OFFSET) + _addedWidth);
    }

    QRect FileItem::getMoreButtonRect() const
    {
        const auto btnSize = Utils::scale_value(QSize(MORE_BUTTON_SIZE, MORE_BUTTON_SIZE));
        const auto x = width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - btnSize.width() / 2;
        const auto y = !Ui::needShowStatus(status_)
            ? (height_ - btnSize.height()) / 2
            : Utils::scale_value(VER_OFFSET) * 2 + getFileStatusIconSize().height();
        return QRect({ x, y }, btnSize);
    }

    QRect FileItem::getStatusRect() const
    {
        if (Ui::needShowStatus(status_))
        {
            const auto x = width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - getFileStatusIconSize().width() / 2;
            const auto y = Utils::scale_value(VER_OFFSET);
            return { QPoint(x, y), getFileStatusIconSize() };
        }
        return {};
    }

    void FileItem::processMetaInfo(const Utils::FileSharingId& /*_fsId*/, const Data::FileSharingMeta& _meta)
    {
        FileItem::setMetaInfo(_meta);
        if (Features::isAntivirusCheckEnabled() && FileItem::getFileStatus().isAntivirusInProgress())
        {
            Ui::GetDispatcher()->subscribeFileSharingAntivirus(getFileSharingId());
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::antivirusCheckResult, this, &FileItem::onAntivirusCheckResult, Qt::UniqueConnection);
        }

        if (parent_)
            parent_->update();
    }

    void FileItem::onAntivirusCheckResult(const Utils::FileSharingId& _fsId, core::antivirus::check::result _result)
    {
        if (auto& status = FileItem::getFileStatus(); !status.isAntivirusInProgress())
        {
            disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::antivirusCheckResult, this, &FileItem::onAntivirusCheckResult);
            status.setAntivirusCheckResult(_result);
            if (parent_)
                parent_->update();
        }
    }

    FilesList::FilesList(QWidget* _parent)
        : MediaContentWidget(MediaContentType::Files, _parent)
        , anim_(new QVariantAnimation(this))
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloading, this, &FilesList::fileDownloading);
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &FilesList::fileDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingError, this, &FilesList::fileError);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, qOverload<>(&FilesList::update));
        connect(Ui::GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, this, qOverload<>(&FilesList::update));

        anim_->setStartValue(0.0);
        anim_->setEndValue(360.0);
        anim_->setDuration(700);
        anim_->setLoopCount(-1);
        connect(anim_, &QVariantAnimation::valueChanged, this, qOverload<>(&FilesList::update));

        setMouseTracking(true);
    }

    void FilesList::initFor(const QString& _aimId)
    {
        MediaContentWidget::initFor(_aimId);
        Items_.clear();

        for (auto& [_, timer] : dataTransferTimeoutList_)
        {
            if (timer)
            {
                timer->stop();
                timer->deleteLater();
            }
        }
        dataTransferTimeoutList_.clear();

        setFixedHeight(0);
    }

    void FilesList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        const auto blocked = Logic::getContactListModel()->isTrustRequired(aimId_);

        auto h = height();
        for (const auto& e : _entries)
        {
            Utils::UrlParser parser;
            parser.process(e.url_);

            const auto isFilesharing = parser.hasUrl() && parser.isFileSharing();
            if (!isFilesharing)
                continue;

            const auto time = QDateTime::fromSecsSinceEpoch(e.time_);

            auto item = std::make_shared<FileItem>(e.url_, formatTimeStr(time), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width(), this);
            Logic::GetFileSharingMetaContainer()->requestFileSharingMetaInfo(Ui::ComplexMessage::extractIdFromFileSharingUri(e.url_), item);

            h += item->getHeight();

            item->getFileStatus().setTrustRequired(blocked);

            Items_.push_back(std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void FilesList::processUpdates(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        const auto blocked = Logic::getContactListModel()->isTrustRequired(aimId_);

        auto h = height();
        for (const auto& e : _entries)
        {
            if (e.action_ == u"del")
            {
                Items_.erase(std::remove_if(Items_.begin(), Items_.end(), [e](const auto& i) { return i->getMsg() == e.msg_id_; }), Items_.end());
                h = 0;
                for (const auto& i : Items_)
                    h += i->getHeight();

                continue;
            }

            Utils::UrlParser parser;
            parser.process(e.url_);

            const auto isFilesharing = parser.hasUrl() && parser.isFileSharing();
            if (!isFilesharing || (e.type_ != u"file" && e.type_ != u"audio"))
                continue;

            auto time = QDateTime::fromSecsSinceEpoch(e.time_);

            auto iter = Items_.cbegin();
            for (; iter != Items_.cend(); ++iter)
            {
                if (iter->get()->isDateItem())
                {
                    auto dt = QDateTime::fromSecsSinceEpoch(iter->get()->time());
                    if (dt.date().month() == time.date().month() && dt.date().year() == time.date().year())
                        continue;
                }

                auto curMsg = iter->get()->getMsg();
                auto curSeq = iter->get()->getSeq();
                if (curMsg > e.msg_id_ || (curMsg == e.msg_id_ && curSeq > e.seq_))
                    continue;

                break;
            }

            auto item = std::make_shared<FileItem>(e.url_, formatTimeStr(time), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width(), this);
            Logic::GetFileSharingMetaContainer()->requestFileSharingMetaInfo(Ui::ComplexMessage::extractIdFromFileSharingUri(e.url_), item);

            h += item->getHeight();

            item->getFileStatus().setTrustRequired(blocked);

            Items_.insert(iter, std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void FilesList::scrolled()
    {
        hideTooltip();
    }

    void FilesList::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        auto h = 0;

        const auto visibleRect = visibleRegion().boundingRect();

        const auto val = anim_->state() == QAbstractAnimation::Running ? anim_->currentValue().toDouble() : 0.0;
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (visibleRect.intersects(r))
                i->draw(p, r, val);

            h += i->getHeight();
        }
    }

    void FilesList::resizeEvent(QResizeEvent* _event)
    {
        for (auto& i : Items_)
            i->setWidth(width());

        MediaContentWidget::resizeEvent(_event);
    }

    void FilesList::mousePressEvent(QMouseEvent* _event)
    {
        auto h = 0;
        for (auto& i : Items_)
        {
            const auto posRel = _event->pos() - QPoint(0, h);
            i->setDateState(false, i->isOverDate(posRel));

            if (i->isOverMoreButton(_event->pos(), h))
                i->setMoreButtonState(ButtonState::PRESSED);

            h += i->getHeight();
        }

        update();
        pos_ = _event->pos();
        MediaContentWidget::mousePressEvent(_event);
    }

    void FilesList::mouseReleaseEvent(QMouseEvent* _event)
    {
        Utils::GalleryMediaActionCont cont(MediaTypeName, aimId_);

        const auto pos = _event->pos();

        if (_event->button() == Qt::RightButton)
        {
            cont.happened();
            return MediaContentWidget::mouseReleaseEvent(_event);
        }

        if (Utils::clicked(pos_, pos))
        {
            const auto trustAllowed = !Logic::getContactListModel()->isTrustRequired(aimId_) || MyInfo()->isTrusted();

            auto h = 0, j = 0;
            for (auto& i : Items_)
            {
                const auto r = QRect(0, h, width(), i->getHeight());
                const auto posRel = pos - QPoint(0, h);
                if (r.contains(pos))
                {
                    cont.happened();

                    if (i->isOverControl(posRel))
                    {
                        if (!i->isAntivirusAllowed())
                        {
                            showFileStatusToast(i->getFileStatus().type());
                        }
                        else if (i->isDownloading())
                        {
                            stopDataTransferTimeoutTimer(i->reqId());
                            Ui::GetDispatcher()->abortSharedFileDownloading(i->getLink(), i->reqId());
                            i->setDownloading(false);
                            Downloading_.erase(std::remove(Downloading_.begin(), Downloading_.end(), i->reqId()), Downloading_.end());
                            if (Downloading_.empty())
                                anim_->stop();

                            update();
                        }
                        else if (trustAllowed)
                        {
                            const auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                            Downloading_.push_back(reqId);
                            i->setReqId(reqId);
                            startDataTransferTimeoutTimer(reqId);
                        }
                        else
                        {
                            showFileStatusToast(i->getFileStatus().type());
                        }
                    }
                    else if (i->isOverLink(posRel))
                    {
#ifdef _WIN32
                        QFileInfo f(i->getLocalPath());
                        if (!f.exists() || f.size() != i->size() || f.lastModified().toTime_t() != i->lastModified())
                        {
                            auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                            Downloading_.push_back(reqId);
                            i->setLocalPath(QString());
                            i->setReqId(reqId);
                            startDataTransferTimeoutTimer(reqId);
                            h += i->getHeight();
                            continue;
                        }
#endif //_WIN32
                        const auto path = i->getLocalPath();
                        const auto openAt = i->isOverIcon(posRel) ? Utils::OpenAt::Launcher : Utils::OpenAt::Folder;
                        Utils::openFileOrFolder(aimId_, path, openAt);
                    }
                }
                if (i->isOverDate(posRel))
                {
                    Q_EMIT Logic::getContactListModel()->select(aimId_, i->getMsg(), Logic::UpdateChatSelection::No);
                    i->setDateState(true, false);
                }
                else
                {
                    i->setDateState(false, false);
                }

                if (i->moreButtonState() == ButtonState::PRESSED)
                {
                    cont.happened();

                    if (i->isOverMoreButton(pos, h))
                        i->setMoreButtonState(ButtonState::HOVERED);
                    else
                        i->setMoreButtonState(ButtonState::NORMAL);

                    if (Utils::clicked(pos_, pos))
                        showContextMenu(ItemData(i->getLink(), i->getMsg(), i->sender(), i->time()), pos, true);
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
        auto forceTooltip = false;
        const auto pos = _event->pos();
        for (auto& i : Items_)
        {
            const auto r = QRect(0, h, width(), i->getHeight());
            const auto posRel = pos - QPoint(0, h);
            if (r.contains(pos))
            {
                if (i->isOverControl(posRel) || i->isOverLink(posRel))
                    point = true;

                const auto needTooltip =
                    i->isOverStatus(posRel) ||
                    (Features::longPathTooltipsAllowed() && i->needsTooltip());

                if (needTooltip)
                {
                    forceTooltip = true;
                    updateTooltip(i, posRel, h);
                }
            }

            if (i->isOverDate(posRel))
            {
                point = true;
                i->setDateState(true, false);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(pos, h))
            {
                if (i->moreButtonState() != ButtonState::PRESSED)
                    i->setMoreButtonState(ButtonState::HOVERED);
                point = true;
            }
            else
            {
                if (r.contains(pos))
                    i->setMoreButtonState(ButtonState::NORMAL);
                else
                    i->setMoreButtonState(ButtonState::HIDDEN);
            }

            h += i->getHeight();
        }

        Tooltip::forceShow(forceTooltip);
        if (!forceTooltip)
            hideTooltip();

        setCursor(point ? Qt::PointingHandCursor : Qt::ArrowCursor);

        update();
        MediaContentWidget::mouseMoveEvent(_event);
    }

    void FilesList::leaveEvent(QEvent* _event)
    {
        for (auto& i : Items_)
            i->setMoreButtonState(ButtonState::HIDDEN);

        hideTooltip();
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

    void FilesList::fileDownloading(qint64 _seq, QString /*_rawUri*/, qint64 _bytesTransferred, qint64 _bytesTotal)
    {
        if (std::find(Downloading_.begin(), Downloading_.end(), _seq) == Downloading_.end() || Items_.empty())
            return;

        if (anim_->state() != QAbstractAnimation::Running)
            anim_->start();

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                stopDataTransferTimeoutTimer(_seq);
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
                        anim_->stop();
                    Utils::showDownloadToast(aimId_, _result);
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
                    Utils::showDownloadToast(aimId_, _result);
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
                stopDataTransferTimeoutTimer(_seq);
                Downloading_.erase(std::remove(Downloading_.begin(), Downloading_.end(), i->reqId()), Downloading_.end());
                if (Downloading_.empty())
                    anim_->stop();

                auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), false, QString(), true);
                Downloading_.push_back(reqId);
                i->setReqId(reqId);
                startDataTransferTimeoutTimer(reqId);
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
        auto prevDt = QDateTime::fromSecsSinceEpoch(iter->get()->time()).date();
        ++iter;

        while (iter != Items_.end())
        {
            auto isDate = iter->get()->isDateItem();
            auto dt = QDateTime::fromSecsSinceEpoch(iter->get()->time()).date();
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

    void FilesList::updateTooltip(const std::shared_ptr<BaseFileItem>& _item, const QPoint& _p, int _h)
    {
        const auto overName = _item->isOverFilename(_p);
        const auto overStatus = _item->isOverStatus(_p);
        if (overName || overStatus)
        {
            if (!isTooltipActivated())
            {
                const auto relRect = overName ? _item->getTooltipRect() : _item->getStatusRect();
                const auto ttRect = relRect.translated(0, _h);
                const auto isFullyVisible = visibleRegion().boundingRect().y() < ttRect.top();
                const auto arrowDir = isFullyVisible ? Tooltip::ArrowDirection::Down : Tooltip::ArrowDirection::Up;
                const auto arrowPos = isFullyVisible ? Tooltip::ArrowPointPos::Top : Tooltip::ArrowPointPos::Bottom;
                const auto text = overName ? _item->getFilename() : getFileStatusText(_item->getFileStatus().type());
                showTooltip(text, QRect(mapToGlobal(ttRect.topLeft()), ttRect.size()), arrowDir, arrowPos);
            }
        }
        else
        {
            hideTooltip();
        }
    }

    void FilesList::startDataTransferTimeoutTimer(qint64 _seq)
    {
        const auto begin = dataTransferTimeoutList_.cbegin();
        const auto end = dataTransferTimeoutList_.cend();
        if (std::any_of(begin, end, [_seq](const auto& _x){ return _x.first == _seq; }))
            return;

        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(getDataTransferTimeout());
        connect(timer, &QTimer::timeout, this, [this, _seq]()
        {
            // emulate 1% of transferred data for the spinner
            fileDownloading(_seq, {}, 1, 100);
        });

        dataTransferTimeoutList_.emplace_back(_seq, timer);
        timer->start();
    }

    void FilesList::stopDataTransferTimeoutTimer(qint64 _seq)
    {
        if (dataTransferTimeoutList_.empty())
            return;

        auto begin = dataTransferTimeoutList_.begin();
        auto end = dataTransferTimeoutList_.end();
        if (auto it = std::find_if(begin, end, [_seq](const auto& _x) { return _x.first == _seq; }); it != end)
        {
            it->second->stop();
            it->second->deleteLater();
            dataTransferTimeoutList_.erase(it);
        }
    }
} // namespace Ui

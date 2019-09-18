#include "stdafx.h"

#include "../MessageStyle.h"
#include "styles/ThemesContainer.h"
#include "../../../utils/utils.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../my_info.h"
#include "../../../core_dispatcher.h"
#include "../../../cache/stickers/stickers.h"
#include "../../mediatype.h"

#include "StickerBlockLayout.h"
#include "Selection.h"

#include "StickerBlock.h"
#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"

using namespace Ui::Stickers;

namespace
{
    QSize getGhostSize()
    {
        return Utils::scale_value(QSize(160, 160));
    }

    QPixmap getGhost()
    {
        static const auto ghost = Utils::renderSvg(qsl("://themes/stickers/ghost_sticker"), getGhostSize());
        return ghost;
    }

    core::sticker_size getStickerSize()
    {
        return core::sticker_size::large;
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

StickerBlock::StickerBlock(ComplexMessageItem *parent,  const HistoryControl::StickerInfoSptr& _info)
    : GenericBlock(parent, QT_TRANSLATE_NOOP("contact_list", "Sticker"), MenuFlagNone, false)
    , Info_(_info)
    , Layout_(nullptr)
    , LastSize_(getGhostSize())
{
    assert(Info_);

    Layout_ = new StickerBlockLayout();
    setLayout(Layout_);

    QuoteAnimation_.setSemiTransparent();
    setMouseTracking(true);

    connections_.reserve(4);
}

StickerBlock::~StickerBlock() = default;

IItemBlockLayout* StickerBlock::getBlockLayout() const
{
    return Layout_;
}

QString StickerBlock::getSelectedText(const bool, const TextDestination) const
{
    return isSelected() ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : QString();
}

QString StickerBlock::getSourceText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

bool StickerBlock::updateFriendly(const QString&/* _aimId*/, const QString&/* _friendly*/)
{
    return false;
}

QString StickerBlock::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

Ui::MediaType StickerBlock::getMediaType() const
{
    return Ui::MediaType::mediaTypeSticker;
}

int StickerBlock::desiredWidth(int) const
{
    return LastSize_.width();
}

QRect StickerBlock::setBlockGeometry(const QRect &ltr)
{
    const auto r = GenericBlock::setBlockGeometry(ltr);
    Geometry_ = QRect(r.topLeft(), LastSize_);
    setGeometry(Geometry_);
    return Geometry_;
}

void StickerBlock::requestPinPreview()
{
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(GetDispatcher(), &core_dispatcher::onSticker, this,
        [conn, this](const qint32 _error, const qint32 _setId, const qint32 _stickerId)
    {
        if (int(Info_->SetId_) == _setId && int(Info_->StickerId_) == _stickerId)
        {
            if (_error == 0 && getParentComplexMessage())
            {
                const auto sticker = getSticker(Info_->SetId_, Info_->StickerId_);
                if (sticker)
                {
                    auto unused = false;
                    auto image = sticker->getImage(getStickerSize(), false, unused);

                    emit getParentComplexMessage()->pinPreview(QPixmap::fromImage(image));
                }
            }
            disconnect(*conn);
        }
    });

    requestSticker();
}

void StickerBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    Utils::PainterSaver ps(p);

    if (Sticker_.isNull())
    {
        if (!Placeholder_.isNull())
        {
            const auto offset = (isOutgoing() ? (width() - getGhostSize().width()) : 0);
            p.drawPixmap(QRect(QPoint(offset, 0), LastSize_), Placeholder_);
        }
    }
    else
    {
        updateStickerSize();

        const auto offset = (isOutgoing() ? (width() - LastSize_.width()) : 0);
        const auto rect = QRect(QPoint(offset, 0), LastSize_);

        p.drawImage(rect, Sticker_);

        if (_quoteColor.isValid())
        {
            p.setBrush(QBrush(_quoteColor));
            p.drawRoundRect(rect, Utils::scale_value(8), Utils::scale_value(8));
        }
    }

    if (isSelected())
        renderSelected(p);
}

void StickerBlock::initialize()
{
    connectStickerSignal(true);

    requestSticker();
}

void StickerBlock::connectStickerSignal(const bool _isConnected)
{
    for (const auto& c : connections_)
        disconnect(c);

    connections_.clear();

    if (_isConnected)
    {
        connections_.assign({
            connect(GetDispatcher(), &core_dispatcher::onSticker,  this, &StickerBlock::onSticker),
            connect(GetDispatcher(), &core_dispatcher::onStickers, this, &StickerBlock::onStickers)
        });
    }
}

void StickerBlock::loadSticker()
{
    assert(Sticker_.isNull());

    const auto sticker = getSticker(Info_->SetId_, Info_->StickerId_);
    if (!sticker)
        return;

    bool scaled = false;
    Sticker_ = sticker->getImage(getStickerSize(), false, scaled);

    if (sticker->isFailed())
        initPlaceholder();

    if (Sticker_.isNull())
        return;

    Sticker_ = FileSharingBlock::scaledSticker(Sticker_);

    connectStickerSignal(false);

    setCursor(Qt::PointingHandCursor);
    updateGeometry();
    update();
}

void StickerBlock::renderSelected(QPainter& _p)
{
    assert(isSelected());

    _p.fillRect(rect(), MessageStyle::getSelectionColor());
}

void StickerBlock::requestSticker()
{
    assert(Sticker_.isNull());

    Ui::GetDispatcher()->getSticker(Info_->SetId_, Info_->StickerId_, getStickerSize());
}

void StickerBlock::updateStickerSize()
{
    auto stickerSize = Sticker_.size();
    if (Utils::is_mac_retina())
        stickerSize = QSize(stickerSize.width() / 2, stickerSize.height() / 2);

    if (LastSize_ != stickerSize)
    {
        LastSize_ = stickerSize;
        notifyBlockContentsChanged();
    }
}

void StickerBlock::initPlaceholder()
{
    if (Placeholder_.isNull())
    {
        Placeholder_ = getGhost();
        LastSize_ = getGhostSize();
    }
}

void StickerBlock::updateStyle()
{
}

void StickerBlock::updateFonts()
{
}

void StickerBlock::onSticker(const qint32 _error, const qint32 _setId, const qint32 _stickerId)
{
    if (_stickerId <= 0)
        return;

    assert(_setId > 0);

    const auto isMySticker = ((qint32(Info_->SetId_) == _setId) && (qint32(Info_->StickerId_) == _stickerId));
    if (!isMySticker)
        return;

    if (_error == 0)
    {
        if (!Sticker_.isNull())
            return;

        loadSticker();
        return;
    }

    initPlaceholder();

    update();
}

void StickerBlock::onStickers()
{
    if (Sticker_.isNull())
        requestSticker();
}

const QImage& StickerBlock::getStickerImage() const
{
    return Sticker_;
}

bool StickerBlock::clicked(const QPoint& _p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());
    if (!rect().contains(mappedPoint))
        return true;

    emit Utils::InterConnector::instance().stopPttRecord();
    Stickers::showStickersPack(Info_->SetId_, Stickers::StatContext::Chat);

    if (auto pack = Stickers::getSet(Info_->SetId_); pack && !pack->isPurchased())
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_sticker_tap_in_chat);

    return true;
}

QString StickerBlock::getStickerId() const
{
    if (!Info_)
        return QString();

    return ql1s("ext:") % QString::number(Info_->SetId_) % ql1s(":sticker:") % QString::number(Info_->StickerId_);
}

UI_COMPLEX_MESSAGE_NS_END

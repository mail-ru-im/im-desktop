#include "stdafx.h"

#include "suggests_widget.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/TooltipWidget.h"
#include "../../utils/InterConnector.h"
#include "../../main_window/MainWindow.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    int getStickerWidth()
    {
        return Utils::scale_value(60);
    }

    int getStickersSpacing()
    {
        return Utils::scale_value(8);
    }

    int getSuggestHeight()
    {
        return Utils::scale_value(70);
    }
}

using namespace Ui;
using namespace Stickers;

StickersWidget::StickersWidget(QWidget* _parent)
    : StickersTable(_parent, -1, getStickerWidth(), getStickerWidth() + getStickersSpacing(), false)
    , stickerPreview_(nullptr)
{
    connect(GetDispatcher(), &core_dispatcher::onSticker, this, [this](const qint32 _error, const qint32 _setId, qint32, const QString& _stickerId)
    {
        redrawSticker(_setId, _stickerId);
    });

    setMouseTracking(true);

    connect(this, &StickersWidget::stickerPreview, this, &StickersWidget::onStickerPreview);
}

void StickersWidget::closeStickerPreview()
{
    if (!stickerPreview_)
        return;

    stickerPreview_->hide();

    delete stickerPreview_;

    stickerPreview_ = nullptr;
}

void StickersWidget::onStickerPreview(const int32_t _setId, const QString& _stickerId)
{
    if (!stickerPreview_)
    {
        stickerPreview_ = new Smiles::StickerPreview(Utils::InterConnector::instance().getMainWindow()->getWidget(), _setId, _stickerId, Smiles::StickerPreview::Context::Popup);
        stickerPreview_->setGeometry(Utils::InterConnector::instance().getMainWindow()->getWidget()->rect());
        stickerPreview_->show();
        stickerPreview_->raise();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_longtap_preview);

        QObject::connect(stickerPreview_, &Smiles::StickerPreview::needClose, this, &StickersWidget::closeStickerPreview, Qt::QueuedConnection); // !!! must be Qt::QueuedConnection
        QObject::connect(this, &StickersTable::stickerHovered, this, &StickersWidget::onStickerHovered);
    }
}

void StickersWidget::onStickerHovered(const int32_t _setId, const QString& _stickerId)
{
    if (stickerPreview_)
        stickerPreview_->showSticker(_setId, _stickerId);
}

void StickersWidget::init(const QString& _text)
{
    stickersArray_.clear();

    Stickers::Suggest suggest;
    Stickers::getSuggestWithSettings(_text, suggest);

    stickersArray_.reserve(suggest.size());
    for (const auto& _sticker : suggest)
    {
        const auto sticker = getSticker(_sticker.fsId_);
        if (!sticker)
            Ui::GetDispatcher()->getSticker(_sticker.fsId_, core::sticker_size::small);

        stickersArray_.emplace_back(_sticker.fsId_);
    }

    const int w = (getStickerWidth() + getStickersSpacing()) * suggest.size();

    setFixedWidth(w);
    setFixedHeight(getSuggestHeight());
}

bool StickersWidget::resize(const QSize& _size, bool _force)
{
    needHeight_ = getStickerWidth();

    columnCount_ = stickersArray_.size();

    rowCount_ = 1;

    return false;
}

void StickersWidget::mouseReleaseEvent(QMouseEvent* _e)
{
    if (stickerPreview_)
        closeStickerPreview();
    else
        StickersTable::mouseReleaseEventInternal(_e->pos());

    QWidget::mouseReleaseEvent(_e);
}

void StickersWidget::mousePressEvent(QMouseEvent* _e)
{
    StickersTable::mousePressEventInternal(_e->pos());

    QWidget::mousePressEvent(_e);
}

void StickersWidget::mouseMoveEvent(QMouseEvent* _e)
{
    StickersTable::mouseMoveEventInternal(_e->pos());

    QWidget::mouseMoveEvent(_e);
}

void StickersWidget::leaveEvent(QEvent* _e)
{
    StickersTable::leaveEventInternal();

    QWidget::leaveEvent(_e);
}

int32_t StickersWidget::getStickerPosInSet(const QString & _stickerId) const
{
    const auto it = std::find_if(stickersArray_.begin(), stickersArray_.end(), [&_stickerId](const auto& _s) { return _s == _stickerId; });
    if (it != stickersArray_.end())
        return std::distance(stickersArray_.begin(), it);
    return -1;
}

const stickersArray& StickersWidget::getStickerIds() const
{
    return stickersArray_;
}

void StickersWidget::setSelected(const std::pair<int32_t, QString>& _sticker)
{
    const auto stickerPosInSet = getStickerPosInSet(_sticker.second);
    if (stickerPosInSet >= 0)
        emit stickerHovered(getStickerRect(stickerPosInSet));

    StickersTable::setSelected(_sticker);
}

StickersSuggest::StickersSuggest(QWidget* _parent)
    : QWidget(_parent)
    , stickers_(nullptr)
    , tooltip_(nullptr)
    , keyboardActive_(false)
{
    setStyleSheet(Styling::getParameters().getBackgroundCommonQss());
    stickers_ = new StickersWidget(_parent);
    tooltip_ = new TooltipWidget(_parent, stickers_, Utils::scale_value(44), QMargins(Utils::scale_value(8), Utils::scale_value(4), Utils::scale_value(8), Utils::scale_value(4)), true, true, true);
    tooltip_->setCursor(Qt::PointingHandCursor);

    connect(stickers_, &StickersWidget::stickerSelected, this, &StickersSuggest::stickerSelected);
    connect(stickers_, &StickersWidget::stickerHovered, this, &StickersSuggest::stickerHovered);
}

void StickersSuggest::showAnimated(const QString& _text, const QPoint& _p, const QSize& _maxSize, const QRect& _rect)
{
    stickers_->init(_text);
    stickers_->setSelected(std::make_pair(-1, QString()));
    tooltip_->scrollToTop();
    tooltip_->showAnimated(_p, _maxSize, _rect);

    keyboardActive_ = false;
}

void StickersSuggest::hideAnimated()
{
    tooltip_->hideAnimated();
}

bool StickersSuggest::isTooltipVisible() const
{
    return tooltip_->isVisible();
}

void StickersSuggest::stickerHovered(const QRect& _stickerRect)
{
    tooltip_->ensureVisible(_stickerRect.left(), _stickerRect.top(), _stickerRect.width(), _stickerRect.height());
}

void StickersSuggest::keyPressEvent(QKeyEvent* _event)
{
    _event->ignore();

    switch (_event->key())
    {
        case Qt::Key_Up:
        {
            stickers_->selectFirst();

            keyboardActive_ = true;

            _event->accept();

            break;
        }
        case Qt::Key_Down:
        {
            stickers_->clearSelection();

            keyboardActive_ = false;

            _event->accept();

            break;
        }
        case Qt::Key_Left:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (!selectedSticker.second.isEmpty())
            {
                stickers_->selectLeft();

                _event->accept();
            }

            keyboardActive_ = true;

            break;
        }
        case Qt::Key_Right:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (!selectedSticker.second.isEmpty())
            {
                stickers_->selectRight();

                _event->accept();
            }

            keyboardActive_ = true;

            break;
        }
        case Qt::Key_Return:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (keyboardActive_ && !selectedSticker.second.isEmpty())
            {
                emit stickerSelected(selectedSticker.first, selectedSticker.second);

                _event->accept();
            }
        }
    }
}
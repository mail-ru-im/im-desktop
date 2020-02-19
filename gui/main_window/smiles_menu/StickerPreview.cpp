#include "stdafx.h"
#include "StickerPreview.h"

#include "core_dispatcher.h"
#include "utils/InterConnector.h"
#include "utils/medialoader.h"
#include "styles/ThemeParameters.h"
#include "main_window/mplayer/VideoPlayer.h"

#include "cache/stickers/stickers.h"
#include "cache/emoji/EmojiDb.h"

namespace
{
    int getCommonImageMargin()
    {
        return Utils::scale_value(20);
    }

    int getStickerPreviewEmojiFrameHeight()
    {
        return Utils::scale_value(32);
    }

    int getBottomImageMargin()
    {
        return 2 * getCommonImageMargin() + getStickerPreviewEmojiFrameHeight();
    }

    int getTopImageMargin(Ui::Smiles::StickerPreview::Context _context)
    {
        if (_context == Ui::Smiles::StickerPreview::Context::Picker)
            return Utils::scale_value(28);
        else
            return Utils::scale_value(32);
    }

    int getStickerPreviewMaxSize()
    {
        return Utils::scale_value(240);
    }

    int getStickerPreviewEmojiHeight()
    {
        return Utils::scale_bitmap_with_value(28);
    }

    int getStickerPreviewEmojiRightMargin()
    {
        return Utils::scale_value(4);
    }
}

namespace Ui::Smiles
{
    StickerPreview::StickerPreview(
        QWidget* _parent,
        const int32_t _setId,
        const QString& _stickerId,
        Context _context)
        : QWidget(_parent)
        , setId_(_setId)
        , stickerId_(_stickerId)
        , context_(_context)
    {
        QObject::connect(GetDispatcher(), &core_dispatcher::onSticker, this, [this](
            const qint32 _error,
            const qint32 _setId,
            const qint32 _stickerId,
            const QString& _fsId)
        {
            if (_fsId == stickerId_)
            {
                sticker_ = QPixmap();
                update();
            }
        });

        setCursor(Qt::PointingHandCursor);

        init(_setId, _stickerId);
    }

    StickerPreview::~StickerPreview() = default;

    void StickerPreview::showSticker(const int32_t _setId, const QString& _stickerId)
    {
        // showSticker can be called after synthetic mouse move event sent from QWidget::hide,
        // so we need this, to prevent player_ object resetting (in StickerPreview::init) during execution of QWidget::hide
        if (hiding_)
            return;

        init(_setId, _stickerId);
    }

    void StickerPreview::hide()
    {
        QScopedValueRollback hidingRollback(hiding_);
        hiding_ = true;

        QWidget::hide();
    }

    void StickerPreview::updateEmoji()
    {
        auto sticker = Stickers::getSticker(setId_, stickerId_);
        if (!sticker)
            return;

        auto emojis = sticker->getEmojis();
        emojis_.clear();

        QVector<uint> codepoints;
        for (const auto & emoji : emojis)
            codepoints.append(emoji.toUcs4());

        auto current = 0;
        while (current < codepoints.size())
        {
            Emoji::EmojiCode code;

            auto i = current;
            while(i < codepoints.size() && code.size() < Emoji::EmojiCode::maxSize())  // create emoji code of max possible length
                code = Emoji::EmojiCode::addCodePoint(code, codepoints[i++]);

            while(code.size())                                                         // find valid emoji code, by decreasing length
            {
                if (Emoji::isEmoji(code))
                    break;
                code = code.chopped(1);
            }

            auto image = Emoji::GetEmoji(code, getStickerPreviewEmojiHeight());
            if (!image.isNull())
                emojis_.push_back(std::move(image));

            current += (code.size() > 0) ? code.size() : 1;
        }
    }

    void StickerPreview::paintEvent(QPaintEvent* _e)
    {
        const auto clientRect = rect();

        if (sticker_.isNull())
            loadSticker();

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QColor color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        color.setAlphaF(0.9);
        p.fillRect(clientRect, color);

        if (!player_)
            drawSticker(p);

        drawEmoji(p);
    }

    void StickerPreview::mouseReleaseEvent(QMouseEvent* _e)
    {
        emit needClose();
    }

    void StickerPreview::onGifLoaded(const QString &_path)
    {
        player_ = std::make_unique<DialogPlayer>(this, DialogPlayer::Flags::is_gif);

        if (!sticker_.isNull())
            player_->setPreview(sticker_);

        player_->setMedia(_path);
        player_->setParent(this);
        player_->updateSize(getAdjustedImageRect());
        player_->setReplay(true);
        player_->start(true);
        player_->show();
    }

    void StickerPreview::onActivationChanged(bool _active)
    {
        if (player_)
            player_->updateVisibility(_active);
    }

    void StickerPreview::init(const int32_t _setId, const QString &_stickerId)
    {
        if (stickerId_ == _stickerId)
            return;

        player_.reset();
        stickerId_ = _stickerId;
        setId_ = _setId;

        sticker_ = QPixmap();

        updateEmoji();

        update();

        const auto sticker = Stickers::getSticker(setId_, stickerId_);
        if (sticker && sticker->isGif())
        {
            loader_ = std::make_unique<Utils::FileSharingLoader>(Stickers::getSendBaseUrl() % sticker->getFsId());
            connect(loader_.get(), &Utils::FileSharingLoader::fileLoaded, this, &StickerPreview::onGifLoaded);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activationChanged, this, &StickerPreview::onActivationChanged);
            loader_->load();
        }
    }

    void StickerPreview::drawSticker(QPainter &_p)
    {
        if (!sticker_.isNull())
            _p.drawPixmap(getAdjustedImageRect(), sticker_);
    }

    void StickerPreview::drawEmoji(QPainter &_p)
    {
        if (emojis_.empty())
            return;

        const auto emojiSize = getStickerPreviewEmojiHeight();
        const auto emojiMargin = getStickerPreviewEmojiRightMargin();

        const auto clientRect = rect();
        const auto imageRect = getImageRect();

        // count max number of emojis to draw
        int emojiCount = (clientRect.width() - 2 * getCommonImageMargin()) / (emojiSize + emojiMargin);
        emojiCount = std::min<int>(emojiCount, emojis_.size());

        int emojiXOffset = (clientRect.width() - ((Utils::unscale_bitmap(emojiSize) + emojiMargin) * emojiCount - emojiMargin)) / 2;

        const int emojiYOffset = imageRect.bottom() + getCommonImageMargin();

        for (auto i = 0; i < emojiCount; ++i)
        {
            _p.drawImage(emojiXOffset, emojiYOffset, emojis_[i]);
            emojiXOffset += Utils::unscale_bitmap(emojiSize) + emojiMargin;
        }
    }

    QRect StickerPreview::getImageRect() const
    {
        const auto clientRect = rect();

        QRect imageRectStart(
            clientRect.left() + getCommonImageMargin(),
            clientRect.top() + getTopImageMargin(context_),
            clientRect.width() - 2 * getCommonImageMargin(),
            clientRect.height() - getBottomImageMargin() - getTopImageMargin(context_));

        QRect imageRect(imageRectStart);

        imageRect.setWidth(std::min(imageRect.width(), getStickerPreviewMaxSize()));
        imageRect.setHeight(std::min(imageRect.height(), getStickerPreviewMaxSize() + getBottomImageMargin()));
        imageRect.moveCenter(imageRectStart.center());

        return imageRect;
    }

    QRect StickerPreview::getAdjustedImageRect() const
    {
        const auto imageRect = getImageRect();

        if (!sticker_.isNull())
        {
            const QSize stickerSize = Utils::unscale_bitmap(sticker_.size());
            const int x = imageRect.left() + ((imageRect.width() - stickerSize.width()) / 2);
            const int y = imageRect.top() + ((imageRect.height() - stickerSize.height()) / 2);

            return QRect(x, y, stickerSize.width(), stickerSize.height());
        }
        else
        {
            return imageRect;
        }
    }

    void StickerPreview::loadSticker()
    {
        sticker_ = QPixmap::fromImage(Stickers::getStickerImage(setId_, stickerId_, core::sticker_size::xxlarge, true));

        if (!sticker_.isNull())
            scaleSticker();
    }

    void StickerPreview::scaleSticker()
    {
        const auto imageRect = getImageRect();

        sticker_ = sticker_.scaled(Utils::scale_bitmap(QSize(std::min(getStickerPreviewMaxSize(), imageRect.width()), std::min(getStickerPreviewMaxSize(), imageRect.height()))),
            Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

        Utils::check_pixel_ratio(sticker_);

        if (player_)                                        // update player size if it is created
            player_->updateSize(getAdjustedImageRect());
    }
}
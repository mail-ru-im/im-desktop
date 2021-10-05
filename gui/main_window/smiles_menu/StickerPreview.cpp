#include "stdafx.h"
#include "StickerPreview.h"

#include "app_config.h"
#include "core_dispatcher.h"
#include "fonts.h"
#include "utils/InterConnector.h"
#include "utils/medialoader.h"
#include "styles/ThemeParameters.h"
#ifndef STRIP_AV_MEDIA
#include "main_window/mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA

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

    constexpr auto stickerSize = core::sticker_size::xxlarge;
}

namespace Ui::Smiles
{
    StickerPreview::StickerPreview(QWidget* _parent, const int32_t _setId, const Utils::FileSharingId& _stickerId, Context _context)
        : QWidget(_parent)
        , context_(_context)
    {
        connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, &StickerPreview::onStickerLoaded);

        setCursor(Qt::PointingHandCursor);

        init(_stickerId);
    }

    StickerPreview::~StickerPreview() = default;

    void StickerPreview::showSticker(const Utils::FileSharingId& _stickerId)
    {
        // showSticker can be called after synthetic mouse move event sent from QWidget::hide,
        // so we need this, to prevent player_ object resetting (in StickerPreview::init) during execution of QWidget::hide
        if (hiding_)
            return;

        init(_stickerId);
    }

    void StickerPreview::hide()
    {
        QScopedValueRollback hidingRollback(hiding_);
        hiding_ = true;

        QWidget::hide();
    }

    void StickerPreview::updateEmoji()
    {
        auto sticker = Stickers::getSticker(stickerId_);
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
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE, 0.9));

        if (sticker_.isNull())
            loadSticker();

#ifndef STRIP_AV_MEDIA
        if (!player_)
#endif // !STRIP_AV_MEDIA
            drawSticker(p);

        drawEmoji(p);

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = getAdjustedImageRect().center().x();
            const auto y = getAdjustedImageRect().bottom() + Utils::scale_value(10);
            Utils::drawText(p, QPointF(x, y), Qt::AlignHCenter | Qt::AlignBottom, stickerId_.fileId + (stickerId_.sourceId ? (qsl("?source=") + *stickerId_.sourceId) : QString()));
        }
    }

    void StickerPreview::mouseReleaseEvent(QMouseEvent* _e)
    {
        Q_EMIT needClose();
    }

    void StickerPreview::resizeEvent(QResizeEvent* _e)
    {
        updatePlayerSize();
    }

    void StickerPreview::onAnimatedStickerLoaded(const QString& _path)
    {
#ifndef STRIP_AV_MEDIA
        if (_path.isEmpty())
            return;

        const auto sticker = Stickers::getSticker(stickerId_);
        if (!sticker)
            return;

        const auto flag = sticker->isGif()
            ? DialogPlayer::Flags::is_gif
            : (DialogPlayer::Flags::lottie | DialogPlayer::Flags::lottie_instant_preview);
        player_ = std::make_unique<DialogPlayer>(this, flag);

        if (!sticker_.isNull())
            player_->setPreview(sticker_);

        player_->setMedia(_path);
        player_->setParent(this);
        player_->updateSize(getAdjustedImageRect());
        player_->setReplay(true);
        player_->start(true);
        player_->show();
#endif // !STRIP_AV_MEDIA
    }

    void StickerPreview::onActivationChanged(bool _active)
    {
#ifndef STRIP_AV_MEDIA
        if (player_)
            player_->updateVisibility(_active);
#endif // !STRIP_AV_MEDIA

    }

    void StickerPreview::onStickerLoaded(int _error, const Utils::FileSharingId& _id)
    {
        if (_error == 0 && stickerId_ == _id)
        {
            const auto& data = Stickers::getStickerData(stickerId_, stickerSize);
            if (data.isLottie())
            {
                if (const auto& path = data.getLottiePath(); !path.isEmpty())
                    onAnimatedStickerLoaded(path);
            }
            else if (data.isPixmap())
            {
                sticker_ = QPixmap();
                update();
            }
        }
    }

    void StickerPreview::init(const Utils::FileSharingId&_stickerId)
    {
        if (stickerId_ == _stickerId)
            return;
#ifndef STRIP_AV_MEDIA
        player_.reset();
#endif // !STRIP_AV_MEDIA
        stickerId_ = _stickerId;

        sticker_ = QPixmap();

        loader_.reset();

        updateEmoji();

        update();

        const auto sticker = Stickers::getSticker(stickerId_);
        if (sticker && sticker->isAnimated())
        {
            if (sticker->isLottie())
            {
                const auto& data = sticker->getData(stickerSize);
                if (const auto& path = data.getLottiePath(); !path.isEmpty())
                    onAnimatedStickerLoaded(path);
            }
            else
            {
                loader_ = std::make_unique<Utils::FileSharingLoader>(Stickers::getSendUrl(stickerId_));
                connect(loader_.get(), &Utils::FileSharingLoader::fileLoaded, this, &StickerPreview::onAnimatedStickerLoaded);
                loader_->load();
            }
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activationChanged, this, &StickerPreview::onActivationChanged, Qt::UniqueConnection);
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

        // count max number of emojis to draw
        int emojiCount = (width() - 2 * getCommonImageMargin()) / (emojiSize + emojiMargin);
        emojiCount = std::min<int>(emojiCount, emojis_.size());

        int emojiXOffset = (width() - ((Utils::unscale_bitmap(emojiSize) + emojiMargin) * emojiCount - emojiMargin)) / 2;

        const int emojiYOffset = height() - getTopImageMargin(context_) - getBottomImageMargin() / 2;

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
        const auto side = std::min(std::min(imageRect.width(), imageRect.height()), getStickerPreviewMaxSize());
        imageRect.setSize(QSize(side, side));
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

        return imageRect;
    }

    void StickerPreview::loadSticker()
    {
        if (const auto& data = Stickers::getStickerData(stickerId_, stickerSize); data.isPixmap())
            sticker_ = data.getPixmap();

        if (!sticker_.isNull())
            scaleSticker();
    }

    void StickerPreview::scaleSticker()
    {
        const auto imageRect = getImageRect();

        sticker_ = sticker_.scaled(Utils::scale_bitmap(QSize(std::min(getStickerPreviewMaxSize(), imageRect.width()), std::min(getStickerPreviewMaxSize(), imageRect.height()))),
            Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

        Utils::check_pixel_ratio(sticker_);

        // update player size if it is created
        updatePlayerSize();
    }

    void StickerPreview::updatePlayerSize()
    {
#ifndef STRIP_AV_MEDIA
        if (player_)
            player_->updateSize(getAdjustedImageRect());
#endif // !STRIP_AV_MEDIA
    }
}
#pragma once

#include "GenericBlock.h"
#include "StickerBlockLayout.h"
#include "../StickerInfo.h"

namespace Ui
{
    enum class MediaType;
}

UI_COMPLEX_MESSAGE_NS_BEGIN

class StickerBlock final : public GenericBlock
{
    friend class StickerBlockLayout;

    Q_OBJECT

public:
    StickerBlock(ComplexMessageItem *parent, const HistoryControl::StickerInfoSptr& _info);

    virtual ~StickerBlock() override;

    IItemBlockLayout* getBlockLayout() const override;

    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool isAllSelected() const override { return isSelected(); }

    QRect setBlockGeometry(const QRect &ltr) override;

    QString getSourceText() const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    QString formatRecentsText() const override;

    Ui::MediaType getMediaType() const override;

    bool isBubbleRequired() const override { return false; }

    bool isSharingEnabled() const override { return false; }

    HistoryControl::StickerInfoSptr getStickerInfo() const override { return Info_; };

    int desiredWidth(int _width = 0) const override;

    PinPlaceholderType getPinPlaceholder() const override { return PinPlaceholderType::Sticker; }

    void requestPinPreview() override;

    bool clicked(const QPoint& _p) override;

    ContentType getContentType() const override { return IItemBlock::ContentType::Sticker; }

    QString getStickerId() const override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;


private Q_SLOTS:
    void onSticker(const qint32 _error, qint32 _setId, qint32 _stickerId);

    void onStickers();

public:
    const QImage& getStickerImage() const;

private:
    void connectStickerSignal(const bool _isConnected);

    void loadSticker();

    void renderSelected(QPainter& _p);

    void requestSticker();

    void updateStickerSize();

    void initPlaceholder();

    void updateStyle() override;
    void updateFonts() override;

    const HistoryControl::StickerInfoSptr Info_;

    QImage Sticker_;

    QPixmap Placeholder_;

    StickerBlockLayout *Layout_;

    ComplexMessageItem* Parent_;

    QSize LastSize_;

    QRect Geometry_;

    std::vector<QMetaObject::Connection> connections_;
};

UI_COMPLEX_MESSAGE_NS_END

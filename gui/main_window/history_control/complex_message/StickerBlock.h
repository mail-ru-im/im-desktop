#pragma once

#include "GenericBlock.h"
#include "StickerBlockLayout.h"
#include "../StickerInfo.h"
#include "utils/SvgUtils.h"

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
    StickerBlock(ComplexMessageItem* _parent, const HistoryControl::StickerInfoSptr& _info);

    virtual ~StickerBlock() override;

    IItemBlockLayout* getBlockLayout() const override;

    Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool isAllSelected() const override { return isSelected(); }

    QRect setBlockGeometry(const QRect &ltr) override;

    Data::FString getSourceText() const override;

    QString formatRecentsText() const override;

    Ui::MediaType getMediaType() const override;

    bool isBubbleRequired() const override { return false; }

    bool isSharingEnabled() const override { return false; }

    int desiredWidth(int _width = 0) const override;

    PinPlaceholderType getPinPlaceholder() const override { return PinPlaceholderType::Sticker; }

    void requestPinPreview() override;

    bool clicked(const QPoint& _p) override;

    ContentType getContentType() const override { return IItemBlock::ContentType::Sticker; }

    Data::StickerId getStickerId() const override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;


private Q_SLOTS:
    void onSticker(const qint32 _error, const Utils::FileSharingId&, qint32 _setId, qint32 _stickerId);

    void onStickers();

public:
    const QImage& getStickerImage() const;

private:
    void connectStickerSignal(const bool _isConnected);

    void loadSticker();

    void requestSticker();

    void updateStickerSize();

    void initPlaceholder();

    void updateFonts() override {};

    const HistoryControl::StickerInfoSptr Info_;

    QImage Sticker_;

    Utils::StyledPixmap Placeholder_;

    StickerBlockLayout* Layout_;

    QSize LastSize_;

    QRect Geometry_;

    std::vector<QMetaObject::Connection> connections_;
};

UI_COMPLEX_MESSAGE_NS_END

#pragma once

#include <QObject>
#include <vector>
#include <array>

namespace Utils
{
class QObjectWatcher;
}

namespace Ui
{
    namespace ComplexMessage
    {
        class ComplexMessageItem;

        class FileSharingBlock;
        class ImagePreviewBlock;
        class LinkPreviewBlock;
        class PttBlock;
        class TextBlock;
        class DebugTextBlock;
        class QuoteBlock;
        class StickerBlock;
        class IItemBlock;
    }
}


class MessageItemMemMonitor: public QObject
{
public:
    using Category = std::pair<qint64 /*count*/, qint64 /*size*/>;
    using CategoriesArray = std::array<Category, 7>;

public:
    static MessageItemMemMonitor& instance();

    bool watchComplexMsgItem(Ui::ComplexMessage::ComplexMessageItem *_msgItem);
    CategoriesArray getMessageItemsFootprint();

private:
    MessageItemMemMonitor(QObject *parent);
    Utils::QObjectWatcher *messageItemsWatcher();

    Ui::ComplexMessage::TextBlock* tryGetTextBlock(Ui::ComplexMessage::IItemBlock* _itemBlock);
    Ui::ComplexMessage::ImagePreviewBlock* tryGetImagePreviewBlock(Ui::ComplexMessage::IItemBlock* _itemBlock);
    Ui::ComplexMessage::LinkPreviewBlock* tryGetLinkPreviewBlock(Ui::ComplexMessage::IItemBlock* _itemBlock);
    Ui::ComplexMessage::StickerBlock* tryGetStickerBlock(Ui::ComplexMessage::IItemBlock* _stickerBlock);
    Ui::ComplexMessage::FileSharingBlock* tryGetFileSharingBlock(Ui::ComplexMessage::IItemBlock* _fileSharingBlock);
    Ui::ComplexMessage::QuoteBlock* tryGetQuoteBlock(Ui::ComplexMessage::IItemBlock* _quoteBlock);

private:
    Utils::QObjectWatcher *messageItemsWatcher_ = nullptr;
};

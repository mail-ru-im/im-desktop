#pragma once

#include "../../../common.shared/message_processing/message_tokenizer.h"

#include "../history/Message.h"

namespace Ui
{
    namespace ComplexMessage
    {
        struct TextChunk
        {
            enum class Type
            {
                Invalid,

                Min,

                Undefined,
                Text,
                GenericLink,
                ImageLink,
                Junk,
                FileSharingImage,
                FileSharingImageSticker,
                FileSharingGif,
                FileSharingGifSticker,
                FileSharingVideo,
                FileSharingVideoSticker,
                FileSharingPtt,
                FileSharingGeneral,
                FileSharingUpload,
                Sticker,
                ProfileLink,

                Max
            };

            static const TextChunk Empty;

            TextChunk();
            TextChunk(const Type type, QString _text, QString imageType, const int32_t durationSec);

            int32_t length() const;

            TextChunk mergeWith(const TextChunk &chunk) const;

            Type Type_;

            QString text_;

            QString ImageType_;

            int32_t DurationSec_;

            HistoryControl::StickerInfoSptr Sticker_;

            HistoryControl::FileSharingInfoSptr FsInfo_;
        };

        using FixedUrls = std::vector<common::tools::url_parser::compare_item>;

        class ChunkIterator final
        {
        public:
            explicit ChunkIterator(const QString& _text);
            ChunkIterator(const QString& _text, FixedUrls&& _urls);

            bool hasNext() const;
            TextChunk current(bool _allowSnippet = true, bool _forcePreview = false) const;
            void next();

        private:
            common::tools::message_tokenizer tokenizer_;
        };
    }
}

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
                FormattedText,
                GenericLink,
                ImageLink,
                Junk,
                FileSharingImage,
                FileSharingImageSticker,
                FileSharingGif,
                FileSharingGifSticker,
                FileSharingVideo,
                FileSharingVideoSticker,
                FileSharingLottieSticker,
                FileSharingPtt,
                FileSharingGeneral,
                FileSharingUpload,
                Sticker,
                ProfileLink,

                Max
            };

            static const TextChunk Empty;

            TextChunk();
            TextChunk(Type _type, QString _text, QString _imageType = {}, int32_t _durationSec = -1);
            TextChunk(Data::FStringView _view, Type _type = Type::FormattedText, QString _imageType = {});

            int32_t length() const;

            TextChunk mergeWith(const TextChunk& chunk) const;


            Data::FString getFText() const { return formattedText_.isEmpty() ? text_ : formattedText_.toFString(); }

            QStringView getPlainText() const { return formattedText_.isEmpty() ? text_ : formattedText_.string(); }

            Data::FStringView getFView() const { return formattedText_; }

            Type Type_;

            Data::FStringView formattedText_;

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
            ChunkIterator(const Data::FString& _text, FixedUrls&& _urls);

            bool hasNext() const;
            TextChunk current(bool _allowSnippet = true, bool _forcePreview = false) const;
            void next();

        private:
            common::tools::message_tokenizer tokenizer_;
            Data::FStringView formattedText_;
        };
    }
}

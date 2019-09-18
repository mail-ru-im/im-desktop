#pragma once

namespace Ui
{
    enum class MediaType
    {
        noMedia = 0,
        mediaTypeSticker = 1,
        mediaTypeVideo = 2,
        mediaTypePhoto = 3,
        mediaTypeGif = 4,
        mediaTypePtt = 5,
        mediaTypeVoip = 6,
        mediaTypeFileSharing = 7,
        mediaTypeContact = 8,
    };


    struct MediaWithText
    {
        QString text_;

        MediaType mediaType_ = MediaType::noMedia;
    };
}

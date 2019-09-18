#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(QWidget *_parent,
        const int64_t _id,
        const QString& _internalId,
        const QDate _date,
        const int64_t _prev,
        const QString& _text,
        const QString& _chatAimid,
        const QString& _senderAimid,
        const QString& _senderFriendly,
        const Data::QuotesVec& _quotes,
        const Data::MentionMap& _mentions,
        const HistoryControl::StickerInfoSptr& _sticker,
        const HistoryControl::FileSharingInfoSptr& _filesharing,
        const bool _isOutgoing,
        const bool _isNotAuth,
        const bool _forcePreview,
        const QString& _description,
        const QString& _url,
        const Data::SharedContact& _sharedContact);

}

UI_COMPLEX_MESSAGE_NS_END

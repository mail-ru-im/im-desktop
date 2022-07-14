#pragma once
#include "../types/message.h"

namespace Ui
{
    class MessagesScrollArea;
}

namespace Data
{
    class FString;
}

namespace core
{
    namespace data
    {
        class format;
    }
} // namespace core

namespace MimeData
{
    QMimeData* toMimeData(Data::FString&& _text, const Data::MentionMap& _mentions, const Data::FilesPlaceholderMap& _files);

    QString getMentionMimeType();
    QString getRawMimeType();
    QString getFileMimeType();
    QString getTextFormatMimeType();
    QString getMimeTextFormat();
    QString getMimeHtmlFormat();

    [[nodiscard]] QByteArray convertMapToArray(const Data::MentionMap& _map);
    [[nodiscard]] Data::MentionMap convertArrayToMap(const QByteArray& _array);

    bool copyMimeData(const Ui::MessagesScrollArea& _area);
    [[nodiscard]] QByteArray serializeTextFormatAsJson(const core::data::format& _format);

    const std::vector<QStringView>& getFilesPlaceholderList();

    bool isMimeDataWithImageDataURI(const QMimeData* _mimeData);
    bool isMimeDataWithImage(const QMimeData* _mimeData);
    QImage getImageFromMimeData(const QMimeData* _mimeData);

    void sendToChat(const QMimeData* _mimeData, const QString& _aimId);
} // namespace MimeData

#include "stdafx.h"

#include <rapidjson/writer.h>

#include "MimeDataUtils.h"
#include "utils.h"
#include "features.h"
#include "InterConnector.h"
#include "stat_utils.h"
#include "async/AsyncTask.h"
#include "../main_window/history_control/MessagesScrollArea.h"
#include "../main_window/containers/InputStateContainer.h"
#include "../previewer/toast.h"
#include "../types/message.h"
#include "../core_dispatcher.h"

namespace MimeData
{
    QByteArray convertMapToArray(const Data::MentionMap& _map)
    {
        QByteArray res;
        if (!_map.empty())
        {
            res.reserve(512);
            for (const auto& [key, value] : _map)
            {
                res += key.toUtf8();
                res += ": ";
                res += value.toUtf8();
                res += '\0';
            }
            res.chop(1);
        }

        return res;
    }

    Data::MentionMap convertArrayToMap(const QByteArray& _array)
    {
        Data::MentionMap res;

        if (!_array.isEmpty())
        {
            if (const auto items = _array.split('\0'); !items.isEmpty())
            {
                for (const auto& item : items)
                {
                    const auto ndx = item.indexOf(": ");
                    if (ndx != -1)
                    {
                        auto key = QString::fromUtf8(item.mid(0, ndx));
                        auto value = QString::fromUtf8(item.mid(ndx + 1).trimmed());

                        res.emplace(std::move(key), std::move(value));
                    }
                }
            }
        }

        return res;
    }

    bool copyMimeData(const Ui::MessagesScrollArea& _area)
    {
        if (auto text = _area.getSelectedText(Ui::MessagesScrollArea::TextFormat::Raw); !text.isEmpty())
        {
            const auto placeholders = _area.getFilesPlaceholders();
            const auto mentions = _area.getMentions();

            QApplication::clipboard()->setMimeData(toMimeData(std::move(text), mentions, placeholders));
            Utils::showCopiedToast();
            return true;
        }
        return false;
    }

    QByteArray serializeTextFormatAsJson(const core::data::format& _format)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        auto value = _format.serialize(a);
        doc.AddMember("format", value, a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return QByteArray(buffer.GetString(), buffer.GetSize());
    }

    const std::vector<QStringView>& getFilesPlaceholderList()
    {
        static const std::vector<QStringView> placeholders =
        {
            u"[GIF: ",
            u"[Video: ",
            u"[Photo: ",
            u"[Voice: ",
            u"[Sticker: ",
            u"[Audio: ",
            u"[Word: ",
            u"[Excel: ",
            u"[Presentation: ",
            u"[APK: ",
            u"[PDF: ",
            u"[Numbers: ",
            u"[Pages: ",
            u"[File: "
        };
        return placeholders;
    }

    QMimeData* toMimeData(Data::FString&& _text, const Data::MentionMap& _mentions, const Data::FilesPlaceholderMap& _files)
    {
        auto dt = new QMimeData();

        {
            auto plainText = Utils::convertMentions(_text.string(), _mentions);
            plainText = Utils::convertFilesPlaceholders(plainText, _files);
            dt->setText(plainText);
        }

        dt->setData(getRawMimeType(), _text.string().toUtf8());
        if (_text.hasFormatting())
            dt->setData(getTextFormatMimeType(), MimeData::serializeTextFormatAsJson(_text.formatting()));

        if (!_mentions.empty())
            dt->setData(getMentionMimeType(), MimeData::convertMapToArray(_mentions));

        if (!_files.empty())
            dt->setData(MimeData::getFileMimeType(), MimeData::convertMapToArray(_files));

        return dt;
    }

    QString getMentionMimeType() { return qsl("icq/mentions"); }

    QString getRawMimeType() { return qsl("icq/rawText"); }

    QString getFileMimeType() { return qsl("icq/files"); }

    QString getTextFormatMimeType() { return qsl("icq/textFormat"); }

    QString getMimeTextFormat() { return qsl("text/plain"); }

    QString getMimeHtmlFormat() { return qsl("text/html"); }

    bool isMimeDataWithImageDataURI(const QMimeData* _mimeData) { return _mimeData->text().startsWith(u"data:image/"); }

    bool isMimeDataWithImage(const QMimeData* _mimeData) { return _mimeData->hasImage() || isMimeDataWithImageDataURI(_mimeData); }

    QImage getImageFromMimeData(const QMimeData* _mimeData)
    {
        if (_mimeData->hasImage())
            return qvariant_cast<QImage>(_mimeData->imageData());

        if (isMimeDataWithImageDataURI(_mimeData))
        {
            auto plainText = _mimeData->text();
            plainText.remove(0, plainText.indexOf(u',') + 1);
            const auto imageData = QByteArray::fromBase64(std::move(plainText).toUtf8());
            return QImage::fromData(imageData);
        }

        return QImage{};
    }

    void sendToChat(const QMimeData* _mimeData, const QString& _aimId)
    {
        const auto& quotes = Logic::InputStateContainer::instance().getQuotes(_aimId);
        bool mayQuotesSent = false;

        const bool isWebpScreenshot = Features::isWebpScreenshotEnabled();

        auto sendAsScreenshot = [isWebpScreenshot, _aimId](const Ui::FileToSend& _f, const Data::QuotesVec& _quotes)
        {
            Async::runAsync([_f, aimId = _aimId, quotes = _quotes, isWebpScreenshot]() mutable
                {
                    QByteArray array = Ui::FileToSend::loadImageData(_f, isWebpScreenshot ? Ui::FileToSend::Format::webp : Ui::FileToSend::Format::png);
                    if (array.isEmpty())
                        return;

                    Async::runInMain([array = std::move(array), aimId = std::move(aimId), quotes = std::move(quotes), isWebpScreenshot]()
                    {
                        Ui::GetDispatcher()->uploadSharedFile(aimId, array, isWebpScreenshot ? u".webp" : u".png", quotes);
                    });
                });
        };

        if (MimeData::isMimeDataWithImage(_mimeData))
        {
            sendAsScreenshot(Ui::FileToSend(QPixmap::fromImage(MimeData::getImageFromMimeData(_mimeData))), quotes);

            mayQuotesSent = true;
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, { { "chat_type", Utils::chatTypeByAimId(_aimId) }, { "count", "single" }, { "type", "dndrecents"} });
            Q_EMIT Utils::InterConnector::instance().messageSent(_aimId);
        }
        else
        {
            auto count = 0;
            auto sendQuotesOnce = true;
            const auto urls = _mimeData->urls();
            bool alreadySentWebp = false;
            for (const QUrl& url : urls)
            {
                if (url.isLocalFile())
                {
                    const auto fileName = url.toLocalFile();
                    const QFileInfo info(fileName);
                    const bool canDrop = (!(info.isBundle() || info.isDir())) && info.size() > 0;

                    if (canDrop)
                    {
                        if (const Ui::FileToSend f(info); !alreadySentWebp && isWebpScreenshot && f.canConvertToWebp())
                        {
                            sendAsScreenshot(f, sendQuotesOnce ? quotes : Data::QuotesVec());
                            alreadySentWebp = true;
                        }
                        else
                        {
                            Ui::GetDispatcher()->uploadSharedFile(_aimId, fileName, sendQuotesOnce ? quotes : Data::QuotesVec());
                        }
                        sendQuotesOnce = false;
                        mayQuotesSent = true;

                        Q_EMIT Utils::InterConnector::instance().messageSent(_aimId);

                        ++count;
                    }
                }
                else if (url.isValid())
                {
                    Ui::GetDispatcher()->sendMessageToContact(_aimId, url.toString(), sendQuotesOnce ? quotes : Data::QuotesVec());
                    sendQuotesOnce = false;
                    mayQuotesSent = true;

                    Q_EMIT Utils::InterConnector::instance().messageSent(_aimId);
                }
            }

            if (count > 0)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, { { "chat_type", Utils::chatTypeByAimId(_aimId) },{ "count", count > 1 ? "multi" : "single" },{ "type", "dndrecents" } });
        }

        if (mayQuotesSent && !quotes.isEmpty())
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
            Q_EMIT Utils::InterConnector::instance().clearInputQuotes(_aimId);
        }
    }

} // namespace MimeData

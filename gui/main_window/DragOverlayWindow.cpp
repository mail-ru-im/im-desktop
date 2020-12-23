#include "stdafx.h"

#include "DragOverlayWindow.h"
#include "ContactDialog.h"
#include "MainWindow.h"
#include "FilesWidget.h"

#include "history_control/HistoryControlPage.h"
#include "input_widget/InputWidget.h"
#include "contact_list/ContactListModel.h"
#include "utils/utils.h"
#include "utils/async/AsyncTask.h"
#include "utils/features.h"
#include "utils/stat_utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "controls/GeneralDialog.h"
#include "core_dispatcher.h"

namespace
{
    constexpr std::chrono::milliseconds dragActivateDelay() noexcept { return std::chrono::milliseconds(500); }

    auto dragOverlayPadding()
    {
        return Utils::scale_value(20);
    }

    auto dragOverlayRadius()
    {
        return Utils::scale_value(5);
    }

    auto dragOverlayOffset()
    {
        return Utils::scale_value(14);
    }

    int overlayModeFromMimeData(const QMimeData* _mimeData)
    {
        const bool oneExternalUrl = [_mimeData]()
        {
            if (_mimeData->hasUrls())
            {
                const auto urls = _mimeData->urls();
                return urls.count() == 1 && !urls.constFirst().isLocalFile();
            }
            return false;
        }();

        const bool mimeDataWithImage = Utils::isMimeDataWithImage(_mimeData);

        if (oneExternalUrl && !mimeDataWithImage)
            return Ui::DragOverlayWindow::QuickSend;
        else
            return Ui::DragOverlayWindow::QuickSend | Ui::DragOverlayWindow::SendWithCaption;
    }
}

namespace Ui
{
    DragOverlayWindow::DragOverlayWindow(ContactDialog* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
        , top_(false)
        , mode_(QuickSend)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAcceptDrops(true);

        dragMouseoverTimer_.setInterval(dragActivateDelay());
        dragMouseoverTimer_.setSingleShot(true);
        connect(&dragMouseoverTimer_, &QTimer::timeout, this, &DragOverlayWindow::onTimer);
    }

    void DragOverlayWindow::onTimer()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void DragOverlayWindow::setMode(const int _mode)
    {
        if (mode_ != _mode)
        {
            mode_ = _mode;
            update();
        }
    }

    void DragOverlayWindow::setModeByMimeData(const QMimeData* _mimeData)
    {
        setMode(overlayModeFromMimeData(_mimeData));
    }

    void DragOverlayWindow::paintEvent(QPaintEvent *)
    {
        QPainter painter(this);

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        static const QColor overlayColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.98);

        painter.fillRect(rect(), overlayColor);

        drawSendWithCaption(painter);
        drawQuickSend(painter);
    }

    void DragOverlayWindow::dragEnterEvent(QDragEnterEvent *_e)
    {
        dragMouseoverTimer_.start();

        _e->acceptProposedAction();
    }

    void DragOverlayWindow::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        dragMouseoverTimer_.stop();

        hide();
        _e->accept();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlayWindow::dragMoveEvent(QDragMoveEvent *_e)
    {
        _e->acceptProposedAction();
        if (mode_ == (QuickSend | SendWithCaption))
            top_ = _e->pos().y() < rect().height() / 2;
        else
            top_ = ((mode_ == QuickSend) ? false : true);
        update();
    }

    void DragOverlayWindow::dropEvent(QDropEvent *_e)
    {
        dragMouseoverTimer_.stop();
        onTimer();

        const QMimeData* mimeData = _e->mimeData();

        auto mimeDataWithImage = Utils::isMimeDataWithImage(mimeData);

        if (mimeData->hasUrls() || mimeDataWithImage)
        {
            const QList<QUrl> urlList = mimeData->urls();

            QString contact = Logic::getContactListModel()->selectedContact();
            FilesToSend files;

            if (mimeDataWithImage)
            {
                files.emplace_back(QPixmap::fromImage(Utils::getImageFromMimeData(mimeData)));
            }
            else
            {
                const auto& quotes = Utils::InterConnector::instance().getContactDialog()->getInputWidget()->getInputQuotes();
                auto sendQuotesOnce = true;
                for (const QUrl& url : urlList)
                {
                    if (url.isLocalFile())
                    {
                        QFileInfo info(url.toLocalFile());
                        const bool canDrop = !(info.isBundle() || info.isDir() || info.size() == 0);
                        if (canDrop)
                            files.emplace_back(std::move(info));
                    }
                    else if (url.isValid())
                    {
                        Ui::GetDispatcher()->sendMessageToContact(contact, url.toString(), sendQuotesOnce ? quotes : Data::QuotesVec());
                        Parent_->onSendMessage(contact);

                        if (sendQuotesOnce && !quotes.isEmpty())
                        {
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                            Q_EMIT Utils::InterConnector::instance().getContactDialog()->getInputWidget()->needClearQuotes();
                        }

                        sendQuotesOnce = false;
                    }
                }
            }

            if (auto page = Utils::InterConnector::instance().getHistoryPage(contact))
                page->hideSmartreplies();

            auto inputWidget = Utils::InterConnector::instance().getContactDialog()->getInputWidget();
            auto inputText = inputWidget->getInputText();
            auto inputMentions = inputWidget->getInputMentions();
            if (top_ && !inputText.isEmpty())
                inputWidget->setInputText(QString());

            QTimer::singleShot(100, this, [this,
                files = std::move(files),
                contact = std::move(contact),
                inputText = std::move(inputText),
                inputMentions = std::move(inputMentions)]()
            {
                const auto& quotes = Utils::InterConnector::instance().getContactDialog()->getInputWidget()->getInputQuotes();
                bool mayQuotesSent = false;

                const auto sendFiles = [this, &contact, &mayQuotesSent, &quotes](const FilesToSend& _files, const QString& desc, const Data::MentionMap& descMentions)
                {
                    if (_files.empty())
                        return;

                    auto descriptionInside = false;
                    if (_files.size() == 1)
                    {
                        if (_files.front().isFile())
                        {
                            const auto suffix = _files.front().getFileInfo().suffix();
                            descriptionInside = Utils::is_image_extension(suffix) || Utils::is_video_extension(suffix);
                        }
                        else
                        {
                            descriptionInside = true;
                        }
                    }

                    if (!desc.isEmpty() && !descriptionInside)
                    {
                        Ui::GetDispatcher()->sendMessageToContact(contact, desc, quotes, descMentions);
                        mayQuotesSent = true;
                    }

                    auto sendQuotesOnce = !mayQuotesSent;
                    bool alreadySentWebp = false;
                    for (const auto& f : _files)
                    {
                        if (f.getSize() == 0)
                            continue;

                        const auto isWebpScreenshot = Features::isWebpScreenshotEnabled() && f.canConvertToWebp();
                        if (f.isFile() && (alreadySentWebp || !isWebpScreenshot))
                        {
                            Ui::GetDispatcher()->uploadSharedFile(contact, f.getFileInfo().absoluteFilePath(), sendQuotesOnce ? quotes : Data::QuotesVec(), descriptionInside ? desc : QString(), descriptionInside ? descMentions : Data::MentionMap());
                        }
                        else
                        {
                            if (isWebpScreenshot)
                                alreadySentWebp = true;
                            Async::runAsync([f, contact, quotes, desc, descMentions, isWebpScreenshot]() mutable
                            {
                                auto array = FileToSend::loadImageData(f, isWebpScreenshot ? FileToSend::Format::webp : FileToSend::Format::png);
                                if (array.isEmpty())
                                    return;

                                Async::runInMain([array = std::move(array), contact = std::move(contact), quotes = std::move(quotes), desc = std::move(desc), descMentions = std::move(descMentions), isWebpScreenshot]()
                                {
                                    Ui::GetDispatcher()->uploadSharedFile(contact, array, isWebpScreenshot ? u".webp" : u".png", quotes, desc, descMentions);
                                });
                            });
                        }

                        Parent_->onSendMessage(contact);
                        if (sendQuotesOnce)
                        {
                            mayQuotesSent = true;
                            sendQuotesOnce = false;
                        }
                    }

                    const core::stats::event_props_type props = { { "chat_type", Utils::chatTypeByAimId(contact) }, { "count", _files.size() > 1 ? "multi" : "single" }, { "type", "dndchat" } };
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, props);
                    if (!desc.isEmpty())
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmediawcapt_action, props);
                };

                if (top_)
                {
                    auto filesWidget = new FilesWidget(this, files);

                    GeneralDialog::Options options;
                    options.preferredSize_.setWidth(filesWidget->width());

                    GeneralDialog generalDialog(filesWidget, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, options);
                    connect(filesWidget, &FilesWidget::setButtonActive, &generalDialog, &GeneralDialog::setButtonActive);
                    generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("files_widget", "Cancel"), QT_TRANSLATE_NOOP("files_widget", "Send"), true);

                    filesWidget->setDescription(inputText, inputMentions);
                    filesWidget->setFocusOnInput();

                    if (generalDialog.showInCenter())
                        sendFiles(filesWidget->getFiles(), filesWidget->getDescription(), filesWidget->getMentions());
                    else if (!inputText.isEmpty())
                        Utils::InterConnector::instance().getContactDialog()->getInputWidget()->setInputText(inputText);
                }
                else
                {
                    sendFiles(files, QString(), {});
                }

                if (mayQuotesSent && !quotes.isEmpty())
                {
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                    Q_EMIT Utils::InterConnector::instance().getContactDialog()->getInputWidget()->needClearQuotes();
                }
            });
        }

        _e->acceptProposedAction();
        hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlayWindow::drawQuickSend(QPainter& _p)
    {
        Utils::PainterSaver saver(_p);
        if (mode_ & QuickSend)
        {
            auto penColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
            auto penStyle = Qt::SolidLine;

            if (mode_ != QuickSend && top_)
            {
                penColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
                penStyle = Qt::CustomDashLine;
            }

            QPen pen(penColor, Utils::scale_value(1), penStyle, Qt::RoundCap);

            if (mode_ != QuickSend && top_)
                pen.setDashPattern({ 2, 4 });
            else
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT));

            int rectHeight = 0;
            int rectY = 0;
            int textY = 0;
            if (mode_ == QuickSend)
            {
                rectY = dragOverlayPadding();
                rectHeight = rect().height();
                textY = rectHeight / 2;
            }
            else
            {
                rectY = dragOverlayPadding() + rect().height() / 2 - dragOverlayOffset();
                rectHeight = rect().height() / 2 + dragOverlayOffset();
                textY = (rect().height() / 4) * 3;
            }

            _p.setPen(pen);
            _p.drawRoundedRect(dragOverlayPadding(),
                               rectY,
                               rect().width() - dragOverlayPadding() * 2,
                               rectHeight - dragOverlayPadding() * 2,
                               dragOverlayRadius(),
                               dragOverlayRadius());

            _p.setFont(Fonts::appFontScaled(24));

            Utils::drawText(_p,
                            QPointF(rect().width() / 2, textY - dragOverlayPadding() / 2),
                            Qt::AlignCenter,
                            QT_TRANSLATE_NOOP("chat_page", "Quick send"));
        }
    }

    void DragOverlayWindow::drawSendWithCaption(QPainter& _p)
    {
        Utils::PainterSaver saver(_p);
        if (mode_ & SendWithCaption)
        {
            auto penColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
            auto penStyle = Qt::SolidLine;

            if (mode_ != SendWithCaption && !top_)
            {
                penColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
                penStyle = Qt::CustomDashLine;
            }

            QPen pen(penColor, Utils::scale_value(1), penStyle, Qt::RoundCap);

            if (mode_ != SendWithCaption && !top_)
            {
                pen.setDashPattern({ 2, 4 });
            }
            else
            {
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT));
            }

            int rectHeight;
            if (mode_ == SendWithCaption)
                rectHeight = rect().height();
            else
                rectHeight = rect().height() / 2;

            _p.setPen(pen);
            _p.drawRoundedRect(dragOverlayPadding(),
                               dragOverlayPadding(),
                               rect().width() - dragOverlayPadding() * 2,
                               rectHeight - dragOverlayPadding() * 2 + dragOverlayOffset(),
                               dragOverlayRadius(),
                               dragOverlayRadius());

            _p.setFont(Fonts::appFontScaled(24));
            Utils::drawText(_p,
                            QPointF(rect().width() / 2, rectHeight / 2 + dragOverlayPadding() / 2),
                            Qt::AlignCenter,
                            QT_TRANSLATE_NOOP("files_widget", "Send with caption"));
        }
    }
}

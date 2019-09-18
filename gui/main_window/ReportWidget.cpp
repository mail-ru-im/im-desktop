#include "stdafx.h"
#include "ReportWidget.h"
#include "MainWindow.h"
#include "../controls/GeneralDialog.h"

#include "../controls/CheckboxList.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../fonts.h"
#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"
#include "../styles/ThemeParameters.h"

namespace
{
    QString getReasonText(const Ui::Reasons& _reason)
    {
        switch (_reason)
        {
        case Ui::Reasons::SPAM:
            return QT_TRANSLATE_NOOP("report_widget", "Spam");

        case Ui::Reasons::PORN:
            return QT_TRANSLATE_NOOP("report_widget", "Porn");

        case Ui::Reasons::VIOLATION:
            return QT_TRANSLATE_NOOP("report_widget", "Violation");

        case Ui::Reasons::OTHER:
        default:
            return QT_TRANSLATE_NOOP("report_widget", "Other");
        }
    }

    QString getActionText(const Ui::BlockActions& _action)
    {
        switch (_action)
        {
            case Ui::BlockActions::BLOCK_ONLY:
                return QT_TRANSLATE_NOOP("report_widget", "Block contact");

            case Ui::BlockActions::REPORT_SPAM:
            default:
                return QT_TRANSLATE_NOOP("report_widget", "Block contact and report");
        }
    }

    constexpr auto REASON_HEIGHT = 44;
    constexpr auto HOR_OFFSET = 16;
    constexpr auto VER_OFFSET = 28;
}

namespace Ui
{
    ReportWidget::ReportWidget(QWidget* _parent, const QString& _title)
        : QWidget(_parent)
    {
        reasons_ = new ComboBoxList(
            this,
            Fonts::appFontScaled(16),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
            Utils::scale_value(HOR_OFFSET),
            Utils::scale_value(REASON_HEIGHT));

        for (const auto reason : getAllReasons())
            reasons_->addItem(getReasonString(reason), getReasonText(reason));

        auto h = reasons_->height();
        if (!_title.isEmpty())
        {
            title_ = TextRendering::MakeTextUnit(_title, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            title_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            title_->evaluateDesiredSize();
            title_->setOffsets(Utils::scale_value(HOR_OFFSET), 0);
            h += title_->cachedSize().height();
        }

        reasons_->move(0, Utils::scale_value(VER_OFFSET) + (title_ ? title_->cachedSize().height() : 0));
        h += Utils::scale_value(VER_OFFSET * 2);
        setFixedHeight(h);
    }

    Reasons ReportWidget::getReason() const
    {
        const auto selected = reasons_->getSelectedItem();

        if (!selected.isEmpty())
        {
            for (const auto reason : getAllReasons())
            {
                if (getReasonString(reason) == selected)
                    return reason;
            }
        }

        return Reasons::SPAM;
    }

    void ReportWidget::paintEvent(QPaintEvent* _event)
    {
        if (!title_)
            return;

        QPainter p(this);
        title_->draw(p);
    }

    void ReportWidget::resizeEvent(QResizeEvent* _event)
    {
        reasons_->setFixedWidth(width());
        if (title_)
            title_->getHeight(width() - title_->horOffset() * 2);

        return QWidget::resizeEvent(_event);
    }

    BlockWidget::BlockWidget(QWidget* _parent, const QString& _title)
        : QWidget(_parent)
    {
        actions_ = new ComboBoxList(
            this,
            Fonts::appFontScaled(16),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
            Utils::scale_value(HOR_OFFSET),
            Utils::scale_value(REASON_HEIGHT));

        for (const auto action : getAllActions())
            actions_->addItem(getActionString(action), getActionText(action));

        auto h = actions_->height();
        if (!_title.isEmpty())
        {
            title_ = TextRendering::MakeTextUnit(_title, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            title_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            title_->evaluateDesiredSize();
            title_->setOffsets(Utils::scale_value(HOR_OFFSET), 0);
            h += title_->cachedSize().height();
        }

        actions_->move(0, Utils::scale_value(VER_OFFSET) + (title_ ? title_->cachedSize().height() : 0));
        h += Utils::scale_value(VER_OFFSET * 2);
        setFixedHeight(h);
    }

    BlockActions BlockWidget::getAction() const
    {
        const auto selected = actions_->getSelectedItem();

        if (!selected.isEmpty())
        {
            for (const auto action : getAllActions())
            {
                if (getActionString(action) == selected)
                    return action;
            }
        }

        return BlockActions::BLOCK_ONLY;
    }

    void BlockWidget::paintEvent(QPaintEvent* _event)
    {
        if (!title_)
            return;

        QPainter p(this);
        title_->draw(p);
    }

    void BlockWidget::resizeEvent(QResizeEvent* _event)
    {
        actions_->setFixedWidth(width());
        if (title_)
            title_->getHeight(width() - title_->horOffset() * 2);

        return QWidget::resizeEvent(_event);
    }

    QString getReasonString(const Reasons _reason)
    {
        switch (_reason)
        {
        case Reasons::SPAM:
            return qsl("spam");

        case Reasons::PORN:
            return qsl("porn");

        case Reasons::VIOLATION:
            return qsl("violation");

        default:
            return qsl("other");
        }
    }

    const std::vector<Reasons>& getAllReasons()
    {
        const static std::vector<Reasons> reasons = { Reasons::SPAM, Reasons::PORN, Reasons::VIOLATION, Reasons::OTHER };
        return reasons;
    }

    QString getActionString(const BlockActions _action)
    {
        switch (_action)
        {
        case BlockActions::BLOCK_ONLY:
            return qsl("block");

        case BlockActions::REPORT_SPAM:
            return qsl("report");

        default:
            return qsl("block");
        }
    }

    std::vector<BlockActions> getAllActions()
    {
        return { BlockActions::BLOCK_ONLY, BlockActions::REPORT_SPAM };
    }

    bool ReportStickerPack(qint32 _id, const QString& _title)
    {
        auto w = new ReportWidget(nullptr, _title);
        GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget","CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        auto result = generalDialog.showInCenter();
        if (result)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_int("id", _id);
            collection.set_value_as_qstring("reason", getReasonString(w->getReason()));
            Ui::GetDispatcher()->post_message_to_core("report/stickerpack", collection.get());
        }
        return result;
    }

    bool ReportSticker(const QString& _aimid, const QString& _chatId, const QString& _stickerId)
    {
        auto w = new ReportWidget(nullptr, QString());
        GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        auto result = generalDialog.showInCenter();
        if (result)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("id", _stickerId);
            collection.set_value_as_qstring("chatId", _chatId);
            collection.set_value_as_qstring("sn", _aimid);
            collection.set_value_as_qstring("reason", getReasonString(w->getReason()));
            Ui::GetDispatcher()->post_message_to_core("report/sticker", collection.get());
        }
        return result;
    }

    bool ReportContact(const QString& _aimId, const QString& _title)
    {
        auto w = new ReportWidget(nullptr, _title);
        GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        auto result = generalDialog.showInCenter();
        if (result)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("reason", getReasonString(w->getReason()));
            Ui::GetDispatcher()->post_message_to_core("report/contact", collection.get());
        }
        return result;
    }

    BlockAndReportResult BlockAndReport(const QString& _aimId, const QString& _title)
    {
        while (1)
        {
            {
                auto w = new BlockWidget(nullptr, _title);
                GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
                generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Block"));
                generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
                auto result = generalDialog.showInCenter();
                if (!result)
                    return BlockAndReportResult::CANCELED;

                if (w->getAction() == BlockActions::BLOCK_ONLY)
                    return BlockAndReportResult::BLOCK;
            }
            {
                auto r = new ReportWidget(nullptr, _title);
                GeneralDialog generalDialog(r, Utils::InterConnector::instance().getMainWindow());
                generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
                generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
                auto result = generalDialog.showInCenter();
                if (result)
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", _aimId);
                    collection.set_value_as_qstring("reason", getReasonString(r->getReason()));
                    Ui::GetDispatcher()->post_message_to_core("report/contact", collection.get());
                    return BlockAndReportResult::SPAM;
                }
            }
        }

        return BlockAndReportResult::CANCELED;
    }

    bool ReportMessage(const QString& _aimId, const QString& _chatId, const int64_t _msgId, const QString& _msgText)
    {
        auto w = new ReportWidget(nullptr, QString());
        GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "CANCEL"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        auto result = generalDialog.showInCenter();
        if (result)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_int64("id", _msgId);
            collection.set_value_as_qstring("text", _msgText);
            collection.set_value_as_qstring("chatId", _chatId);
            collection.set_value_as_qstring("sn", _aimId);
            collection.set_value_as_qstring("reason", getReasonString(w->getReason()));
            Ui::GetDispatcher()->post_message_to_core("report/message", collection.get());
        }
        return result;
    }

}

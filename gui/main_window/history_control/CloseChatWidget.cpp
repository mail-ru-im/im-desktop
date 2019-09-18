#include "stdafx.h"

#include "CloseChatWidget.h"
#include "../ReportWidget.h"
#include "../MainWindow.h"
#include "../contact_list/ContactListModel.h"
#include "../friendly/FriendlyContainer.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../controls/CheckboxList.h"

#include "../../controls/GeneralDialog.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    constexpr auto REASON_HEIGHT = 44;
    constexpr auto HOR_OFFSET = 16;
    constexpr auto VER_OFFSET = 28;

    const auto ignoreItem = qsl("ignore");
    const auto reportItem = qsl("report");
}

namespace Ui
{
    CloseChatWidget::CloseChatWidget(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , aimId_(_aimId)
    {
        options_ = new CheckboxList(
            this,
            Fonts::appFontScaled(16),
            Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY),
            Utils::scale_value(HOR_OFFSET),
            Utils::scale_value(REASON_HEIGHT));

        options_->addItem(ignoreItem, QT_TRANSLATE_NOOP("popup_window", "Block"));
        options_->addItem(reportItem, QT_TRANSLATE_NOOP("popup_window", "Report"));

        title_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(aimId_));
        title_->init(Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        title_->evaluateDesiredSize();
        title_->setOffsets(Utils::scale_value(HOR_OFFSET), 0);

        options_->move(0, Utils::scale_value(VER_OFFSET) + title_->cachedSize().height());

        const auto h = options_->height() + title_->cachedSize().height() + Utils::scale_value(VER_OFFSET * 2);
        setFixedHeight(h);
    }

    bool CloseChatWidget::isIgnoreSelected() const
    {
        const auto selected = options_->getSelectedItems();
        return std::any_of(selected.begin(), selected.end(), [](const auto& _item) { return _item == ignoreItem; });
    }

    bool CloseChatWidget::isReportSelected() const
    {
        const auto selected = options_->getSelectedItems();
        return std::any_of(selected.begin(), selected.end(), [](const auto& _item) { return _item == reportItem; });
    }

    void CloseChatWidget::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent(_event);
        if (!title_)
            return;

        QPainter p(this);
        title_->draw(p);
    }

    void CloseChatWidget::resizeEvent(QResizeEvent* _event)
    {
        options_->setFixedWidth(width());

        return QWidget::resizeEvent(_event);
    }

    bool ReportContactOnClose(const QString& _aimId, Out Reasons& _reason)
    {
        auto w = new ReportWidget(nullptr, Logic::GetFriendlyContainer()->getFriendly(_aimId));
        auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addLabel(QT_TRANSLATE_NOOP("report_widget", "Report"));
        generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "BACK"), QT_TRANSLATE_NOOP("popup_window", "OK"), true);
        auto result = generalDialog->showInCenter();
        if (result)
            _reason = w->getReason();
        return result;
    }

    void CloseChat(const QString& _aimId)
    {
        auto w = new CloseChatWidget(nullptr, _aimId);
        auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addLabel(QT_TRANSLATE_NOOP("auth_widget", "Close chat"));
        generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "OK"), true);

        Reasons reportReason = Reasons::SPAM;
        while (true)
        {
            generalDialog->setFocus();
            const auto res = generalDialog->showInCenter();
            if (!res)
                return;

            if (w->isReportSelected())
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_unknown_report);

                if (!ReportContactOnClose(_aimId, reportReason))
                    continue;
            }

            break;
        }

        if (w->isIgnoreSelected())
            Logic::getContactListModel()->ignoreContact(_aimId, true);

        if (w->isReportSelected())
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("reason", getReasonString(reportReason));
            collection.set_value_as_bool("ignoreAndClose", false);
            GetDispatcher()->post_message_to_core("report/contact", collection.get());
        }

        Logic::getContactListModel()->removeContactFromCL(_aimId);
    }
}
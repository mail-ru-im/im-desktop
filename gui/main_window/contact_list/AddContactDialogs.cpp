#include "AddContactDialogs.h"

#include "AddNewContactStackedWidget.h"
#include "utils/InterConnector.h"
#include "controls/GeneralDialog.h"
#include "main_window/MainWindow.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "../common.shared/config/config.h"

static bool ShowingAddContactsDialog = false;

namespace
{
const std::string StatFromParam = "from";
const std::string StatContactsTab = "contacts";
const std::string StatChatsTab = "chats";
}

namespace Utils
{

void sendStatsFor(const AddContactDialogs::Initiator& _initiator);

void showAddContactsDialog(const QString& _name, const QString& _phone, AddContactCallback _callback, const AddContactDialogs::Initiator& _initiator)
{
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::addcontactbutton_click);
    sendStatsFor(_initiator);

    if (ShowingAddContactsDialog)
        return;

    ShowingAddContactsDialog = true;

    auto w = new Ui::AddNewContactStackedWidget(nullptr);

    Ui::GeneralDialog::Options options;
    options.preferredWidth_ = Utils::scale_value(380);
    options.ignoredInfos_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter,
                                       Utils::CloseWindowInfo::Reason::MacEventFilter);

    auto gd = std::make_unique<Ui::GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow(), options);
    gd->setIgnoredKeys({
                           Qt::Key_Return,
                           Qt::Key_Enter
                       });

    QObject::connect(w, &Ui::AddNewContactStackedWidget::finished, gd.get(), [&gd]()
    {
        gd->rejectDialog(Utils::CloseWindowInfo());
    });

    auto okCancelBtns = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                                           QT_TRANSLATE_NOOP("popup_window", "Add"),
                                           Ui::ButtonsStateFlag::AcceptingForbidden | Ui::ButtonsStateFlag::RejectionForbidden);
    w->setOkCancelButtons(okCancelBtns);

    if (!_name.isEmpty())
        w->setName(_name);

    if (!_phone.isEmpty())
        w->setPhone(_phone);

    gd->execute();

    if (w->getResult() == Ui::AddContactResult::Added && _callback)
        _callback();

    ShowingAddContactsDialog = false;
}

void showAddContactsDialog(const AddContactDialogs::Initiator &_initiator)
{
    auto w = new Ui::MultipleOptionsWidget(nullptr,
        {{qsl(":/add_by_phone"), QT_TRANSLATE_NOOP("add_widget", "Phone number")},
         {qsl(":/add_by_nick"), config::get().is_on(config::features::has_nicknames) ? QT_TRANSLATE_NOOP("add_widget", "Nickname") : QT_TRANSLATE_NOOP("add_widget", "Email")}});
    Ui::GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
    generalDialog.addLabel(config::get().is_on(config::features::has_nicknames)
                           ? QT_TRANSLATE_NOOP("add_widget", "Add contact by phone or nickname?")
                           : QT_TRANSLATE_NOOP("add_widget", "Add contact by phone or email?"));

    generalDialog.addCancelButton(QT_TRANSLATE_NOOP("report_widget", "Cancel"), true);
    if (generalDialog.execute())
    {
        if (w->selectedIndex() == 0)
            showAddContactsDialog(QString(), QString(), AddContactCallback(), _initiator);
        else
            Q_EMIT Utils::InterConnector::instance().addByNick();
    }
}

void sendStatsFor(const AddContactDialogs::Initiator& _initiator)
{
    core::stats::stats_event_names evName;
    core::stats::event_props_type props;

    switch (_initiator.from_)
    {
        case AddContactDialogs::Initiator::From::ConlistScr:
        {
            evName = core::stats::stats_event_names::conlistscr_addcon_action;
        }
        break;
        case AddContactDialogs::Initiator::From::ScrChatsTab:
        {
            evName = core::stats::stats_event_names::scr_addcon_action;
            props = { { StatFromParam, StatChatsTab } };
        }
        break;
        case AddContactDialogs::Initiator::From::ScrContactTab:
        {
            evName = core::stats::stats_event_names::scr_addcon_action;
            props = { { StatFromParam, StatContactsTab } };
        }
        break;

        default:
            return;
    }

    Ui::GetDispatcher()->post_stats_to_core(evName, props);
}

}

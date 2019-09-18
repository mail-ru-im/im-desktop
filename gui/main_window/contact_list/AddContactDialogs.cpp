#include "AddContactDialogs.h"

#include "AddNewContactStackedWidget.h"
#include "utils/InterConnector.h"
#include "controls/GeneralDialog.h"
#include "main_window/MainWindow.h"
#include "core_dispatcher.h"
#include "utils/utils.h"

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
    options.preferredSize_ = QSize(Utils::scale_value(380), -1);
    options.ignoreRejectDlgPairs_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter,
                                               Utils::CloseWindowInfo::Reason::MacEventFilter);

    auto gd = std::make_unique<Ui::GeneralDialog>(w,
                                                  Utils::InterConnector::instance().getMainWindow(),
                                                  false,
                                                  true,
                                                  true,
                                                  true,
                                                  options);
    gd->setIgnoredKeys({
                           Qt::Key_Return,
                           Qt::Key_Enter
                       });

    QObject::connect(w, &Ui::AddNewContactStackedWidget::finished,
            gd.get(), [&gd]()
    {
        gd->rejectDialog(Utils::CloseWindowInfo());
    });

    auto okCancelBtns = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                                           QT_TRANSLATE_NOOP("popup_window", "ADD"),
                                           true,   /* isActive */
                                           false,  /* rejectable - handle it in the widget */
                                           false); /* _acceptable - handle it in the widget */
    w->setOkCancelButtons(okCancelBtns);

    if (!_name.isEmpty())
        w->setName(_name);

    if (!_phone.isEmpty())
        w->setPhone(_phone);

    gd->showInCenter();

    if (w->getResult() == Ui::AddContactResult::Added && _callback)
        _callback();

    ShowingAddContactsDialog = false;
}

void showAddContactsDialog(const AddContactDialogs::Initiator &_initiator)
{
    showAddContactsDialog(QString(), QString(), AddContactCallback(),  _initiator);
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

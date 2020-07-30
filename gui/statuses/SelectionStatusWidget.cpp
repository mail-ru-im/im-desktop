#include "stdafx.h"
#include "SelectionStatusWidget.h"

#include "../main_window/contact_list/ContactListUtils.h"
#include "../main_window/contact_list/StatusListModel.h"
#include "../main_window/contact_list/StatusSearchModel.h"
#include "../styles/ThemeParameters.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"
#include "../fonts.h"

namespace Ui
{
    SelectionStatusWidget::SelectionStatusWidget(
        const QString& _labelText,
        QWidget* _parent)
        : SelectContactsWidget(
            nullptr,
            Logic::MembersWidgetRegim::STATUS_LIST,
            _labelText,
            QString(),
            _parent,
            true)
        , updateTimer_(new QTimer(this))
    {
        menu_ = new FlatMenu(this, BorderStyle::SHADOW);

        menu_->setFixedWidth(Utils::scale_value(201));

        auto* pLabel = new QLabel(QT_TRANSLATE_NOOP("status", "Status timing"));
        pLabel->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
        Utils::ApplyStyle(pLabel,
                          qsl("background: transparent; height: 52dip; padding-right: 64dip; padding-top: 20dip; padding-left: 16dip; padding-bottom: 20dip; color: %1;")
                                  .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));
        auto* separator = new QWidgetAction(nullptr);
        separator->setDefaultWidget(pLabel);
        menu_->addAction(separator);

        static const auto& times = Statuses::getTimeList();
        for (const auto& v : times)
            menu_->addAction(Statuses::getTimeString(v, Statuses::TimeMode::AlwaysOn));

        connect(menu_, &QMenu::triggered, this, [this](QAction* a)
        {
            int ix = -1;
            static const auto& times = Statuses::getTimeList();
            const QList<QAction*> allActions = menu_->actions();
            for (QAction* action : allActions)
            {
                ++ix;
                if (a == action && ix > 0)
                {
                    Logic::getStatusModel()->setTime(curStatus_, times[ix - 1]);
                    Logic::getStatusSearchModel()->updateTime(curStatus_);
                    if (Logic::getStatusModel()->isStatusSelected() && Logic::getStatusModel()->getSelected()->toString() == curStatus_)
                        Q_EMIT resendStatus();
                    update();
                }
            }
        });

        updateTimer_->setInterval(1000);
        connect(updateTimer_, &QTimer::timeout, this, &SelectionStatusWidget::updateModel);
        connect(this, &SelectContactsWidget::moreClicked, this, &SelectionStatusWidget::showMoreMenu);
        updateModel();
    }

    void SelectionStatusWidget::updateModel()
    {
        SelectContactsWidget::UpdateContactList();
        updateTimer_->start();
    }

    void SelectionStatusWidget::showMoreMenu(const QString &_status)
    {
        if (!menu_ || Logic::getStatusModel()->getStatus(_status)->isEmpty())
            return;

        assert(!_status.isEmpty());
        curStatus_ = _status;
        menu_->show();
        auto pos = QCursor::pos();
        pos.rx() -= menu_->width();
        if (const auto mw = Utils::InterConnector::instance().getMainWindow(); pos.y() + menu_->height() > mw->geometry().bottom())
            pos.ry() -= menu_->height();
        menu_->move(pos);
    }
}

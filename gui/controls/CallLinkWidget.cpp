#include "stdafx.h"
#include "CallLinkWidget.h"
#include "GeneralDialog.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../main_window/sidebar/SidebarUtils.h"
#include "../main_window/MainWindow.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "main_window/contact_list/ContactListModel.h"
#include "../styles/ThemeParameters.h"
#include "../previewer/toast.h"

namespace
{
    QString getCaption(Ui::ConferenceType _type)
    {
        return _type == Ui::ConferenceType::Call
                ? QT_TRANSLATE_NOOP("call_link", "Link on call")
                : QT_TRANSLATE_NOOP("call_link", "Link on webinar");
    }

    QString getDescription(Ui::ConferenceType _type, Utils::CallLinkFrom _from, QString _aimId)
    {
        QString desc;
        if (_from == Utils::CallLinkFrom::CallLog)
        {
            desc = (_type == Ui::ConferenceType::Call)
                   ? QT_TRANSLATE_NOOP("call_link", "Send the link to people you want to add to the call")
                   : QT_TRANSLATE_NOOP("call_link", "Send the link to people you want to invite to the webinar");
        }
        else if (_from == Utils::CallLinkFrom::Chat || _from == Utils::CallLinkFrom::Input)
        {
            const auto type = (_type == Ui::ConferenceType::Call)
                              ? QT_TRANSLATE_NOOP("call_link", "call")
                              : QT_TRANSLATE_NOOP("call_link", "webinar");
            desc = QT_TRANSLATE_NOOP("call_link", "Send the %1 link to current chat or copy it").arg(type);
        }
        else
        {
            const auto name = Logic::GetFriendlyContainer()->getFriendly(_aimId);
            desc = QT_TRANSLATE_NOOP("call_link", "Send the call link %1 to current chat or copy it").arg(name);
        }
        return desc;
    }
}

using namespace Ui;

CallLinkWidget::CallLinkWidget(QWidget* _parent, Utils::CallLinkFrom _from, const QString& _url, ConferenceType _type, const QString& _aimId)
    : QDialog(_parent)
    , url_(_url)
{
    auto mainWidget_ = new QWidget(this);
    auto mainLayout = Utils::emptyVLayout(mainWidget_);
    mainLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(8), QSizePolicy::Expanding, QSizePolicy::Fixed));
    linkLabel_ = addText(url_, mainWidget_, mainLayout, 0, 1);
    linkLabel_->setMargins(Utils::scale_value(20), Utils::scale_value(16), Utils::scale_value(20), Utils::scale_value(16));
    linkLabel_->showButtons();
    linkLabel_->allowOnlyCopy();
    linkLabel_->setText(url_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
    linkLabel_->setBackgroundColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.05));
    linkLabel_->setIconColors(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY),
                        Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER),
                        Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
    Testing::setAccessibleName(linkLabel_, qsl("AS CallLinkWidget linkLabel"));

    connect(linkLabel_, &TextLabel::textClicked, this, &CallLinkWidget::copyLink);
    connect(linkLabel_, &TextLabel::copyClicked, this, &CallLinkWidget::copyLink);
    mainLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(8), QSizePolicy::Expanding, QSizePolicy::Fixed));

    GeneralDialog::Options opt;
    opt.threadBadge_ = Logic::getContactListModel()->isThread(_aimId);
    mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_, Utils::InterConnector::instance().getMainWindow(), opt);
    mainDialog_->addLabel(getCaption(_type), Qt::AlignVCenter | Qt::AlignLeft);
    mainDialog_->addText(getDescription(_type, _from, _aimId), Utils::scale_value(12), Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Send"), true);
    Testing::setAccessibleName(mainDialog_.get(), ql1s("AS CallLinkWidget %1").arg((_type == ConferenceType::Call) ? ql1s("call") : ql1s("webinar")));
}

CallLinkWidget::~CallLinkWidget() = default;

void CallLinkWidget::copyLink() const
{
    QApplication::clipboard()->setText(url_);
    Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("toast", "Link copied"), Utils::scale_value(20));
}

bool CallLinkWidget::show()
{
    return mainDialog_->showInCenter();
}
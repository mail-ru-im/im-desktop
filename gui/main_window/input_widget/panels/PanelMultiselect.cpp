#include "stdafx.h"

#include "PanelMultiselect.h"

#include "controls/CustomButton.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"

#include "../../contact_list/ContactListModel.h"

namespace
{
    int deleteButtonSize()
    {
        return Utils::scale_value(36);
    }

    int spacing()
    {
        return Utils::scale_value(8);
    }

    int minimum_width()
    {
        return Utils::scale_value(600);
    }

    int delete_icon_size()
    {
        return 24;
    }

    int icon_size()
    {
        return 20;
    }

    QString getDeleteIcon()
    {
        return qsl(":/input/delete");
    }

    QString getCopyIcon()
    {
        return qsl(":/context_menu/copy");
    }

    QString getForwardIcon()
    {
        return qsl(":/context_menu/forward");
    }

    QString getReplyIcon()
    {
        return qsl(":/context_menu/reply");
    }
}

namespace Ui
{
    InputPanelMultiselect::InputPanelMultiselect(QWidget* _parent)
        : QWidget(_parent)
        , delete_(new RoundButton(this))
        , copy_(new RoundButton(this))
        , reply_(new RoundButton(this))
        , forward_(new RoundButton(this))
    {
        setFixedHeight(getDefaultInputHeight());
        delete_->setFixedWidth(deleteButtonSize());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->setSpacing(spacing());
        rootLayout->addSpacing(getHorMargin());
        rootLayout->addWidget(delete_);
        rootLayout->addWidget(copy_);
        rootLayout->addWidget(reply_);
        rootLayout->addWidget(forward_);
        rootLayout->addSpacing(getHorMargin());

        connect(delete_, &RoundButton::clicked, &Utils::InterConnector::instance(), &Utils::InterConnector::multiselectDelete);
        connect(copy_, &RoundButton::clicked, &Utils::InterConnector::instance(), &Utils::InterConnector::multiselectCopy);
        connect(reply_, &RoundButton::clicked, &Utils::InterConnector::instance(), &Utils::InterConnector::multiselectReply);
        connect(forward_, &RoundButton::clicked, this, [this]() { emit Utils::InterConnector::instance().multiselectForward(aimid_); });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentElementChanged, this, &InputPanelMultiselect::multiSelectCurrentElementChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectedCount, this, &InputPanelMultiselect::selectedCount);

        updateStyle(InputStyleMode::Default);
        selectedCount(0);
    }

    void InputPanelMultiselect::updateElements()
    {
        const auto role = Logic::getContactListModel()->getYourRole(Logic::getContactListModel()->selectedContact());
        const auto notAMember = Logic::getContactListModel()->youAreNotAMember(Logic::getContactListModel()->selectedContact());
        const auto isDisabled = role == ql1s("notamember") || role == ql1s("readonly") || notAMember;
        reply_->setVisible(!isDisabled);
        delete_->setVisible(!notAMember);
    }

    void InputPanelMultiselect::setContact(const QString& _aimid)
    {
        aimid_ = _aimid;
    }

    void InputPanelMultiselect::updateStyleImpl(const InputStyleMode _mode)
    {
    }

    void InputPanelMultiselect::resizeEvent(QResizeEvent* _event)
    {
        copy_->setText(width() > minimum_width() ? QT_TRANSLATE_NOOP("context_menu", "Copy") : QString());
        reply_->setText(width() > minimum_width() ? QT_TRANSLATE_NOOP("context_menu", "Reply") : QString());
        forward_->setText(width() > minimum_width() ? QT_TRANSLATE_NOOP("context_menu", "Forward") : QString());
        return QWidget::resizeEvent(_event);
    }

    void InputPanelMultiselect::multiSelectCurrentElementChanged()
    {
        delete_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Delete);
        copy_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Copy);
        reply_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Reply);
        forward_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Forward);
    }

    void InputPanelMultiselect::selectedCount(int _count)
    {
        const auto bgNormal = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
        const auto bgHover = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
        const auto bgActive = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);

        const auto textNormal = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
        const auto textDisabled = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);

        const bool enabled = _count > 0;
        const auto cursor = enabled ? Qt::PointingHandCursor : Qt::ArrowCursor;

        for (auto btn : { delete_, copy_, reply_, forward_ })
        {
            btn->setCursor(cursor);
            btn->setEnabled(enabled);

            if (enabled)
            {
                btn->setColors(bgNormal, bgHover, bgActive);
                btn->setTextColor(textNormal);
            }
            else
            {
                btn->setColors(bgNormal, bgNormal, bgNormal);
                btn->setTextColor(textDisabled);
            }
        }

        if (enabled)
            delete_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));

        delete_->setIcon(getDeleteIcon(), delete_icon_size());
        copy_->setIcon(getCopyIcon(), icon_size());
        reply_->setIcon(getReplyIcon(), icon_size());
        forward_->setIcon(getForwardIcon(), icon_size());
    }
}

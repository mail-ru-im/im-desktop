#include "stdafx.h"

#include "PanelMultiselect.h"

#include "controls/CustomButton.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "main_window/contact_list/FavoritesUtils.h"

#include "../../contact_list/ContactListModel.h"

namespace
{
    int fixedButtonWidth() noexcept
    {
        return Utils::scale_value(36);
    }

    int spacing() noexcept
    {
        return Utils::scale_value(8);
    }

    int minimumPanelWidth() noexcept
    {
        return Utils::scale_value(600);
    }

    constexpr int deleteIconSize() noexcept
    {
        return 24;
    }

    constexpr int iconSize() noexcept
    {
        return 20;
    }

    constexpr QStringView getDeleteIcon() noexcept { return u":/input/delete"; }
    constexpr QStringView getFavoritesIcon() noexcept { return u":/context_menu/favorites"; }
    constexpr QStringView getCopyIcon() noexcept { return u":/context_menu/copy"; }
    constexpr QStringView getForwardIcon() noexcept { return u":/context_menu/forward"; }
    constexpr QStringView getReplyIcon() noexcept { return u":/context_menu/reply"; }
}

namespace Ui
{
    InputPanelMultiselect::InputPanelMultiselect(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , delete_(new RoundButton(this))
        , favorites_(new RoundButton(this))
        , copy_(new RoundButton(this))
        , reply_(new RoundButton(this))
        , forward_(new RoundButton(this))
        , aimid_(_aimId)
    {
        setFixedHeight(getDefaultInputHeight());
        delete_->setFixedWidth(fixedButtonWidth());
        favorites_->setFixedWidth(fixedButtonWidth());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->setSpacing(spacing());
        rootLayout->addSpacing(getHorMargin());

        rootLayout->addWidget(delete_);
        Testing::setAccessibleName(delete_, qsl("AS Multiselect Delete"));

        rootLayout->addWidget(favorites_);
        Testing::setAccessibleName(favorites_, qsl("AS Multiselect Favorites"));

        rootLayout->addWidget(copy_);
        Testing::setAccessibleName(copy_, qsl("AS Multiselect Copy"));

        rootLayout->addWidget(reply_);
        Testing::setAccessibleName(reply_, qsl("AS Multiselect Reply"));

        rootLayout->addWidget(forward_);
        Testing::setAccessibleName(forward_, qsl("AS Multiselect Forward"));

        rootLayout->addSpacing(getHorMargin());

        connect(delete_, &RoundButton::clicked, this, [this]() { Q_EMIT Utils::InterConnector::instance().multiselectDelete(aimid_); });
        connect(favorites_, &RoundButton::clicked, this, [this]() { Q_EMIT Utils::InterConnector::instance().multiselectFavorites(aimid_); });
        connect(copy_, &RoundButton::clicked, this, [this]() { Q_EMIT Utils::InterConnector::instance().multiselectCopy(aimid_); });
        connect(reply_, &RoundButton::clicked, this, [this]() { Q_EMIT Utils::InterConnector::instance().multiselectReply(aimid_); });
        connect(forward_, &RoundButton::clicked, this, [this]() { Q_EMIT Utils::InterConnector::instance().multiselectForward(aimid_); });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentElementChanged, this, &InputPanelMultiselect::multiSelectCurrentElementChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectedCount, this, &InputPanelMultiselect::selectedCount);

        updateStyle(InputStyleMode::Default);
        selectedCount({}, 0, 0, 0);
    }

    void InputPanelMultiselect::updateElements()
    {
        const auto role = Logic::getContactListModel()->getYourRole(aimid_);
        const auto notAMember = Logic::getContactListModel()->areYouNotAMember(aimid_);
        const auto isDisabled = role == u"notamember" || role == u"readonly" || notAMember;
        const auto isThread = Logic::getContactListModel()->isThread(aimid_);
        reply_->setVisible(!isDisabled);
        delete_->setVisible(!notAMember && !isThread);
        favorites_->setVisible(!Favorites::isFavorites(aimid_) && !isThread);
        forward_->setVisible(!isThread);
    }

    void InputPanelMultiselect::updateStyleImpl(const InputStyleMode _mode)
    {
    }

    void InputPanelMultiselect::resizeEvent(QResizeEvent* _event)
    {
        const auto isWide = width() > minimumPanelWidth();

        copy_->setText(isWide ? QT_TRANSLATE_NOOP("context_menu", "Copy") : QString());
        reply_->setText(isWide ? QT_TRANSLATE_NOOP("context_menu", "Reply") : QString());
        forward_->setText(isWide ? QT_TRANSLATE_NOOP("context_menu", "Forward") : QString());

        delete_->setTooltipText(QT_TRANSLATE_NOOP("context_menu", "Remove"));
        favorites_->setTooltipText(QT_TRANSLATE_NOOP("context_menu", "Add to favorites"));
        copy_->setTooltipText(isWide ? QString() : QT_TRANSLATE_NOOP("context_menu", "Copy"));
        reply_->setTooltipText(isWide ? QString() : QT_TRANSLATE_NOOP("context_menu", "Reply"));
        forward_->setTooltipText(isWide ? QString() : QT_TRANSLATE_NOOP("context_menu", "Forward"));

        QWidget::resizeEvent(_event);
    }

    void InputPanelMultiselect::multiSelectCurrentElementChanged()
    {
        delete_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Delete);
        favorites_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Favorites);
        copy_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Copy);
        reply_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Reply);
        forward_->forceHover(Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Forward);
    }

    void InputPanelMultiselect::selectedCount(const QString& _aimId, int _totalCount, int _unsupported, int _plainFiles)
    {
        if (aimid_ != _aimId)
            return;

        const auto bgNormal = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
        const auto bgHover = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
        const auto bgActive = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);

        const auto textNormal = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
        const auto textDisabled = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);

        const bool enabled = _totalCount > 0;
        const bool enabledReplyForward = (_totalCount - _unsupported) > 0;
        const auto cursor = enabled ? Qt::PointingHandCursor : Qt::ArrowCursor;

        const auto plainFilesOnly = (_totalCount - _plainFiles) <= 0;
        const auto delReplyOnly = plainFilesOnly && Logic::getContactListModel()->isTrustRequired(aimid_);

        for (auto btn : { delete_, favorites_, copy_, reply_, forward_ })
        {
            bool enbl = enabled;
            if (btn == reply_ || btn == forward_)
                enbl = enabled && enabledReplyForward;

            btn->setCursor(cursor);
            btn->setEnabled(enbl);

            if (enbl)
            {
                btn->setColors(bgNormal, bgHover, bgActive);
                btn->setTextColor(textNormal);
            }
            else
            {
                btn->setColors(bgNormal, bgNormal, bgNormal);
                btn->setTextColor(textDisabled);
            }

            btn->setVisible(!delReplyOnly || (delReplyOnly && (btn == delete_ || btn == reply_)));
        }

        if (enabled)
            delete_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));

        delete_->setIcon(getDeleteIcon(), deleteIconSize());
        favorites_->setIcon(getFavoritesIcon(), iconSize());
        copy_->setIcon(getCopyIcon(), iconSize());
        reply_->setIcon(getReplyIcon(), iconSize());
        forward_->setIcon(getForwardIcon(), iconSize());
    }
}

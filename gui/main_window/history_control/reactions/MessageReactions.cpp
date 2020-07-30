#include "stdafx.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "gui_settings.h"
#include "../HistoryControlPageItem.h"
#include "controls/GeneralDialog.h"
#include "main_window/MainWindow.h"
#include "previewer/toast.h"
#include "core_dispatcher.h"
#include "ReactionsPlate.h"
#include "AddReactionButton.h"
#include "AddReactionPlate.h"
#include "MessageReactions.h"
#include "ReactionsLoader.h"
#include "ReactionsList.h"

namespace
{

int32_t setReactionPlateBottomOffset()
{
    return Utils::scale_value(4);
}

int32_t buttonHoverAreaWidth()
{
    return Utils::scale_value(32);
}

}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// MessageReactions_p
//////////////////////////////////////////////////////////////////////////

class MessageReactions_p
{
public:
    MessageReactions_p(MessageReactions* _q) : q(_q) {}

    AddReactionPlate* createAddReactionPlate()
    {
        auto plate = new AddReactionPlate(reactions_);

        plate->setOutgoingPosition(item_->isOutgoingPosition());
        plate->setMsgId(item_->getId());
        plate->setChatId(item_->getContact());

        QObject::connect(plate, &AddReactionPlate::plateShown, button_, &AddReactionButton::onAddReactionPlateShown);
        QObject::connect(plate, &AddReactionPlate::plateClosed, button_, &AddReactionButton::onAddReactionPlateClosed);
        QObject::connect(plate, &AddReactionPlate::addReactionClicked, q, &MessageReactions::onAddReactionClicked);
        QObject::connect(plate, &AddReactionPlate::removeReactionClicked, q, &MessageReactions::onRemoveReactionClicked);

        return plate;
    }

    QRegion buttonHoverRegion()
    {
        QRegion region;
        region += item_->messageRect();

        if (plate_->isVisible())
            region += plate_->geometry();

        auto messageRect = item_->messageRect();
        if (item_->isOutgoingPosition())
            region += QRect(messageRect.left() - buttonHoverAreaWidth(), messageRect.top(), buttonHoverAreaWidth(), messageRect.height());
        else
            region += QRect(messageRect.right(), messageRect.top(), buttonHoverAreaWidth(), messageRect.height());

        return region;
    }

    Data::Reactions reactions_;
    HistoryControlPageItem* item_;
    ReactionsPlate* plate_;
    AddReactionButton* button_;
    bool reactionsLoaded_ = false;
    int64_t seq_ = 0;

    MessageReactions* q;
};

//////////////////////////////////////////////////////////////////////////
// MessageReactions
//////////////////////////////////////////////////////////////////////////

MessageReactions::MessageReactions(HistoryControlPageItem* _item)
    : d(std::make_unique<MessageReactions_p>(this))
{
    d->item_ = _item;
    d->plate_ = new ReactionsPlate(_item);
    d->button_ = new AddReactionButton(_item);

    d->plate_->setOutgoingPosition(_item->isOutgoingPosition());
    d->button_->setOutgoingPosition(_item->isOutgoingPosition());

    connect(d->button_, &AddReactionButton::clicked, this, &MessageReactions::onReactionButtonClicked);
    connect(d->plate_, &ReactionsPlate::reactionClicked, this, &MessageReactions::onReactionClicked);
    connect(GetDispatcher(), &core_dispatcher::reactionAddResult, this, &MessageReactions::onReactionAddResult);
    connect(GetDispatcher(), &core_dispatcher::reactionRemoveResult, this, &MessageReactions::onReactionRemoveResult);
    connect(d->plate_, &ReactionsPlate::platePositionChanged, d->button_, &AddReactionButton::onPlatePositionChanged);
}

MessageReactions::~MessageReactions()
{

}

void MessageReactions::setReactions(const Data::Reactions& _reactions)
{
    const auto oldEmpty = d->reactions_.isEmpty();
    const auto newEmpty = _reactions.isEmpty();

    d->plate_->setReactions(_reactions);
    d->reactions_ = _reactions;
    d->reactionsLoaded_ = true;

    if (newEmpty || oldEmpty != newEmpty)
        d->item_->updateSize();
}

void MessageReactions::setOutgoing(bool _outgoing)
{
    d->plate_->setOutgoingPosition(_outgoing);
    d->button_->setOutgoingPosition(_outgoing);
}

void MessageReactions::subscribe()
{
    ReactionsLoader::instance()->subscribe(this);
}

QString MessageReactions::contact() const
{
    return d->item_->buddy().AimId_;
}

int64_t MessageReactions::msgId() const
{
    return d->item_->buddy().Id_;
}

bool MessageReactions::hasReactions() const
{
    return !d->reactionsLoaded_ || !d->reactions_.isEmpty();
}

QRect MessageReactions::plateRect() const
{
    return d->plate_->geometry();
}

void MessageReactions::onMouseLeave()
{
    d->button_->onMouseLeave();
}

void MessageReactions::onMouseMove(const QPoint& _pos)
{
    d->button_->onMouseMove(_pos, d->buttonHoverRegion());
}

void MessageReactions::onMousePress(const QPoint& _pos)
{
    d->button_->onMousePress(_pos);
}

void MessageReactions::onMouseRelease(const QPoint& _pos)
{
    d->button_->onMouseRelease(_pos);
}

void MessageReactions::onMessageSizeChanged()
{
    d->plate_->onMessageSizeChanged();
    d->button_->onMessageSizeChanged();
}

void MessageReactions::deleteControls()
{
    d->plate_->deleteLater();
    d->button_->deleteLater();
}

bool MessageReactions::enabled()
{
    return Features::reactionsEnabled() && get_gui_settings()->get_value<bool>(settings_show_reactions, settings_show_reactions_default());
}

void MessageReactions::onReactionButtonClicked()
{
    auto plate = d->createAddReactionPlate(); // will delete itself
    const auto buttonRect = d->button_->geometry();
    const auto buttonBottomCenterGlobal = d->item_->mapToGlobal(buttonRect.center() - QPoint(0, buttonRect.height() / 2));
    plate->showOverButton(buttonBottomCenterGlobal);
}

void MessageReactions::onReactionClicked(const QString& _reaction)
{
    auto w = new ReactionsList(d->item_->getId(), d->item_->getContact(), _reaction, d->reactions_);
    GeneralDialog dialog(w, Utils::InterConnector::instance().getMainWindow(), false, false);
    dialog.addCancelButton(QT_TRANSLATE_NOOP("reactions", "Close"), true);
    dialog.showInCenter();
}

void MessageReactions::onAddReactionClicked(const QString& _reaction)
{
    d->seq_ = GetDispatcher()->addReaction(d->item_->getId(), d->item_->getContact(), _reaction, Features::getReactionsSet());
}

void MessageReactions::onRemoveReactionClicked()
{
    d->seq_ = GetDispatcher()->removeReaction(d->item_->getId(), d->item_->getContact());
}

void MessageReactions::onReactionAddResult(int64_t _seq, const Data::Reactions& _reactions, bool _success)
{
    if (d->seq_ != _seq)
        return;

    if (!_success)
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("reactions", "Add reaction: an error occurred"));
}

void MessageReactions::onReactionRemoveResult(int64_t _seq, bool _success)
{
    if (d->seq_ != _seq)
        return;

    if (!_success)
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("reactions", "Remove reaction: an error occurred"));
}

}

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
#include "DefaultReactions.h"

#include "../common.shared/config/config.h"

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
    AddReactionButton* button_ = nullptr;
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
    d->plate_->setOutgoingPosition(_item->isOutgoingPosition());

    connect(d->plate_, &ReactionsPlate::reactionClicked, this, &MessageReactions::onReactionClicked);
    connect(d->plate_, &ReactionsPlate::platePositionChanged, this, &MessageReactions::platePositionChanged);
    connect(GetDispatcher(), &core_dispatcher::reactionAddResult, this, &MessageReactions::onReactionAddResult);
    connect(GetDispatcher(), &core_dispatcher::reactionRemoveResult, this, &MessageReactions::onReactionRemoveResult);
}

MessageReactions::~MessageReactions() = default;

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

const Data::Reactions& MessageReactions::getReactions() const
{
    return d->reactions_;
}

void MessageReactions::setOutgoing(bool _outgoing)
{
    d->plate_->setOutgoingPosition(_outgoing);
    if (d->button_)
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
    if (d->button_)
        d->button_->onMouseLeave();
}

void MessageReactions::onMouseMove(const QPoint& _pos)
{
    if (d->button_)
        d->button_->onMouseMove(_pos, d->buttonHoverRegion());
}

void MessageReactions::onMousePress(const QPoint& _pos)
{
    if (d->button_)
        d->button_->onMousePress(_pos);
}

void MessageReactions::onMouseRelease(const QPoint& _pos)
{
    if (d->button_)
        d->button_->onMouseRelease(_pos);
}

void MessageReactions::onMessageSizeChanged()
{
    d->plate_->onMessageSizeChanged();
    if (d->button_)
        d->button_->onMessageSizeChanged();
}

void MessageReactions::deleteControls()
{
    d->plate_->deleteLater();
    if (d->button_)
        d->button_->deleteLater();
}

void MessageReactions::initAddReactionButton()
{
    if (d->button_)
        return;

    d->button_ = new AddReactionButton(d->item_);
    d->button_->setOutgoingPosition(d->item_->isOutgoingPosition());

    connect(d->button_, &AddReactionButton::clicked, this, &MessageReactions::onReactionButtonClicked);
    connect(d->plate_, &ReactionsPlate::platePositionChanged, d->button_, &AddReactionButton::onPlatePositionChanged);
}

bool MessageReactions::enabled()
{
    return Features::reactionsEnabled() && get_gui_settings()->get_value<bool>(settings_show_reactions, settings_show_reactions_default());
}

AddReactionPlate* MessageReactions::createAddReactionPlate(const HistoryControlPageItem* const _item, const MessageReactions* const _reactions)
{
    im_assert(_item);
    im_assert(_reactions);
    if (!_item || !_reactions)
        return nullptr;

    auto plate = new AddReactionPlate(_reactions->getReactions()); // will delete itself

    plate->setOutgoingPosition(_item->isOutgoingPosition());
    plate->setMsgId(_item->getId());
    plate->setChatId(_item->getContact());
    plate->setThreadFeedFlag(_item->isThreadFeedMessage());

    connect(plate, &AddReactionPlate::addReactionClicked, _reactions, &MessageReactions::onAddReactionClicked);
    connect(plate, &AddReactionPlate::removeReactionClicked, _reactions, &MessageReactions::onRemoveReactionClicked);

    return plate;
}

void MessageReactions::onReactionButtonClicked()
{
    im_assert(d->button_);
    if (!d->button_)
        return;

    auto plate = createAddReactionPlate(d->item_, this);
    connect(plate, &AddReactionPlate::plateShown, d->button_, &AddReactionButton::onAddReactionPlateShown);
    connect(plate, &AddReactionPlate::plateCloseStarted, d->button_, &AddReactionButton::onAddReactionPlateCloseStarted);
    connect(plate, &AddReactionPlate::plateCloseFinished, d->button_, &AddReactionButton::onAddReactionPlateCloseFinished);

    const auto& buttonRect = d->button_->geometry();
    const auto buttonTopCenterGlobal = d->item_->mapToGlobal(buttonRect.center() - QPoint(0, buttonRect.height() / 2));
    plate->showOverButton(buttonTopCenterGlobal);
}

void MessageReactions::onReactionClicked(const QString& _reaction)
{
    auto w = new ReactionsList(d->item_->getId(), d->item_->getContact(), _reaction, d->reactions_);
    GeneralDialog::Options opt;
    opt.fixedSize_ = false;
    GeneralDialog dialog(w, Utils::InterConnector::instance().getMainWindow(), opt);
    dialog.addCancelButton(QT_TRANSLATE_NOOP("reactions", "Close"), true);
    dialog.execute();
}

void MessageReactions::onAddReactionClicked(const QString& _reaction)
{
    d->seq_ = GetDispatcher()->addReaction(d->item_->getId(), d->item_->getContact(), _reaction);
    Utils::InterConnector::instance().setFocusOnInput(d->item_->getContact());
}

void MessageReactions::onRemoveReactionClicked()
{
    d->seq_ = GetDispatcher()->removeReaction(d->item_->getId(), d->item_->getContact());
    Utils::InterConnector::instance().setFocusOnInput(d->item_->getContact());
}

void MessageReactions::onReactionAddResult(int64_t _seq, const Data::Reactions& _reactions, bool _success)
{
    if (d->seq_ != _seq)
        return;

    if (_success)
        setReactions(_reactions);
    else
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

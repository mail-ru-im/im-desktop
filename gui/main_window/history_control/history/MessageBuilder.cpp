#include "MessageBuilder.h"

#include "../ChatEventItem.h"
#include "../MessageStyle.h"
#include "../ServiceMessageItem.h"

#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/RecentsModel.h"
#include "../../contact_list/UnknownsModel.h"
#include "../../containers/FriendlyContainer.h"
#include "../../history_control/ChatEventInfo.h"
#include "../../history_control/FileSharingInfo.h"
#include "../../history_control/VoipEventInfo.h"
#include "../../history_control/VoipEventItem.h"
#include "../../history_control/HistoryControlPageItem.h"
#include "../../history_control/complex_message/ComplexMessageItem.h"
#include "../../history_control/complex_message/ComplexMessageItemBuilder.h"
#include "../../../my_info.h"
#include "../../../core_dispatcher.h"
#include "History.h"
#include "utils/features.h"
#include "spellcheck/Spellchecker.h"
#include "../../../gui_settings.h"

namespace hist::MessageBuilder
{
    std::unique_ptr<Ui::HistoryControlPageItem> makePageItem(const Data::MessageBuddy& _msg, int _itemWidth, QWidget* _parent)
    {
        if (_msg.IsEmpty())
            return nullptr;

        if (_msg.IsDeleted())
            return nullptr;

        const auto isServiceMessage = (!_msg.IsBase() && !_msg.IsFileSharing() && !_msg.IsSticker() && !_msg.IsChatEvent() && !_msg.IsVoipEvent());
        if (isServiceMessage)
        {
            auto serviceMessageItem = std::make_unique<Ui::ServiceMessageItem>(_parent);
            serviceMessageItem->setContact(_msg.AimId_);
            serviceMessageItem->setDate(_msg.GetDate());
            if (_itemWidth > 0)
                serviceMessageItem->setWidth(_itemWidth);
            serviceMessageItem->setDeleted(_msg.IsDeleted());
            serviceMessageItem->setEdited(_msg.IsEdited());
            serviceMessageItem->setIsChat(_msg.Chat_);
            serviceMessageItem->setBuddy(_msg);

            return serviceMessageItem;
        }

        if (_msg.IsChatEvent())
        {
            auto item = std::make_unique<Ui::ChatEventItem>(_parent, _msg.GetChatEvent(), _msg.Id_, _msg.Prev_);
            item->setContact(_msg.AimId_);
            item->updateStyle();
            item->setHasAvatar(false);
            if (_itemWidth > 0)
                item->setFixedWidth(_itemWidth);
            item->setDeleted(_msg.IsDeleted());
            item->setEdited(_msg.IsEdited());
            item->setIsChat(Utils::isChat(_msg.AimId_));
            item->setBuddy(_msg);
            return item;
        }

        if (_msg.IsVoipEvent())
        {
            const auto &voipEvent = _msg.GetVoipEvent();
            if (!voipEvent->isVisible())
                return nullptr;

            auto item = std::make_unique<Ui::VoipEventItem>(_parent, voipEvent);
            item->setTopMargin(_msg.GetIndentBefore());
            if (_itemWidth > 0)
                item->setFixedWidth(_itemWidth);
            item->setHasAvatar(_msg.HasAvatar());
            item->setHasSenderName(_msg.HasSenderName());
            item->setChainedToPrev(_msg.isChainedToPrev());
            item->setChainedToNext(_msg.isChainedToNext());
            item->setId(_msg.Id_, _msg.InternalId_);
            item->setDeleted(_msg.IsDeleted());
            item->setEdited(_msg.IsEdited());
            item->setIsChat(_msg.Chat_);
            item->setBuddy(_msg);
            return item;
        }

        auto item = Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(_parent, _msg, Ui::ComplexMessage::ComplexMessageItemBuilder::ForcePreview::No);
        item->setContact(_msg.AimId_);
        item->setTime(_msg.GetTime());
        item->setHasAvatar(_msg.HasAvatar());
        item->setHasSenderName(_msg.HasSenderName());
        item->setChainedToPrev(_msg.isChainedToPrev());
        item->setChainedToNext(_msg.isChainedToNext());
        item->setTopMargin(_msg.GetIndentBefore());
        item->setEdited(_msg.IsEdited());
        item->setIsChat(_msg.Chat_);
        item->setUpdatePatchVersion(_msg.GetUpdatePatchVersion());

        if (_msg.Chat_)
        {
            const auto senderAimid = Logic::getContactListModel()->isChannel(_msg.AimId_) ? _msg.AimId_ : _msg.getSender();
            item->setMchatSender(Logic::GetFriendlyContainer()->getFriendly(senderAimid));
        }

        if (_itemWidth > 0)
            item->setFixedWidth(_itemWidth);

        QObject::connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, item.get(),
            [item = item.get()](const QString& _aimId, const QString& _friendly)
        {
            item->updateFriendly(_aimId, _friendly);
        });

        if (Features::isSpellCheckEnabled()/* && item->isEditable()*/)
        {
            QObject::connect(Ui::get_gui_settings(), &Ui::qt_gui_settings::changed, item.get(), [item = item.get()](const QString& _text)
            {
                if (_text == ql1s(settings_spell_check))
                    item->updateStyle();
            });

            QObject::connect(&spellcheck::Spellchecker::instance(), &spellcheck::Spellchecker::dictionaryChanged, item.get(), [item = item.get()]()
            {
                item->updateStyle();
            });
        }

        QObject::connect(
            item.get(),
            &Ui::ComplexMessage::ComplexMessageItem::removeMe,
            [aimId = _msg.AimId_, key = _msg.ToKey()]()
        {
            Q_EMIT Ui::GetDispatcher()->removePending(aimId, key);
        });

        return item;
    }

    Ui::MediaWithText formatRecentsText(const Data::MessageBuddy &buddy)
    {
        if (buddy.IsServiceMessage())
            return {};

        if (buddy.IsChatEvent())
        {
            auto item = std::make_unique<Ui::ChatEventItem>(buddy.GetChatEvent(), buddy.Id_, buddy.Prev_);
            item->setContact(buddy.AimId_);
            return { item->formatRecentsText(), item->getMediaType() };
        }

        if (buddy.IsVoipEvent())
            return { buddy.GetVoipEvent()->formatRecentsText(), Ui::MediaType::mediaTypeVoip };

        if (buddy.IsFileSharing())
        {
            if (buddy.ContainsGif())
                return { QT_TRANSLATE_NOOP("contact_list", "GIF"), Ui::MediaType::mediaTypeVideo };
            else if (buddy.ContainsImage())
                return { QT_TRANSLATE_NOOP("contact_list", "Photo"), Ui::MediaType::mediaTypePhoto };
            else if (buddy.ContainsVideo())
                return { QT_TRANSLATE_NOOP("contact_list", "Video"), Ui::MediaType::mediaTypeVideo };
        }

        const auto emptyTextAllowed = buddy.IsSticker() || !buddy.Quotes_.isEmpty() || buddy.sharedContact_ || buddy.poll_ || buddy.task_;

        if (!emptyTextAllowed && Data::FStringView(buddy.GetSourceText()).trimmed().isEmpty())
            return {};

        auto item = Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(nullptr, buddy, Ui::ComplexMessage::ComplexMessageItemBuilder::ForcePreview::Yes);

        return { item->formatRecentsText(), item->getMediaType(Ui::MediaRequestMode::Recents) };
    }

    std::unique_ptr<Ui::ServiceMessageItem> createNew(const QString& _aimId, int _itemWidth, QWidget* _parent)
    {
        im_assert(_parent);

        std::unique_ptr<Ui::ServiceMessageItem> newPlate = std::make_unique<Ui::ServiceMessageItem>(_parent);

        newPlate->setContact(_aimId);
        newPlate->setWidth(_itemWidth);
        newPlate->setNew();

        return newPlate;
    }
}

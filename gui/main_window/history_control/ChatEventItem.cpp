#include "stdafx.h"


#include "../../controls/TextUnit.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../fonts.h"
#include "../../app_config.h"

#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/sidebar/SidebarUtils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../mediatype.h"

#include "ChatEventInfo.h"
#include "MessageStyle.h"
#include "ChatEventItem.h"

namespace Ui
{
    namespace
    {
        qint32 getBubbleHorMargin(int _width)
        {
            if (_width >= Ui::MessageStyle::fiveHeadsWidth())
                return Utils::scale_value(136);

            if (_width >= Ui::MessageStyle::fourHeadsWidth())
                return Utils::scale_value(116);

            if (_width >= Ui::MessageStyle::threeHeadsWidth())
                return Utils::scale_value(96);

            return Utils::scale_value(32);
        }

        qint32 getTextHorPadding()
        {
            return Utils::scale_value(8);
        }

        qint32 getTextTopPadding()
        {
            return Utils::scale_value(4);
        }

        qint32 getTextBottomPadding()
        {
            return Utils::scale_value(5);//ask Andrew to explain
        }

        qint32 getWidgetMinHeight()
        {
            return Utils::scale_value(22);
        }

        qint32 getButtonsHeight()
        {
            return Utils::scale_value(40);
        }

        qint32 getButtonsRadius()
        {
            return Utils::scale_value(12);
        }

        qint32 getButtonsSpacing()
        {
            return Utils::scale_value(8);
        }

        constexpr auto fontSize() noexcept { return 12; }
    }

    ChatEventItem::ChatEventItem(const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : MessageItemBase(nullptr)
        , TextWidget_(nullptr)
        , EventInfo_(_eventInfo)
        , height_(0)
        , id_(_id)
        , prevId_(_prevId)
        , textPlaceholder_(nullptr)
        , addAvatar_(nullptr)
        , addDescription_(nullptr)
        , buttonsVisible_(false)
        , descriptionButtonVisible_(false)
        , avatarButtonVisible_(false)
        , modChatAboutSeq_(-1)
    {
        setMouseTracking(true);
        setMultiselectEnabled(false);
    }

    ChatEventItem::ChatEventItem(QWidget* _parent, const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : MessageItemBase(_parent)
        , TextWidget_(nullptr)
        , EventInfo_(_eventInfo)
        , height_(0)
        , id_(_id)
        , prevId_(_prevId)
        , textPlaceholder_(nullptr)
        , addAvatar_(nullptr)
        , addDescription_(nullptr)
        , buttonsVisible_(false)
        , descriptionButtonVisible_(false)
        , avatarButtonVisible_(false)
        , modChatAboutSeq_(-1)
    {
        assert(EventInfo_);

        TextWidget_ = TextRendering::MakeTextUnit(
            EventInfo_->formatEventText(),
            {},
            EventInfo_->isCaptchaPresent() ? TextRendering::LinksVisible::SHOW_LINKS : TextRendering::LinksVisible::DONT_SHOW_LINKS
        );

        TextWidget_->applyLinks(EventInfo_->getMembersLinks());

        auto layout = Utils::emptyVLayout(this);
        layout->setAlignment(Qt::AlignHCenter);
        textPlaceholder_ = new QWidget(this);
        layout->addWidget(textPlaceholder_);

        buttonsWidget_ = new QWidget(this);
        {
            const auto bgNormal = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
            const auto bgHover = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
            const auto bgActive = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);

            auto hLayout = Utils::emptyHLayout(buttonsWidget_);
            hLayout->setSpacing(getButtonsSpacing());
            hLayout->setContentsMargins(getButtonsSpacing(), 0, getButtonsSpacing(), 0);
            addAvatar_ = new RoundButton(buttonsWidget_, getButtonsRadius());
            addAvatar_->setText(QT_TRANSLATE_NOOP("chat_event", "Add avatar"));
            addAvatar_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            addAvatar_->setColors(bgNormal, bgHover, bgActive);
            addAvatar_->setFixedHeight(getButtonsHeight());
            hLayout->addWidget(addAvatar_);

            connect(addAvatar_, &RoundButton::clicked, this, &ChatEventItem::addAvatarClicked);

            addDescription_ = new RoundButton(buttonsWidget_, getButtonsRadius());
            addDescription_->setText(QT_TRANSLATE_NOOP("chat_event", "Add description"));
            addDescription_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            addDescription_->setColors(bgNormal, bgHover, bgActive);
            addDescription_->setFixedHeight(getButtonsHeight());
            hLayout->addWidget(addDescription_);

            connect(addDescription_, &RoundButton::clicked, this, &ChatEventItem::addDescriptionClicked);
        }
        buttonsWidget_->setFixedHeight(getButtonsHeight());
        layout->addWidget(buttonsWidget_);

        updateButtons();

        setMouseTracking(true);
        setMultiselectEnabled(false);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &ChatEventItem::chatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::modChatAboutResult, this, &ChatEventItem::modChatAboutResult);
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ChatEventItem::avatarChanged);
    }

    ChatEventItem::~ChatEventItem()
    {
    }

    QString ChatEventItem::formatRecentsText() const
    {
        return EventInfo_->formatEventText();
    }

    MediaType ChatEventItem::getMediaType(MediaRequestMode) const
    {
        return MediaType::noMedia;
    }

    void ChatEventItem::setLastStatus(LastStatus _lastStatus)
    {
        if (getLastStatus() != _lastStatus)
        {
            assert(_lastStatus == LastStatus::None);
            HistoryControlPageItem::setLastStatus(_lastStatus);
            updateGeometry();
            update();
        }
    }

    int32_t ChatEventItem::evaluateTextWidth(const int32_t _widgetWidth)
    {
        assert(_widgetWidth > 0);

        const auto maxBubbleWidth = _widgetWidth - 2 * getBubbleHorMargin(_widgetWidth);
        const auto maxBubbleContentWidth = maxBubbleWidth - 2 * getTextHorPadding();

        return maxBubbleContentWidth;
    }

    void ChatEventItem::updateStyle()
    {
        const auto textColor = Styling::getParameters(getContact()).getColor(Styling::StyleVariable::CHATEVENT_TEXT);

        TextWidget_->init(Fonts::adjustedAppFont(fontSize(), Fonts::defaultAppFontWeight(), Fonts::FontAdjust::NoAdjust), textColor, textColor, MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::CENTER);
        TextWidget_->applyFontToLinks(Fonts::adjustedAppFont(fontSize(), Fonts::FontWeight::SemiBold, Fonts::FontAdjust::NoAdjust));
        TextWidget_->getHeight(evaluateTextWidth(width()));

        updateSize();
    }

    void ChatEventItem::updateFonts()
    {
        updateStyle();
    }

    void ChatEventItem::updateSize()
    {
        updateSize(size());
    }

    void ChatEventItem::updateSize(const QSize& _newSize)
    {
        updateButtons();

        // setup the text control and get it dimensions
        const auto maxTextWidth = evaluateTextWidth(_newSize.width());
        const auto textHeight = TextWidget_->getHeight(maxTextWidth);
        const auto textWidth = TextWidget_->getMaxLineWidth();

        // evaluate bubble width
        auto bubbleWidth = textWidth + 2 * getTextHorPadding();
        if (hasButtons())
        {
            auto buttonsDesiredWidth = 0;
            {
                if (avatarButtonVisible_ && descriptionButtonVisible_)
                    buttonsDesiredWidth = std::max(addAvatar_->textDesiredWidth(), addDescription_->textDesiredWidth()) * 2 + getButtonsSpacing() * 3;
                else if (avatarButtonVisible_)
                    buttonsDesiredWidth = addAvatar_->textDesiredWidth() + getButtonsSpacing() * 2;
                else if (descriptionButtonVisible_)
                    buttonsDesiredWidth = addDescription_->textDesiredWidth() + getButtonsSpacing() * 2;
            }

            bubbleWidth = std::max(bubbleWidth, buttonsDesiredWidth);
            buttonsWidget_->setFixedWidth(bubbleWidth);
        }

        // evaluate bubble height
        auto bubbleHeight = textHeight + getTextTopPadding() + getTextBottomPadding();
        const auto topPadding = Utils::scale_value(8);
        if (hasButtons())
        {
            textPlaceholder_->setFixedHeight(bubbleHeight);
            bubbleHeight += buttonsWidget_->height() - topPadding - bottomOffset();
        }

        BubbleRect_ = QRect(0, 0, bubbleWidth, bubbleHeight);
        BubbleRect_.moveCenter(QRect(0, 0, _newSize.width(), bubbleHeight).center());

        BubbleRect_.moveTop(topPadding);

        // setup geometry
        height_ = bubbleHeight + topPadding + bottomOffset();
        if (!hasButtons())
            textPlaceholder_->setFixedHeight(height_);

        TextWidget_->setOffsets((width() - TextWidget_->cachedSize().width()) / 2, BubbleRect_.top() + getTextTopPadding());
    }

    bool ChatEventItem::hasButtons() const
    {
        return buttonsVisible_;
    }

    void ChatEventItem::updateButtons()
    {
        if (EventInfo_->eventType() != core::chat_event_type::mchat_invite)
        {
            buttonsWidget_->setVisible(false);
            return;
        }

        const auto aimId = EventInfo_->getAimId();
        descriptionButtonVisible_ = Logic::getContactListModel()->getChatDescription(aimId).isEmpty();
        avatarButtonVisible_ = Logic::GetAvatarStorage()->isDefaultAvatar(aimId);
        buttonsVisible_ = Logic::getContactListModel()->isYouAdmin(aimId) && (avatarButtonVisible_ || descriptionButtonVisible_);

        addDescription_->setVisible(descriptionButtonVisible_);
        addAvatar_->setVisible(avatarButtonVisible_);

        buttonsWidget_->setVisible(buttonsVisible_);
    }

    bool ChatEventItem::isOutgoing() const
    {
        return false;
    }

    int32_t ChatEventItem::getTime() const
    {
        return 0;
    }

    int ChatEventItem::bottomOffset() const
    {
        auto margin = Utils::scale_value(2);

        if (isChat() && hasHeads() && headsAtBottom())
            margin += MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset();

        return margin;
    }

    void ChatEventItem::chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int)
    {
        updateSize();
    }

    void ChatEventItem::modChatAboutResult(qint64 _seq, int)
    {
        if (_seq == modChatAboutSeq_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            const auto aimid = EventInfo_->getAimId();
            collection.set_value_as_qstring("aimid", aimid);

            if (aimid.isEmpty())
            {
                const auto stamp = Logic::getContactListModel()->getChatStamp(aimid);
                if (!stamp.isEmpty())
                    collection.set_value_as_qstring("stamp", stamp);
                else
                    return;
            }

            collection.set_value_as_int("limit", 0);
            Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
        }
    }

    void ChatEventItem::avatarChanged(const QString&)
    {
        updateSize();
    }

    void ChatEventItem::addAvatarClicked()
    {
        auto avatar = new ContactAvatarWidget(this, EventInfo_->getAimId(), QString(), 0, true);
        avatar->selectFileForAvatar();
        auto pixmap = avatar->croppedImage();
        if (!pixmap.isNull())
            avatar->applyAvatar(pixmap);
    }

    void ChatEventItem::addDescriptionClicked()
    {
        const auto model = Logic::getContactListModel();
        const auto aimId = EventInfo_->getAimId();

        QString name = model->getChatName(aimId);
        QString description = model->getChatDescription(aimId);
        QString rules = model->getChatRules(aimId);

        auto avatar = editGroup(aimId, name, description, rules);
        if (name != model->getChatName(aimId))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId);
            collection.set_value_as_qstring("name", name);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/name", collection.get());
        }

        if (description != model->getChatDescription(aimId))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId);
            collection.set_value_as_qstring("about", description);
            modChatAboutSeq_ = Ui::GetDispatcher()->post_message_to_core("chats/mod/about", collection.get());
        }

        if (rules != model->getChatRules(aimId))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId);
            collection.set_value_as_qstring("rules", rules);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/rules", collection.get());
        }

        if (!avatar.isNull())
        {
            const auto byteArray = avatarToByteArray(avatar);

            Ui::gui_coll_helper helper(GetDispatcher()->create_collection(), true);

            core::ifptr<core::istream> data_stream(helper->create_stream());
            if (!byteArray.isEmpty())
                data_stream->write((const uint8_t*)byteArray.data(), (uint32_t)byteArray.size());

            helper.set_value_as_stream("avatar", data_stream.get());
            helper.set_value_as_qstring("aimid", aimId);

            GetDispatcher()->post_message_to_core("set_avatar", helper.get());
        }
    }

    void ChatEventItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setPen(Qt::NoPen);

        p.setBrush(Styling::getParameters(getContact()).getColor(Styling::StyleVariable::CHATEVENT_BACKGROUND));
        p.drawRoundedRect(BubbleRect_, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());

        if (hasButtons())
            TextWidget_->draw(p);
        else
            TextWidget_->drawSmart(p, BubbleRect_.center().y());

        drawHeads(p);

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            p.setPen(TextWidget_->getColor());
            p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = BubbleRect_.right() + 1 - MessageStyle::getBorderRadius();
            const auto y = BubbleRect_.bottom() + 1;
            Utils::drawText(p, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, QString::number(getId()));
        }
    }

    QSize ChatEventItem::sizeHint() const
    {
        return QSize(0, height_);
    }

    void ChatEventItem::resizeEvent(QResizeEvent* _event)
    {
        HistoryControlPageItem::resizeEvent(_event);
        updateSize(_event->size());
    }

    void ChatEventItem::mousePressEvent(QMouseEvent* _event)
    {
        pressPos_ = _event->pos();
        return HistoryControlPageItem::mousePressEvent(_event);
    }

    void ChatEventItem::mouseMoveEvent(QMouseEvent* _event)
    {
        if (TextWidget_->isOverLink(_event->pos()))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        return HistoryControlPageItem::mouseMoveEvent(_event);
    }

    void ChatEventItem::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(pressPos_, _event->pos()))
            TextWidget_->clicked(_event->pos());

        return HistoryControlPageItem::mouseReleaseEvent(_event);
    }

    void ChatEventItem::clearSelection(bool)
    {
        TextWidget_->clearSelection();
    }

    qint64 ChatEventItem::getId() const
    {
        return id_;
    }

    qint64 ChatEventItem::getPrevId() const
    {
        return prevId_;
    }
};

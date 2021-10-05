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
#include "../../main_window/contact_list/RecentsModel.h"
#include "../../main_window/contact_list/IgnoreMembersModel.h"
#include "../../main_window/sidebar/SidebarUtils.h"
#include "../../main_window/ReportWidget.h"
#include "../../main_window/containers/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../mediatype.h"

#include "omicron/omicron_helper.h"
#include "../../../common.shared/omicron_keys.h"

#include "ChatEventInfo.h"
#include "MessageStyle.h"
#include "ChatEventItem.h"

namespace Ui
{
    namespace
    {
        qint32 defaultFontSize()
        {
            return 12;
        }

        qint32 getTextHorPadding()
        {
            return Utils::scale_value(8);
        }

        qint32 getTextTopPadding(int _fontSize)
        {
            if (_fontSize == defaultFontSize())
                return Utils::scale_value(4);

            return Utils::scale_value(platform::is_apple() ? 7 : 6);
        }

        qint32 getTextBottomPadding()
        {
            return Utils::scale_value(5);//ask Andrew to explain
        }

        qint32 additionalButtonsPadding()
        {
            return Utils::scale_value(4);
        }

        qint32 getButtonsRadius()
        {
            return Utils::scale_value(12);
        }

        qint32 getButtonsSpacing()
        {
            return Utils::scale_value(8);
        }

        QString blockButtonText()
        {
            if (Utils::GetTranslator()->getLang() != ql1s("ru"))
                return QT_TRANSLATE_NOOP("chat_event", "Block contact");

            return Omicron::_o(omicron::keys::block_stranger_button_text, QT_TRANSLATE_NOOP("chat_event", "Block contact"));
        }

        qint32 maxButtonWidth(int _buttonsCount)
        {
            return _buttonsCount == 1 ? Utils::scale_value(408) : Utils::scale_value(200);
        }
    }

    ChatEventItem::ChatEventItem(const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : HistoryControlPageItem(nullptr)
        , EventInfo_(_eventInfo)
        , id_(_id)
        , prevId_(_prevId)
    {
        im_assert(EventInfo_);

        init();
    }

    ChatEventItem::ChatEventItem(QWidget* _parent, const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : HistoryControlPageItem(_parent)
        , EventInfo_(_eventInfo)
        , id_(_id)
        , prevId_(_prevId)
    {
        im_assert(EventInfo_);

        const auto showLinks = EventInfo_->isCaptchaPresent() ? TextRendering::LinksVisible::SHOW_LINKS : TextRendering::LinksVisible::DONT_SHOW_LINKS;
        TextWidget_ = TextRendering::MakeTextUnit(EventInfo_->formatEventText(), {}, showLinks);
        TextWidget_->setLineSpacing(lineSpacing());
        TextWidget_->applyLinks(EventInfo_->getMembersLinks());

        initTextWidget();
        init();
    }

    ChatEventItem::ChatEventItem(QWidget* _parent, std::unique_ptr<TextRendering::TextUnit> _textUnit)
        : HistoryControlPageItem(_parent)
        , TextWidget_(std::move(_textUnit))
        , id_(-1)
        , prevId_(-1)
    {
        init();
    }

    ChatEventItem::~ChatEventItem() = default;

    void ChatEventItem::init()
    {
        setMouseTracking(true);
        setMultiselectEnabled(false);

        if (parentWidget())
        {
            auto layout = Utils::emptyVLayout(this);
            layout->setAlignment(Qt::AlignHCenter);
            textPlaceholder_ = new QWidget(this);
            layout->addWidget(textPlaceholder_);

            updateButtons();

            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &ChatEventItem::onChatInfo);
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::modChatAboutResult, this, &ChatEventItem::modChatAboutResult);
            connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ChatEventItem::avatarChanged);
            connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, this, &ChatEventItem::dlgStateChanged);
            connect(Logic::getIgnoreModel(), &Logic::IgnoreMembersModel::changed, this, &ChatEventItem::ignoreListChanged);
        }
    }

    void ChatEventItem::initTextWidget()
    {
        const auto color = getTextColor(getContact());
        const auto linkColor = getLinkColor(getContact());
        TextWidget_->init(getTextFont(textFontSize()), color, linkColor, QColor(), QColor(), textAligment());
        TextWidget_->applyFontToLinks(getTextFontBold(textFontSize()));
    }

    QString ChatEventItem::formatRecentsText() const
    {
        if (EventInfo_)
            return EventInfo_->formatRecentsEventText();

        if (TextWidget_)
            return TextWidget_->getText();

        return QString();
    }

    MediaType ChatEventItem::getMediaType(MediaRequestMode) const
    {
        return MediaType::noMedia;
    }

    void ChatEventItem::setLastStatus(LastStatus _lastStatus)
    {
        if (getLastStatus() != _lastStatus)
        {
            im_assert(_lastStatus == LastStatus::None);
            HistoryControlPageItem::setLastStatus(_lastStatus);
            updateGeometry();
            update();
        }
    }

    int32_t ChatEventItem::evaluateTextWidth(const int32_t _widgetWidth)
    {
        if (_widgetWidth <= 0)
            return 0;

        const auto maxBubbleWidth = _widgetWidth - Utils::scale_value(36);
        const auto maxBubbleContentWidth = maxBubbleWidth - 2 * getTextHorPadding();

        return maxBubbleContentWidth;
    }

    void ChatEventItem::updateStyle()
    {
        TextWidget_->setColor(getTextColor(getContact()));
        update();
    }

    void ChatEventItem::updateFonts()
    {
        initTextWidget();
        updateSize();
        update();
    }

    void ChatEventItem::updateSize()
    {
        updateSize(size());
    }

    void ChatEventItem::updateSize(const QSize& _newSize)
    {
        updateButtons();

        auto buttonsDesiredWidth = 0;
        const auto maxTextWidth = evaluateTextWidth(_newSize.width());
        if (hasButtons())
        {
            auto btnWidth = 0;
            auto needToResize = false;
            do
            {
                auto visibleCount = 0;
                btnWidth = 0;

                if (leftButtonVisible_)
                {
                    ++visibleCount;
                    btnWidth = std::max(btnWidth, btnLeft_->textDesiredWidth());
                }

                if (rightButtonVisible_)
                {
                    ++visibleCount;
                    btnWidth = std::max(btnWidth, btnRight_->textDesiredWidth());
                }

                qint32 maxBntWidth = (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger)) ? maxButtonWidth(visibleCount) : 0;
                btnWidth = needToResize ? std::min(btnWidth, maxBntWidth) : std::max(btnWidth, maxBntWidth);
                buttonsDesiredWidth = btnWidth * visibleCount + getButtonsSpacing() * (visibleCount + 1);
                needToResize = buttonsDesiredWidth > maxTextWidth + 2 * getTextHorPadding() + 1 && !needToResize;
            } while (needToResize);

            if (leftButtonVisible_)
                btnLeft_->setFixedWidth(btnWidth);
            if (rightButtonVisible_)
                btnRight_->setFixedWidth(btnWidth);

            buttonsWidget_->setFixedWidth(buttonsDesiredWidth);
        }

        const auto textHeight = TextWidget_->getHeight(std::max(maxTextWidth, buttonsDesiredWidth));
        const auto bubbleWidth = std::max(TextWidget_->getMaxLineWidth() + 2 * getTextHorPadding(), buttonsDesiredWidth);

        // evaluate bubble height
        auto bubbleHeight = textHeight + getTextTopPadding(textFontSize()) + getTextBottomPadding();
        const auto topPadding = Utils::scale_value(8);
        if (hasButtons())
        {
            textPlaceholder_->setFixedHeight(bubbleHeight);
            bubbleHeight += (buttonsWidget_->height() - topPadding - bottomOffset());
        }

        BubbleRect_ = QRect(0, 0, bubbleWidth, bubbleHeight);
        BubbleRect_.moveCenter(QRect(0, 0, _newSize.width(), bubbleHeight).center());

        BubbleRect_.moveTop(topPadding);

        // setup geometry
        height_ = bubbleHeight + topPadding + bottomOffset();
        if (!hasButtons())
            textPlaceholder_->setFixedHeight(height_);

        auto additionalOffset = TextWidget_->getLinesCount() > 1 ? (lineSpacing() / 2) : 0;
        TextWidget_->setOffsets(textAligment() == TextRendering::HorAligment::CENTER ? (width() - TextWidget_->cachedSize().width()) / 2 : BubbleRect_.x() + getTextHorPadding(),
            BubbleRect_.top() + getTextTopPadding(textFontSize()) + additionalOffset);
    }

    void ChatEventItem::initButtons()
    {
        im_assert(!buttonsWidget_);
        buttonsWidget_ = new QWidget(this);
        buttonsWidget_->setFixedHeight(getButtonsHeight() + additionalButtonsPadding());

        const auto bgNormal = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
        const auto bgHover = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
        const auto bgActive = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);
        const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);

        auto hLayout = Utils::emptyHLayout(buttonsWidget_);
        hLayout->setSpacing(getButtonsSpacing());
        hLayout->setContentsMargins(getButtonsSpacing(), 0, getButtonsSpacing(), 0);
        btnLeft_ = new RoundButton(buttonsWidget_, getButtonsRadius());
        Testing::setAccessibleName(btnLeft_, qsl("AS ChatIvent LeftButton"));
        btnLeft_->setTextColor(textColor);
        btnLeft_->setColors(bgNormal, bgHover, bgActive);
        btnLeft_->setFixedHeight(getButtonsHeight());
        hLayout->addWidget(btnLeft_);

        btnRight_ = new RoundButton(buttonsWidget_, getButtonsRadius());
        Testing::setAccessibleName(btnRight_, qsl("AS ChatIvent RightButton"));
        btnRight_->setTextColor(textColor);
        btnRight_->setColors(bgNormal, bgHover, bgActive);
        btnRight_->setFixedHeight(getButtonsHeight());
        hLayout->addWidget(btnRight_);

        layout()->addWidget(buttonsWidget_);

        connect(btnLeft_, &RoundButton::clicked, this, &ChatEventItem::leftButtonClicked);
        connect(btnRight_, &RoundButton::clicked, this, &ChatEventItem::rightButtonClicked);
    }

    bool ChatEventItem::hasButtons() const
    {
        return buttonsWidget_ && buttonsVisible_;
    }

    void ChatEventItem::updateButtons()
    {
        if (!EventInfo_)
            return;

        leftButtonVisible_ = rightButtonVisible_ = buttonsVisible_ = false;
        const auto aimId = EventInfo_->getAimId();
        const auto isStranger = Logic::getRecentsModel()->isStranger(aimId);
        const auto isIgnored = Logic::getIgnoreModel()->contains(aimId);

        if (!buttonsWidget_)
            initButtons();

        switch (EventInfo_->eventType())
        {
        case core::chat_event_type::mchat_invite:
            btnLeft_->setText(QT_TRANSLATE_NOOP("chat_event", "Add avatar"), buttonsfontSize());
            btnRight_->setText(QT_TRANSLATE_NOOP("chat_event", "Add description"), buttonsfontSize());

            leftButtonVisible_ = Logic::GetAvatarStorage()->isDefaultAvatar(aimId);
            rightButtonVisible_ = Logic::getContactListModel()->getChatDescription(aimId).isEmpty();
            buttonsVisible_ = Logic::getContactListModel()->areYouAdmin(aimId) && (leftButtonVisible_ || rightButtonVisible_);

            btnLeft_->setVisible(leftButtonVisible_);
            btnRight_->setVisible(rightButtonVisible_);

            buttonsWidget_->setVisible(buttonsVisible_);
            break;

        case core::chat_event_type::warn_about_stranger:
            btnLeft_->setText(blockButtonText(), buttonsfontSize());
            btnRight_->setText(QT_TRANSLATE_NOOP("chat_event", "OK"), buttonsfontSize());

            leftButtonVisible_ = rightButtonVisible_ = buttonsVisible_ = (isStranger && !isIgnored);

            btnLeft_->setVisible(leftButtonVisible_);
            btnRight_->setVisible(rightButtonVisible_);

            buttonsWidget_->setVisible(buttonsVisible_);
            break;

        default:
            buttonsWidget_->setVisible(false);
            break;
        }
    }

    int ChatEventItem::textFontSize() const
    {
        if (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger))
            return platform::is_apple() ? 14 : 15;

        return defaultFontSize();
    }

    int ChatEventItem::buttonsfontSize() const
    {
        if (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger))
            return 14;

        return defaultFontSize();
    }

    int ChatEventItem::getButtonsHeight() const
    {
        if (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger))
            return Utils::scale_value(44);

        return Utils::scale_value(40);
    }

    int ChatEventItem::lineSpacing() const
    {
        if (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger))
            return platform::is_apple() ? Utils::scale_value(4) : 0;

        return 0;
    }

    TextRendering::HorAligment ChatEventItem::textAligment() const
    {
        if (EventInfo_ && (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger || EventInfo_->eventType() == core::chat_event_type::no_longer_stranger))
        {
            const auto text = EventInfo_->formatEventText();
            return text.contains(QChar(0x2023)) ? TextRendering::HorAligment::LEFT : TextRendering::HorAligment::CENTER;
        }

        return TextRendering::HorAligment::CENTER;
    }

    bool ChatEventItem::membersLinksEnabled() const
    {
        if (EventInfo_)
        {
            switch (EventInfo_->eventType())
            {
                case core::chat_event_type::status_reply:
                case core::chat_event_type::custom_status_reply:
                    return false;
                default:
                    break;
            }
        }

        return true;
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

    QColor ChatEventItem::getTextColor(const QString& _contact)
    {
        return Styling::getParameters(_contact).getColor(Styling::StyleVariable::CHATEVENT_TEXT);
    }

    QColor ChatEventItem::getLinkColor(const QString& _contact)
    {
        return Styling::getParameters(_contact).getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    QFont ChatEventItem::getTextFont(int _size)
    {
        return Fonts::adjustedAppFont(_size == - 1 ? defaultFontSize() : _size, Fonts::defaultAppFontWeight(), Fonts::FontAdjust::NoAdjust);
    }

    QFont ChatEventItem::getTextFontBold(int _size)
    {
        return Fonts::adjustedAppFont(_size == -1 ? defaultFontSize() : _size, Fonts::FontWeight::SemiBold, Fonts::FontAdjust::NoAdjust);
    }

    void ChatEventItem::onChatInfo()
    {
        updateSize();
    }

    void ChatEventItem::modChatAboutResult(qint64 _seq, int)
    {
        if (_seq == modChatAboutSeq_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            const auto& aimid = getContact();
            collection.set_value_as_qstring("aimid", aimid);

            if (aimid.isEmpty())
            {
                const auto stamp = Logic::getContactListModel()->getChatStamp(aimid);
                if (stamp.isEmpty())
                    return;

                collection.set_value_as_qstring("stamp", stamp);
            }

            collection.set_value_as_int("limit", 0);
            Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
        }
    }

    void ChatEventItem::avatarChanged(const QString&)
    {
        updateSize();
    }

    void ChatEventItem::leftButtonClicked()
    {
        if (!EventInfo_)
            return;

        if (EventInfo_->eventType() == core::chat_event_type::mchat_invite)
        {
            auto avatar = new ContactAvatarWidget(this, EventInfo_->getAimId(), QString(), 0, true);
            avatar->selectFileForAvatar();
            auto pixmap = avatar->croppedImage();
            if (!pixmap.isNull())
                avatar->applyAvatar(pixmap);
        }
        else if (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger)
        {
            const auto& aimid = EventInfo_->getAimId();
            Logic::getContactListModel()->ignoreContact(aimid, true);
            ReportContact(aimid, Logic::GetFriendlyContainer()->getFriendly(aimid), false);
        }
    }

    void ChatEventItem::dlgStateChanged(const Data::DlgState& _state)
    {
        if (EventInfo_ && _state.AimId_ == EventInfo_->getAimId())
            updateSize();
    }

    void ChatEventItem::ignoreListChanged()
    {
        updateSize();
    }

    void ChatEventItem::rightButtonClicked()
    {
        if (!EventInfo_)
            return;

        if (EventInfo_->eventType() == core::chat_event_type::mchat_invite)
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
        else if (EventInfo_->eventType() == core::chat_event_type::warn_about_stranger)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", EventInfo_->getAimId());
            Ui::GetDispatcher()->post_message_to_core("dlg_state/close_stranger", collection.get());
        }
    }

    void ChatEventItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
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
        HistoryControlPageItem::mousePressEvent(_event);
    }

    void ChatEventItem::mouseMoveEvent(QMouseEvent* _event)
    {
        if (membersLinksEnabled() && TextWidget_->isOverLink(_event->pos()))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        HistoryControlPageItem::mouseMoveEvent(_event);
    }

    void ChatEventItem::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (membersLinksEnabled() && Utils::clicked(pressPos_, _event->pos()))
            TextWidget_->clicked(_event->pos());

        HistoryControlPageItem::mouseReleaseEvent(_event);
    }

    void ChatEventItem::clearSelection(bool)
    {
        TextWidget_->clearSelection();
    }
};

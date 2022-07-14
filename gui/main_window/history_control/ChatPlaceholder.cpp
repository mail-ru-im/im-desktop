#include "stdafx.h"

#include "ChatPlaceholder.h"
#include "ChatEventItem.h"
#include "ChatEventInfo.h"

#include "core_dispatcher.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "cache/avatars/AvatarStorage.h"
#include "cache/emoji/Emoji.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/containers/LastseenContainer.h"
#include "main_window/contact_list/ContactListModel.h"

#include "../common.shared/config/config.h"

#include "utils/stat_utils.h"
#include "../MainWindow.h"
#include "utils/InterConnector.h"

namespace
{
    QSize avatarSize()
    {
        return Utils::scale_value(QSize(144, 144));
    }

    int emojiSize()
    {
        return Utils::scale_bitmap_with_value(96);
    }

    int widgetMaxWidth()
    {
        return Utils::scale_value(328);
    }

    const QString& getSupportContact()
    {
        static const QString contact = []()
        {
            auto str = config::get().string(config::values::support_uin);
            return QString::fromUtf8(str.data(), str.size());
        }();
        return contact;
    }

    constexpr int maxAboutLength() noexcept
    {
        return 120;
    }

    const QString& clockEmoji()
    {
        static const auto str = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x23f0));
        return str;
    }

    const QString& crossEmoji()
    {
        static const auto str = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x274c));
        return str;
    }
}

namespace Ui
{
    ChatPlaceholderAvatar::ChatPlaceholderAvatar(QWidget* _parent, const QString& _contact)
        : QWidget(_parent)
        , contact_(_contact)
    {
        setFixedSize(avatarSize());

        setMouseTracking(true);

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, [this](const QString& _aimId)
        {
            if (_aimId == getContact())
               updatePixmap();
        });
    }

    void ChatPlaceholderAvatar::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.setBrush(Styling::getParameters(contact_).getColor(Styling::StyleVariable::CHATEVENT_BACKGROUND));
        p.drawEllipse(rect());

        const auto x = (width() - Utils::unscale_bitmap(pixmap_.width())) / 2;
        const auto y = (height() - Utils::unscale_bitmap(pixmap_.height())) / 2;

        p.drawPixmap(x, y, pixmap_);
    }

    void ChatPlaceholderAvatar::showEvent(QShowEvent* _event)
    {
        if (pixmap_.isNull())
            updatePixmap();
    }

    void ChatPlaceholderAvatar::mousePressEvent(QMouseEvent* _event)
    {
        if (containsCursor(_event->pos()))
            clicked_ = _event->pos();
    }

    void ChatPlaceholderAvatar::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(clicked_, _event->pos()) && !defaultAvatar_ && containsCursor(_event->pos()))
            Q_EMIT avatarClicked(QPrivateSignal());
    }

    void ChatPlaceholderAvatar::mouseMoveEvent(QMouseEvent* _event)
    {
        setCursor(containsCursor(_event->pos()) && !defaultAvatar_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void ChatPlaceholderAvatar::resizeEvent(QResizeEvent* _event)
    {
        region_ = QRegion(rect(), QRegion::Ellipse);
    }

    void ChatPlaceholderAvatar::updatePixmap()
    {
        if (state_ == PlaceholderState::hidden)
            return;

        if (state_ == PlaceholderState::avatar)
        {
            pixmap_ = Logic::GetAvatarStorage()->GetRounded(
                contact_,
                Logic::GetFriendlyContainer()->getFriendly(contact_),
                Utils::scale_bitmap(avatarSize().width()),
                defaultAvatar_,
                false,
                false);

            Testing::setAccessibleName(this, qsl("AS ChatPlaceholder avatar"));
        }
        else if (state_ == PlaceholderState::privateChat)
        {
            defaultAvatar_ = true;
            constexpr auto doorEmoji = Emoji::EmojiCode(0x1f6aa);
            pixmap_ = QPixmap::fromImage(Emoji::GetEmoji(doorEmoji, emojiSize()));
            Testing::setAccessibleName(this, qsl("AS ChatPlaceholder door"));
        }
        else
        {
            defaultAvatar_ = true;
            constexpr auto lockEmoji = Emoji::EmojiCode(0x1f512);
            pixmap_ = QPixmap::fromImage(Emoji::GetEmoji(lockEmoji, emojiSize()));
            Testing::setAccessibleName(this, qsl("AS ChatPlaceholder lock"));
        }

        Utils::check_pixel_ratio(pixmap_);

        update();
    }

    void ChatPlaceholderAvatar::setState(PlaceholderState _state)
    {
        if (state_ != _state)
        {
            im_assert(_state != PlaceholderState::hidden);
            state_ = _state;
            updatePixmap();
        }
    }

    bool ChatPlaceholderAvatar::containsCursor(const QPoint& _pos) const
    {
        return region_.contains(_pos);
    }

    ChatPlaceholder::ChatPlaceholder(QWidget* _parent, const QString& _contact)
        : QWidget(_parent)
        , avatar_(new ChatPlaceholderAvatar(this, _contact))
    {
        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(avatar_, 0, Qt::AlignCenter);

        setFixedWidth(widgetMaxWidth());

        if (_contact == getSupportContact())
        {
            updateCaptions();
        }
        else if (Utils::isChat(_contact))
        {
            connect(GetDispatcher(), &core_dispatcher::chatInfo, this, [this](const auto, const auto& _chatInfo)
            {
                if (_chatInfo->AimId_ == avatar_->getContact())
                {
                    avatar_->updatePixmap();
                    updateCaptions();
                }
            });
            updateCaptions();
        }
        else
        {
            connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, [this](const QString& _aimId)
            {
                if (!infoPending_ && contactAbout_.isEmpty() && _aimId == avatar_->getContact())
                    updateCaptions();
            });

            connect(GetDispatcher(), &core_dispatcher::userInfo, this, [this](const int64_t, const QString& _aimId, const Data::UserInfo& _info)
            {
                if (_aimId == avatar_->getContact())
                {
                    infoPending_ = false;
                    contactAbout_ = _info.about_;
                    updateCaptions();
                }
            });
            infoPending_ = true;
            Ui::GetDispatcher()->getUserInfo(_contact);
        }
        connect(avatar_, &ChatPlaceholderAvatar::avatarClicked, this, &ChatPlaceholder::avatarClicked);
    }

    void ChatPlaceholder::updateCaptions()
    {
        const auto& contact = avatar_->getContact();
        if (contact.isEmpty())
            return;

        if (contact == getSupportContact())
            setCaptions({ { QT_TRANSLATE_NOOP("placeholders", "Tell us about your problem. We will try to help you"), qsl("support") } });
        else if (Utils::isChat(contact))
            updateCaptionsChat();
        else if (!infoPending_)
            updateCaptionsContact();
    }

    void ChatPlaceholder::setCaptions(std::vector<Caption> _captions)
    {
        if (std::equal(captions_.begin(), captions_.end(), _captions.begin(), _captions.end(), [](const auto& c1, const auto& c2){ return c1.caption_ == c2.caption_; }))
            return;

        captions_ = std::move(_captions);

        clearCaptions();

        auto h = avatar_->height();
        const auto& contact = avatar_->getContact();
        const auto color = ChatEventItem::getTextColor(contact);
        for (const auto& c : captions_)
        {
            auto text = TextRendering::MakeTextUnit(Data::FString(c.caption_), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS,
                TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS, TextRendering::EmojiSizeType::REGULAR, ParseBackticksPolicy::ParseSingles);
            TextRendering::TextUnit::InitializeParameters params{ ChatEventItem::getTextFont(), color };
            params.monospaceFont_ = ChatEventItem::getTextFontBold();
            params.linkColor_ = color;
            params.align_ = TextRendering::HorAligment::CENTER;
            text->init(params);

            auto item = new ChatEventItem(this, std::move(text));
            item->setContact(contact);
            item->setFixedWidth(widgetMaxWidth());
            item->updateSize();
            h += item->sizeHint().height();

            Testing::setAccessibleName(item, u"AS ChatPlaceholder caption" % c.accessibleName_);

            layout()->addWidget(item);
        }

        setFixedHeight(h);

        adjustSize();
        updateGeometry();
        Q_EMIT resized(QPrivateSignal());
    }

    void ChatPlaceholder::clearCaptions()
    {
        const auto captions = findChildren<ChatEventItem*>();
        for (auto c : captions)
        {
            layout()->removeWidget(c);
            c->deleteLater();
        }
    }

    void ChatPlaceholder::updateCaptionsChat()
    {
        auto state = getState();
        if (state == PlaceholderState::hidden || state == PlaceholderState::avatar)
        {
            setCaptions({});
            return;
        }

        const auto isChannel = Logic::getContactListModel()->isChannel(avatar_->getContact());
        if (state == PlaceholderState::privateChat)
        {
            if (isChannel)
                setCaptions({ { QT_TRANSLATE_NOOP("placeholders", "Subscribe the channel to see the messages"), qsl("subChan") } });
            else
                setCaptions({ { QT_TRANSLATE_NOOP("placeholders", "Join the group to start chatting"), qsl("subGroup") } });
        }
        else
        {
            std::vector<Caption> lines;
            if (isChannel)
                lines.push_back({ QT_TRANSLATE_NOOP("placeholders", "This channel can be subscribed only after admin approval"), qsl("approval") });
            else
                lines.push_back({ QT_TRANSLATE_NOOP("placeholders", "This group can be joined only after admin approval"), qsl("approval") });

            if (state == PlaceholderState::pending)
            {
                if (isChannel)
                    lines.push_back({ QT_TRANSLATE_NOOP("chat_event", "%1 Wait for subscription request approval").arg(clockEmoji()), qsl("wait") });
                else
                    lines.push_back({ QT_TRANSLATE_NOOP("chat_event", "%1 Wait for join request approval").arg(clockEmoji()), qsl("wait") });
            }
            else if (state == PlaceholderState::rejected)
            {
                if (isChannel)
                    lines.push_back({ QT_TRANSLATE_NOOP("chat_event", "%1 Your subscription request was rejected").arg(crossEmoji()), qsl("rejected") });
                else
                    lines.push_back({ QT_TRANSLATE_NOOP("chat_event", "%1 Your join request was rejected").arg(crossEmoji()), qsl("rejected") });
            }

            setCaptions(std::move(lines));
        }
    }

    void ChatPlaceholder::updateCaptionsContact()
    {
        const auto& contact = avatar_->getContact();
        if (contact.isEmpty())
            return;

        if (contactAbout_.isEmpty())
        {
            auto name = Logic::GetFriendlyContainer()->getFriendly(contact);
            name.remove(u'`');
            Utils::removeLineBreaks(name);

            const QString pre = Logic::GetLastseenContainer()->isBot(contact)
                                ? QT_TRANSLATE_NOOP("placeholders", "Start working with")
                                : QT_TRANSLATE_NOOP("placeholders", "Start chatting with");
            setCaptions({ { pre % ql1s("\n`%1`").arg(name), qsl("start") } });

            return;
        }

        QString about = contactAbout_;
        if (about.size() > maxAboutLength())
        {
            about.truncate(maxAboutLength());
            about += getEllipsis();
        }

        if (Logic::GetLastseenContainer()->isBot(contact))
        {
            about.remove(u'`');
            about.prepend(u'`' % QT_TRANSLATE_NOOP("placeholders", "What this bot can do?") % u'`' % QChar::LineFeed);
        }

        setCaptions({ { std::move(about), qsl("about") } });
    }

    void ChatPlaceholder::avatarClicked()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_avatar_click, { { "chat_type", Utils::chatTypeByAimId(avatar_->getContact()) }});
        Utils::InterConnector::instance().getMainWindow()->openAvatar(avatar_->getContact());
    }

    void ChatPlaceholder::updateStyle()
    {
        captions_.clear();
        updateCaptions();
    }

    void ChatPlaceholder::setState(PlaceholderState _state)
    {
        if (getState() != _state)
        {
            avatar_->setState(_state);
            updateCaptions();
        }
    }

    PlaceholderState ChatPlaceholder::getState() const
    {
        return avatar_->getState();
    }

    void ChatPlaceholder::resizeEvent(QResizeEvent*)
    {
        Q_EMIT resized(QPrivateSignal());
    }
}
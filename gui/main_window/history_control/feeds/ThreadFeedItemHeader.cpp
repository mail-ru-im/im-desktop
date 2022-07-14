#include "types/chat.h"
#include "utils/utils.h"
#include "fonts.h"
#include "core_dispatcher.h"
#include "ThreadFeedItemHeader.h"
#include "main_window/containers/FriendlyContainer.h"
#include "cache/avatars/AvatarStorage.h"
#include "utils/PolygonRounder.h"

namespace
{
    int topMargin() noexcept { return Utils::scale_value(4); }

    int bottomMargin() noexcept { return Utils::scale_value(8); }

    constexpr int headerAvatarSize() { return 32; }

    int headerHeight() noexcept { return Utils::scale_value(headerAvatarSize() + 2 * 12); }

    int headerAvatarSizeScaled() noexcept { return Utils::scale_value(headerAvatarSize()); }

    int headerAvatarRightMargin() noexcept { return Utils::scale_value(8); }

    int headerLeftMargin() noexcept { return Utils::scale_value(8); }

    int headerRightMargin() noexcept { return Utils::scale_value(16); }

    int newMessagesPlateTopMargin() noexcept { return Utils::scale_value(4); }

    int newMessagesPlateBottomMargin() noexcept { return Utils::scale_value(16); }

    auto headerTextColor() { return Styling::ThemeColorKey { Styling::StyleVariable::TEXT_SOLID }; }

    const QColor& headerBackgroundColor(bool _hovered, bool _pressed)
    {
        if (_pressed)
        {
            static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE, 0.15);
            return c;
        }

        if (_hovered)
        {
            static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER, 0.15);
            return c;
        }

        static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY, 0.15);
        return c;
    }

    int headerTextOffsetV() noexcept { return Utils::scale_value(20); }

    int headerAvatarLeftMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    double chatThreadBubbleBorderRadius() noexcept
    {
        return (double)Utils::scale_value(8);
    }

    auto contactStatusFont()
    {
        return Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    }
}

namespace Ui
{
    ThreadFeedItemHeader::ThreadFeedItemHeader(const QString& _chatId, QWidget* _parent)
        : ClickableWidget(_parent)
        , chatId_(_chatId)
        , chatMembersCount_(0)
    {
        Testing::setAccessibleName(this, qsl("AS ThreadFeedItemHeader"));

        chatName_ = TextRendering::MakeTextUnit(
            Logic::GetFriendlyContainer()->getFriendly(_chatId),
            {},
            TextRendering::LinksVisible::DONT_SHOW_LINKS,
            TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        TextRendering::TextUnit::InitializeParameters params { Fonts::adjustedAppFont(16, Fonts::FontWeight::Medium), headerTextColor() };
        params.maxLinesCount_ = 1;
        chatName_->init(params);

        countMembers_ = TextRendering::MakeTextUnit(
            qsl(""),
            {},
            TextRendering::LinksVisible::DONT_SHOW_LINKS,
            TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        TextRendering::TextUnit::InitializeParameters countMembersParams { contactStatusFont(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } };
        countMembers_->init(countMembersParams);

        setFixedHeight(headerHeight());
        setCursor(Qt::PointingHandCursor);
        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addStretch();

        connect(this, &ThreadFeedItemHeader::pressed, this, [this]() { update(); });

        Ui::GetDispatcher()->getChatInfo(_chatId);
        connect(GetDispatcher(), &core_dispatcher::chatInfo, this, [this](const auto, const auto& _chatInfo){ updateFromChatInfo(_chatInfo); });
    }

    ThreadFeedItemHeader::~ThreadFeedItemHeader() = default;

    void ThreadFeedItemHeader::updateContent(int _width)
    {
        if (!chatName_ || !countMembers_)
            return;

        const auto availableWidth = _width - headerLeftMargin() - headerAvatarRightMargin() - headerAvatarSizeScaled() - headerRightMargin();
        const auto h = chatName_->getHeight(availableWidth);
        chatName_->getLastLineWidth();

        auto x = headerAvatarLeftMargin();
        avatarRect_ = QRect(x, (headerHeight() - headerAvatarSizeScaled()) / 2, headerAvatarSizeScaled(), headerAvatarSizeScaled());
        x += headerAvatarSizeScaled() + headerAvatarRightMargin();
        chatName_->setOffsets(x, headerTextOffsetV());
        countMembers_->setOffsets(x, headerHeight() / 2 + headerTextOffsetV() / 2);
    }

    void ThreadFeedItemHeader::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillPath(path_, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        bool isDefault;
        const auto avatarSize = Utils::scale_bitmap(headerAvatarSizeScaled());
        p.drawPixmap(
            avatarRect_,
            Logic::GetAvatarStorage()->GetRounded(chatId_, Logic::GetFriendlyContainer()->getFriendly(chatId_), avatarSize, isDefault, false, false));

        chatName_->draw(p, TextRendering::VerPosition::MIDDLE);
        countMembers_->draw(p, TextRendering::VerPosition::MIDDLE);

        p.setRenderHint(QPainter::Antialiasing, false);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT), Utils::scale_value(1)));
        p.drawLine(rect().bottomLeft(), rect().bottomRight());
    }

    void ThreadFeedItemHeader::resizeEvent(QResizeEvent* _event)
    {
        const double radius = chatThreadBubbleBorderRadius();

        Utils::PolygonRounder rounder;
        path_ = rounder(rect(), { radius, radius });
        updateContent(_event->size().width());
    }

    void ThreadFeedItemHeader::updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info || _info->AimId_ != chatId_)
            return;
        chatMembersCount_ = Data::ChatInfo::areYouMember(_info) ? _info->MembersCount_ : 0;
        countMembers_->setText(Utils::getMembersString(chatMembersCount_, _info->isChannel()));
    }
}

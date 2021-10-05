#include "stdafx.h"

#include "AssigneeEdit.h"
#include "../../utils/utils.h"
#include "../containers/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../contact_list/TaskAssigneeModel.h"

namespace
{
    auto avatarSize() noexcept
    {
        return Utils::scale_value(20);
    }

    auto avatarRightMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto textMarginWithAvatar() noexcept
    {
        return avatarSize() + avatarRightMargin();
    }

    constexpr auto textMarginWithoutAvatar() noexcept
    {
        return 0;
    }
}

namespace Ui
{
    AssigneeEdit::AssigneeEdit(QWidget* _parent, const Options& _options)
        : LineEditEx(_parent, _options)
    {
        setRenderMargins(RenderMargins::TextMargin);
        connect(this, &LineEditEx::textChanged, this, &AssigneeEdit::resetSelection);
    }

    AssigneeEdit::~AssigneeEdit() = default;

    void AssigneeEdit::selectContact(const QString& _aimId)
    {
        if (Logic::TaskAssigneeModel::noAssigneeAimId() == _aimId)
        {
            resetSelection();
            setText({});
            return;
        }

        if (selectedContact_ && *selectedContact_ == _aimId)
            return;

        const auto friendly = Logic::GetFriendlyContainer()->getFriendly(_aimId);
        setText(friendly);
        selectedContact_ = _aimId;

        int left, top, right, bottom;
        getTextMargins(&left, &top, &right, &bottom);
        QMargins lineEditMargins(textMarginWithAvatar(), top, right, bottom);
        setTextMargins(lineEditMargins);

        auto isDefault = false;
        avatar_ = Logic::GetAvatarStorage()->GetRounded(_aimId, friendly, Utils::scale_bitmap(avatarSize()), isDefault, false, false);

        update();
        Q_EMIT selectedContactChanged(_aimId);
    }

    std::optional<QString> AssigneeEdit::selectedContact() const
    {
        return selectedContact_;
    }

    void AssigneeEdit::paintEvent(QPaintEvent* _event)
    {
        LineEditEx::paintEvent(_event);

        if (!avatar_.isNull())
        {
            QPainter p(this);
            p.drawPixmap({ 0, 0, avatarSize(), avatarSize() }, avatar_);
        }
    }

    void AssigneeEdit::resetSelection()
    {
        if (!selectedContact_)
            return;

        selectedContact_.reset();
        avatar_ = {};
        int left, top, right, bottom;
        getTextMargins(&left, &top, &right, &bottom);
        QMargins lineEditMargins(textMarginWithoutAvatar(), top, right, bottom);
        setTextMargins(lineEditMargins);
        update();
        Q_EMIT selectedContactChanged({});
    }
}

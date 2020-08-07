#include "stdafx.h"
#include "Sidebar.h"
#include "SidebarUtils.h"
#include "GroupProfile.h"
#include "UserProfile.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/SemitransparentWindowAnimated.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../MainPage.h"

namespace
{
    constexpr int sidebar_width = 328;

    constexpr int ANIMATION_DURATION = 200;
    constexpr int max_step = 200;
    constexpr int min_step = 0;

    enum SidebarPages
    {
        group_profile = 0,
        user_profile = 1,
    };
}

namespace Ui
{
    Sidebar::Sidebar(QWidget* _parent)
        : QStackedWidget(_parent)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, 0))
        , animSidebar_(new QPropertyAnimation(this, QByteArrayLiteral("anim"), this))
        , anim_(min_step)
        , needShadow_(true)
        , frameCountMode_(FrameCountMode::_1)
    {
        semiWindow_->hide();
        Testing::setAccessibleName(semiWindow_, qsl("AS Sidebar semiWindow"));

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setAttribute(Qt::WA_NoMousePropagation);

        Utils::setDefaultBackground(this);

        animSidebar_->setDuration(ANIMATION_DURATION);

        {
            pages_.insert(group_profile, new GroupProfile(this));
        }

        {
            auto userProfile = new UserProfile(this);
            connect(userProfile, &UserProfile::headerBackButtonClicked, this, &Sidebar::backButtonClicked);
            pages_.insert(user_profile, userProfile);
        }

        for (auto page = pages_.cbegin(), end = pages_.cend(); page != end; ++page)
            insertWidget(page.key(), page.value());

        setWidth(getDefaultWidth());

        setParent(semiWindow_);
        connect(animSidebar_, &QPropertyAnimation::finished, this, &Sidebar::onAnimationFinished);
    }

    void Sidebar::showAnimated()
    {
        updateSize();

        if (isNeedShadow())
        {
            semiWindow_->raise();
            semiWindow_->Show();
        }

        show();
    }

    void Sidebar::hideAnimated()
    {
        if (isNeedShadow())
            semiWindow_->Hide();

        for (const auto& p : pages_)
            p->close();

        currentAimId_.clear();
        hide();
    }

    void Sidebar::updateSize()
    {
        semiWindow_->updateSize();
        move(semiWindow_->width() - width(), 0);
        setFixedHeight(semiWindow_->height());
    }

    bool Sidebar::isNeedShadow() const noexcept
    {
        return needShadow_;
    }

    void Sidebar::setNeedShadow(bool _value)
    {
        if (needShadow_ != _value)
        {
            needShadow_ = _value;
            const auto visible = isVisible();
            if (needShadow_)
                setParent(semiWindow_);
            if (visible)
            {
                if (needShadow_)
                {
                    semiWindow_->Show();
                }
                else
                {
                    semiWindow_->Hide();
                }
            }
        }
    }

    int Sidebar::getDefaultWidth()
    {
        return Utils::scale_value(sidebar_width);
    }

    QString Sidebar::getSelectedText() const
    {
        return pages_[currentIndex()]->getSelectedText();
    }

    void Sidebar::onAnimationFinished()
    {
        if (animSidebar_->endValue() == min_step)
            hide();
    }

    void Sidebar::preparePage(const QString& aimId, SidebarParams _params, bool _selectionChanged)
    {
        auto index = Utils::isChat(aimId) ? group_profile : user_profile;
        auto page = pages_[index];
        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        page->setPrev(_selectionChanged || aimId == selectedContact || currentAimId_ == aimId ? QString() : currentAimId_);

        page->initFor(!aimId.isEmpty() ? aimId : selectedContact, std::move(_params));
        setCurrentIndex(index);
        currentAimId_ = !aimId.isEmpty() ? aimId : selectedContact;

        // close inactive sidebar pages
        for (auto i = 0; i < count(); ++i)
            if (i != index && pages_[i]->isActive())
                pages_[i]->close();
    }

    void Sidebar::showMembers()
    {
        pages_[currentIndex()]->toggleMembersList();
    }

    QString Sidebar::currentAimId() const
    {
        return currentAimId_;
    }

    void Sidebar::setAnim(int _val)
    {
        anim_ = _val;
        auto w = width() * (anim_ / (double)max_step);
        move(semiWindow_->width() - w, 0);
    }

    int Sidebar::getAnim() const
    {
        return anim_;
    }

    void Sidebar::setFrameCountMode(FrameCountMode _mode)
    {
        frameCountMode_ = _mode;
        for (auto page : std::as_const(pages_))
            page->setFrameCountMode(_mode);
    }

    void Sidebar::setWidth(int _width)
    {
        if (width() != _width)
            setFixedWidth(_width);
    }
}

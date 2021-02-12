#include "stdafx.h"
#include "Sidebar.h"
#include "SidebarUtils.h"
#include "GroupProfile.h"
#include "UserProfile.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/SemitransparentWindowAnimated.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/animations/SlideController.h"
#include "../../styles/ThemeParameters.h"
#include "../MainPage.h"

namespace
{
    constexpr int kSidebarWidth = 328;
    constexpr int kMaxCacheSize = 2;
    constexpr std::chrono::milliseconds kSlideDuration = std::chrono::milliseconds(125);
    constexpr std::chrono::milliseconds kFadeDuration = std::chrono::milliseconds(100);

    enum SidebarPages
    {
        SP_GroupProfile = 0,
        SP_UserProfile = 1,
    };
}

namespace Ui
{
    class SidebarPageCreatorBase
    {
    public:
        virtual ~SidebarPageCreatorBase() {}
        virtual SidebarPage* create(QWidget* _parent = nullptr) const = 0;
    };

    template<class _Page>
    class SidebarPageCreator : public SidebarPageCreatorBase
    {
    public:
        SidebarPage* create(QWidget* _parent = nullptr) const
        {
            return new _Page(_parent);
        }
    };

    class SidebarPageFactory
    {
        using SidebarPageCreatorBasePtr = std::unique_ptr<SidebarPageCreatorBase>;
        using CreatorMap = std::unordered_map<int, SidebarPageCreatorBasePtr>;
    public:
        void registrate(int _key, SidebarPageCreatorBasePtr _creator)
        {
            auto it = creators_.find(_key);
            if (it == creators_.end())
            {
                creators_.emplace(_key, std::move(_creator));
                return;
            }

            if (_creator == nullptr)
                creators_.erase(it);
            else
                it->second = std::move(_creator);
        }

        template<class _Page>
        void registrate(int _key)
        {
            registrate(_key, std::make_unique<SidebarPageCreator<_Page>>());
        }

        SidebarPage* createPage(int _key, QWidget* _parent) const
        {
            auto it = creators_.find(_key);
            return (it != creators_.end() ? it->second->create(_parent) : nullptr);
        }

        bool contains(int _key) const
        {
            return (creators_.find(_key) == creators_.end());
        }

        bool empty() const
        {
            return creators_.empty();
        }

    private:
        CreatorMap creators_;
    };


    class SidebarPrivate
    {
    public:
        QPixmap backbuffer_;
        QString currentAimId_;
        Sidebar* qptr_;
        SemitransparentWindowAnimated* semiWindow_;
        QVariantAnimation* animSidebar_;
        Utils::SlideController* slideController_;
        FrameCountMode frameCountMode_;
        bool needShadow_;

        static SidebarPageFactory factory_;

        SidebarPrivate(Sidebar* _q, QWidget* _parent)
            : qptr_(_q)
            , semiWindow_(new SemitransparentWindowAnimated(_parent, 0))
            , animSidebar_(new QVariantAnimation(_q))
            , slideController_(new Utils::SlideController(_q))
            , frameCountMode_(FrameCountMode::_1)
            , needShadow_(true)
        {
        }

        SidebarPage* pageWidget(int _index) const
        {
            return static_cast<SidebarPage*>(qptr_->widget(_index));
        }

        void removePage(int _index)
        {
            QWidget* w = qptr_->widget(_index);
            if (w != nullptr)
            {
                qptr_->removeWidget(w);
                delete w;
            }
        }

        SidebarPage* createPage(int _key) const
        {
            SidebarPage* page = factory_.createPage(_key, qptr_);
            if (page != nullptr)
                QObject::connect(page, &SidebarPage::contentsChanged, qptr_, &Sidebar::onContentChanged);
            return page;
        }

        int findPage(std::string_view _name, const QVariant& _value) const
        {
            for (int i = 0, n = qptr_->count(); i < n; ++i)
                if (qptr_->widget(i)->property(_name.data()) == _value)
                    return i;
            return -1;
        }

        int changePage(const QString& _aimId, SidebarParams _params, bool _selectionChanged)
        {
            const auto key = Utils::isChat(_aimId) ? SP_GroupProfile : SP_UserProfile;

            const auto& selectedContact = Logic::getContactListModel()->selectedContact();
            const auto& aimId = (!_aimId.isEmpty() ? _aimId : selectedContact);
            const bool hasChanges = _selectionChanged || _aimId == selectedContact || currentAimId_ == _aimId;

            SidebarPage* page = pageWidget(qptr_->currentIndex());

            int index = findPage("aimId", aimId);
            if (index != -1)
            {
                page = pageWidget(index);
                page->scrollToTop();

                page->setPrev(hasChanges ? QString() : currentAimId_);
                // trigger request for user/chat info
                page->initFor(!_aimId.isEmpty() ? _aimId : selectedContact, std::move(_params));
                if (index == qptr_->currentIndex())
                    return -1;
            }
            else
            {
                index = qptr_->count();
                page = createPage(key);
                page->setProperty("aimId", aimId);
                page->resize(qptr_->size());

                page->setPrev(hasChanges ? QString() : currentAimId_);
                // trigger request for user/chat info
                page->initFor(!_aimId.isEmpty() ? _aimId : selectedContact, std::move(_params));

                const int n = qptr_->count();
                if (n >= kMaxCacheSize)
                {
                    for (index = 0; index < n; ++index)
                        if (index != qptr_->currentIndex())
                            break;
                    assert(index != n);
                    removePage(index);
                }

                qptr_->insertWidget(index, page);
            }
            currentAimId_ = aimId;

            return index;
        }

        void showPage()
        {
            if (qptr_->currentWidget())
            {
                qptr_->currentWidget()->show();
                updatePixmap();
            }
        }

        void hidePage()
        {
            if (qptr_->currentWidget())
                qptr_->currentWidget()->hide();
        }

        void updatePixmap()
        {
            if (animSidebar_->state() == QAbstractAnimation::Running)
                return;

            if (qptr_->width() == 0)
                qptr_->setFixedWidth(qptr_->getDefaultWidth());
            backbuffer_ = qptr_->grab();
        }

        void initializeSlideController()
        {
            using Utils::SlideController;
            slideController_->setWidget(qptr_);
            slideController_->setDuration(kFadeDuration);
            slideController_->setCachingPolicy(SlideController::CachingPolicy::CacheNone);
            slideController_->setFading(SlideController::Fading::FadeIn);
            slideController_->setEffects(SlideController::SlideEffect::NoEffect);
            slideController_->setSlideDirection(SlideController::SlideDirection::NoSliding);
        }

        void initializeAnimationController()
        {
            animSidebar_->setDuration(kSlideDuration.count());
            animSidebar_->setStartValue(0);
            animSidebar_->setEndValue(qptr_->getDefaultWidth());
            QObject::connect(animSidebar_, &QVariantAnimation::valueChanged, qptr_, &Sidebar::animate);
            QObject::connect(animSidebar_, &QVariantAnimation::finished, qptr_, &Sidebar::onAnimationFinished);
        }

        void initializePageFactory()
        {
            if (!factory_.empty())
                return;

            factory_.registrate<GroupProfile>(SP_GroupProfile);
            factory_.registrate<UserProfile>(SP_UserProfile);
        }
    };

    SidebarPageFactory SidebarPrivate::factory_;

    Sidebar::Sidebar(QWidget* _parent)
        : QStackedWidget(_parent)
        , d(std::make_unique<SidebarPrivate>(this, _parent))
    {
        setStyleSheet(qsl("background-color: %1;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));

        d->initializeAnimationController();
        d->initializeSlideController();
        d->initializePageFactory();

        d->semiWindow_->hide();
        Testing::setAccessibleName(d->semiWindow_, qsl("AS Sidebar semiWindow"));

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setAttribute(Qt::WA_NoMousePropagation);

        Utils::setDefaultBackground(this);

        setWidth(getDefaultWidth());
        setParent(d->semiWindow_);
    }

    Sidebar::~Sidebar() = default;

    void Sidebar::showAnimated()
    {
        if (isVisible())
            return;

        if (d->frameCountMode_ == FrameCountMode::_1)
        {
            setFixedSize(d->semiWindow_->size());
            show();
            d->showPage();
            return;
        }

        if (isNeedShadow())
        {
            d->semiWindow_->raise();
            d->semiWindow_->showAnimated();
        }

        d->showPage();
        setFixedWidth(getDefaultWidth());
        d->updatePixmap();

        setFixedWidth(0);

        d->animSidebar_->setDirection(QVariantAnimation::Forward);
        d->animSidebar_->start();

        show();
        d->hidePage();
    }

    void Sidebar::hideAnimated()
    {
        if (!isVisible())
            return;

        if (d->frameCountMode_ == FrameCountMode::_1)
        {
            hide();
            return;
        }
        d->updatePixmap();
        d->hidePage();
        d->animSidebar_->setDirection(QVariantAnimation::Backward);
        d->animSidebar_->start();
    }

    void Sidebar::animate(const QVariant& value)
    {
        if (d->frameCountMode_ == FrameCountMode::_1)
        {
            update();
        }
        else
        {
            d->semiWindow_->updateSize();
            move(d->semiWindow_->width() - value.toInt(), 0);
            setFixedWidth(value.toInt());
        }
    }

    void Sidebar::onAnimationFinished()
    {
        if (d->animSidebar_->direction() == QVariantAnimation::Forward)
        {
            d->showPage();
        }
        else
        {
            SidebarPage* page = d->pageWidget(currentIndex());
            if (page->isMembersVisible())
                page->setMembersVisible(false);
            page->close();

            hide();
            if (isNeedShadow())
                d->semiWindow_->hideAnimated();
            while (count() > 0)
            {
                QWidget* w = widget(0);
                removeWidget(w);
                delete w;
            }
            d->backbuffer_ = QPixmap();
        }
    }

    void Sidebar::onCurrentChanged(int _index)
    {
        setCurrentIndex(_index);
        if (d->frameCountMode_ == FrameCountMode::_1)
            setFixedSize(d->semiWindow_->size());
        else
            setFixedWidth(getDefaultWidth());

        d->updatePixmap();
    }

    void Sidebar::onContentChanged()
    {
        if (width() != 0)
            d->updatePixmap();
    }

    void Sidebar::updateSize()
    {
        d->semiWindow_->updateSize();
        move(d->semiWindow_->width() - width(), 0);
        setFixedHeight(d->semiWindow_->height());
    }

    bool Sidebar::isNeedShadow() const noexcept
    {
        return d->needShadow_;
    }

    void Sidebar::setNeedShadow(bool _value)
    {
        if (d->needShadow_ == _value)
            return;

        d->needShadow_ = _value;
        const auto visible = isVisible();
        if (d->needShadow_)
            setParent(d->semiWindow_);

        if (visible)
        {
            if (d->needShadow_)
                d->semiWindow_->showAnimated();
            else
                d->semiWindow_->hideAnimated();
        }
    }

    int Sidebar::getDefaultWidth()
    {
        return Utils::scale_value(kSidebarWidth);
    }

    QString Sidebar::getSelectedText() const
    {
        SidebarPage* page = d->pageWidget(currentIndex());
        return (page != nullptr ? page->getSelectedText() : QString());
    }

    void Sidebar::paintEvent(QPaintEvent* _event)
    {
        static const double factor = 1.0 / (double)getDefaultWidth();
        if (d->animSidebar_->state() == QVariantAnimation::Running)
        {
            double w = d->animSidebar_->currentValue().toInt();
            QPainter painter(this);
            painter.setOpacity(w * factor);
            painter.drawPixmap(0, 0, d->backbuffer_);
        }
        else
        {
            QStackedWidget::paintEvent(_event);
        }
    }

    void Sidebar::preparePage(const QString& _aimId, SidebarParams _params, bool _selectionChanged)
    {
        const int index = d->changePage(_aimId, _params, _selectionChanged);
        if (index != -1)
        {
            if (isVisible())
                QTimer::singleShot(SidebarPage::kLoadDelay, [guard = QPointer(this), index] { if (guard) guard->onCurrentChanged(index); });
            else
                onCurrentChanged(index);
        }
        if (auto page = d->pageWidget(index == -1 ? currentIndex() : index))
            if (page->isMembersVisible())
                page->setMembersVisible(false);
    }

    void Sidebar::showMembers(const QString& _aimId)
    {
        const int index = d->changePage(_aimId, {}, false);
        const int pageIndex = index != -1 ? index : currentIndex();
        const bool membersVisible = d->pageWidget(pageIndex)->isMembersVisible();
        if (!isVisible())
        {
            if (!membersVisible)
                d->pageWidget(pageIndex)->setMembersVisible(true);
            if (index != -1)
                onCurrentChanged(index);
            showAnimated();
        }
        else
        {
            if (membersVisible)
                hideAnimated();
            else
                d->pageWidget(pageIndex)->setMembersVisible(true);

            if (index != -1)
                onCurrentChanged(index);
        }
    }

    QString Sidebar::currentAimId() const
    {
        return d->currentAimId_;
    }

    void Sidebar::setFrameCountMode(FrameCountMode _mode)
    {
        d->frameCountMode_ = _mode;
        for (int i = 0, n = count(); i < n; ++i)
            d->pageWidget(i)->setFrameCountMode(_mode);

        if (isVisible() && width() > 0)
            d->backbuffer_ = grab();
    }

    void Sidebar::setWidth(int _width)
    {
        if (width() != _width)
            setFixedWidth(_width);
    }
}
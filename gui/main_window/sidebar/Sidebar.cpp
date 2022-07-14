#include "stdafx.h"
#include "Sidebar.h"
#include "SidebarUtils.h"
#include "GroupProfile.h"
#include "UserProfile.h"
#include "ThreadPage.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/SemitransparentWindowAnimated.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/animations/SlideController.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../MainWindow.h"
#ifdef HAS_WEB_ENGINE
#include "../WebAppPage.h"
#endif
#include "core_dispatcher.h"

namespace
{
    constexpr int kMaxCacheSize = 2;
    constexpr std::chrono::milliseconds kSlideDuration = std::chrono::milliseconds(125);
    constexpr std::chrono::milliseconds kFadeDuration = std::chrono::milliseconds(100);

    enum SidebarPages
    {
        SP_GroupProfile = 0,
        SP_UserProfile = 1,
        SP_ThreadDialog = 2,
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
        int desiredWidth_ = 0;
        bool needShadow_;
        bool wasVisible_ = false;

        static SidebarPageFactory factory_;

        SidebarPrivate(Sidebar* _q, QWidget* _parent)
            : qptr_(_q)
            , semiWindow_(new SemitransparentWindowAnimated(_parent, 0))
            , animSidebar_(new QVariantAnimation(_q))
            , slideController_(new Utils::SlideController(_q))
            , frameCountMode_(FrameCountMode::_1)
            , needShadow_(true)
        {
            QObject::connect(slideController_, &Utils::SlideController::currentIndexChanged, qptr_, &Sidebar::onIndexChanged);
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
            const auto prefixIdx = _aimId.indexOf(Sidebar::getThreadPrefix());
            const auto key = prefixIdx != -1 ? SP_ThreadDialog : Utils::isChat(_aimId) ? SP_GroupProfile : SP_UserProfile;

            const auto& selectedContact = Logic::getContactListModel()->selectedContact();
            const auto& aimId = _aimId.isEmpty() ? selectedContact : _aimId;

            auto showProfileFromTasksPageThreadInfo = false;
            #ifdef HAS_WEB_ENGINE
            if (Utils::InterConnector::instance().getMainWindow()->isWebAppTasksPage())
            {
                const auto tasksPage = Utils::InterConnector::instance().getTasksPage();
                showProfileFromTasksPageThreadInfo = tasksPage ? tasksPage->isProfileFromThreadInfo() : true;
            }
            #endif

            const bool hasChanges = !showProfileFromTasksPageThreadInfo || _selectionChanged || _aimId == selectedContact || currentAimId_ == _aimId;
            const auto initAimid = prefixIdx != -1 ? _aimId.mid(prefixIdx + Sidebar::getThreadPrefix().size()) : aimId;

            SidebarPage* page = pageWidget(qptr_->currentIndex());

            int index = findPage("aimId", aimId);
            if (index != -1)
            {
                page = pageWidget(index);
                page->scrollToTop();

                page->setPrev(hasChanges ? QString() : currentAimId_);
                // trigger request for user/chat info
                page->initFor(initAimid, std::move(_params));
                page->setFrameCountMode(frameCountMode_);
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
                page->initFor(initAimid, std::move(_params));
                page->setFrameCountMode(frameCountMode_);

                const int n = qptr_->count();
                if (n >= kMaxCacheSize)
                {
                    for (index = 0; index < n; ++index)
                        if (index != qptr_->currentIndex())
                            break;
                    im_assert(index != n);
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
                qptr_->setFixedWidth(qptr_->getCurrentWidth());
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
            slideController_->setFillColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        }

        void initializeAnimationController()
        {
            animSidebar_->setDuration(kSlideDuration.count());
            animSidebar_->setStartValue(0);
            animSidebar_->setEndValue(qptr_->getCurrentWidth());
            QObject::connect(animSidebar_, &QVariantAnimation::valueChanged, qptr_, &Sidebar::animate);
            QObject::connect(animSidebar_, &QVariantAnimation::finished, qptr_, &Sidebar::onAnimationFinished);

            auto updateWidth = [anim = animSidebar_, q = qptr_]()
            {
                anim->setEndValue(q->getCurrentWidth());
            };
            QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, animSidebar_, updateWidth);
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, animSidebar_, updateWidth);
        }

        void initializePageFactory()
        {
            if (!factory_.empty())
                return;

            factory_.registrate<GroupProfile>(SP_GroupProfile);
            factory_.registrate<UserProfile>(SP_UserProfile);
            factory_.registrate<ThreadPage>(SP_ThreadDialog);
        }
    };

    SidebarPageFactory SidebarPrivate::factory_;

    Sidebar::Sidebar(QWidget* _parent)
        : QStackedWidget(_parent)
        , IEscapeCancellable(this)
        , d(std::make_unique<SidebarPrivate>(this, _parent))
    {
        d->initializeAnimationController();
        d->initializeSlideController();
        d->initializePageFactory();

        d->semiWindow_->hide();
        Testing::setAccessibleName(d->semiWindow_, qsl("AS Sidebar semiWindow"));

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setAttribute(Qt::WA_NoMousePropagation);

        restoreWidth();
        setParent(d->semiWindow_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &Sidebar::changeHeight);
        connect(d->animSidebar_, &QVariantAnimation::finished, this, [this]()
        {
            Q_EMIT visibilityAnimationFinished(isVisible());
        });

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &Sidebar::onThemeChanged);
    }

    Sidebar::~Sidebar() = default;

    void Sidebar::showAnimated()
    {
        if (d->frameCountMode_ == FrameCountMode::_1)
        {
            show();
            return;
        }

        if (isVisible())
            return;

        d->wasVisible_ = true;

        if (isNeedShadow())
        {
            d->semiWindow_->raise();
            d->semiWindow_->showAnimated();
        }

        d->showPage();
        setFixedWidth(getCurrentWidth());
        d->updatePixmap();

        setFixedWidth(0);

        d->animSidebar_->setDirection(QVariantAnimation::Forward);
        d->animSidebar_->start();

        show();
        d->hidePage();
    }

    void Sidebar::hideAnimated()
    {
        if (d->frameCountMode_ == FrameCountMode::_1)
        {
            d->semiWindow_->hideAnimated();
            hide();
            return;
        }

        if (!isVisible())
            return;

        d->wasVisible_ = false;
        d->updatePixmap();
        d->hidePage();
        d->animSidebar_->setDirection(QVariantAnimation::Backward);
        d->animSidebar_->start();
    }

    void Sidebar::show()
    {
        if (isVisible())
            return;

        if (isNeedShadow() && !d->semiWindow_->isVisible())
        {
            d->semiWindow_->raise();
            d->semiWindow_->showAnimated();
        }
        d->wasVisible_ = true;
        updateSizePrivate();
        QWidget::show();
        d->showPage();
    }

    void Sidebar::hide()
    {
        d->semiWindow_->hideAnimated();
        d->wasVisible_ = false;
        QWidget::hide();
        Q_EMIT hidden();
    }

    void Sidebar::updateSizePrivate()
    {
        if (isNeedShadow())
            setFixedHeight(d->semiWindow_->height());
        else
            setFixedHeight(d->semiWindow_->height() - Utils::InterConnector::instance().getHeaderHeight());
    }

    void Sidebar::moveToWindowEdge()
    {
        move(d->semiWindow_->width() - width(), 0);
    }

    void Sidebar::closePage(SidebarPage* _page)
    {
        if (_page)
        {
            _page->close();
            escCancel_->removeChild(_page);
        }
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
            setFixedWidth(value.toInt());
            moveToWindowEdge();
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
            auto page = d->pageWidget(currentIndex());
            if (page && page->isMembersVisible())
                page->setMembersVisible(false);

            closePage(page);

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
            updateSizePrivate();

        d->updatePixmap();

        if (auto page = d->pageWidget(_index))
            escCancel_->addChild(page);
    }

    void Sidebar::onContentChanged()
    {
        if (width() != 0)
            d->updatePixmap();
    }

    void Sidebar::onIndexChanged(int _index)
    {
        if (auto page = d->pageWidget(_index))
            page->onPageBecomeVisible();
    }

    void Sidebar::onThemeChanged()
    {
        if (auto page = d->pageWidget(currentIndex()))
            page->updateWidgetsTheme();
    }

    void Sidebar::updateSize()
    {
        d->semiWindow_->updateSize();
        if (d->frameCountMode_ == FrameCountMode::_2)
            moveToWindowEdge();
        updateSizePrivate();
    }

    void Sidebar::changeHeight()
    {
        d->semiWindow_->updateSize();
        updateSizePrivate();
    }

    bool Sidebar::isThreadOpen() const
    {
        return currentAimId().startsWith(getThreadPrefix());
    }

    bool Sidebar::wasVisible() const noexcept
    {
        return d->wasVisible_;
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
        {
            setParent(d->semiWindow_);
            moveToWindowEdge();
        }

        if (visible || d->wasVisible_)
        {
            if (d->needShadow_)
                d->semiWindow_->showAnimated();
            else
                d->semiWindow_->hideAnimated();
        }

        updateSize();
    }

    int Sidebar::getDefaultWidth() noexcept
    {
        return Features::isThreadsEnabled() ? Utils::scale_value(380) : Utils::scale_value(328);
    }

    int Sidebar::getDefaultMaximumWidth() noexcept
    {
        return Utils::scale_value(581);
    }

    QString Sidebar::getSelectedText() const
    {
        SidebarPage* page = d->pageWidget(currentIndex());
        return (page ? page->getSelectedText() : QString());
    }

    bool Sidebar::hasInputOrHistoryFocus() const
    {
        SidebarPage* page = d->pageWidget(currentIndex());
        return page && page->hasInputOrHistoryFocus();
    }

    bool Sidebar::hasSearchFocus() const
    {
        SidebarPage* page = d->pageWidget(currentIndex());
        return page && page->hasSearchFocus();
    }

    void Sidebar::paintEvent(QPaintEvent* _event)
    {
        if (d->animSidebar_->state() == QVariantAnimation::Running)
        {
            const double factor = 1.0 / (double)getCurrentWidth();
            const double w = d->animSidebar_->currentValue().toInt();
            QPainter painter(this);
            painter.setOpacity(w * factor);
            painter.drawPixmap(0, 0, d->backbuffer_);
        }
        else
        {
            QStackedWidget::paintEvent(_event);
        }
    }

    void Sidebar::keyPressEvent(QKeyEvent* _event)
    {
        auto w = Utils::InterConnector::instance().getMainWindow();
        if (w && w->isWebAppTasksPage() && _event->key() == Qt::Key_Escape)
            Q_EMIT Utils::InterConnector::instance().closeSidebar();

        QStackedWidget::keyPressEvent(_event);
    }

    void Sidebar::preparePage(const QString& _aimId, SidebarParams _params, bool _selectionChanged)
    {
        const int index = d->changePage(_aimId, _params, _selectionChanged);
        if (index != -1)
        {
            auto changePage = [_aimId, index, this]()
            {
                onCurrentChanged(index);

                if (const auto page = d->pageWidget(currentIndex()))
                {
                    if (const auto& prev = page->prev(); !prev.isEmpty())
                    {
                        if (const auto index = d->findPage("aimId", prev); index != -1)
                            closePage(d->pageWidget(index));
                    }
                }

                Q_EMIT pageChanged(_aimId, QPrivateSignal());
            };

            if (isVisible())
                QTimer::singleShot(SidebarPage::kLoadDelay, [guard = QPointer(this), changePage] { if (guard) changePage(); });
            else
                changePage();
        }
        if (auto page = d->pageWidget(index == -1 ? currentIndex() : index))
        {
            if (page->isMembersVisible())
                page->setMembersVisible(false);
        }
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

    QString Sidebar::currentAimId(ResolveThread _resolve) const
    {
        if (_resolve == ResolveThread::Yes && isThreadOpen())
            return d->currentAimId_.mid(d->currentAimId_.indexOf(Sidebar::getThreadPrefix()) + Sidebar::getThreadPrefix().size());

        return d->currentAimId_;
    }

    QString Sidebar::parentAimId() const
    {
        if (auto page = d->pageWidget(currentIndex()))
            return page->parentAimId();
        return {};
    }

    FrameCountMode Sidebar::getFrameCountMode() const
    {
        return d->frameCountMode_;
    }

    void Sidebar::setFrameCountMode(FrameCountMode _mode)
    {
        d->frameCountMode_ = _mode;
        for (int i = 0, n = count(); i < n; ++i)
            d->pageWidget(i)->setFrameCountMode(_mode);

        if ((isVisible() || wasVisible()) && width() > 0)
            d->backbuffer_ = grab();
    }

    void Sidebar::setWidth(int _width)
    {
        if (width() != _width)
            setFixedWidth(_width);
    }

    void Sidebar::saveWidth()
    {
        d->desiredWidth_ = width();
        d->animSidebar_->setEndValue(getCurrentWidth());
    }

    void Sidebar::restoreWidth()
    {
        setWidth(getCurrentWidth());
    }

    int Sidebar::getCurrentWidth() const
    {
        return std::clamp(d->desiredWidth_, getDefaultWidth(), getDefaultMaximumWidth());
    }
}

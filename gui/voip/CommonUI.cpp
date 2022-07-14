#include "stdafx.h"
#include "CommonUI.h"
#include "VideoWindow.h"
#include "utils/features.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../main_window/MainWindow.h"
#include "SelectionContactsForConference.h"
#include "../main_window/contact_list/RecentsTab.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/MainPage.h"
#include "../main_window/GroupChatOperations.h"
#include "media/permissions/MediaCapturePermissions.h"
#include "styles/ThemeParameters.h"
#include "../controls/GeneralDialog.h"
#include "../previewer/toast.h"

namespace
{
    void resendKeyEvent(QObject* _receiver, QKeyEvent* _event)
    {
        auto e = new QKeyEvent(_event->type(), _event->key(), _event->modifiers(), _event->text(), _event->isAutoRepeat(), _event->count());
        QCoreApplication::postEvent(_receiver, e);
    }
}

Ui::ResizeEventFilter::ResizeEventFilter(std::vector<QPointer<BaseVideoPanel>>& panels,
    ShadowWindow* shadow,
    QObject* _parent)
    : QObject(_parent)
    , panels_(panels)
    , shadow_(shadow)
{
}

bool Ui::ResizeEventFilter::eventFilter(QObject* _obj, QEvent* _e)
{
    QWidget* parent = qobject_cast<QWidget*>(_obj);
    if (parent && (_e->type() == QEvent::Resize ||
        _e->type() == QEvent::Move ||
        _e->type() == QEvent::WindowActivate ||
        _e->type() == QEvent::NonClientAreaMouseButtonPress ||
        _e->type() == QEvent::ZOrderChange ||
        _e->type() == QEvent::ShowToParent ||
        _e->type() == QEvent::WindowStateChange ||
        _e->type() == QEvent::UpdateRequest))
    {
        const QRect rc = parent->geometry();
        bool bActive = parent->isActiveWindow();

        for (unsigned ix = 0; ix < panels_.size(); ix++)
        {
            auto panel = panels_[ix];
            if (!panel)
                continue;
            bActive = bActive || panel->isActiveWindow();
            panel->updatePosition(*parent);
        }
        if (shadow_)
        {
            int shadowWidth = get_gui_settings()->get_shadow_width();
            shadow_->move(rc.topLeft().x() - shadowWidth, rc.topLeft().y() - shadowWidth);
            shadow_->resize(rc.width() + 2 * shadowWidth, rc.height() + 2 * shadowWidth);
            shadow_->setActive(bActive);
#ifdef _WIN32
            SetWindowPos((HWND)shadow_->effectiveWinId(), (HWND)parent->effectiveWinId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE);
#endif
        }
    }
    return QObject::eventFilter(_obj, _e);
}

void Ui::ResizeEventFilter::addPanel(BaseVideoPanel* _panel)
{
    panels_.push_back(_panel);
}

void  Ui::ResizeEventFilter::removePanel(BaseVideoPanel* _panel)
{
    panels_.erase(std::remove(panels_.begin(), panels_.end(), _panel), panels_.end());
}

Ui::ShadowWindowParent::ShadowWindowParent(QWidget* parent) : shadow_(nullptr)
{
    if constexpr (platform::is_windows())
    {
        const auto shadowWidth = get_gui_settings()->get_shadow_width();
        shadow_ = new ShadowWindow(shadowWidth);

        QPoint pos = parent->mapToGlobal(QPoint(parent->rect().x(), parent->rect().y()));
        shadow_->move(pos.x() - shadowWidth, pos.y() - shadowWidth);
        shadow_->resize(parent->rect().width() + 2 * shadowWidth, parent->rect().height() + 2 * shadowWidth);
    }
}

Ui::ShadowWindowParent::~ShadowWindowParent()
{
    delete shadow_;
}

void Ui::ShadowWindowParent::showShadow()
{
    if (shadow_)
        shadow_->show();
}

void Ui::ShadowWindowParent::hideShadow()
{
    if (shadow_)
        shadow_->hide();
}

Ui::ShadowWindow* Ui::ShadowWindowParent::getShadowWidget()
{
    return shadow_;
}

void Ui::ShadowWindowParent::setActive(bool _value)
{
    if (shadow_)
        shadow_->setActive(_value);
}

Ui::UIEffects::UIEffects(QWidget& _obj, bool opacity, bool geometry)
    : fadedIn_(true)
    , fadeEffect_(nullptr)
    , animation_(nullptr)
    , resizeAnimation_(nullptr)
    , obj_(_obj)
{
    if (opacity)
    {
        fadeEffect_ = new QGraphicsOpacityEffect(&_obj);
        animation_  = std::unique_ptr<QPropertyAnimation>(new QPropertyAnimation(fadeEffect_, QByteArrayLiteral("opacity"), &_obj));
        _obj.setGraphicsEffect(fadeEffect_);
        animation_->setEasingCurve(QEasingCurve::InOutQuad);
        fadeEffect_->setOpacity(1.0);
    }
    if (geometry)
    {
        resizeAnimation_ = std::unique_ptr<QPropertyAnimation>(new QPropertyAnimation(&_obj, QByteArrayLiteral("geometry"), &_obj));
        resizeAnimation_->setEasingCurve(QEasingCurve::InOutQuad);
    }
}

Ui::UIEffects::~UIEffects()
{
}

void Ui::UIEffects::geometryTo(const QRect& _rc, unsigned _interval)
{
    if (resizeAnimation_)
    {
        resizeAnimation_->stop();
        resizeAnimation_->setDuration(_interval);
        resizeAnimation_->setEndValue(_rc);
        resizeAnimation_->start();
    }
}

void Ui::UIEffects::geometryTo(const QRect& _rcStart, const QRect& _rcFinish, unsigned _interval)
{
    if (resizeAnimation_)
    {
        resizeAnimation_->stop();
        resizeAnimation_->setDuration(_interval);
        //resizeAnimation_->setStartValue(_rcStart);
        resizeAnimation_->setEndValue(_rcFinish);
        resizeAnimation_->start();
    }
}

void Ui::UIEffects::fadeOut(unsigned _interval)
{
    if (animation_ && fadedIn_)
    {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(0.01);
        animation_->start();
    }
    fadedIn_ = false;
}

void Ui::UIEffects::fadeIn(unsigned _interval)
{
    if (animation_ && !fadedIn_)
    {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(1.0);
        animation_->start();
    }
    fadedIn_ = true;
}

bool Ui::UIEffects::isFadedIn()
{
    return fadedIn_;
}

bool Ui::UIEffects::isGeometryRunning()
{
    return resizeAnimation_ ? resizeAnimation_->state() == QVariantAnimation::Running : false;
}

void Ui::UIEffects::forceFinish()
{
    if (isGeometryRunning())
        resizeAnimation_->setCurrentTime(resizeAnimation_->duration());
}

Ui::BaseVideoPanel::BaseVideoPanel(QWidget* parent, Qt::WindowFlags f) :
    QWidget(parent, f), grabMouse(false)
{
}

void Ui::BaseVideoPanel::fadeIn(unsigned int duration)
{
    if (!effect_)
        effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this, true, false));
    if (effect_)
        effect_->fadeIn(duration);
}

void Ui::BaseVideoPanel::fadeOut(unsigned int duration)
{
    if (!effect_)
        effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this, true, false));
    if (effect_)
        effect_->fadeOut(duration);
}

bool Ui::BaseVideoPanel::isGrabMouse()
{
    return grabMouse;
}

bool Ui::BaseVideoPanel::isFadedIn()
{
    return effect_ && effect_->isFadedIn();
}

void Ui::BaseVideoPanel::keyPressEvent(QKeyEvent* _event)
{
    if (auto p = parent(); p && (windowFlags() & Qt::Window))
        resendKeyEvent(p, _event);
    else
        QWidget::keyPressEvent(_event);
}

void Ui::BaseVideoPanel::keyReleaseEvent(QKeyEvent* _event)
{
    if (auto p = parent(); p && (windowFlags() & Qt::Window))
        resendKeyEvent(p, _event);
    else
        QWidget::keyReleaseEvent(_event);
}

void Ui::BaseVideoPanel::forceFinishFade()
{
    if (effect_)
        effect_->forceFinish();
}

Ui::BaseTopVideoPanel::BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f) : BaseVideoPanel(parent, f) {}

void Ui::BaseTopVideoPanel::updatePosition(const QWidget& parent)
{
    auto rc = parentWidget()->geometry();
    move(0, verShift_);
    setFixedWidth(rc.width());
}

void Ui::BaseTopVideoPanel::setVerticalShift(int _shift)
{
    verShift_ = _shift;
}

Ui::BaseBottomVideoPanel::BaseBottomVideoPanel(QWidget* parent, Qt::WindowFlags f) : BaseVideoPanel(parent, f) {}

void Ui::BaseBottomVideoPanel::updatePosition(const QWidget& parent)
{
    auto rc = parentWidget()->geometry();
    move(0, rc.height() - rect().height());
    setFixedWidth(rc.width());
}

Ui::PanelBackground::PanelBackground(QWidget* parent) : QWidget(parent)
{
    videoPanelEffect_ = std::make_unique<UIEffects>(*this);
    videoPanelEffect_->fadeOut(0);
}

void Ui::PanelBackground::updateSizeFromParent()
{
    auto parent = parentWidget();
    if (parent)
        resize(parent->size());
}

Ui::TransparentPanel::TransparentPanel(QWidget* _parent, BaseVideoPanel* _eventWidget) : BaseVideoPanel(_parent),
    eventWidget_(_eventWidget)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* rootLayout = Utils::emptyHLayout();
    rootLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setLayout(rootLayout);

#ifndef __APPLE__
    backgroundWidget_ = new PanelBackground(this);
    backgroundWidget_->updateSizeFromParent();
#endif

    auto rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    layout()->addWidget(rootWidget_);
}

Ui::TransparentPanel::~TransparentPanel()
{
}

void Ui::TransparentPanel::updatePosition(const QWidget& parent)
{
    move(eventWidget_->pos());
    setFixedSize(eventWidget_->size());
}

void Ui::TransparentPanel::resizeEvent(QResizeEvent * event)
{
    if (backgroundWidget_)
        backgroundWidget_->updateSizeFromParent();
}

void Ui::TransparentPanel::mouseMoveEvent(QMouseEvent* _e)
{
    resendMouseEventToPanel(_e);
}

void Ui::TransparentPanel::mouseReleaseEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

void Ui::TransparentPanel::mousePressEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

void Ui::TransparentPanel::resendMouseEventToPanel(QMouseEvent *event)
{
    if (eventWidget_->isVisible() && (eventWidget_->rect().contains(event->pos()) || eventWidget_->isGrabMouse()))
    {
        QApplication::sendEvent(eventWidget_, event);
    }
}

Ui::FullVideoWindowPanel::FullVideoWindowPanel(QWidget* parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void Ui::FullVideoWindowPanel::resizeEvent(QResizeEvent *event)
{
    Q_EMIT(onResize());

    auto dialog = findChild<GeneralDialog*>();
    if (dialog)
        dialog->updateSize();
}

void showAddUserToVideoConverenceDialog(QObject* _parent, QWidget* _parentWindow,
                                        std::function<void(Ui::SelectionContactsForConference*)> _connectSignal,
                                        std::function<void()> _disconnectSignal)
{
    Q_EMIT Utils::InterConnector::instance().searchEnd();
    Logic::getContactListModel()->clearCheckedItems();

    Logic::ChatMembersModel conferenceMembers(_parent);
    Logic::SearchModel searchModel(nullptr);
    Logic::ChatMembersModel chatModel(_parent);

    const auto chatRoomCall = Ui::GetDispatcher()->getVoipController().isCallPinnedRoom();
    if (chatRoomCall)
    {
        chatModel.setAimId(QString::fromStdString(Ui::GetDispatcher()->getVoipController().getChatId()));
        chatModel.loadAllMembers();
        chatModel.setServerSearchEnabled(Features::isGlobalContactSearchAllowed());
        chatModel.setSearchPattern(QString());
    }
    else
    {
        searchModel.setSearchInDialogs(false);
        searchModel.setExcludeChats(Logic::SearchDataSource::localAndServer);
        searchModel.setHideReadonly(false);
        searchModel.setCategoriesEnabled(false);
        searchModel.setServerSearchEnabled(Features::isGlobalContactSearchAllowed());
    }

    Ui::ConferenceSearchModel conferenceSearchModel(chatRoomCall ? static_cast<Logic::AbstractSearchModel*>(&chatModel) : static_cast<Logic::AbstractSearchModel*>(&searchModel));

    Ui::SelectContactsWidgetOptions options;
    options.searchModel_ = &conferenceSearchModel;
    Ui::SelectionContactsForConference contactsWidget(
        &conferenceMembers,
        QT_TRANSLATE_NOOP("voip_pages", "Call members"),
        _parentWindow,
        chatRoomCall,
        options);

    const auto maxMembers = Ui::GetDispatcher()->getVoipController().maxVideoConferenceMembers();
    contactsWidget.setMaximumSelectedCount(maxMembers);
    _connectSignal(&contactsWidget);

    const auto action = contactsWidget.show();
    std::vector<QString> selectedContacts = Logic::getContactListModel()->getCheckedContacts();
    if (chatRoomCall)
    {
        selectedContacts = chatModel.getCheckedMembers();
    }
    else
    {
        selectedContacts = Logic::getContactListModel()->getCheckedContacts();
        Logic::getContactListModel()->clearCheckedItems();
        Logic::getContactListModel()->removeTemporaryContactsFromModel();
    }

    if (action == QDialog::Accepted)
    {
        for (const auto& contact : selectedContacts)
            Ui::GetDispatcher()->getVoipController().setStartCall({ contact }, false, true);
    }

    _disconnectSignal();
}

void Ui::showAddUserToVideoConverenceDialogVideoWindow(QObject* parent, FullVideoWindowPanel* parentWindow)
{
    showAddUserToVideoConverenceDialog(parent, parentWindow,
        [parentWindow](Ui::SelectionContactsForConference* dialog)
        {
            QObject::connect(parentWindow, &FullVideoWindowPanel::onResize, dialog, &Ui::SelectionContactsForConference::updateSize);
        },
        [parentWindow]()
        {
            parentWindow->disconnect();
        }
    );
}

void Ui::showAddUserToVideoConverenceDialogMainWindow(QObject* parent, QWidget* parentWindow)
{
    showAddUserToVideoConverenceDialog(parent, parentWindow, [](Ui::SelectionContactsForConference* dialog) {}, []() {});
}

void Ui::showInviteVCSDialogVideoWindow(FullVideoWindowPanel* _parentWindow, const QString& _url)
{
    auto shareDialog = std::make_unique<SelectContactsWidget>(nullptr, Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE, QT_TRANSLATE_NOOP("voip_pages", "Forward"),
                                                              QT_TRANSLATE_NOOP("voip_pages", "Forward"), _parentWindow);

    if (_parentWindow)
        QObject::connect(_parentWindow, &FullVideoWindowPanel::onResize, shareDialog.get(), &Ui::SelectContactsWidget::updateSize);

    const auto action = shareDialog->show();
    const auto selectedContacts = Logic::getContactListModel()->getCheckedContacts();
    Logic::getContactListModel()->removeTemporaryContactsFromModel();
    Logic::getContactListModel()->clearCheckedItems();

    if (action == QDialog::Accepted)
    {
        if (!selectedContacts.empty())
        {
            Data::Quote q;
            q.text_ = _url;
            q.type_ = Data::Quote::Type::link;

            sendForwardedMessages({ std::move(q) }, selectedContacts, ForwardWithAuthor::No, ForwardSeparately::No);
        }

        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    }

    if (_parentWindow)
        _parentWindow->disconnect();
}

Ui::MoveablePanel::MoveablePanel(QWidget* _parent, Qt::WindowFlags f)
    : BaseTopVideoPanel(_parent, f)
    , parent_(_parent)
{
    dragState_.isDraging = false;
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

Ui::MoveablePanel::MoveablePanel(QWidget* _parent)
    : BaseTopVideoPanel(_parent)
    , parent_(_parent)
{
    dragState_.isDraging = false;
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

Ui::MoveablePanel::~MoveablePanel()
{
}

void Ui::MoveablePanel::mouseReleaseEvent(QMouseEvent* _e)
{
    // We check visible flag for drag&drop, because mouse events
    // were be called when window is invisible by VideoWindow.
    if (isVisible())
    {
        grabMouse = false;
        dragState_.isDraging = false;
    }
    QWidget::mouseReleaseEvent(_e);
}

void Ui::MoveablePanel::mousePressEvent(QMouseEvent* _e)
{
    if (isVisible() && parent_ && !parent_->isFullScreen() && (_e->buttons() & Qt::LeftButton))
    {
        grabMouse = true;
        dragState_.isDraging = true;
        dragState_.posDragBegin = QCursor::pos();
        return;
    }
    QWidget::mousePressEvent(_e);
}

void Ui::MoveablePanel::mouseMoveEvent(QMouseEvent* _e)
{
    if (isVisible() && (_e->buttons() & Qt::LeftButton) && dragState_.isDraging)
    {
        QPoint diff = QCursor::pos() - dragState_.posDragBegin;
        dragState_.posDragBegin = QCursor::pos();
        QPoint newpos = parent_->pos() + diff;
        parent_->move(newpos);
        return;
    }
    QWidget::mouseMoveEvent(_e);
}

void Ui::MoveablePanel::keyReleaseEvent(QKeyEvent* _e)
{
    BaseVideoPanel::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        Q_EMIT onkeyEscPressed();
    }
}


namespace Ui
{


    AbstractToastProvider::Options::Options()
        : focusPolicy_(Qt::NoFocus)
        , alignment_(Qt::AlignCenter)
        , direction_((int)ToastBase::MoveDirection::BottomToTop)
        , duration_(4000)
        , maxLineCount_(1)
        , isMultiScreen_(false)
        , animateMove_(false)
    {
    }

    ToastBase* AbstractToastProvider::showToast(const Options& _options, QWidget* _parent, const QRect& _rect)
    {
        if (_options.isMultiScreen_)
        {
            Utils::showMultiScreenToast(_options.text_, _options.maxLineCount_);
            return nullptr;
        }

        ToastBase* toast = createToast(_options, _parent);
        setupOptions(toast, _options);
        const QPoint pos = position(_rect.marginsRemoved(_options.margins_), _options.alignment_);
        ToastManager::instance()->showToast(toast, pos);
        return toast;
    }

    ToastBase* AbstractToastProvider::showToast(const Options& _options, QWidget* _parent)
    {
        return showToast(_options, _parent, _parent ? _parent->rect() : qApp->primaryScreen()->geometry());
    }

    void AbstractToastProvider::setupOptions(ToastBase* _toast, const Options& _opts) const
    {
        for (int i = 0; i < _opts.attributes_.size(); ++i)
            _toast->setAttribute(_opts.attributes_[i], true);

        _toast->setDirection((ToastBase::MoveDirection)_opts.direction_);
        _toast->enableMoveAnimation(_opts.animateMove_);
        _toast->enableMultiScreenShowing(_opts.isMultiScreen_);

        if (_opts.backgroundColor_.isValid())
            _toast->setBackgroundColor(_opts.backgroundColor_);
    }

    QPoint AbstractToastProvider::position(const QRect& _rect, Qt::Alignment _align) const
    {
        QPoint pos = _rect.topLeft();

        if (_align & Qt::AlignTop)
            pos.setY(_rect.top());
        if (_align & Qt::AlignBottom)
            pos.setY(_rect.bottom());

        if (_align & Qt::AlignLeft)
            pos.setX(_rect.left());
        if (_align & Qt::AlignRight)
            pos.setX(_rect.right());

        if (_align & Qt::AlignVCenter)
            pos.setY(_rect.y() + _rect.height() / 2);
        if (_align & Qt::AlignHCenter)
            pos.setX(_rect.x() + _rect.width() / 2);

        return pos;
    }


    VideoWindowToastProvider& VideoWindowToastProvider::instance()
    {
        static VideoWindowToastProvider gInst;
        return gInst;
    }

    ToastBase* VideoWindowToastProvider::createToast(const Options& _options, QWidget* _parent) const
    {
        return new Toast(_options.text_, _parent, _options.maxLineCount_);
    }

    void VideoWindowToastProvider::hide()
    {
        if (currentToast_)
            ToastManager::instance()->hideToast();
    }

    VideoWindowToastProvider::Options VideoWindowToastProvider::defaultOptions() const
    {
        Options options;
        options.attributes_ << Qt::WA_ShowWithoutActivating;
        options.alignment_ = Qt::AlignBottom | Qt::AlignHCenter;
        options.margins_.setBottom(getToastVerOffset());
        options.animateMove_ = true;
        return options;
    }

    void VideoWindowToastProvider::show(Type _type, QWidget* _parent, const QRect& _rect)
    {
        hide();
        if (!_parent)
            return;

        const QString shortcut = QKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_A).toString(QKeySequence::NativeText);

        Options options = defaultOptions();
        switch(_type)
        {
        case Type::CamNotAllowed:
        case Type::DesktopNotAllowed:
            if (GetDispatcher()->getVoipController().isWebinar())
            {
                options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can show the video");
            }
            else
            {
                const auto maxUsers = GetDispatcher()->getVoipController().maxUsersWithVideo();
                options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "Ask one of the participants to turn off the video. In calls with over %1 people only one video can be shown").arg(maxUsers);
                options.maxLineCount_ = 2;
            }
            break;
        case Type::MicNotAllowed:
            options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can use a microphone");
            break;
        case Type::LinkCopied:
            options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "Link copied");
            break;
        case Type::EmptyLink:
            options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "Wait for connection");
            break;
        case Type::DeviceUnavailable:
            options.text_ = QT_TRANSLATE_NOOP("voip_video_panel", "No devices found");
            break;
        case Type::NotifyMicMuted:
            options.alignment_ = Qt::AlignCenter;
            options.maxLineCount_ = 4;
            options.text_ = QT_TRANSLATE_NOOP("voip_pages", "You are muted now\nPress %1 to unmute your microphone\n\nPress and hold Space key for temporarily unmute").arg(shortcut);
            options.isMultiScreen_ = !_parent->isActiveWindow();
            options.animateMove_ = false;
            break;
        case Type::NotifyFullScreen:
            options.alignment_ = Qt::AlignCenter;
            options.text_ = QT_TRANSLATE_NOOP("voip_pages", "Press Esc to exit from fullscreen mode");
            options.margins_ = QMargins(0, Utils::scale_value(16), 0, 0);
            options.animateMove_ = false;
            break;
        }

        if (options.isMultiScreen_)
        {
            showToast(options, nullptr);
            return;
        }

        currentToast_ = createToast(options, _parent);
        setupOptions(currentToast_, options);
        const QPoint pos = position(_rect.marginsRemoved(options.margins_), options.alignment_);
        ToastManager::instance()->showToast(currentToast_, pos);
    }

    void VideoWindowToastProvider::show(Type _type)
    {
        if (auto mainPage = Utils::InterConnector::instance().getMessengerPage())
            if (auto videoWindow = mainPage->getVideoWindow())
                show(_type, videoWindow, videoWindow->toastRect());
    }

    int VideoWindowToastProvider::getToastVerOffset() const noexcept
    {
        return Utils::scale_value(100);
    }
} // end namespace Ui

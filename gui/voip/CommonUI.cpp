#include "stdafx.h"
#include "CommonUI.h"
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
            SetWindowPos((HWND)shadow_->winId(), (HWND)parent->winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE);
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
    hide();
}

void Ui::BaseVideoPanel::fadeIn(unsigned int duration)
{
    if (!effect_)
        effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this, true, false));
    if (effect_)
    {
#ifndef __linux__
        effect_->fadeIn(duration);
#endif
    }
}

void Ui::BaseVideoPanel::fadeOut(unsigned int duration)
{
    if (!effect_)
        effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this, true, false));
    if (effect_)
    {
#ifndef __linux__
        effect_->fadeOut(duration);
#endif
    }
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
    if (platform::is_linux())
    {
        auto rc = parentWidget()->geometry();
        move(0, 0);
        setFixedWidth(rc.width());
    }
    else
    {
        auto rc = parentWidget()->geometry();
        move(rc.x(), rc.y() + verShift_);
        setFixedWidth(rc.width());
    }
}

void Ui::BaseTopVideoPanel::setVerticalShift(int _shift)
{
    verShift_ = _shift;
}

Ui::BaseBottomVideoPanel::BaseBottomVideoPanel(QWidget* parent, Qt::WindowFlags f) : BaseVideoPanel(parent, f) {}

void Ui::BaseBottomVideoPanel::updatePosition(const QWidget& parent)
{
    if (platform::is_linux())
    {
        auto rc = parentWidget()->geometry();
        move(0, rc.height() - rect().height());
        setFixedWidth(rc.width());
    }
    else
    {
        auto rc = parentWidget()->geometry();
        move(rc.x(), rc.y() + rc.height() - rect().height());
        setFixedWidth(rc.width());
    }
}

Ui::PanelBackground::PanelBackground(QWidget* parent) : QWidget(parent)
{
    //setStyleSheet("background-color: #640000;");
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
    Ui::BaseVideoPanel(parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void Ui::FullVideoWindowPanel::updatePosition(const QWidget& parent)
{
    auto rc = parentWidget()->geometry();
    setFixedSize(rc.width(), rc.height());
    move(rc.x(), rc.y());

#ifdef __APPLE__
    // Make round corners for mac.
    QWindow* window = parent.window() ? parent.window()->windowHandle(): nullptr;
    if (window && window->visibility() != QWindow::FullScreen)
    {
        auto rc = rect();
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), Utils::scale_value(5), Utils::scale_value(5));

        QRegion region(path.toFillPolygon().toPolygon());
        setMask(region);
    }
#endif
}

void Ui::FullVideoWindowPanel::resizeEvent(QResizeEvent *event)
{
    Q_EMIT(onResize());
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
#ifndef __linux__
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif
}

Ui::MoveablePanel::MoveablePanel(QWidget* _parent)
    : BaseTopVideoPanel(_parent)
    , parent_(_parent)
{
    dragState_.isDraging = false;
#ifndef __linux__
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif
}

Ui::MoveablePanel::~MoveablePanel()
{
}

void Ui::MoveablePanel::mouseReleaseEvent(QMouseEvent* /*e*/)
{
    // We check visible flag for drag&drop, because mouse events
    // were be called when window is invisible by VideoWindow.
    if (isVisible())
    {
        grabMouse = false;
        dragState_.isDraging = false;
    }
}

void Ui::MoveablePanel::mousePressEvent(QMouseEvent* _e)
{
    if (isVisible() && parent_ && !parent_->isFullScreen() && (_e->buttons() & Qt::LeftButton))
    {
        grabMouse = true;
        dragState_.isDraging = true;
        dragState_.posDragBegin = QCursor::pos();
    }
}

void Ui::MoveablePanel::mouseMoveEvent(QMouseEvent* _e)
{
    if (isVisible() && (_e->buttons() & Qt::LeftButton) && dragState_.isDraging)
    {
        QPoint diff = QCursor::pos() - dragState_.posDragBegin;
        dragState_.posDragBegin = QCursor::pos();
        QPoint newpos = parent_->pos() + diff;
        parent_->move(newpos);
    }
}

void Ui::MoveablePanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || uiWidgetIsActive())
        {
            if (parent_)
            {
                parent_->raise();
                raise();
            }
        }
    }
}

void Ui::MoveablePanel::keyReleaseEvent(QKeyEvent* _e)
{
    BaseVideoPanel::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        Q_EMIT onkeyEscPressed();
    }
}

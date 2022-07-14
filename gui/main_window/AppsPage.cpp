#include "stdafx.h"
#include "AppsPage.h"
#include "MainPage.h"
#include "MainWindow.h"
#include "TabBar.h"
#include "ConnectionWidget.h"
#ifdef HAS_WEB_ENGINE
#include "WebAppPage.h"
#endif
#include "gui_settings.h"

#include "../my_info.h"
#include "../utils/features.h"
#include "../utils/keyboard.h"
#include "../utils/InterConnector.h"
#include "../utils/ObjectFactory.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/ContextMenu.h"
#include "../controls/CustomButton.h"
#include "../controls/TransparentScrollBar.h"
#include "../controls/GradientWidget.h"
#include "../controls/GeneralDialog.h"
#include "../previewer/toast.h"
#include "../styles/ThemeParameters.h"
#include "../styles/ThemesContainer.h"
#include "../core_dispatcher.h"
#include "../url_config.h"

#include "contact_list/RecentsModel.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/UnknownsModel.h"

#include "../common.shared/config/config.h"

#include "mini_apps/MiniApps.h"
#include "mini_apps/MiniAppsUtils.h"
#include "mini_apps/MiniAppsContainer.h"

#include "containers/StatusContainer.h"

namespace
{
    const auto MIN_APPS_BAR_HEIGHT = 400;
    const auto APPS_BAR_WIDTH = 66;
    const auto NAV_BAR_WIDTH = 69;
    const auto AVATAR_HEIGHT = 32;
    const auto LEFT_SHIFT = 6;
    const auto STATUS_TOP_SHIFT = 10;
    const auto STATUS_LEFT_SHIFT = 5;
    const auto APPS_BAR_TOP_SHIFT = 0;
    const auto APPS_BAR_LSHIFT = 2;
    const auto STATUS_WIDGET_SIZE = 38;

    Styling::ThemeColorKey appsNavigationBarBackgroundColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
    }

    Styling::ThemeColorKey appsNavigationBarBorderColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT };
    }

    QColor appsNavigationBarBackgroundColor()
    {
        return Styling::getColor(appsNavigationBarBackgroundColorKey());
    }

    QColor appsNavigationBarBorderColor()
    {
        return Styling::getColor(appsNavigationBarBorderColorKey());
    }

    void setButtonDefaultColors(Ui::CustomButton* _button, const QString& _color, bool _enabled)
    {
        if (!_button)
            return;

        using st = Styling::StyleVariable;
        auto getColor = [](Styling::StyleVariable _color) { return Styling::ThemeColorKey{ _color }; };

        if (!_enabled || _color == qsl("gray") || _color.isEmpty())
        {
            _button->setNormalTextColor(getColor(st::BASE_SECONDARY));
            _button->setHoveredTextColor(getColor(st::BASE_SECONDARY_HOVER));
            _button->setPressedTextColor(getColor(st::BASE_SECONDARY_ACTIVE));
        }
        else if (_color == qsl("green"))
        {
            _button->setNormalTextColor(getColor(st::PRIMARY));
            _button->setHoveredTextColor(getColor(st::PRIMARY_HOVER));
            _button->setPressedTextColor(getColor(st::PRIMARY_ACTIVE));
        }
        else
        {
            im_assert(!"unknown color");
        }
    }

    void setButtonImageDefaultColors(Ui::CustomButton* _button, const QString& _icon, const QString& _color, bool _enabled)
    {
        if (!_button)
            return;

        using st = Styling::StyleVariable;

        if (!_enabled || _color == qsl("gray") || _color.isEmpty())
        {
            _button->setDefaultImage(_icon, Styling::ThemeColorKey{ st::BASE_SECONDARY });
            _button->setHoverImage(_icon, Styling::ThemeColorKey{ st::BASE_SECONDARY_HOVER });
            _button->setPressedImage(_icon, Styling::ThemeColorKey{ st::BASE_SECONDARY_ACTIVE });
        }
        else if (_color == qsl("green"))
        {
            _button->setDefaultImage(_icon, Styling::ThemeColorKey{ st::PRIMARY });
            _button->setHoverImage(_icon, Styling::ThemeColorKey{ st::PRIMARY_HOVER });
            _button->setPressedImage(_icon, Styling::ThemeColorKey{ st::PRIMARY_ACTIVE });
        }
        else
        {
            im_assert(!"unknown color");
        }
    }

    int getLeftRightMargin() noexcept
    {
        return Utils::scale_value(24);
    }

    int getConnectionWidgetHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    constexpr auto getMessengerPageIndex() noexcept { return 0; }

    auto getByTypePred(const QString& _type)
    {
        return [_type](const Ui::SimpleListItem* _item)
        {
            auto i = qobject_cast<const Ui::AppBarItem*>(_item);
            return i && i->getType() == _type;
        };
    }

    QString getIconByText(const QString& _text)
    {
        if (_text == qsl("+"))
            return qsl(":/plus_icon");

        if (_text == qsl("<"))
            return qsl(":/controls/back_icon_thin");

        return {};
    }

    QSize getIconSize() noexcept
    {
        return { Utils::scale_value(32), Utils::scale_value(32) };
    }

    bool isMessengerApp(const QString& _type) noexcept
    {
        const auto& apps = Utils::MiniApps::getMessengerAppTypes();
        return (apps.find(_type) != apps.end());
    }

    bool isWebApp(const QString& _type) noexcept
    {
        return !isMessengerApp(_type);
    }

    std::pair<QUrl, bool> makeWebAppUrl(const QString& _type, const Ui::MiniApp& _app)
    {
#ifdef HAS_WEB_ENGINE
        QUrl appUrl(_app.getUrl(Styling::getThemesContainer().getCurrentTheme()->isDark()));
        if(!_app.needsAuth())
            return { appUrl, true };

        return Utils::InterConnector::instance().signUrl(_type, appUrl);
#else
        return { QUrl(), false };
#endif
    }

    void requestMiniAppAuthParams(const QString& _id)
    {
        Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);
        coll.set_value_as_qstring("id", _id);
        Ui::GetDispatcher()->post_message_to_core("miniapp/start_session", coll.get());
    }

    void startMiniAppSession(const QString& _type)
    {
#ifdef HAS_WEB_ENGINE
        if (!Utils::InterConnector::instance().isMiniAppAuthParamsValid(_type, false))
            requestMiniAppAuthParams(_type);
#endif
    }

    Ui::TabType appPageTypeToMessengerTabType(const QString& _pageType)
    {
        im_assert(isMessengerApp(_pageType));

        if (_pageType == Utils::MiniApps::getMessengerId())
            return Ui::TabType::Recents;

        if (_pageType == Utils::MiniApps::getSettingsId())
            return Ui::TabType::Settings;

        if (_pageType == Utils::MiniApps::getCallsId())
            return Ui::TabType::Calls;

        return Ui::TabType::Recents;
    }

    int getGradientHeight() noexcept
    {
        return Utils::scale_value(26);
    }
}

void Ui::AppsHeaderData::setLeftButton(const QString& _text, const QString& _color, bool _enabled)
{
    leftButtonText_ = _text;
    leftButtonColor_ = _color;
    leftButtonEnabled_ = _enabled;
}

void Ui::AppsHeaderData::setRightButton(const QString& _text, const QString& _color, bool _enabled)
{
    rightButtonText_ = _text;
    rightButtonColor_ = _color;
    rightButtonEnabled_ = _enabled;
}

void Ui::AppsHeaderData::clear()
{
    title_.clear();
    leftButtonText_.clear();
    rightButtonText_.clear();
}

Ui::AppsHeader::AppsHeader(QWidget* _parent)
    : QWidget(_parent)
    , left_(new CustomButton(this))
    , right_(new CustomButton(this))
    , connectionWidget_(new ConnectionWidget(this))
    , state_(ConnectionState::stateUnknown)
    , currentType_(Utils::MiniApps::getMessengerId())
    , visible_(true)
{
    left_->setMinimumSize(getIconSize());
    left_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
    right_->setMinimumSize(getIconSize());
    right_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));

    connectionWidget_->setFixedHeight(getConnectionWidgetHeight());
    setFixedHeight(Utils::getTopPanelHeight());
    auto layout = Utils::emptyHLayout(this);
    setContentsMargins(getLeftRightMargin(), 0, getLeftRightMargin(), 0);
    layout->addWidget(left_);
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    layout->addWidget(right_);

    connect(left_, &CustomButton::clicked, this, [this] { Q_EMIT AppsHeader::leftClicked(currentType_); });
    connect(right_, &CustomButton::clicked, this, [this] { Q_EMIT AppsHeader::rightClicked(currentType_); });

    connect(GetDispatcher(), &Ui::core_dispatcher::connectionStateChanged, this, &AppsHeader::connectionStateChanged);
    connectionStateChanged(GetDispatcher()->getConnectionState());
}

void Ui::AppsHeader::setData(const QString& _type, const AppsHeaderData& _data)
{
    currentType_ = _type;
    visible_ = !_data.isEmpty();
    setVisible(visible_);
    setTitleText(_data.title());

    auto setButtonText = [](CustomButton* _button, const QString& _text, const QString& _color, bool _enabled)
    {
        _button->setVisible(!_text.isEmpty());
        if (auto icon = getIconByText(_text); !icon.isEmpty())
        {
            _button->setText({});
            setButtonImageDefaultColors(_button, icon, _color, _enabled);
            _button->setEnabled(_enabled);
            _button->update();
            return;
        }

        _button->clearIcon();
        _button->setText(_text);
        setButtonDefaultColors(_button, _color, _enabled);
        _button->setEnabled(_enabled);
        _button->update();
    };

    setButtonText(left_, _data.leftButtonText(), _data.leftButtonColor(), _data.leftButtonEnabled());
    setButtonText(right_, _data.rightButtonText(), _data.rightButtonColor(), _data.rightButtonEnabled());

    update();
}

void Ui::AppsHeader::updateVisibility(bool _isVisible)
{
    setVisible(visible_ && _isVisible);
}

void Ui::AppsHeader::setTitleText(const QString& _text)
{
    if (!title_)
    {
        title_ = Ui::TextRendering::MakeTextUnit(_text);
        title_->init({ Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        title_->evaluateDesiredSize();
    }
    else
    {
        title_->setText(_text);
    }

    connectionWidget_->move(width() / 2 - connectionWidget_->width() / 2, height() / 2 - connectionWidget_->height() / 2);
}

void Ui::AppsHeader::connectionStateChanged(const Ui::ConnectionState& _state)
{
    state_ = _state;
    connectionWidget_->setVisible(state_ != ConnectionState::stateOnline);
}

void Ui::AppsHeader::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    auto getColor = [](Styling::StyleVariable _color){ return Styling::getParameters().getColor(_color); };
    using st = Styling::StyleVariable;
    Utils::drawBackgroundWithBorders(p, rect(), getColor(st::BASE_GLOBALWHITE), getColor(st::BASE_BRIGHT), Qt::AlignBottom);
    if (title_ && state_ == ConnectionState::stateOnline)
    {
        title_->setOffsets(width() / 2 - title_->cachedSize().width() / 2, height() / 2);
        title_->draw(p, TextRendering::VerPosition::MIDDLE);
    }
    else if (state_ != ConnectionState::stateOnline)
    {
        connectionWidget_->adjustSize();
        connectionWidget_->move(width() / 2 - connectionWidget_->width() / 2, height() / 2 - connectionWidget_->height() / 2);
    }
}

void Ui::AppsHeader::resizeEvent(QResizeEvent* _event)
{
    connectionWidget_->adjustSize();
    connectionWidget_->move(width() / 2 - connectionWidget_->width() / 2, height() / 2 - connectionWidget_->height() / 2);
    QWidget::resizeEvent(_event);
}

using Factory = Utils::ObjectFactory<Ui::AppBarItem, QString, const QString&, const QString&, const QString&, QWidget*>;
std::unique_ptr<Factory> itemFactory = []()
{
    auto factory = std::make_unique<Factory>();
    factory->registerClass<Ui::CalendarItem>(Utils::MiniApps::getCalendarId());
    factory->registerDefaultClass<Ui::AppBarItem>();
    return factory;
}();


Ui::AppsNavigationBar::AppsNavigationBar(QWidget* _parent)
    : QWidget(_parent)
    , appsBar_(new AppsBar())
    , bottomAppsBar_(new AppsBar(this))
    , gradientTop_(new GradientWidget(this, appsNavigationBarBackgroundColorKey(), Styling::ColorParameter{ Qt::transparent }, Qt::Vertical))
    , gradientBottom_(new GradientWidget(this, Styling::ColorParameter{ Qt::transparent }, appsNavigationBarBackgroundColorKey(), Qt::Vertical))
{
    Testing::setAccessibleName(appsBar_, qsl("AS AppsNavigationBar AppsBar"));
    auto rootLayout = Utils::emptyVLayout(this);

    if (Features::isStatusInAppsNavigationBar())
    {
        avatarWidget_ = new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(AVATAR_HEIGHT), true);
        avatarWidget_->SetMode(ContactAvatarWidget::Mode::ChangeStatus, true);
        avatarWidget_->setStatusTooltipEnabled(true);
        Testing::setAccessibleName(avatarWidget_, qsl("AS AppsNavigationBar statusButton"));
        auto statusLayout = Utils::emptyHLayout();
        statusLayout->setContentsMargins(Utils::scale_value(STATUS_LEFT_SHIFT), Utils::scale_value(STATUS_TOP_SHIFT), 0, 0);
        avatarWidget_->setFixedSize(Utils::scale_value(STATUS_WIDGET_SIZE), Utils::scale_value(STATUS_WIDGET_SIZE));
        statusLayout->addStretch();
        statusLayout->addWidget(avatarWidget_);
        statusLayout->setAlignment(Qt::AlignCenter);
        statusLayout->addStretch();
        rootLayout->addLayout(statusLayout);

        connect(MyInfo(), &my_info::received, this, [this]()
        {
            avatarWidget_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
        });
    }

    // See https://jira.mail.ru/browse/IMDESKTOP-17256?focusedCommentId=12423296&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-12423296
    appsBar_->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

    addScrollArea(rootLayout);
    bottomAppsBar_->setContentsMargins(Utils::scale_value(LEFT_SHIFT), Utils::scale_value(LEFT_SHIFT), Utils::scale_value(LEFT_SHIFT), Utils::scale_value(LEFT_SHIFT));
    rootLayout->addWidget(bottomAppsBar_);

    setFixedWidth(Utils::scale_value(NAV_BAR_WIDTH));

    connect(appsBar_, &AppsBar::clicked, this, [this](int _index)
    {
        selectTab(appsBar_, _index);
        scrollArea_->ensureWidgetVisible(appsBar_->itemAt(_index));
    });
    connect(appsBar_, &AppsBar::rightClicked, this, [this](int _index) { onRightClick(appsBar_, _index); });
    connect(bottomAppsBar_, &AppsBar::clicked, this, [this](int _index) { selectTab(bottomAppsBar_, _index); });
    connect(bottomAppsBar_, &AppsBar::rightClicked, this, [this](int _index) { onRightClick(bottomAppsBar_, _index); });
}

void Ui::AppsNavigationBar::addScrollArea(QVBoxLayout* _rootLayout)
{
    scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea_->setStyleSheet(ql1s("border: none; background-color: transparent;"));
    scrollArea_->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea_->viewport(), true);
    scrollArea_->setMaximumWidth(Utils::scale_value(APPS_BAR_WIDTH));
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto mainWidget = new QWidget(scrollArea_);
    mainWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    Utils::grabTouchWidget(mainWidget);
    Utils::ApplyStyle(mainWidget, Styling::getParameters().getGeneralSettingsQss());
    mainWidget->setContentsMargins(Utils::scale_value(APPS_BAR_LSHIFT), Utils::scale_value(APPS_BAR_TOP_SHIFT), 0, 0);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    scrollArea_->setWidget(mainWidget);
    scrollArea_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    scrollArea_->setAlignment(Qt::AlignCenter);
    QObject::connect(scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, this, &AppsNavigationBar::onScroll);
    QObject::connect(scrollArea_->verticalScrollBar(), &QScrollBar::rangeChanged, this, &AppsNavigationBar::resetGradient);

    mainLayout->addWidget(appsBar_);
    _rootLayout->addWidget(scrollArea_);
}

void Ui::AppsNavigationBar::resetGradient()
{
    onScroll(scrollArea_->verticalScrollBar()->value());
}

Ui::AppBarItem* Ui::AppsNavigationBar::addTab(const std::vector<QString>& _order, const AppPageData& _data)
{
    auto findTabIndexAccordingToTabsOrder = [this, _order](const QString& _type)
    {
        auto targetIndex = 0;
        auto precedingTypeIt = std::find(_order.cbegin(), _order.cend(), _type);
        im_assert(precedingTypeIt != _order.cend());
        while (precedingTypeIt != _order.cbegin() && targetIndex == 0)
        {
            precedingTypeIt = std::prev(precedingTypeIt);
            for (int i = 0; i < appsBar_->count(); ++i)
            {
                if (auto existingItem = qobject_cast<AppBarItem*>(appsBar_->itemAt(i)))
                {
                    if (existingItem->getType() == *precedingTypeIt)
                    {
                        targetIndex = i + 1;
                        break;
                    }
                }
            }
        }
        return targetIndex;
    };

    return addTab(appsBar_, findTabIndexAccordingToTabsOrder(_data.type_), _data);
}

Ui::AppBarItem* Ui::AppsNavigationBar::addTab(AppsBar* _bar, int _index, const AppPageData& _data)
{
    const auto type = _data.type_;
    _bar->layout()->invalidate();
    auto index = _bar->indexOf(getByTypePred(type));

    if (_bar->isValidIndex(index))
        _bar->removeItemAt(index);

    auto item = itemFactory->create(type, type, _data.icon_, _data.iconActive_, this);
    item->setName(_data.name_);
    index = _bar->addItem(item, _index);
    if (_bar->count() == 1)
        _bar->setCurrentIndex(0);
    _bar->updateGeometry();
    resetGradient();
    return item;
}

std::pair<Ui::AppsBar*, int> Ui::AppsNavigationBar::getBarAndIndexByType(const QString& _type)
{
    for (const auto bar : std::array{ appsBar_, bottomAppsBar_ })
    {
        for (auto index = 0; index < bar->count(); ++index)
        {
            if (_type == getTabType(bar, index))
                return {bar, index};
        }
    }
    return {nullptr, -1};
}

QString Ui::AppsNavigationBar::getTabType(AppsBar* _bar, int _index) const
{
    if (const auto item = qobject_cast<AppBarItem*>(_bar->itemAt(_index)))
        return item->getType();

    im_assert(!"tab type cannot be identified");
    return {};
}

QString Ui::AppsNavigationBar::currentPage() const
{
    auto index = -1;
    AppsBar* bar = nullptr;

    if (index = appsBar_->getCurrentIndex(); index != -1)
        bar = appsBar_;
    else if (index = bottomAppsBar_->getCurrentIndex(); index != -1)
        bar = bottomAppsBar_;

    if (index == -1 || !bar)
        return {};

    return getTabType(bar, index);
}

void Ui::AppsNavigationBar::clearAppsSelection()
{
    appsBar_->clearSelection();
}

void Ui::AppsNavigationBar::resetAvatar()
{
    if (avatarWidget_)
        avatarWidget_->UpdateParams(QString(), QString());
}

Ui::AppBarItem* Ui::AppsNavigationBar::addBottomTab(const AppPageData& _data)
{
    auto bottomTab = addTab(bottomAppsBar_, -1, _data);
    bottomAppsBar_->clearSelection();
    return bottomTab;
}

void Ui::AppsNavigationBar::updateTabIcons(const QString& _type)
{
    const auto index = appsBar_->indexOf(getByTypePred(_type));
    if (appsBar_->isValidIndex(index))
    {
        if (const auto& app = Logic::GetAppsContainer()->getApp(_type); app.isValid())
        {
            auto tab = qobject_cast<AppBarItem*>(appsBar_->itemAt(index));
            if (const auto& px = app.getIcon(false); !px.cachedPixmap().isNull())
            {
                tab->setNormalIconPixmap(px);
                tab->setHoveredIconPixmap(px);
            }
            if (const auto& px = app.getIcon(true); !px.cachedPixmap().isNull())
                tab->setActiveIconPixmap(px);
        }
    }
}

void Ui::AppsNavigationBar::updateTabInfo(const QString& _type)
{
    const auto index = appsBar_->indexOf(getByTypePred(_type));
    if (!appsBar_->isValidIndex(index))
        return;
    const auto& app = Logic::GetAppsContainer()->getApp(_type);
    if (!app.isValid())
        return;
    auto tab = qobject_cast<AppBarItem*>(appsBar_->itemAt(index));
    if (const auto& px = app.getIcon(false); !px.cachedPixmap().isNull())
    {
        tab->setNormalIconPixmap(px);
        tab->setHoveredIconPixmap(px);
    }
    if (const auto& px = app.getIcon(true); !px.cachedPixmap().isNull())
        tab->setActiveIconPixmap(px);
    tab->setName(app.getTitle());
}

void Ui::AppsNavigationBar::removeTab(const QString& _type)
{
    if (const auto [bar, index] = getBarAndIndexByType(_type); bar && index != -1)
        bar->removeItemAt(index);

    resetGradient();
}

bool Ui::AppsNavigationBar::contains(const QString& _type) const
{
    auto pred = getByTypePred(_type);
    return appsBar_->contains(pred) || bottomAppsBar_->contains(pred);
}

int Ui::AppsNavigationBar::defaultWidth()
{
    return Utils::scale_value(68);
}

void Ui::AppsNavigationBar::selectTab(AppsBar* _bar, int _index, bool _emitClick)
{
    const auto oldTab = currentPage();
    const auto newTab = getTabType(_bar, _index);

    if (_emitClick)
    {
        if (oldTab != newTab)
        {
            if (newTab != Utils::MiniApps::getMessengerId())
                MainPage::instance()->saveSidebarState();
            else
                MainPage::instance()->restoreSidebarState();
        }
        Q_EMIT appTabClicked(newTab, oldTab, QPrivateSignal());
    }

    if (newTab != oldTab)
    {
        if (!Utils::MiniApps::isTarmApp(newTab))
        {
            for (auto& bar : { appsBar_, bottomAppsBar_ })
            {
                if (bar != _bar)
                    bar->clearSelection();
            }

            _bar->setCurrentIndex(_index);
        }
        else
        {
            // TODO: Temporary, for the correct logic of work
            _bar->itemAt(_index)->setHovered(false);
        }
    }
}

void Ui::AppsNavigationBar::selectTab(const QString& _pageType, bool _emitClick)
{
    if (const auto [bar, pageIndex] = getBarAndIndexByType(_pageType); bar && pageIndex != -1)
    {
        if (pageIndex != bar->getCurrentIndex())
            selectTab(bar, pageIndex, _emitClick);
    }
}

void Ui::AppsNavigationBar::onRightClick(AppsBar* _bar, int _index)
{
    const auto pageType = getTabType(_bar, _index);
    const auto areModifiersPressed = Utils::keyboardModifiersCorrespondToKeyComb(qApp->keyboardModifiers(), Utils::KeyCombination::Ctrl_or_Cmd_AltShift);
    if (!areModifiersPressed)
        return;

    if (pageType == Utils::MiniApps::getSettingsId())
    {
        Q_EMIT Utils::InterConnector::instance().showDebugSettings();
    }
    else if (isWebApp(pageType) && !Utils::MiniApps::isFeedApp(pageType))
    {
        auto menu = new ContextMenu(this);
        Testing::setAccessibleName(menu, qsl("AS WebApp menu"));
        menu->addActionWithIcon(qsl(":/controls/reload"), QT_TRANSLATE_NOOP("tooltips", "Refresh"), this, [this, pageType]()
        {
            Q_EMIT reloadWebPage(pageType, QPrivateSignal());
        });
        menu->popup(QCursor::pos());
    }
}

void Ui::AppsNavigationBar::onScroll(int _value)
{
    const auto updateVisibility = [](GradientWidget* _w, bool _visible)
    {
        if (_w->isVisible() != _visible)
        {
            _w->setVisible(_visible);
            if (_visible)
                _w->raise();
        }
    };

    updateVisibility(gradientTop_, _value != 0);
    const auto scrollMax = scrollArea_->verticalScrollBar()->maximum();
    updateVisibility(gradientBottom_, (scrollMax != 0 && _value != scrollMax) || appsBar_->height() > scrollArea_->height());
}

void Ui::AppsNavigationBar::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    Utils::drawBackgroundWithBorders(p, rect(), appsNavigationBarBackgroundColor(), appsNavigationBarBorderColor(), Qt::AlignRight);
}

void Ui::AppsNavigationBar::resizeEvent(QResizeEvent* _event)
{
    const auto sRect = scrollArea_->geometry();

    gradientTop_->setGeometry(sRect.left(), sRect.top(), sRect.width(), getGradientHeight());
    gradientBottom_->setGeometry(sRect.left(), sRect.bottom() - getGradientHeight() + 1, sRect.width(), getGradientHeight());
    resetGradient();

    QWidget::resizeEvent(_event);
}

void Ui::AppsPage::initNavigationBar()
{
    navigationBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    Testing::setAccessibleName(navigationBar_, qsl("AS AppsPage AppsNavigationBar"));
    connect(navigationBar_, &AppsNavigationBar::appTabClicked, this, &AppsPage::onTabClicked);
    connect(navigationBar_, &AppsNavigationBar::reloadWebPage, this, &AppsPage::reloadWebAppPage);
}

void Ui::AppsPage::addMessengerPage()
{
    messengerPage_ = MainPage::instance(this);
    Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));
    messengerItem_ = addTab(messengerPage_, { Utils::MiniApps::getMessengerId(), QT_TRANSLATE_NOOP("tab", "Chats"), qsl(":/tab/chats"), qsl(":/tab/chats_active") });
}

void Ui::AppsPage::addInterConnectorConnections()
{
    using uic = Utils::InterConnector;
    auto interConnector = &uic::instance();
    connect(interConnector, &uic::authParamsChanged, this, &AppsPage::onAuthParamsChanged);
    connect(interConnector, &uic::omicronUpdated, this,&AppsPage::onConfigChanged);
    connect(interConnector, &uic::logout, this, &AppsPage::onLogout);
    connect(interConnector, &uic::composeEmail, this, &AppsPage::composeEmail);
    connect(interConnector, &uic::headerBack, this, [this]()
    {
        if (navigationBar_ && navigationBar_->contains(Utils::MiniApps::getMessengerId()))
            navigationBar_->selectTab(Utils::MiniApps::getMessengerId());
    });
}

void Ui::AppsPage::addDispatcherConnections()
{
    auto dispatcher = Ui::GetDispatcher();
    connect(dispatcher, &core_dispatcher::externalUrlConfigUpdated, this, &AppsPage::onConfigChanged);
    connect(dispatcher, &core_dispatcher::im_created, this, &AppsPage::loggedIn);
    connect(dispatcher, &core_dispatcher::appUnavailable, this, &AppsPage::appUnavailable);
}

Ui::AppsPage::AppsPage(QWidget* _parent)
    : QWidget(_parent)
    , pages_(new QStackedWidget(this))
    , messengerPage_(nullptr)
    , navigationBar_(new AppsNavigationBar(this))
    , messengerItem_(nullptr)
    , semiWindow_(new SemitransparentWindowAnimated(this))
    , header_(new AppsHeader(this))
{
    appTabsOrder_.push_back(Utils::MiniApps::getMessengerId());
    appTabsOrder_.push_back(Utils::MiniApps::getCallsId());
    appTabsOrder_.push_back(Utils::MiniApps::getSettingsId());

    auto rootLayout = Utils::emptyHLayout(this);

    initNavigationBar();
    rootLayout->addWidget(navigationBar_);
    addMessengerPage();
    updateNavigationBarVisibility();
    addSettingsTab();

    addInterConnectorConnections();
    addDispatcherConnections();
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::appIconUpdated, this, &AppsPage::updateTabIcon);
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::updateApps, this, &AppsPage::onAppsUpdate, Qt::QueuedConnection);
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::removeTab, this, &AppsPage::onRemoveTab);
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::removeApp, this, &AppsPage::onRemoveApp);
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::updateAppFields, this, &AppsPage::onUpdateAppFields);
    connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::removeHiddenPage, this, &AppsPage::onRemoveHiddenPage, Qt::QueuedConnection);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::updateMiniAppCounter, this, &AppsPage::updateTabCounter);
    connect(Ui::MyInfo(), &Ui::my_info::received, this, &AppsPage::onMyInfoReceived);
    connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &AppsPage::onThemeChanged);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::requestApp, this, &AppsPage::openApp);

    loggedIn();

    auto vLayout = Utils::emptyVLayout();
    vLayout->addWidget(header_);
    vLayout->addWidget(pages_);
    rootLayout->addLayout(vLayout);

    Testing::setAccessibleName(pages_, qsl("AS AppsPage AppsPages"));

    const auto messengerType = Utils::MiniApps::getMessengerId();
    header_->setData(messengerType, headerData_[messengerType]);

    semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    hideSemiWindow();

#ifdef HAS_WEB_ENGINE
    QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
#endif //HAS_WEB_ENGINE
}

Ui::AppsPage::~AppsPage()
{
    MainPage::reset();
    messengerItem_ = nullptr;
    messengerPage_ = nullptr;
}

Ui::MainPage* Ui::AppsPage::messengerPage() const
{
    return messengerPage_;
}

Ui::WebAppPage* Ui::AppsPage::tasksPage() const
{
#ifdef HAS_WEB_ENGINE
    const auto tasksIter = pagesByType_.find(Utils::MiniApps::getTasksId());
    return tasksIter != pagesByType_.end() ? qobject_cast<Ui::WebAppPage*>(tasksIter->second.page_.data()) : nullptr;
#else
    return nullptr;
#endif
}

bool Ui::AppsPage::isMessengerPage() const
{
    return MainPage::hasInstance() && (MainPage::instance() == pages_->currentWidget());
}

bool Ui::AppsPage::isContactDialog() const
{
    return isMessengerPage() && MainPage::instance()->isContactDialog();
}

bool Ui::AppsPage::isTasksPage() const
{
    const auto tasksIter = pagesByType_.find(Utils::MiniApps::getTasksId());
    return (tasksIter != pagesByType_.end()) && (tasksIter->second.page_ == pages_->currentWidget());
}

bool Ui::AppsPage::isFeedPage() const
{
    return navigationBar_ && Utils::MiniApps::isFeedApp(navigationBar_->currentPage());
}

void Ui::AppsPage::resetPages()
{
    messengerPage_ = nullptr;
    messengerItem_ = nullptr;
    authDataValid_ = false;

    for (auto& type : appTabsOrder_)
        removeTab(type, false);

    MainPage::reset();

    if (messengerItem_)
        messengerItem_->resetItemInfo();
}

void Ui::AppsPage::prepareForShow()
{
    Logic::GetStatusContainer()->setAvatarVisible(MyInfo()->aimId(), true);

    if (!MainPage::hasInstance())
    {
        messengerPage_ = MainPage::instance(this);
        Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));
        pages_->insertWidget(getMessengerPageIndex(), messengerPage_);
        pages_->setCurrentIndex(getMessengerPageIndex());
        const auto messengerId = Utils::MiniApps::getMessengerId();
        pagesByType_[messengerId].page_ = messengerPage_;
        selectTab(messengerId);
    }
}

void Ui::AppsPage::showMessengerPage()
{
    if (!navigationBar_ || !navigationBar_->contains(Utils::MiniApps::getMessengerId()))
        return;
    selectTab(Utils::MiniApps::getMessengerId());
}

Ui::AppBarItem* Ui::AppsPage::addTab(QWidget* _widget, const AppPageData& _data, const QString& _defaultTitle, bool _selected)
{
    if (!navigationBar_)
    {
        im_assert(!"Can't add tab without navigation bar");
        return nullptr;
    }

    const auto addToNavigationBar = [this, _widget, &_data, _defaultTitle, _selected](int index) -> Ui::AppBarItem*
    {
        const auto type = _data.type_;
        auto tabItem = navigationBar_->addTab(appTabsOrder_, _data);
        headerData_[type] = _defaultTitle;
        Testing::setAccessibleName(tabItem, u"AS AppTab " % _widget->accessibleName());
        if (_selected)
            pages_->setCurrentIndex(index);
        pagesByType_[type].tabItem_ = tabItem;
        return tabItem;
    };

    const auto type = _data.type_;
    if (int index = pages_->indexOf(_widget); index == -1)
    {
        index = pages_->addWidget(_widget);
        im_assert(type != Utils::MiniApps::getMessengerId() || index == getMessengerPageIndex());
        im_assert(pagesByType_.count(type) == 0);
        pagesByType_[type].page_ = _widget;
        return addToNavigationBar(index);
    }
    else
    {
        if (!navigationBar_->contains(type))
            return addToNavigationBar(index);
    }

    return nullptr;
}

void Ui::AppsPage::removeTab(const QString& _type, bool _showDefaultPage)
{
    if (_type == Utils::MiniApps::getSettingsId())
        return;

    if (const auto it = pagesByType_.find(_type); it != pagesByType_.end())
    {
        auto page = it->second;
        im_assert(page.page_);
        if (const int index = pages_->indexOf(page.page_); index != -1)
        {
            pages_->removeWidget(page.page_);
            headerData_[_type].clear();
            page.page_->deleteLater();
            page.tabItem_->deleteLater();
            page.tabItem_ = nullptr;
        }
        else if (!Utils::MiniApps::isFeedApp(_type))
        {
            im_assert(!"pages are supposed to have page of this type");
        }
        pagesByType_.erase(it);
    }

    if (navigationBar_ && navigationBar_->contains(_type))
        navigationBar_->removeTab(_type);
}

void Ui::AppsPage::showSemiWindow()
{
    semiWindow_->setFixedSize(size());
    semiWindow_->raise();
    semiWindow_->showAnimated();
}

void Ui::AppsPage::hideSemiWindow()
{
    semiWindow_->forceHide();
}

bool Ui::AppsPage::isSemiWindowVisible() const
{
    return semiWindow_->isSemiWindowVisible();
}

int Ui::AppsPage::getHeaderHeight() const
{
    return (header_ && header_->isVisible()) ? header_->height() : 0;
}

void Ui::AppsPage::resizeEvent(QResizeEvent* _event)
{
    semiWindow_->setFixedSize(size());
    QWidget::resizeEvent(_event);
}

void Ui::AppsPage::openApp(const QString& _appId, const QString& _urlQuery, const QString& _urlFragment)
{
    if (const auto iter = pagesByType_.find(_appId); iter != pagesByType_.end())
    {
#ifdef HAS_WEB_ENGINE
        if (auto page = qobject_cast<Ui::WebAppPage*>(iter->second.page_.data()))
        {
            const auto& app = Logic::GetAppsContainer()->getApp(_appId);
            if (!app.isValid() || !app.isEnabled())
                return;

            const bool isDarkTheme = Styling::getThemesContainer().getCurrentTheme()->isDark();
            QUrl url = app.getUrl(isDarkTheme);
            url = Utils::addQueryItemsToUrl(std::move(url), QUrlQuery(_urlQuery), Utils::AddQueryItemPolicy::KeepExistingItem);
            url.setFragment(_urlFragment);

            if (app.needsAuth())
                url = Utils::InterConnector::instance().signUrl(_appId, std::move(url)).first;

            if (!url.isEmpty())
                page->setUrl(url, true);
        }
#endif //HAS_WEB_ENGINE
        selectTab(_appId, true);
        MainPage::instance()->setSidebarState(SidebarStateFlag::Closed);
    }
    else
    {
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("toast", "App not found"), Utils::defaultToastVerOffset());
    }
}

void Ui::AppsPage::selectTab(const QString& _pageType, bool _emitClick)
{
    if (navigationBar_)
    {
        navigationBar_->selectTab(_pageType, _emitClick);
    }
    else
    {
        if (auto it = pagesByType_.find(_pageType); it != pagesByType_.end())
            pages_->setCurrentWidget(it->second.page_);
        else
            im_assert(false);
    }
}

void Ui::AppsPage::addCallsTab()
{
    if (!navigationBar_ || navigationBar_->contains(Utils::MiniApps::getCallsId()))
        return;

    auto callsTab = navigationBar_->addTab(appTabsOrder_, { Utils::MiniApps::getCallsId(), QT_TRANSLATE_NOOP("tab", "Calls"), qsl(":/tab/calls"), qsl(":/tab/calls_active") });
    Testing::setAccessibleName(callsTab, qsl("AS Tab ") % MainPage::callsTabAccessibleName());
}

void Ui::AppsPage::updateCallsTab()
{
    const auto isTabInConfig = std::find(appTabsOrder_.begin(), appTabsOrder_.end(), Utils::MiniApps::getCallsId()) != appTabsOrder_.end();
    if (isTabInConfig)
        addCallsTab();
    else
        navigationBar_->removeTab(Utils::MiniApps::getCallsId());
}

void Ui::AppsPage::addSettingsTab()
{
    if (!navigationBar_)
        return;
    auto settingsTab = navigationBar_->addBottomTab({ Utils::MiniApps::getSettingsId(), QT_TRANSLATE_NOOP("tab", "Settings"), qsl(":/tab/settings"), qsl(":/tab/settings_active") });
    Testing::setAccessibleName(settingsTab, qsl("AS Tab ") % MainPage::settingsTabAccessibleName());
}

void Ui::AppsPage::updateWebPages()
{
    const auto currentPage = getCurrentPage();
    Logic::GetAppsContainer()->setCurrentApp(currentPage);
    Logic::GetAppsContainer()->updateAppsConfig();
    navigationBar_->clearAppsSelection();
}

void Ui::AppsPage::onAppsUpdate(const QString& _selectedId)
{
    auto miniApps = Logic::GetAppsContainer()->getAppsOrder();
    miniApps.insert(miniApps.begin(), Utils::MiniApps::getMessengerId());
    auto iter = std::unique(miniApps.begin(), miniApps.end());
    if (iter != miniApps.end())
        miniApps.erase(iter, miniApps.end());
    appTabsOrder_ = std::move(miniApps);

    auto makeWebPage = [this](const QString& _type, const QString& _accessibleName)
    {
#ifndef HAS_WEB_ENGINE
        auto w = new QWidget(this);
#else
        auto w = new WebAppPage(_type, this);

        const auto& app = Logic::GetAppsContainer()->getApp(_type); // if we are here, app is definitely in map
        const auto isSigned = Utils::InterConnector::instance().isMiniAppAuthParamsValid(_type, false);
        const auto url = makeWebAppUrl(_type, app).first;
        if (isSigned)
            w->markUnavailable(false);

        w->setUrl(url, true);

        if (authDataValid_ && !isSigned && !w->isUnavailable())
            startMiniAppSession(_type);

        connect(w, &WebAppPage::setTitle, this, &AppsPage::onSetTitle);
        connect(w, &WebAppPage::setLeftButton, this, &AppsPage::onSetLeftButton);
        connect(w, &WebAppPage::setRightButton, this, &AppsPage::onSetRightButton);
        connect(w, &WebAppPage::pageIsHidden, this, [type = _type]() { Logic::GetAppsContainer()->onAppHidden(type); });
        connect(header_, &AppsHeader::leftClicked, w, &WebAppPage::headerLeftButtonClicked);
        connect(header_, &AppsHeader::rightClicked, w, &WebAppPage::headerRightButtonClicked);
#endif
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(w, _accessibleName);
        return w;
    };

    auto updateTab = [this, makeWebPage](const AppPageData& _data, const QString& _accName, const QString& _defaultTitle, bool _selected)
    {
        const auto& type = _data.type_;
        const auto widget = containsTab(type) ? pagesByType_[type].page_.data() : makeWebPage(type, _accName);
        addTab(widget, _data, _defaultTitle, _selected);
        updateTabIcon(type);
    };

    updateNavigationBarVisibility();
    for (const auto& tab : appTabsOrder_)
    {
        if (tab != Utils::MiniApps::getMessengerId() && tab != Utils::MiniApps::getCallsId() && tab != Utils::MiniApps::getSettingsId())
        {
            if (const auto& app = Logic::GetAppsContainer()->getApp(tab); app.isValid() && app.isEnabled())
            {
                const auto defaultTitle = tab != Utils::MiniApps::getMailId() ? app.getTitle() : QString();
                const AppPageData appData(tab, app.getTitle(), app.getDefaultIconPath(false), app.getDefaultIconPath(true));

                if (Utils::MiniApps::isFeedApp(tab))
                {
                    auto feedTab = navigationBar_->addTab(appTabsOrder_, appData);
                    pagesByType_[tab].tabItem_ = feedTab;
                    Logic::getContactListModel()->markAsFeed(app.getAimid(), app.getStamp(), tab);
                }
                else
                {
                    updateTab(appData, app.getAccessibleName(), defaultTitle, tab == _selectedId);
                }
            }
        }
        else if (tab == Utils::MiniApps::getCallsId())
        {
            updateCallsTab();
        }

        if (tab == _selectedId)
            selectTab(tab, false);
    }
}

QString Ui::AppsPage::getCurrentPage() const
{
    auto currentPage = navigationBar_->currentPage();
#ifdef HAS_WEB_ENGINE
    if (currentPage.isEmpty())
    {
        if (auto page = qobject_cast<WebAppPage*>(pages_->currentWidget()))
            currentPage = page->type();
    }
#endif

    if (currentPage.isEmpty())
        currentPage = Utils::MiniApps::getMessengerId();
    return currentPage;
}

void Ui::AppsPage::onRemoveTab(const QString& _id)
{
    auto currentPage = getCurrentPage();
    navigationBar_->removeTab(_id);
    if (_id == currentPage)
        navigationBar_->clearAppsSelection();
}

void Ui::AppsPage::onRemoveApp(const QString& _id, Logic::KeepWebPage _keepAppPage)
{
    if (_keepAppPage == Logic::KeepWebPage::Yes)
        onRemoveTab(_id);
    else
        removeTab(_id);
}

#ifdef HAS_WEB_ENGINE
Ui::WebAppPage* Ui::AppsPage::getWebPage(const QString& _id)
{
    auto it = pagesByType_.find(_id);
    if (it == pagesByType_.end())
        return nullptr;
    return qobject_cast<WebAppPage*>(it->second.page_);
}

void Ui::AppsPage::updateWebPage(const QString& _id)
{
    auto webPage = getWebPage(_id);
    if (!webPage)
        return;
    const auto& app = Logic::GetAppsContainer()->getApp(_id);
    if (!app.isValid())
        return;

    auto oldUrl = webPage->getUrl();
    webPage->setUrl(makeWebAppUrl(_id, app).first);
    webPage->setTitle(_id, app.getTitle());

    if (header_->currentType() == _id && oldUrl != webPage->getUrl())
        reloadWebAppPage(_id);
}
#endif

void Ui::AppsPage::onUpdateAppFields(const QString& _id)
{
#ifdef HAS_WEB_ENGINE
    updateWebPage(_id);
#endif
    if (navigationBar_)
        navigationBar_->updateTabInfo(_id);
}

void Ui::AppsPage::onRemoveHiddenPage(const QString& _id)
{
    if (const auto it = pagesByType_.find(_id); it != pagesByType_.end())
    {
        auto page = it->second;
        if (const int index = pages_->indexOf(page.page_); index != -1)
        {
            pages_->removeWidget(page.page_);
            headerData_[_id].clear();
            page.page_->deleteLater();
            page.tabItem_->deleteLater();
        }
        pagesByType_.erase(it);
    }
}

void Ui::AppsPage::onThemeChanged()
{
#ifdef HAS_WEB_ENGINE
    for (auto [type, pageData] : pagesByType_)
    {
        if (type == Utils::MiniApps::getMessengerId())
            continue;

        if (auto page = qobject_cast<WebAppPage*>(pageData.page_))
        {
            if (const auto& app = Logic::GetAppsContainer()->getApp(type); app.isValid())
                page->setUrl(makeWebAppUrl(type, app).first);
        }
    }
#endif
}

void Ui::AppsPage::updatePagesAimsids()
{
#ifdef HAS_WEB_ENGINE
    for (auto [type, pageData] : pagesByType_)
    {
        if (type == Utils::MiniApps::getMessengerId())
            continue;

        auto page = qobject_cast<WebAppPage*>(pageData.page_);
        if (!page)
            return;

        if (!Utils::InterConnector::instance().isMiniAppAuthParamsValid(type, false))
        {
            if (!page->isUnavailable())
                requestMiniAppAuthParams(type);
        }
        else
        {
            page->markUnavailable(false);
            if (const auto& app = Logic::GetAppsContainer()->getApp(type); app.isValid())
                page->setUrl(makeWebAppUrl(type, app).first);
        }
    }
#endif
}

void Ui::AppsPage::updateNavigationBarVisibility()
{
    if (navigationBar_)
        navigationBar_->setVisible(Features::isAppsNavigationBarVisible());
}

void Ui::AppsPage::onAuthParamsChanged(bool _aimsidChanged)
{
    const auto isValid = Utils::InterConnector::instance().isAuthParamsValid();
    if (!isValid)
        return;

    const auto oldValid = std::exchange(authDataValid_, isValid);
    if (!oldValid) // first time receiving auth params
    {
        Logic::GetAppsContainer()->initApps();
        updateWebPages();
    }
    else if (_aimsidChanged)
    {
        updatePagesAimsids();
    }
}

void Ui::AppsPage::onConfigChanged()
{
    if (authDataValid_)
        updateWebPages();
}

void Ui::AppsPage::onMyInfoReceived()
{
    if (authDataValid_)
        updateWebPages();
}

void Ui::AppsPage::onLogout()
{
    Logic::GetStatusContainer()->setAvatarVisible(MyInfo()->aimId(), false);
    navigationBar_->resetAvatar();
#ifdef HAS_WEB_ENGINE
    auto profile = Utils::InterConnector::instance().getLocalProfile();
    profile->cookieStore()->deleteAllCookies();
    profile->clearHttpCache();
#endif //HAS_WEB_ENGINE
    currentMessengerDialog_.clear();
}

void Ui::AppsPage::updateTabIcon(const QString& _id)
{
    if (navigationBar_)
        navigationBar_->updateTabIcons(_id);
}

void Ui::AppsPage::updateTabCounter(const QString& _miniAppId, int _counter)
{
    if (const auto it = pagesByType_.find(_miniAppId); it != pagesByType_.end())
    {
        if (auto item = qobject_cast<AppBarItem*>(it->second.tabItem_))
            item->setBadgeText(Utils::getUnreadsBadgeStr(_counter));
    }
}

bool Ui::AppsPage::containsTab(const QString& _type) const
{
    return pagesByType_.find(_type) != pagesByType_.end();
}

void Ui::AppsPage::onTabClicked(const QString& _newId, const QString& _oldId)
{
    if (const auto& app = Logic::GetAppsContainer()->getApp(_newId); app.isValid())
    {
        if (app.isExternal())
            return Utils::openUrl(makeWebAppUrl(_newId, app).first.toString());
    }

    const auto isSameTabClicked = _newId == _oldId;
    const auto toMessenger = _newId == Utils::MiniApps::getMessengerId();
    const auto toSettings = _newId == Utils::MiniApps::getSettingsId();
    const auto toMessengerOrSettings = toMessenger || toSettings;
    const auto fromSettingsOrFeed = Utils::MiniApps::isFeedApp(_oldId) || _oldId == Utils::MiniApps::getSettingsId();

    if (isMessengerApp(_newId) && messengerPage_)
        messengerPage_->onTabClicked(appPageTypeToMessengerTabType(_newId), isSameTabClicked);

    if (!isSameTabClicked)
    {
        if (Utils::MiniApps::isFeedApp(_newId))
        {
            if (_oldId == Utils::MiniApps::getMessengerId() && currentMessengerDialog_.isEmpty())
                currentMessengerDialog_ = Logic::getContactListModel()->selectedContact();
            if (!_oldId.isEmpty())
                Q_EMIT Utils::InterConnector::instance().searchEnd();
            Utils::openFeedContact(Logic::GetAppsContainer()->getApp(_newId).getAimid());
        }
        else if (toSettings)
        {
            if (_oldId == Utils::MiniApps::getMessengerId() && currentMessengerDialog_.isEmpty())
                currentMessengerDialog_ = Logic::getContactListModel()->selectedContact();
        }

        if (!_oldId.isEmpty())
            Q_EMIT Utils::InterConnector::instance().clearSelection();

        onTabChanged(_newId);

        if (toMessengerOrSettings || Utils::MiniApps::isFeedApp(_newId))
        {
            const auto aimId = toMessenger ? (Utils::MiniApps::isFeedApp(_oldId) ? currentMessengerDialog_ : Logic::getContactListModel()->selectedContact())
                                           : Logic::GetAppsContainer()->getApp(_newId).getAimid();
            messengerPage_->setOpenedAs(toMessengerOrSettings ? PageOpenedAs::MainPage : PageOpenedAs::FeedPage);
            Q_EMIT Utils::InterConnector::instance().changeBackButtonVisibility(aimId, toMessengerOrSettings);
        }
    }

    if (fromSettingsOrFeed && toMessenger)
        QMetaObject::invokeMethod(this, [this, contact = currentMessengerDialog_]() { reopenDialog(contact); }, Qt::QueuedConnection);
}

void Ui::AppsPage::onTabChanged(const QString& _pageType)
{
    if (isWebApp(_pageType))
    {
        if (auto it = pagesByType_.find(_pageType); it != pagesByType_.end())
            pages_->setCurrentWidget(it->second.page_);
        else
            im_assert(false);
    }
    else
    {
        if (pages_ && !Utils::MiniApps::isFeedApp(_pageType))
            pages_->setCurrentIndex(getMessengerPageIndex());
        if (messengerPage_)
            messengerPage_->selectTab(appPageTypeToMessengerTabType(_pageType));
    }

    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    auto type = Utils::MiniApps::getMessengerId();
#ifdef HAS_WEB_ENGINE
    if (auto page = qobject_cast<WebAppPage*>(pages_->currentWidget()))
    {
        type = page->type();
        if (page->isUnavailable())
            startMiniAppSession(type);
    }
#endif
    header_->setData(type, headerData_[type]);
    Q_EMIT Utils::InterConnector::instance().currentPageChanged();
}

void Ui::AppsPage::loggedIn()
{
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
}

void Ui::AppsPage::chatUnreadChanged()
{
    if (messengerPage_)
        messengerPage_->chatUnreadChanged();
    if (messengerItem_)
        messengerItem_->updateItemInfo();

    const auto& feeds = Utils::MiniApps::getFeedAppTypes();
    for (const auto& feed : feeds)
    {
        if (const auto aimId = Logic::getContactListModel()->getFeedAimId(feed))
            updateTabCounter(feed, Logic::getRecentsModel()->getUnreadCount(*aimId));
    }

}

void Ui::AppsPage::reloadWebAppPage(const QString& _pageType)
{
#ifdef HAS_WEB_ENGINE
    im_assert(isWebApp(_pageType));
    if (auto webPage = getWebPage(_pageType))
        webPage->reloadPage();
#endif
}

void Ui::AppsPage::onSetTitle(const QString& _type, const QString& _title)
{
    auto& data = headerData_[_type];
    data.setTitle(_title);
    if (header_->currentType() == _type)
        header_->setData(_type, data);
}

void Ui::AppsPage::onSetLeftButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled)
{
    auto& data = headerData_[_type];
    data.setLeftButton(_text, _color, _enabled);
    if (header_->currentType() == _type)
        header_->setData(_type, data);
}

void Ui::AppsPage::onSetRightButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled)
{
    auto& data = headerData_[_type];
    data.setRightButton(_text, _color, _enabled);
    if (header_->currentType() == _type)
        header_->setData(_type, data);
}

void Ui::AppsPage::onSettingsClicked()
{
    if (!navigationBar_ || !navigationBar_->bottomAppsBar())
        return;
    navigationBar_->selectTab(navigationBar_->bottomAppsBar(), 0);
}

void Ui::AppsPage::composeEmail(const QString& _email)
{
    if (std::find(appTabsOrder_.begin(), appTabsOrder_.end(), Utils::MiniApps::getMailId()) == appTabsOrder_.end())
        return;

#ifdef HAS_WEB_ENGINE
    const auto mail = Utils::MiniApps::getMailId();
    if (const auto iter = pagesByType_.find(mail); iter != pagesByType_.end())
    {
        auto query = QUrlQuery();
        query.addQueryItem(qsl("to"), _email);
        openApp(mail, query.toString());
    }
#endif//HAS_WEB_ENGINE
}

void Ui::AppsPage::appUnavailable(const QString& _id)
{
#ifdef HAS_WEB_ENGINE
    im_assert(isWebApp(_id));
    const auto iter = pagesByType_.find(_id);
    if (iter != pagesByType_.end())
    {
        if (auto page = dynamic_cast<Ui::WebAppPage*>(iter->second.page_.data()))
            page->markUnavailable(true);
    }
#endif //HAS_WEB_ENGINE
}

void Ui::AppsPage::setHeaderVisible(bool _isVisible)
{
    if (!header_)
        return;

    header_->updateVisibility(_isVisible);

    if (pages_)
    {
        pages_->move(pages_->geometry().left(), getHeaderHeight());
        pages_->resize(pages_->width(), height() - getHeaderHeight());
    }
}

void Ui::AppsPage::reopenDialog(const QString& _aimid)
{
    Q_EMIT Utils::InterConnector::instance().searchEnd();
    currentMessengerDialog_.clear();
    Utils::InterConnector::instance().openDialog(_aimid, -1, true, Ui::PageOpenedAs::ReopenedMainPage);
}

void Ui::AppsPage::setPageToReopen(const QString& _aimId)
{
    currentMessengerDialog_ = _aimId;
}

#include "stdafx.h"
#include "TopPanel.h"
#include "SearchModel.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../../my_info.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SChar.h"
#include "../../utils/gui_coll_helper.h"

#include "../common.shared/config/config.h"

#include "../../controls/TextUnit.h"
#include "../../main_window/contact_list/SearchWidget.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "styles/ThemeParameters.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"

#include "../../controls/ContactAvatarWidget.h"

namespace
{
    constexpr auto left_margin = 16;
    constexpr auto right_margin = 12;
    constexpr auto arrow_top_margin = 2;
    constexpr auto arrow_left_margin = 4;
    constexpr auto arrow_size = 16;

    QSize defaultHeaderButtonSize()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    int badgeTopOffset()
    {
        return Utils::scale_value(8);
    }

    QFont getBadgeTextFont()
    {
        return Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
    }

    auto badgeTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
    }

    int getHeaderHeight()
    {
        return Utils::scale_value(56);
    }

    std::unique_ptr<Styling::BaseStyleSheetGenerator> deleteButtonStyleSheet()
    {
        QString qss = ql1s("QPushButton { color: %1; }"
                           "QPushButton:hover { color: %2;}"
                           "QPushButton:press { color: %3;}");

        return std::make_unique<Styling::ArrayStyleSheetGenerator>(qss, std::vector<Styling::ThemeColorKey>{
            Styling::ThemeColorKey { Styling::StyleVariable::SECONDARY_ATTENTION },
            Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_HOVER },
            Styling::ThemeColorKey { Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE }
        });
    }

    int getButtonsHorMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    int getSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    int titleTopMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    int getThreadHeaderLeftMargin() noexcept
    {
        return Utils::scale_value(8);
    }
}

namespace Ui
{
    OverlayTopWidget::OverlayTopWidget(HeaderTitleBar* _parent)
        : QWidget(_parent)
        , parent_(_parent)
    {
        // BGCOLOR
        setStyleSheet(ql1s("background: %1; border-style: none;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    OverlayTopWidget::~OverlayTopWidget() = default;

    void OverlayTopWidget::paintEvent(QPaintEvent* _event)
    {
        if (parent_ && !parent_->buttons_.empty())
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            for (const auto& b : parent_->buttons_)
            {
                if (b && b->isVisible() && !b->getBadgeText().isEmpty())
                    b->drawBadge(p);
            }
        }
    }


    //----------------------------------------------------------------------

    TopPanelHeader::TopPanelHeader(QWidget * _parent)
        : QWidget(_parent)
        , state_(LeftPanelState::min)
    {
    }

    LeftPanelState TopPanelHeader::getState() const
    {
        return state_;
    }

    void TopPanelHeader::setState(const LeftPanelState _state)
    {
        state_ = _state;
    }

    //----------------------------------------------------------------------

    SearchHeader::SearchHeader(QWidget* _parent, const QString& _text)
        : TopPanelHeader(_parent)
        , titleBar_(new HeaderTitleBar(this))
    {
        auto layout = Utils::emptyVLayout(this);

        setStyleSheet(ql1s("background: %1; border-style: none;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE)));

        titleBar_->setTitle(_text);
        Testing::setAccessibleName(titleBar_, qsl("AS ResentSearch titleBar"));
        layout->addWidget(titleBar_);

        auto cancel = new HeaderTitleBarButton(this);
        cancel->setText(QT_TRANSLATE_NOOP("contact_list", "Cancel"));
        cancel->setFont(Fonts::appFontScaled(15));
        cancel->setPersistent(true);

        cancel->setNormalTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        cancel->setHoveredTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER });
        cancel->setPressedTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
        Testing::setAccessibleName(cancel, qsl("AS ResentSearch cancelButton"));

        titleBar_->addButtonToRight(cancel);
        cancel->setFixedHeight(Utils::scale_value(20));
        cancel->setFixedWidth(cancel->sizeHint().width());

        connect(cancel, &CustomButton::clicked, this, &SearchHeader::cancelClicked);
        setFixedHeight(getHeaderHeight());
    }

    //----------------------------------------------------------------------

    UnknownsHeader::UnknownsHeader(QWidget* _parent)
        : TopPanelHeader(_parent)
    {
        auto layout = Utils::emptyHLayout(this);

        leftSpacer_ = new QWidget(this);
        leftSpacer_->setFixedWidth(Utils::scale_value(left_margin));
        layout->addWidget(leftSpacer_);

        backBtn_ = new CustomButton(this, qsl(":/controls/back_icon"), QSize(20, 20));
        backBtn_->setFixedSize(Utils::scale_value(QSize(12, 32)));
        Styling::Buttons::setButtonDefaultColors(backBtn_);
        Testing::setAccessibleName(backBtn_, qsl("AS Unknown backButton"));
        layout->addWidget(backBtn_);

        spacer_ = new QWidget(this);
        spacer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(spacer_);

        delAllBtn_ = new QPushButton(QT_TRANSLATE_NOOP("contact_list", "CLOSE ALL"), this);
        delAllBtn_->setFlat(true);
        delAllBtn_->setFocusPolicy(Qt::NoFocus);
        delAllBtn_->setCursor(Qt::PointingHandCursor);
        // FONT NOT FIXED
        delAllBtn_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
        Testing::setAccessibleName(delAllBtn_, qsl("AS Unknown closeAllButton"));
        Styling::setStyleSheet(delAllBtn_, deleteButtonStyleSheet(), Styling::StyleSheetPolicy::UseSetStyleSheet);

        layout->addWidget(delAllBtn_);

        rightSpacer_ = new QWidget(this);
        rightSpacer_->setFixedWidth(Utils::scale_value(right_margin));
        layout->addWidget(rightSpacer_);

        connect(backBtn_, &CustomButton::clicked, this, &UnknownsHeader::backClicked);
        connect(delAllBtn_, &QPushButton::clicked, this, &UnknownsHeader::deleteAllClicked);
    }

    void UnknownsHeader::deleteAllClicked()
    {
        Q_EMIT Utils::InterConnector::instance().unknownsDeleteThemAll();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_closeall);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_unknown_senders_close_all);
    }

    void UnknownsHeader::setState(const LeftPanelState _state)
    {
        const auto showFull = _state != LeftPanelState::picture_only;
        delAllBtn_->setVisible(showFull);
        spacer_->setVisible(showFull);

        const auto isCompact = (_state == LeftPanelState::picture_only);

        leftSpacer_->setVisible(!isCompact);
        rightSpacer_->setVisible(!isCompact);

        TopPanelHeader::setState(_state);
    }

    //----------------------------------------------------------------------

    RecentsHeader::RecentsHeader(QWidget* _parent)
        : TopPanelHeader(_parent)
        , titleBar_(new HeaderTitleBar(this))
    {
        auto layout = Utils::emptyVLayout(this);

        titleBar_->setTitle(QT_TRANSLATE_NOOP("head", "Chats"));

        if (!Features::isStatusInAppsNavigationBar())
        {
            statusWidget_ = new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(24), true);
            statusWidget_->SetMode(ContactAvatarWidget::Mode::ChangeStatus);
            statusWidget_->setStatusTooltipEnabled(true);
            statusWidget_->setFixedSize(Utils::scale_value(32), Utils::scale_value(32));
            Testing::setAccessibleName(statusWidget_, qsl("AS RecentsTab statusButton"));
            titleBar_->addCentralWidget(statusWidget_);

            connect(MyInfo(), &my_info::received, this, [this]()
            {
                statusWidget_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
            });
        }

        Testing::setAccessibleName(titleBar_, qsl("AS RecentsTab titleBar"));
        layout->addWidget(titleBar_);

        pencil_ = new HeaderTitleBarButton(this);
        pencil_->setPersistent(true);
        pencil_->setDefaultImage(qsl(":/header/pencil"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }, QSize(24, 24));
        pencil_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Write"));

        addButtonToRight(pencil_);

        Testing::setAccessibleName(pencil_, qsl("AS RecentsTab writeButton"));

        connect(pencil_, &CustomButton::clicked, this, &RecentsHeader::pencilClicked);
        connect(pencil_, &CustomButton::clicked, this, &RecentsHeader::onPencilClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::titleButtonsUpdated, this, &RecentsHeader::titleButtonsUpdated);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideSearchDropdown, this, [this]() { pencil_->setActive(false); });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &RecentsHeader::updateTitle);
        connect(get_gui_settings(), &qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_allow_statuses))
                updateTitle();
        });

        updateTitle();
    }

    void RecentsHeader::onPencilClicked()
    {
        Q_EMIT Utils::InterConnector::instance().setSearchFocus();
        Q_EMIT Utils::InterConnector::instance().showSearchDropdownFull();

        if constexpr (platform::is_linux())
            Q_EMIT Utils::InterConnector::instance().hideTitleButtons();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::pencil_click);
        pencil_->setActive(true);
    }

    void RecentsHeader::updateTitle()
    {
        if (!Features::isStatusInAppsNavigationBar())
        {
            const auto showStatus = Statuses::isStatusEnabled();
            titleBar_->setTitleVisible(!showStatus);
            titleBar_->setCentralWidgetVisible(showStatus);
        }
        refresh();
    }

    void RecentsHeader::titleButtonsUpdated()
    {
        if (platform::is_linux() && state_ == LeftPanelState::picture_only)
            Q_EMIT Utils::InterConnector::instance().hideTitleButtons();
    }

    void RecentsHeader::addButtonToLeft(HeaderTitleBarButton* _button)
    {
        titleBar_->addButtonToLeft(_button);
    }

    void RecentsHeader::addButtonToRight(HeaderTitleBarButton* _button)
    {
        titleBar_->addButtonToRight(_button);
    }

    void RecentsHeader::refresh()
    {
        titleBar_->refresh();
        update();
    }

    void RecentsHeader::setState(const LeftPanelState _state)
    {
        titleBar_->setCompactMode(_state == LeftPanelState::picture_only);
        updateTitle();
        TopPanelHeader::setState(_state);
        refresh();
    }

    //----------------------------------------------------------------------

    TopPanelWidget::TopPanelWidget(QWidget* _parent)
        : TopPanelHeader(_parent)
        , stackWidget_(new QStackedWidget(this))
        , regime_(Regime::Invalid)
    {
        auto layout = Utils::emptyVLayout(this);

        auto addHeader = [this](auto _header, const QString& _accessibleName)
        {
            Testing::setAccessibleName(_header, _accessibleName);
            stackWidget_->addWidget(_header);

            return _header;
        };

        recentsHeader_ = addHeader(new RecentsHeader(this), qsl("AS Recents header"));
        unknownsHeader_ = addHeader(new UnknownsHeader(this), qsl("AS Unknown header"));
        searchHeader_ = addHeader(new SearchHeader(this, QT_TRANSLATE_NOOP("head", "Search")), qsl("AS Search header"));

        stackWidget_->setCurrentWidget(recentsHeader_);

        layout->addWidget(stackWidget_);

        searchWidget_ = new SearchWidget(this);

        searchWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        stackWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        Testing::setAccessibleName(searchWidget_, qsl("AS Search widget"));
        layout->addWidget(searchWidget_);

        connect(searchWidget_, &Ui::SearchWidget::activeChanged, this, &TopPanelWidget::searchActiveChanged);
        connect(searchWidget_, &Ui::SearchWidget::escapePressed, this, &TopPanelWidget::searchEscapePressed);

        connect(recentsHeader_, &RecentsHeader::needSwitchToRecents, this, &TopPanelWidget::needSwitchToRecents);
        connect(unknownsHeader_, &UnknownsHeader::backClicked, this, &TopPanelWidget::back);
        connect(searchHeader_, &SearchHeader::cancelClicked, this, &TopPanelWidget::searchCancelled);
        connect(searchHeader_, &SearchHeader::cancelClicked, searchWidget_, &SearchWidget::searchCompleted);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &TopPanelWidget::updateHeader);
        connect(get_gui_settings(), &qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_allow_statuses))
                updateHeader();
        });

        setRegime(Regime::Recents);
        setState(LeftPanelState::normal);
        updateGeometry();

        Testing::setAccessibleName(stackWidget_, qsl("AS TopPanelWidget stackWidget"));
    }

    void TopPanelWidget::setState(const LeftPanelState _state)
    {
        if (state_ == _state)
        {
            updateHeader();
            return;
        }
        TopPanelHeader::setState(_state);

        recentsHeader_->setState(_state);
        unknownsHeader_->setState(_state);

        const auto searchWasVisible = searchWidget_->isVisible();
        updateSearchWidgetVisibility();
        if (searchWasVisible && !searchWidget_->isVisible())
            endSearch();
        else
            updateHeader();
    }

    void TopPanelWidget::setRegime(const Regime _regime)
    {
        im_assert(_regime != Regime::Invalid);
        if (regime_ == _regime)
            return;

        if (isSearchRegime(regime_) && !isSearchRegime(_regime))
            Q_EMIT Utils::InterConnector::instance().searchClosed();

        regime_ = _regime;

        switch (_regime)
        {
        case Regime::Recents:
            stackWidget_->setCurrentWidget(recentsHeader_);
            break;
        case Regime::Unknowns:
            stackWidget_->setCurrentWidget(unknownsHeader_);
            break;
        case Regime::Search:
            stackWidget_->setCurrentWidget(searchHeader_);
            break;

        default:
            break;
        }
        updateSearchWidgetVisibility();
        updateHeader();
    }

    TopPanelWidget::Regime TopPanelWidget::getRegime() const
    {
        return regime_;
    }

    SearchWidget* TopPanelWidget::getSearchWidget() const noexcept
    {
        return searchWidget_;
    }

    RecentsHeader* TopPanelWidget::getRecentsHeader() const noexcept
    {
        return recentsHeader_;
    }

    void TopPanelWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    bool TopPanelWidget::isSearchRegime(const Regime _regime)
    {
        return _regime == Regime::Search;
    }

    void TopPanelWidget::onPencilClicked()
    {
        if (!searchWidget_->isEmpty())
            endSearch();
    }

    void TopPanelWidget::searchActiveChanged(const bool _active)
    {
        if (const auto mp = Utils::InterConnector::instance().getMessengerPage())
        {
            if (_active && mp->isSearchInDialog() && config::get().is_on(config::features::add_contact))
                Q_EMIT Utils::InterConnector::instance().showSearchDropdownAddContact();
        }
    }

    void TopPanelWidget::searchEscapePressed()
    {
        if (state_ != LeftPanelState::spreaded)
            endSearch();
    }

    void TopPanelWidget::endSearch()
    {
        Q_EMIT needSwitchToRecents();
        Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();

        searchWidget_->setDefaultPlaceholder();
        updateHeader();
    }

    void TopPanelWidget::updateSearchWidgetVisibility()
    {
        searchWidget_->setVisible(isSearchWidgetVisible());
    }

    void TopPanelWidget::updateHeader()
    {
        recentsHeader_->updateTitle();
        stackWidget_->adjustSize();
        stackWidget_->updateGeometry();
        setFixedHeight(stackWidget_->height() + (isSearchWidgetVisible() ? searchWidget_->height() : 0));
        update();
    }

    bool TopPanelWidget::isSearchWidgetVisible()
    {
        return state_ != LeftPanelState::picture_only && (regime_ == Regime::Recents || isSearchRegime(regime_));
    }

    //----------------------------------------------------------------------

    HeaderTitleBarButton::HeaderTitleBarButton(QWidget* _parent)
        : CustomButton(_parent)
        , visibility_(true)
        , isPersistent_(false)
    {
        setFocusPolicy(Qt::NoFocus);
        badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText_);

        TextRendering::TextUnit::InitializeParameters params{ getBadgeTextFont(), badgeTextColor() };
        params.align_ = TextRendering::HorAligment::CENTER;
        badgeTextUnit_->init(params);
        setStyleSheet(qsl("border: none; outline: none;"));
    }

    HeaderTitleBarButton::~HeaderTitleBarButton()
    {
    }

    void HeaderTitleBarButton::setBadgeText(const QString& _text)
    {
        if (badgeText_ != _text)
        {
            badgeText_ = _text;
            badgeTextUnit_->setText(badgeText_);
            if (auto w = parentWidget())
                w->update();
        }
    }

    QString HeaderTitleBarButton::getBadgeText() const
    {
        return badgeText_;
    }

    void HeaderTitleBarButton::setPersistent(bool _value) noexcept
    {
        isPersistent_ = _value;
    }

    bool HeaderTitleBarButton::isPersistent() const noexcept
    {
        return isPersistent_;
    }

    void HeaderTitleBarButton::drawBadge(QPainter& _p) const
    {
        Utils::PainterSaver saver(_p);
        const auto badgeWidth = Utils::Badge::getSize(badgeTextUnit_->getSourceText().size()).width();
        Utils::Badge::drawBadge(badgeTextUnit_, _p, geometry().right() + badgeWidth / 2, badgeTopOffset(), Utils::Badge::Color::Green);
    }

    void HeaderTitleBarButton::setVisibility(bool _value)
    {
        if (visibility_ != _value)
            visibility_ = _value;
    }

    bool HeaderTitleBarButton::getVisibility() const noexcept
    {
        return visibility_;
    }

    HeaderTitleBar::HeaderTitleBar(QWidget* _parent)
        : QWidget(_parent)
        , mainLayout_(Utils::emptyGridLayout(this))
        , leftWidget_(new QWidget())
        , centerWidget_(new QWidget())
        , rightWidget_(new QWidget())
        , leftSpacer_(nullptr)
        , rightSpacer_(nullptr)
        , overlayWidget_(new OverlayTopWidget(this))
        , isCompactMode_(false)
        , buttonSize_(defaultHeaderButtonSize())
        , arrowVisible_(false)
        , titleVisible_(true)
    {
        leftLayout_ = Utils::emptyHLayout(leftWidget_);
        centerLayout_ = Utils::emptyHLayout(centerWidget_);
        rightLayout_ = Utils::emptyHLayout(rightWidget_);

        leftLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        centerLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        centerLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        rightLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        leftSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Preferred);
        rightSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Preferred);

        leftLayout_->setSpacing(getSpacing());
        rightLayout_->setSpacing(getSpacing());

        leftWidget_->setStyleSheet(qsl("background: transparent; border: none;"));
        centerWidget_->setStyleSheet(qsl("background: transparent; border: none;"));
        rightWidget_->setStyleSheet(qsl("background: transparent; border: none;"));

        leftLayout_->setContentsMargins(getButtonsHorMargin(), 0, 0, 0);
        centerLayout_->setContentsMargins(Utils::scale_value(6), Utils::scale_value(8), 0, 0);
        rightLayout_->setContentsMargins(0, 0, getButtonsHorMargin(), 0);

        mainLayout_->addWidget(leftWidget_, 0, 0);
        mainLayout_->addItem(leftSpacer_, 0, 1);
        mainLayout_->addWidget(centerWidget_, 0, 2);
        mainLayout_->addItem(rightSpacer_, 0, 3);
        mainLayout_->addWidget(rightWidget_, 0, 4);

        titleTextUnit_ = TextRendering::MakeTextUnit(title_);
        titleTextUnit_->init({ Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });

        subtitleTextUnit_ = TextRendering::MakeTextUnit(subtitle_);
        subtitleTextUnit_->init({ Fonts::appFontScaled(14), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });

        for (auto w : { leftWidget_, centerWidget_, rightWidget_, (QWidget*)this })
            w->setMouseTracking(true);

        refresh();
    }

    HeaderTitleBar::~HeaderTitleBar() = default;

    void HeaderTitleBar::setTitle(const QString& _title)
    {
        if (title_ != _title)
        {
            title_ = _title;
            titleVisible_ = true;
            titleTextUnit_->setText(title_);
            update();
        }
        refresh();
    }

    void HeaderTitleBar::setSubTitle(const QString& _subtitle)
    {
        if (subtitle_ != _subtitle)
        {
            subtitle_ = _subtitle;
            subtitleTextUnit_->setText(subtitle_);
            update();
        }
        refresh();
    }

    void HeaderTitleBar::addButtonToLeft(HeaderTitleBarButton* _button)
    {
        leftLayout_->insertWidget(leftLayout_->count() - 1, _button);
        addButtonImpl(_button);
    }

    void HeaderTitleBar::addButtonToRight(HeaderTitleBarButton* _button)
    {
        rightLayout_->insertWidget(1, _button);
        addButtonImpl(_button);
    }

    void HeaderTitleBar::addCentralWidget(QWidget* _widget)
    {
        centerLayout_->insertWidget(1, _widget);
        refresh();
    }

    void HeaderTitleBar::addButtonImpl(HeaderTitleBarButton* _button)
    {
        Styling::Buttons::setButtonDefaultColors(_button);
        _button->setFixedSize(buttonSize_);
        buttons_.emplace_back(_button);
        refresh();
    }

    void HeaderTitleBar::setButtonSize(QSize _size)
    {
        if (buttonSize_ != _size)
        {
            buttonSize_ = _size;
            if (!buttons_.empty())
            {
                for (const auto& button : buttons_)
                    button->setFixedSize(buttonSize_);
                refresh();
            }
        }
    }

    void HeaderTitleBar::setCompactMode(bool _value)
    {
        if (isCompactMode_ != _value)
        {
            isCompactMode_ = _value;
            refresh();
        }
    }

    void HeaderTitleBar::refresh()
    {
        for (const auto& b : buttons_)
        {
            if (b)
                b->setVisible(b->isPersistent() || (!isCompactMode_ && b->getVisibility()));
        }

        leftWidget_->setVisible(!isCompactMode_);
        if (isCompactMode_)
        {
            setFixedHeight(Statuses::isStatusEnabled() ? 2 * getHeaderHeight() : getHeaderHeight());
            rightLayout_->setContentsMargins(0, 0, Utils::scale_value(22), 0);
            mainLayout_->removeWidget(leftWidget_);
            mainLayout_->removeWidget(centerWidget_);
            mainLayout_->removeWidget(rightWidget_);
            mainLayout_->removeItem(leftSpacer_);
            mainLayout_->removeItem(rightSpacer_);
            if (!titleVisible_)
            {
                mainLayout_->addWidget(centerWidget_, 0, 0);
                mainLayout_->addWidget(rightWidget_, 1, 0);
            }
            else
            {
                mainLayout_->addWidget(rightWidget_, 0, 0);
            }
        }
        else
        {
            setFixedHeight(getHeaderHeight());
            rightLayout_->setContentsMargins(0, 0, getButtonsHorMargin(), 0);

            mainLayout_->removeWidget(leftWidget_);
            mainLayout_->removeWidget(centerWidget_);
            mainLayout_->removeWidget(rightWidget_);
            mainLayout_->removeItem(leftSpacer_);
            mainLayout_->removeItem(rightSpacer_);
            mainLayout_->addWidget(leftWidget_, 0, 0);
            mainLayout_->addItem(leftSpacer_, 0, 1);
            mainLayout_->addWidget(centerWidget_, 0, 2);
            mainLayout_->addItem(rightSpacer_, 0, 3);
            mainLayout_->addWidget(rightWidget_, 0, 4);
        }
        mainLayout_->invalidate();
        updateSpacers();
        update();
    }

    void HeaderTitleBar::setArrowVisible(bool _visible)
    {
        arrowVisible_ = _visible;
        updateCursor();
        update();
    }

    void HeaderTitleBar::setTitleVisible(bool _visible)
    {
        titleVisible_ = _visible;
        update();
    }

    void HeaderTitleBar::setCentralWidgetVisible(bool _visible)
    {
        centerWidget_->setVisible(_visible);
        update();
    }

    void HeaderTitleBar::setTitleLeftOffset(int _offset)
    {
        titleLeftOffset_ = _offset;
    }

    /*
    static void debugLayout(QPainter *painter, QLayoutItem *item)
    {
        QLayout *layout = item->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i)
                debugLayout(painter, layout->itemAt(i));
        }
        painter->drawRect(item->geometry());
    }
    */

    void HeaderTitleBar::paintEvent(QPaintEvent* _event)
    {
        overlayWidget_->raise();

        /*if (layout())
        debugLayout(&p, layout());
        */

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (!isCompactMode_)
        {
            const auto r = rect();

            if (titleVisible_ && !titleTextUnit_->isEmpty())
            {
                const auto withSubtitle = !subtitleTextUnit_->isEmpty();
                const auto offsetX = withSubtitle ? titleLeftOffset_ + getSpacing() + getButtonsHorMargin() + getThreadHeaderLeftMargin()
                                                  : r.width() / 2 - titleTextUnit_->cachedSize().width() / 2;
                const auto offsetY = withSubtitle ? titleTopMargin() : r.height() / 2;
                titleTextUnit_->setOffsets(offsetX, offsetY);
                titleTextUnit_->draw(p, withSubtitle ? TextRendering::VerPosition::TOP : TextRendering::VerPosition::MIDDLE);

                if (!subtitleTextUnit_->isEmpty())
                {
                    subtitleTextUnit_->setOffsets(offsetX, r.height() - getSpacing());
                    subtitleTextUnit_->draw(p, TextRendering::VerPosition::BOTTOM);
                }
            }

            if (arrowVisible_)
            {
                static auto source = Utils::StyledPixmap::scaled(qsl(":/controls/down_icon"), QSize(arrow_size, arrow_size), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
                const auto arrow = source.actualPixmap();
                p.drawPixmap(r.width() / 2 + titleTextUnit_->cachedSize().width() / 2 + Utils::scale_value(arrow_left_margin), r.height() / 2 - arrow.height() / 2 / Utils::scale_bitmap(1) + Utils::scale_value(arrow_top_margin), arrow);
            }
        }
    }

    void HeaderTitleBar::resizeEvent(QResizeEvent* _event)
    {
        QWidget::resizeEvent(_event);
        overlayWidget_->resize(size());

        int buttonsWidth = 0;
        for (const auto& b : buttons_)
        {
            if (b)
                buttonsWidth += b->width();
        }

        if (subtitleTextUnit_)
        {
            buttonsWidth += 2 * (getSpacing() + getButtonsHorMargin()) + getThreadHeaderLeftMargin();
            subtitleTextUnit_->elide(width() - buttonsWidth);
        }
    }

    void HeaderTitleBar::mouseMoveEvent(QMouseEvent* _event)
    {
        updateCursor();
        QWidget::mouseMoveEvent(_event);
    }

    void HeaderTitleBar::mousePressEvent(QMouseEvent* _event)
    {
        clicked_ = _event->pos();
        QWidget::mousePressEvent(_event);
    }

    void HeaderTitleBar::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (arrowVisible_ && Utils::clicked(clicked_, _event->pos()))
        {
            auto r = QRect(titleTextUnit_->offsets().x(), titleTextUnit_->offsets().y() - titleTextUnit_->cachedSize().height() / 2,
                titleTextUnit_->cachedSize().width() + Utils::scale_value(arrow_size + arrow_left_margin * 2),
                titleTextUnit_->cachedSize().height() + Utils::scale_value(arrow_top_margin * 2));

            if (r.contains(_event->pos()))
                Q_EMIT arrowClicked();
        }

        QWidget::mouseReleaseEvent(_event);
    }

    void HeaderTitleBar::leaveEvent(QEvent* _event)
    {
        updateCursor();
        QWidget::leaveEvent(_event);
    }

    void HeaderTitleBar::enterEvent(QEvent* _event)
    {
        updateCursor();
        QWidget::enterEvent(_event);
    }

    void HeaderTitleBar::updateSpacers()
    {
        const auto dX = (leftWidget_->width() - rightWidget_->width()) / 2;

        auto spacerW = (width() - centerWidget_->width()) / 2 - leftWidget_->width();
        leftSpacer_->changeSize(spacerW, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
        rightSpacer_->changeSize(spacerW - dX, 0, QSizePolicy::Preferred, QSizePolicy::Expanding);
        layout()->invalidate();
    }

    void HeaderTitleBar::updateCursor()
    {
        if (arrowVisible_)
        {
            const auto r = QRect(
                titleTextUnit_->offsets().x(),
                titleTextUnit_->offsets().y() - titleTextUnit_->cachedSize().height() / 2,
                titleTextUnit_->cachedSize().width() + Utils::scale_value(arrow_size + arrow_left_margin * 2),
                titleTextUnit_->cachedSize().height() + Utils::scale_value(arrow_top_margin * 2));

            if (r.contains(mapFromGlobal(QCursor::pos())))
            {
                setCursor(Qt::PointingHandCursor);
                return;
            }
        }
        setCursor(Qt::ArrowCursor);
    }

    EmailTitlebarButton::EmailTitlebarButton(QWidget* _parent)
        : HeaderTitleBarButton(_parent)
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::mailStatus, this, &EmailTitlebarButton::mailStatus);
        connect(this, &HeaderTitleBarButton::clicked, this, &EmailTitlebarButton::openMailBox);
    }

    EmailTitlebarButton::~EmailTitlebarButton() = default;

    void EmailTitlebarButton::mailStatus(const QString& _email, unsigned _unreads, bool)
    {
        email_ = _email;
        setBadgeText(Utils::getUnreadsBadgeStr(int(_unreads)));
    }

    void EmailTitlebarButton::openMailBox()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::titlebar_mail);

        const auto email = email_.isEmpty() ? MyInfo()->aimId() : email_;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("email", email);

        Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [email](core::icollection* _collection)
        {
            Utils::openMailBox(email, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")), QString());
        });
    }
}

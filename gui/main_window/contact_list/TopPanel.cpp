#include "stdafx.h"
#include "TopPanel.h"
#include "SearchModel.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../../my_info.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SChar.h"
#include "../../utils/gui_coll_helper.h"

#include "../../controls/TextUnit.h"
#include "../../main_window/contact_list/SearchWidget.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto left_margin = 16;
    constexpr auto right_margin = 12;
    constexpr auto right_margin_linux = 4;
    constexpr auto additional_spacing_linux = 8;
    constexpr auto arrow_top_margin = 2;
    constexpr auto arrow_left_margin = 4;
    constexpr auto arrow_size = 16;

    constexpr auto key_combination_timeout_ms = 1000;

    QSize defaultHeaderButtonSize()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    int badgeTextPadding()
    {
        return Utils::scale_value(4);
    }

    int badgeBorderWidth()
    {
        return Utils::scale_value(1);
    }

    int badgeHeight()
    {
        return Utils::scale_value(18);
    }

    int badgeOffset()
    {
        return Utils::scale_value(12);
    }

    int badgeTopOffset()
    {
        return Utils::scale_value(8);
    }

    QFont getBadgeTextFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(10, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(11);
    }

    QColor badgeTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor badgeBalloonColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    int getHeaderHeight()
    {
        return Utils::scale_value(56);
    }
}

namespace Ui
{
    OverlayTopWidget::OverlayTopWidget(HeaderTitleBar* _parent)
        : QWidget(_parent)
        , parent_(_parent)
    {
        // BGCOLOR
        setStyleSheet(qsl("background: %1; border-style: none;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    OverlayTopWidget::~OverlayTopWidget() = default;

    void OverlayTopWidget::paintEvent(QPaintEvent* _event)
    {
        if (parent_)
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            for (const auto& b : parent_->buttons_)
            {
                if (b && b->isVisible() && !(b->getBadgeText().isEmpty()))
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

        setStyleSheet(qsl("background: %1; border-style: none;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE)));

        titleBar_->setTitle(_text);
        Testing::setAccessibleName(titleBar_, qsl("AS top search titleBar_"));
        layout->addWidget(titleBar_);

        auto cancel = new HeaderTitleBarButton(this);
        cancel->setText(QT_TRANSLATE_NOOP("contact_list", "Cancel"));
        cancel->setFont(Fonts::appFontScaled(15));
        cancel->setPersistent(true);

        cancel->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        cancel->setHoveredTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER));
        cancel->setPressedTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
        Testing::setAccessibleName(cancel, qsl("AS headerbutton search cancel"));

        titleBar_->addButtonToRight(cancel);
        cancel->setFixedHeight(Utils::scale_value(20));
        cancel->setFixedWidth(cancel->sizeHint().width());

        connect(cancel, &CustomButton::clicked, this, &SearchHeader::cancelClicked);
    }

    //----------------------------------------------------------------------

    UnknownsHeader::UnknownsHeader(QWidget* _parent)
        : TopPanelHeader(_parent)
    {
        auto layout = Utils::emptyHLayout(this);

        leftSpacer_ = new QWidget(this);
        leftSpacer_->setFixedWidth(Utils::scale_value(left_margin));
        Testing::setAccessibleName(leftSpacer_, qsl("AS top leftSpacer_"));
        layout->addWidget(leftSpacer_);

        backBtn_ = new CustomButton(this, qsl(":/controls/back_icon"), QSize(20, 20));
        backBtn_->setFixedSize(Utils::scale_value(QSize(12, 32)));
        Styling::Buttons::setButtonDefaultColors(backBtn_);
        Testing::setAccessibleName(backBtn_, qsl("AS top backBtn_"));
        layout->addWidget(backBtn_);

        spacer_ = new QWidget(this);
        spacer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        Testing::setAccessibleName(spacer_, qsl("AS top spacer_"));
        layout->addWidget(spacer_);

        delAllBtn_ = new QPushButton(QT_TRANSLATE_NOOP("contact_list", "CLOSE ALL"), this);
        delAllBtn_->setFlat(true);
        delAllBtn_->setFocusPolicy(Qt::NoFocus);
        delAllBtn_->setCursor(Qt::PointingHandCursor);
        // FONT NOT FIXED
        delAllBtn_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
        Utils::ApplyStyle(delAllBtn_,
            qsl(
                "QPushButton { color: %1; }"
                "QPushButton:hover { color: %2;}"
                "QPushButton:press { color: %3;}"
            )
            .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::SECONDARY_ATTENTION),
                Styling::getParameters().getColorHex(Styling::StyleVariable::SECONDARY_ATTENTION_HOVER),
                Styling::getParameters().getColorHex(Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE))
        );

        Testing::setAccessibleName(delAllBtn_, qsl("AS top delAllBtn_"));
        layout->addWidget(delAllBtn_);

        rightSpacer_ = new QWidget(this);
        rightSpacer_->setFixedWidth(Utils::scale_value(right_margin));
        Testing::setAccessibleName(rightSpacer_, qsl("AS top rightSpacer_"));
        layout->addWidget(rightSpacer_);

        connect(backBtn_, &CustomButton::clicked, this, &UnknownsHeader::backClicked);
        connect(delAllBtn_, &QPushButton::clicked, this, &UnknownsHeader::deleteAllClicked);
    }

    void UnknownsHeader::deleteAllClicked()
    {
        emit Utils::InterConnector::instance().unknownsDeleteThemAll();
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

        Testing::setAccessibleName(titleBar_, qsl("AS top titleBar_"));
        layout->addWidget(titleBar_);

        pencil_ = new HeaderTitleBarButton(this);
        pencil_->setPersistent(true);
        pencil_->setDefaultImage(qsl(":/header/pencil"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY), QSize(24, 24));
        pencil_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Write"));

        addButtonToRight(pencil_);

        Testing::setAccessibleName(pencil_, qsl("AS headerbutton pencil_"));

        connect(pencil_, &CustomButton::clicked, this, &RecentsHeader::pencilClicked);
        connect(pencil_, &CustomButton::clicked, this, &RecentsHeader::onPencilClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::titleButtonsUpdated, this, &RecentsHeader::titleButtonsUpdated);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideSearchDropdown, this, [this]() { pencil_->setActive(false); });
    }

    void RecentsHeader::onPencilClicked()
    {
        emit Utils::InterConnector::instance().setSearchFocus();
        emit Utils::InterConnector::instance().showSearchDropdownFull();

        if constexpr (platform::is_linux())
            emit Utils::InterConnector::instance().hideTitleButtons();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::pencil_click);
        pencil_->setActive(true);
    }

    void RecentsHeader::titleButtonsUpdated()
    {
        if (platform::is_linux() && state_ == LeftPanelState::picture_only)
            emit Utils::InterConnector::instance().hideTitleButtons();
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
    }

    void RecentsHeader::setState(const LeftPanelState _state)
    {
        titleBar_->setCompactMode(_state == LeftPanelState::picture_only);

        TopPanelHeader::setState(_state);
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
            _header->setFixedHeight(getHeaderHeight());

            _header->setStyleSheet(qsl("background: %1; border-style: none;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
            Testing::setAccessibleName(_header, qsl("AS top ") % _accessibleName);
            stackWidget_->addWidget(_header);

            return _header;
        };

        recentsHeader_ = addHeader(new RecentsHeader(this), qsl("recentsHeader_"));
        unknownsHeader_ = addHeader(new UnknownsHeader(this), qsl("unknownsHeader_"));
        searchHeader_ = addHeader(new SearchHeader(this, QT_TRANSLATE_NOOP("head", "Search")), qsl("searchHeader_"));

        stackWidget_->setCurrentWidget(recentsHeader_);

        stackWidget_->setFixedHeight(getHeaderHeight());
        layout->addWidget(stackWidget_);

        searchWidget_ = new SearchWidget(this);

        searchWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        Testing::setAccessibleName(searchWidget_, qsl("AS top searchWidget_"));
        layout->addWidget(searchWidget_);

        // BGCOLOR
        Utils::ApplyStyle(this,
            qsl("background-color: %1;"
                /*"border-right-style: solid;"
                "border-right-color: %2;"
                "border-radius: 16dip;"*/)
            .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE))
            //.arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE))
        );

        connect(searchWidget_, &Ui::SearchWidget::activeChanged, this, &TopPanelWidget::searchActiveChanged);
        connect(searchWidget_, &Ui::SearchWidget::escapePressed, this, &TopPanelWidget::searchEscapePressed);

        connect(recentsHeader_, &RecentsHeader::needSwitchToRecents, this, &TopPanelWidget::needSwitchToRecents);
        connect(unknownsHeader_, &UnknownsHeader::backClicked, this, &TopPanelWidget::back);
        connect(searchHeader_, &SearchHeader::cancelClicked, this, &TopPanelWidget::searchCancelled);
        connect(searchHeader_, &SearchHeader::cancelClicked, searchWidget_, &SearchWidget::searchCompleted);

        setRegime(Regime::Recents);
        setState(LeftPanelState::normal);

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    void TopPanelWidget::setState(const LeftPanelState _state)
    {
        if (state_ == _state)
            return;
        TopPanelHeader::setState(_state);

        recentsHeader_->setState(_state);
        unknownsHeader_->setState(_state);

        const auto searchWasVisible = searchWidget_->isVisible();
        updateSearchWidgetVisibility();
        if (searchWasVisible && !searchWidget_->isVisible())
            endSearch();
    }

    void TopPanelWidget::setRegime(const Regime _regime)
    {
        assert(_regime != Regime::Invalid);
        if (regime_ == _regime)
            return;

        if (isSearchRegime(regime_) && !isSearchRegime(_regime))
            emit Utils::InterConnector::instance().searchClosed();

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
        if (const auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            if (const auto mp = mw->getMainPage())
            {
                if (_active && mp->isSearchInDialog() && !build::is_biz())
                    emit Utils::InterConnector::instance().showSearchDropdownAddContact();
            }
        }
    }

    void TopPanelWidget::searchEscapePressed()
    {
        if (state_ != LeftPanelState::spreaded)
            endSearch();
    }

    void TopPanelWidget::endSearch()
    {
        emit needSwitchToRecents();
        emit Utils::InterConnector::instance().hideSearchDropdown();

        searchWidget_->setDefaultPlaceholder();
    }

    void TopPanelWidget::updateSearchWidgetVisibility()
    {
        searchWidget_->setVisible(state_ != LeftPanelState::picture_only && (regime_ == Regime::Recents || isSearchRegime(regime_)));
    }

    //----------------------------------------------------------------------

    HeaderTitleBarButton::HeaderTitleBarButton(QWidget* _parent)
        : CustomButton(_parent)
        , visibility_(true)
        , isPersistent_(false)
    {
        setFocusPolicy(Qt::NoFocus);
        badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText_);
        badgeTextUnit_->init(getBadgeTextFont(), badgeTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
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
        const auto geom = geometry();
        Utils::Badge::drawBadge(badgeTextUnit_, _p, geom.x() + badgeOffset(), badgeTopOffset(), Utils::Badge::Color::Green);
    }

    void HeaderTitleBarButton::setVisibility(bool _value)
    {
        if (visibility_ != _value)
        {
            visibility_ = _value;
        }
    }

    bool HeaderTitleBarButton::getVisibility() const noexcept
    {
        return visibility_;
    }

    HeaderTitleBar::HeaderTitleBar(QWidget* _parent)
        : QWidget(_parent)
        , LeftLayout_(Utils::emptyHLayout())
        , RightLayout_(Utils::emptyHLayout())
        , overlayWidget_(new OverlayTopWidget(this))
        , isCompactMode_(false)
        , buttonSize_(defaultHeaderButtonSize())
        , arrowVisible_(false)
    {
        setFixedHeight(getHeaderHeight());

        LeftLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
        RightLayout_->setContentsMargins(0, 0, Utils::scale_value(16), 0);

        LeftLayout_->addSpacerItem(new QSpacerItem(0, height(), QSizePolicy::Expanding));
        RightLayout_->addSpacerItem(new QSpacerItem(0, height(), QSizePolicy::Expanding));

        LeftLayout_->setSpacing(Utils::scale_value(8));
        RightLayout_->setSpacing(Utils::scale_value(8));

        auto layout = Utils::emptyHLayout(this);
        layout->addLayout(LeftLayout_);
        layout->addSpacerItem(new QSpacerItem(0, height(), QSizePolicy::Expanding));
        layout->addLayout(RightLayout_);

        titleTextUnit_ = TextRendering::MakeTextUnit(title_);
        // FONT
        titleTextUnit_->init(Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold),
                             Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        setMouseTracking(true);
    }

    HeaderTitleBar::~HeaderTitleBar()
    {
    }

    void HeaderTitleBar::setTitle(const QString& _title)
    {
        if (title_ != _title)
        {
            title_ = _title;
            // FONT NOT FIXED
            titleTextUnit_->setText(title_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            update();
        }
    }

    void HeaderTitleBar::addButtonToLeft(HeaderTitleBarButton* _button)
    {
        LeftLayout_->insertWidget(LeftLayout_->count() - 1, _button);
        addButtonImpl(_button);
    }

    void HeaderTitleBar::addButtonToRight(HeaderTitleBarButton * _button)
    {
        RightLayout_->insertWidget(1, _button);
        addButtonImpl(_button);
    }

    void HeaderTitleBar::addButtonImpl(HeaderTitleBarButton * _button)
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

        if (isCompactMode_)
        {
            LeftLayout_->setContentsMargins(Utils::scale_value(22), 0, 0, 0);
            RightLayout_->setContentsMargins(0, 0, Utils::scale_value(22), 0);
        }
        else
        {
            LeftLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
            RightLayout_->setContentsMargins(0, 0, Utils::scale_value(16), 0);
        }

        update();
    }

    void HeaderTitleBar::setArrowVisible(bool _visible)
    {
        arrowVisible_ = _visible;
        update();
    }

    static void debugLayout(QPainter *painter, QLayoutItem *item)
    {
        QLayout *layout = item->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i)
                debugLayout(painter, layout->itemAt(i));
        }
        painter->drawRect(item->geometry());
    }

    void HeaderTitleBar::paintEvent(QPaintEvent* _event)
    {
        overlayWidget_->raise();

        /*if (layout())
        debugLayout(&p, layout());
        */

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        if (!isCompactMode_)
        {
            const auto r = rect();

            titleTextUnit_->setOffsets(r.width() / 2 - titleTextUnit_->cachedSize().width() / 2, r.height() / 2);
            titleTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);

            if (arrowVisible_)
            {
                static auto arrow = Utils::renderSvgScaled(qsl(":/controls/down_icon"), QSize(arrow_size, arrow_size), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
                p.drawPixmap(r.width() / 2 + titleTextUnit_->cachedSize().width() / 2 + Utils::scale_value(arrow_left_margin), r.height() / 2 - arrow.height() / 2 / Utils::scale_bitmap(1) + Utils::scale_value(arrow_top_margin), arrow);
            }
        }
    }

    void HeaderTitleBar::resizeEvent(QResizeEvent* _event)
    {
        QWidget::resizeEvent(_event);
        overlayWidget_->resize(_event->size());
    }

    void HeaderTitleBar::mouseMoveEvent(QMouseEvent* _event)
    {
        if (arrowVisible_)
        {
            auto r = QRect(titleTextUnit_->offsets().x(), titleTextUnit_->offsets().y() - titleTextUnit_->cachedSize().height() / 2,
                titleTextUnit_->cachedSize().width() + Utils::scale_value(arrow_size + arrow_left_margin * 2),
                titleTextUnit_->cachedSize().height() + Utils::scale_value(arrow_top_margin * 2));

            setCursor(r.contains(_event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }

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
                emit arrowClicked();
        }

        QWidget::mouseReleaseEvent(_event);
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

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("email", email_.isEmpty() ? MyInfo()->aimId() : email_);
        Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [this](core::icollection* _collection)
        {
            Utils::openMailBox(email_, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")), QString());
        });
    }
}

#include "stdafx.h"

#include "ContactListUtils.h"
#include "ContactListModel.h"
#include "ContactListWithHeaders.h"
#include "ContactListItemDelegate.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModel.h"
#include "CommonChatsModel.h"
#include "Common.h"
#include "../friendly/FriendlyContainer.h"

#include "RecentItemDelegate.h"

#include "../ReportWidget.h"
#include "../GroupChatOperations.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"

#include "../../controls/CustomButton.h"

#include "styles/ThemeParameters.h"

namespace Logic
{
    bool is_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::IGNORE_LIST;
    }

    bool is_admin_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS;
    }

    bool is_select_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS
            || _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    bool is_video_conference_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    QString aimIdFromIndex(const QModelIndex& _index)
    {
        QString aimid;
        if (const auto searchRes = _index.data().value<Data::AbstractSearchResultSptr>())
        {
            aimid = searchRes->getAimId();
        }
        else if (auto cont = _index.data().value<Data::Contact*>())
        {
            aimid = cont->AimId_;
        }
        else if (const auto memberInfo = _index.data().value<Data::ChatMemberInfo*>())
        {
            aimid = memberInfo->AimId_;
        }
        else if (qobject_cast<const Logic::CommonChatsModel*>(_index.model()))
        {
            Data::CommonChatInfo ccInfo = _index.data().value<Data::CommonChatInfo>();
            aimid = ccInfo.aimid_;
        }
        else if (qobject_cast<const Logic::UnknownsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else if (qobject_cast<const Logic::RecentsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }

        if (aimid.startsWith(ql1c('~')))
            return QString();

        return aimid;
    }

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("contact")] = _aimid;
        return result;
    }

    void showContactListPopup(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const QString command = params[qsl("command")].toString();
        const QString aimId = params[qsl("contact")].toString();

        if (command == ql1s("recents/mark_read"))
        {
            const auto dlgState = Logic::getRecentsModel()->getDlgState(aimId);
            if (dlgState.Attention_)
            {
                Logic::getRecentsModel()->setAttention(aimId, false);
            }
            else
            {
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_read);
                Logic::getRecentsModel()->sendLastRead(aimId, true, Logic::RecentsModel::ReadMode::ReadAll);
            }
        }
        else if (command == ql1s("recents/mute"))
        {
            Logic::getRecentsModel()->muteChat(aimId, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mute_recents_menu);
        }
        else if (command == ql1s("recents/unmute"))
        {
            Logic::getRecentsModel()->muteChat(aimId, false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unmute);
        }
        else if (command == ql1s("contacts/ignore"))
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "ContactList_Menu" } });
        }

        else if (command == ql1s("recents/ignore"))
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "Recents_Menu" } });
        }

        else if (command == ql1s("recents/close"))
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_close);
            Logic::getRecentsModel()->hideChat(aimId);
        }
        else if (command == ql1s("recents/read_all"))
        {
            auto readCnt = Logic::getRecentsModel()->markAllRead();
            core::stats::event_props_type props = {
                { "From" , "Menu" },
                { "Chats_Read", std::to_string(readCnt) }
            };

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_read_all, props);
        }
        else if (command == ql1s("recents/mark_unread"))
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_unread);
            if (Logic::getContactListModel()->selectedContact() == aimId)
            {
                emit Logic::getContactListModel()->leave_dialog(aimId);
                Logic::getContactListModel()->setCurrent(QString(), -1);
            }
            Logic::getRecentsModel()->setAttention(aimId, true);
        }
        else if (command == ql1s("contacts/call"))
        {
            Ui::GetDispatcher()->getVoipController().setStartV(aimId.toUtf8().constData(), false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_audio, { { "From", "ContactList_Menu" } });
        }
        else if (command == ql1s("contacts/Profile"))
        {
            emit Utils::InterConnector::instance().profileSettingsShow(aimId);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_open, { { "From", "CL_Popup" } });
        }
        else if (command == ql1s("contacts/spam"))
        {
            if (Ui::ReportContact(aimId, Logic::GetFriendlyContainer()->getFriendly(aimId)))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "ContactList_Menu" } });
            }
        }
        else if (command == ql1s("contacts/remove"))
        {
            const QString text = QT_TRANSLATE_NOOP("popup_window", Logic::getContactListModel()->isChat(aimId)
                ? "Are you sure you want to leave chat?" : "Are you sure you want to delete contact?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("popup_window", "YES"),
                text,
                Logic::GetFriendlyContainer()->getFriendly(aimId),
                nullptr
            );

            if (confirm)
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_contact, { { "From", "ContactList_Menu" } });
            }
        }
        else if (command == ql1s("recents/favorite"))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core("favorite", collection.get());
        }
        else if (command == ql1s("recents/unfavorite"))
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core("unfavorite", collection.get());
        }
        else if (command == ql1s("recents/clear_history"))
        {
            const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("popup_window", "YES"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
                Logic::GetFriendlyContainer()->getFriendly(aimId),
                nullptr);

            if (confirmed)
                Ui::eraseHistory(aimId);
        }
        else if (command == ql1s("recents/report"))
        {
            if (Ui::ReportContact(aimId, Logic::GetFriendlyContainer()->getFriendly(aimId)))
            {
                Logic::getContactListModel()->ignoreContact(aimId, true);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "ContactList_Menu" } });
            }
        }
    }
}

namespace Ui
{
    EmptyIgnoreListLabel::EmptyIgnoreListLabel(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    EmptyIgnoreListLabel::~EmptyIgnoreListLabel()
    {

    }

    void EmptyIgnoreListLabel::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (!serviceContact_)
            serviceContact_ = std::make_unique<Ui::ServiceContact>(QT_TRANSLATE_NOOP("sidebar", "You have no ignored contacts"), Data::ContactType::EMPTY_IGNORE_LIST);

        serviceContact_->paint(painter);
    }

    namespace
    {
        constexpr auto btnHeight = 24;
        constexpr auto btnMaxWidth = 256;
        constexpr auto leftMargin = 8;
        constexpr auto rightMargin = 8;
        const QSize btnSize = QSize(12, 12);

        constexpr auto headerViewHeight = 10 + 24 + 10;
    }

    SearchCategoryButton::SearchCategoryButton(QWidget* _parent, const QString& _text)
        : ClickableWidget(_parent)
        , selected_(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);

        label_ = TextRendering::MakeTextUnit(_text, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

        label_->init(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold), getTextColor());
        label_->setOffsets(Utils::scale_value(leftMargin), Utils::scale_value(btnHeight / 2));
        label_->evaluateDesiredSize();

        setFixedSize(label_->desiredWidth() + Utils::scale_value(leftMargin + rightMargin), Utils::scale_value(btnHeight));
        update();
    }

    SearchCategoryButton::~SearchCategoryButton()
    {
    }

    bool SearchCategoryButton::isSelected() const
    {
        return selected_;
    }

    void SearchCategoryButton::setSelected(const bool _sel)
    {
        if (selected_ == _sel)
            return;

        selected_ = _sel;

        label_->setColor(getTextColor());

        update();
    }

    void SearchCategoryButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        if (selected_)
        {
            p.setRenderHint(QPainter::Antialiasing);

            static const auto radius = Utils::scale_value(btnHeight / 2);
            static const auto bgColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

            p.setBrush(bgColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect(), radius, radius);
        }

        if (label_)
            label_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    QColor SearchCategoryButton::getTextColor() const
    {
        return Styling::getParameters().getColor( selected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_SOLID);
    }

    HeaderScrollOverlay::HeaderScrollOverlay(QWidget* _parent, QScrollArea* _scrollArea)
        : QWidget(_parent)
        , scrollArea_(_scrollArea)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void HeaderScrollOverlay::paintEvent(QPaintEvent * _event)
    {
        const auto scrollWidget = scrollArea_->widget();
        const auto posWidget = mapToGlobal(scrollWidget->pos());
        const auto posViewport = mapToGlobal(pos());

        const QSize gradientSize(Utils::scale_value(32), height());

        const auto drawGradient = [this, &gradientSize](const int _left, const QColor& _start, const QColor& _end)
        {
            QRect grRect(QPoint(_left, 0), gradientSize);

            QLinearGradient gradient(grRect.topLeft(), grRect.topRight());
            gradient.setColorAt(0, _start);
            gradient.setColorAt(1, _end);

            QPainter p(this);
            p.fillRect(grRect, gradient);
        };

        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

        if (posViewport.x() > posWidget.x())
            drawGradient(0, bg, Qt::transparent);

        if (posWidget.x() + scrollWidget->width() > posViewport.x() + width())
            drawGradient(width() - gradientSize.width(), Qt::transparent, bg);
    }

    bool HeaderScrollOverlay::eventFilter(QObject* _watched, QEvent* _event)
    {
        if (_watched == parentWidget() && _event->type() == QEvent::Resize)
            setGeometry(parentWidget()->geometry());

        return QWidget::eventFilter(_watched, _event);
    }

    GlobalSearchViewHeader::GlobalSearchViewHeader(QWidget* _parent)
        : QWidget(_parent)
        , animScroll_(nullptr)
    {
        setFixedHeight(Utils::scale_value(headerViewHeight));

        auto rootLayout = Utils::emptyHLayout(this);

        viewArea_ = new QScrollArea(this);
        viewArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto scrollAreaWidget = new QWidget(viewArea_);
        viewArea_->setWidget(scrollAreaWidget);
        viewArea_->setWidgetResizable(true);
        viewArea_->setFocusPolicy(Qt::NoFocus);

        Testing::setAccessibleName(viewArea_, qsl("AS clu viewArea_"));
        rootLayout->addWidget(viewArea_);

        Utils::grabTouchWidget(viewArea_->viewport(), true);
        Utils::grabTouchWidget(scrollAreaWidget);

        auto layout = Utils::emptyHLayout();
        layout->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        scrollAreaWidget->setLayout(layout);

        auto overlay = new HeaderScrollOverlay(this, viewArea_);
        overlay->raise();
        installEventFilter(overlay);

        const auto addCat = [layout, this](const QString& _text, const QString& _accName)
        {
            auto cat = new SearchCategoryButton(this, _text);
            Testing::setAccessibleName(cat, qsl("AS clu ") % _accName);
            layout->addWidget(cat);

            connect(cat, &SearchCategoryButton::clicked, this, &GlobalSearchViewHeader::onCategoryClicked);

            return cat;
        };

        buttons_ =
        {
            { SearchCategory::ContactsAndGroups, addCat(QT_TRANSLATE_NOOP("search", "CONTACTS AND GROUPS"), qsl("contgroups")) },
            { SearchCategory::Messages, addCat(QT_TRANSLATE_NOOP("search", "MESSAGES"), qsl("messages")) },
        };

        selectFirstVisible();
    }

    void GlobalSearchViewHeader::selectCategory(const SearchCategory _cat, const SelectType _selectType)
    {
        for (const auto[cat, btn] : buttons_)
        {
            const auto isNeededCat = cat == _cat;

            if (_selectType != SelectType::Click)
                btn->setSelected(isNeededCat);

            if (isNeededCat)
                viewArea_->ensureWidgetVisible(btn);
        }

        if (_selectType == SelectType::Click)
            emit categorySelected(_cat);
    }

    void GlobalSearchViewHeader::setCategoryVisible(const SearchCategory _cat, const bool _isVisible)
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [_cat](const auto& it) { return it.first == _cat; });
        if (it != buttons_.end())
            it->second->setVisible(_isVisible);
    }

    void GlobalSearchViewHeader::reset()
    {
        for (const auto [_, btn] : buttons_)
            btn->setVisible(true);

        selectFirstVisible();
    }

    void GlobalSearchViewHeader::selectFirstVisible()
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [](const auto& it) { return it.second->isVisible(); });
        if (it != buttons_.end())
            selectCategory(it->first);
    }

    bool GlobalSearchViewHeader::hasVisibleCategories() const
    {
        return std::any_of(buttons_.begin(), buttons_.end(), [](const auto& it) { return it.second->isVisible(); });
    }

    void GlobalSearchViewHeader::wheelEvent(QWheelEvent* _e)
    {
        const int numDegrees = _e->delta() / 8;
        const int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(direction::left);
        else
            scrollStep(direction::right);

        QWidget::wheelEvent(_e);
    }

    void GlobalSearchViewHeader::scrollStep(direction _direction)
    {
        QRect viewRect = viewArea_->viewport()->geometry();
        auto scrollbar = viewArea_->horizontalScrollBar();

        const int curVal = scrollbar->value();
        const int step = viewRect.width() / 20;
        int to = 0;

        if (_direction == direction::right)
            to = std::min(curVal + step, scrollbar->maximum());
        else
            to = std::max(curVal - step, scrollbar->minimum());

        constexpr auto duration = std::chrono::milliseconds(300);

        if (!animScroll_)
        {
            animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
            animScroll_->setDuration(duration.count());
            animScroll_->setEasingCurve(QEasingCurve::InQuad);
        }

        animScroll_->stop();
        animScroll_->setStartValue(curVal);
        animScroll_->setEndValue(to);
        animScroll_->start();
    }

    void GlobalSearchViewHeader::onCategoryClicked()
    {
        auto newBtn = qobject_cast<SearchCategoryButton*>(sender());
        if (!newBtn)
        {
            assert(false);
            return;
        }

        auto it = std::find_if(buttons_.begin(), buttons_.end(), [newBtn](const auto& it) { return it.second == newBtn; });
        if (it != buttons_.end())
            selectCategory(it->first, SelectType::Click);
    }

    SearchFilterButton::SearchFilterButton(QWidget* _parent)
        : QWidget(_parent)
    {
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

        auto layout = Utils::emptyHLayout(this);
        layout->addStretch();

        auto btn = new CustomButton(this, qsl(":/controls/close_icon_round"), btnSize, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        btn->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        btn->setPressedColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        btn->setFixedSize(Utils::scale_value(QSize(btnSize.width() + rightMargin * 2, btnHeight)));
        Testing::setAccessibleName(btn, qsl("AS clu btn"));
        layout->addWidget(btn);

        connect(btn, &CustomButton::clicked, this, &SearchFilterButton::onRemoveClicked);
    }

    SearchFilterButton::~SearchFilterButton()
    {
    }

    void SearchFilterButton::setLabel(const QString& _label)
    {
        label_ = TextRendering::MakeTextUnit(_label, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        label_->init(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold),
                     Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        label_->setOffsets(Utils::scale_value(leftMargin), Utils::scale_value(btnHeight / 2));
        label_->evaluateDesiredSize();

        resize(sizeHint());
        elideLabel();
        update();
    }

    QSize SearchFilterButton::sizeHint() const
    {
        const auto labelWidth = label_ ? label_->horOffset() + label_->desiredWidth() : 0;
        const auto fullWidth = labelWidth + Utils::scale_value(btnSize.width() + rightMargin * 2);
        const auto w = std::min(Utils::scale_value(btnMaxWidth), fullWidth);
        const auto h = Utils::scale_value(btnHeight);
        return QSize(w, h);
    }

    void SearchFilterButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        static const auto radius = Utils::scale_value(btnHeight / 2);
        static const auto bgColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        p.setBrush(bgColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), radius, radius);

        if (label_)
            label_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void SearchFilterButton::resizeEvent(QResizeEvent* _event)
    {
        elideLabel();
        QWidget::resizeEvent(_event);
    }

    void SearchFilterButton::elideLabel()
    {
        if (label_)
            label_->elide(width() - label_->horOffset() - Utils::scale_value(btnSize.width() + rightMargin * 2));
    }

    void SearchFilterButton::onRemoveClicked()
    {
        emit removeClicked(QPrivateSignal());
    }

    DialogSearchViewHeader::DialogSearchViewHeader(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(headerViewHeight));

        auto layout = Utils::emptyHLayout(this);
        layout->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);
        layout->setSpacing(Utils::scale_value(8));
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    void DialogSearchViewHeader::setChatNameFilter(const QString& _name)
    {
        auto filter = new SearchFilterButton(this);
        filter->setLabel(_name);

        connect(filter, &SearchFilterButton::removeClicked, this, &DialogSearchViewHeader::removeChatNameFilter);
        addFilter(qsl("dialog"), filter);
    }

    void DialogSearchViewHeader::removeChatNameFilter()
    {
        removeFilter(qsl("dialog"));

        emit contactFilterRemoved();
    }

    void DialogSearchViewHeader::addFilter(const QString& _filterName, SearchFilterButton* _button)
    {
        removeFilter(_filterName);

        filters_[_filterName] = _button;
        Testing::setAccessibleName(_button, qsl("AS clu _button"));
        layout()->addWidget(_button);
    }

    void DialogSearchViewHeader::removeFilter(const QString& _filterName)
    {
        auto& btn = filters_[_filterName];
        if (btn)
        {
            layout()->removeWidget(btn);
            btn->deleteLater();

            btn = nullptr;
        }
    }
}

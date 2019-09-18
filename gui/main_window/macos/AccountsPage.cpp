#include "stdafx.h"
#include "AccountsPage.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../controls/PictureWidget.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/MainWindow.h"

namespace Ui
{
    bool is_number(const QString& _value)
    {
        int num = _value.toInt();

        QString v = QString::number(num);
        return _value == v;
    }

    QString format_uin(const QString& _uin)
    {
        if (!is_number(_uin))
            return _uin;

        QString ss_uin;

        int counter = 0;
        for (QChar c : _uin)
        {
            if (counter >= 3)
            {
                ss_uin.append(ql1c('-'));
                counter = 0;
            }

            counter++;
            ss_uin.append(c);
        }


        return ss_uin;
    }

    QString account_widget::uid() const
    {
        return uid_;
    }

    MacProfileType account_widget::profileType() const
    {
        return type_;
    }

    account_widget::account_widget(QWidget* _parent, const MacProfile &uid)
        : QPushButton(_parent),
        account_(uid),
        uid_(uid.uin()),
        type_(uid.type())
    {
//        setAttribute(WA_HOVER, true);

        if (build::is_icq()) {
            const int account_widget_height = Utils::scale_value(64);

            setObjectName(qsl("account_widget"));

            setCursor(Qt::PointingHandCursor);

            const int h_margin = Utils::scale_value(12);

            QHBoxLayout* root_layout = new QHBoxLayout();
            root_layout->setSpacing(0);
            root_layout->setContentsMargins(0, h_margin, 0, h_margin);

            avatar_ = new QLabel(this);
            avatar_h_ = account_widget_height - h_margin - h_margin;

            avatar_->setFixedHeight(avatar_h_);
            avatar_->setFixedWidth(avatar_h_);
            avatar_loaded(uid.uin());

            Testing::setAccessibleName(avatar_, qsl("AS ap avatar_"));
            root_layout->addWidget(avatar_);

            root_layout->addSpacing(Utils::scale_value(12));

            QVBoxLayout* name_layout = new QVBoxLayout();
            {
                name_layout->setAlignment(Qt::AlignTop);

                QLabel* name_label = new QLabel(this);
                name_label->setText(account_.name());
                name_label->setObjectName(qsl("label_account_nick"));

                Testing::setAccessibleName(name_label, qsl("AS ap name_label"));
                name_layout->addWidget(name_label);

                QLabel* uin_label = new QLabel(this);
                uin_label->setText(format_uin(account_.uin()));
                uin_label->setObjectName(qsl("label_account_uin"));

                Testing::setAccessibleName(uin_label, qsl("AS ap uin_label"));
                name_layout->addWidget(uin_label);
            }
            root_layout->addLayout(name_layout);

            root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            QLabel* arrow_right = new QLabel(this);
            arrow_right->setFixedHeight(Utils::scale_value(20));
            arrow_right->setFixedWidth(Utils::scale_value(20));
            arrow_right->setObjectName(qsl("arrow_right"));
            Testing::setAccessibleName(arrow_right, qsl("AS ap arrow_right"));
            root_layout->addWidget(arrow_right);

            setLayout(root_layout);

            setFixedHeight(account_widget_height);
        }
        else
        {
            setObjectName(qsl("account_widget"));

            const int account_widget_height = Utils::scale_value(60);

            setFixedHeight(account_widget_height);
            setCheckable(true);
            setCursor(Qt::PointingHandCursor);

            avatar_h_ = Utils::scale_value(44);

            QHBoxLayout* root_layout = new QHBoxLayout();
            root_layout->setSpacing(0);
            root_layout->setContentsMargins(0, 0, 0, 0);

            avatar_ = new QLabel(this);
            avatar_->setFixedHeight(avatar_h_);
            avatar_->setFixedWidth(avatar_h_);
            avatar_loaded(uid.uin());
            Testing::setAccessibleName(avatar_, qsl("AS ap avatar_"));
            root_layout->addWidget(avatar_);

            root_layout->addSpacing(Utils::scale_value(12));

            QVBoxLayout* name_layout = new QVBoxLayout();
            {
                name_layout->addSpacing(Utils::scale_value(4));
                name_layout->setAlignment(Qt::AlignTop);

                QLabel* name_label = new QLabel(this);
                name_label->setText(account_.name());
                name_label->setObjectName(qsl("label_account_nick"));

                Testing::setAccessibleName(name_label, qsl("AS ap name_label"));
                name_layout->addWidget(name_label);
                name_layout->addSpacing(8);

                QLabel* uin_label = new QLabel(this);
                uin_label->setText(format_uin(account_.uin()));
                uin_label->setObjectName(qsl("label_account_uin"));

                Testing::setAccessibleName(uin_label, qsl("AS ap uin_label"));
                name_layout->addWidget(uin_label);
            }
            root_layout->addLayout(name_layout);

            root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            check_ = new QWidget(this);
            check_->setFixedHeight(Utils::scale_value(20));
            check_->setFixedWidth(Utils::scale_value(20));
            check_->setObjectName(qsl("check_right"));
            check_->setProperty("state", qsl("normal"));
            Testing::setAccessibleName(check_, qsl("AS ap check_"));
            root_layout->addWidget(check_);

            setLayout(root_layout);

            if (account_.selected()) {
                setChecked(true);
                check_->setProperty("state", qsl("checked"));
                check_->setStyle(QApplication::style());
            }

            connect(this, &QPushButton::toggled, this, [this](bool _checked)
            {
                check_->setProperty("state", _checked ? qsl("checked") : qsl("normal"));
                check_->setStyle(QApplication::style());
                emit checked();
            });
        }
    }

    void account_widget::avatar_loaded(const QString& uid)
    {
        if (uid == account_.uin())
        {
            bool isDefault = false;
            const auto avatarPix = Logic::GetAvatarStorage()->GetRounded(
                account_.uin(), account_.name(),
                Utils::scale_bitmap(Utils::scale_value(avatar_h_)),
                QString(), isDefault, false, false);
            avatar_->setPixmap(avatarPix->copy());
            parentWidget()->update();
        }
    }

    account_widget::~account_widget()
    {
    }

    void account_widget::enterEvent(QEvent * event)
    {
        check_->setProperty("state", isChecked() ? qsl("checked") : qsl("hover"));
        check_->setStyle(QApplication::style());
        QWidget::enterEvent(event);
    }

    void account_widget::leaveEvent(QEvent * event)
    {
        check_->setProperty("state", isChecked() ? qsl("checked") : qsl("normal"));
        check_->setStyle(QApplication::style());
        QWidget::leaveEvent(event);
    }

    AccountsPage::AccountsPage(QWidget* _parent, MacMigrationManager * manager)
        : QWidget(_parent),
        active_step_(-1),
        icq_accounts_count_(0),
        agent_accounts_count_(0),
        selected_icq_accounts_count_(0),
        selected_agent_accounts_count_(0),
        buttonLayout_(nullptr),
        agentPageIcon_(nullptr),
        agentPageText_(nullptr),
        icqPageIcon_(nullptr),
        icqPageText_(nullptr),
        buttonSkip_(nullptr),
        accounts_area_widget_(nullptr),
        accounts_area_(nullptr),
        accounts_layout_(nullptr),
        manager_(manager)
    {
        const auto& accounts = manager_->getProfiles();
        for (auto account : accounts)
        {
            profiles_.append(MacProfile(account));
        }
    }

    void AccountsPage::init_with_step(const int _step)
    {
        active_step_ = _step;

        MacProfileType active_account_type =
            (_step == 1 ? MacProfileType::Agent : MacProfileType::ICQ);

        icq_accounts_count_ = 0;
        agent_accounts_count_ = 0;
        selected_icq_accounts_count_ = 0;
        selected_agent_accounts_count_ = 0;

        for (auto _widget : account_widgets_)
        {
            accounts_layout_->removeWidget(_widget);
            delete _widget;
        }

        account_widgets_.clear();

        for (MacProfile &account : profiles_)
        {
            if (account.type() == active_account_type)
            {
                account_widget* widget = new account_widget(accounts_area_widget_, account);

                connect(widget, &account_widget::checked, this, &AccountsPage::updateNextButtonState, Qt::QueuedConnection);

                Testing::setAccessibleName(widget, qsl("AS ap widget"));
                accounts_layout_->addWidget(widget);

                account_widgets_.push_back(widget);

                connect(widget, &QPushButton::toggled, this, [this, widget](bool _checked)
                {
                    if (_checked)
                    {
                        for (auto _widget : account_widgets_)
                        {
                            if (_widget != widget && _widget->isChecked())
                            {
                                _widget->setChecked(false);
                            }
                        }
                    }

                    selected_agent_accounts_count_ = 0;
                    selected_icq_accounts_count_ = 0;

                    for (auto _widget : account_widgets_)
                    {
                        if (_widget->isChecked())
                        {
                            if (_widget->profileType() == MacProfileType::Agent)
                                selected_agent_accounts_count_ = 1;
                            else if (_widget->profileType() == MacProfileType::ICQ)
                                selected_icq_accounts_count_ = 1;
                        }

                        for (MacProfile &account : profiles_)
                        {
                            if (account.uin() == _widget->uid()) {
                                account.setSelected(_widget->isChecked());
                            }
                        }
                    }

                    update_tabs();
                    update_buttons();
                });
            }

            if (account.type() == MacProfileType::ICQ)
            {
                ++icq_accounts_count_;
                if (account.selected())
                    ++selected_icq_accounts_count_;
            }
            else if (account.type() == MacProfileType::Agent)
            {
                ++agent_accounts_count_;
                if (account.selected())
                    ++selected_agent_accounts_count_;
            }
        }

        updateNextButtonState();
        buttonSkip_->setVisible(build::is_agent() && active_step_ == 2);

        update_tabs();
        update_buttons();
    }

    void AccountsPage::update_tabs()
    {
        if (agent_accounts_count_ <= 1)
        {
            agentPageIcon_->setVisible(false);
            agentPageText_->setVisible(false);
            icqPageIcon_->setVisible(false);
        }

        if (icq_accounts_count_ <= 1)
        {
            icqPageIcon_->setVisible(false);
            icqPageText_->setVisible(false);
            agentPageIcon_->setVisible(false);
        }

        QString state_agent, state_icq;

        switch (active_step_)
        {
            case 1:
            {
                if (icq_accounts_count_ > 1)
                {
                    state_agent = (selected_agent_accounts_count_ ? qsl("normalchecked") : qsl("normal"));
                    state_icq = (selected_icq_accounts_count_ ? qsl("disabledchecked") : qsl("disabled"));
                }
                else
                {
                    state_agent = qsl("gray");
                }

                break;
            }
            case 2:
            {
                if (agent_accounts_count_ > 1)
                {
                    state_agent = (selected_agent_accounts_count_ ? qsl("disabledchecked") : qsl("disabled"));
                    state_icq = (selected_icq_accounts_count_ ? qsl("normalchecked") : qsl("normal"));
                }
                else
                {
                    state_icq = qsl("gray");
                }

                break;
            }
            default:
            {
                assert(false);
                break;
            }
        }

        agentPageIcon_->setProperty("state", state_agent);
        agentPageText_->setProperty("state", state_agent);
        icqPageIcon_->setProperty("state", state_icq);
        icqPageText_->setProperty("state", state_icq);

        agentPageIcon_->setStyle(QApplication::style());
        agentPageText_->setStyle(QApplication::style());
        icqPageIcon_->setStyle(QApplication::style());
        icqPageText_->setStyle(QApplication::style());
    }

    void AccountsPage::update_buttons()
    {
        switch (active_step_)
        {
            case 1:
            {
                button_prev_->setVisible(false);
                button_next_->setVisible(true);
                button_next_->setDisabled(selected_agent_accounts_count_ <= 0);
                break;
            }
            case 2:
            {
                button_prev_->setVisible(agent_accounts_count_ > 1);
                button_next_->setVisible(true);
                break;
            }
            default:
                break;
        }
    }

    void AccountsPage::nextClicked(bool /*_checked*/)
    {
        switch (active_step_)
        {
            case 1:
            {
                if (icq_accounts_count_ <= 1)
                {
                    store_accounts_and_exit();
                }
                else
                {
                    init_with_step(2);
                }

                break;
            }
            case 2:
            {
                store_accounts_and_exit();

                break;
            }
        }
    }

    void AccountsPage::skipClicked(bool /*_checked*/)
    {
        bool found = false;
        for (const auto& account : std::as_const(profiles_))
        {
            if (account.selected() &&
                account.type() != MacProfileType::ICQ)
            {
                manager_->loginProfile(account, MacProfile());
                found = true;
                break;
            }
        }

        if (!found)
        {
            MacProfilesList tmp;
            for (const auto& a : std::as_const(profiles_))
            {
                if (tmp.isEmpty() || tmp.back().type() == a.type())
                    tmp.push_back(a);
            }

            if (tmp.size() == 1)
            {
                manager_->loginProfile(tmp.constLast(), MacProfile());
                emit account_selected();
                return;
            }

            Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);

            auto w = Utils::InterConnector::instance().getMainWindow();
            if (w)
                w->showLoginPage(false);

            return;
        }

        emit account_selected();
    }

    void AccountsPage::store_accounts_and_exit()
    {
        bool isFirst = true;
        MacProfile main, slave;

        for (const auto& account : std::as_const(profiles_))
        {
            if (account.selected())
            {
                if (isFirst)
                {
                    main = account;
                    isFirst = false;
                }
                else
                {
                    slave = account;
                    break;
                }
            }
        }

        if (!main.uin().isEmpty() && slave.uin().isEmpty())
        {
            MacProfilesList tmp;
            for (const auto& a : std::as_const(profiles_))
            {
                if (a.type() != main.type())
                    tmp.push_back(a);
            }

            if (tmp.size() == 1)
                slave = tmp.constFirst();
        }

        if (!isFirst)
            manager_->loginProfile(main, slave);

        emit account_selected();
    }

    void AccountsPage::prevClicked(bool /*_checked*/)
    {
        init_with_step(1);
    }

    void AccountsPage::summon()
    {
        if (build::is_icq()) {
            setStyleSheet(Utils::LoadStyle(qsl(":/qss/accounts_page")));

            QVBoxLayout* verticalLayout = new QVBoxLayout(this);
            verticalLayout->setSpacing(0);
            verticalLayout->setObjectName(qsl("verticalLayout"));
            verticalLayout->setContentsMargins(0, 0, 0, 0);

            QWidget* main_widget = new QWidget(this);
            main_widget->setObjectName(qsl("main_widget"));
            QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
            sizePolicy.setHorizontalStretch(0);
            sizePolicy.setVerticalStretch(0);
            sizePolicy.setHeightForWidth(main_widget->sizePolicy().hasHeightForWidth());
            main_widget->setSizePolicy(sizePolicy);
            QHBoxLayout* main_layout = new QHBoxLayout(main_widget);
            main_layout->setSpacing(0);
            main_layout->setObjectName(qsl("main_layout"));
            main_layout->setContentsMargins(0, 0, 0, 0);

            QWidget* controls_widget = new QWidget(main_widget);
            controls_widget->setObjectName(qsl("controls_widget"));
            QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
            sizePolicy1.setHorizontalStretch(0);
            sizePolicy1.setVerticalStretch(0);
            sizePolicy1.setHeightForWidth(controls_widget->sizePolicy().hasHeightForWidth());
            controls_widget->setSizePolicy(sizePolicy1);
            controls_widget->setFixedWidth(500);
            QVBoxLayout* controls_layout = new QVBoxLayout(controls_widget);
            controls_layout->setSpacing(0);
            controls_layout->setObjectName(qsl("controls_layout"));
            controls_layout->setContentsMargins(0, 0, 0, 0);

            controls_layout->addSpacing(Utils::scale_value(50));

            PictureWidget* logo_widget = new PictureWidget(controls_widget, qsl(":/resources/main_window/content_logo_100.png"));
            logo_widget->setFixedHeight(Utils::scale_value(80));
            logo_widget->setFixedWidth(Utils::scale_value(80));
            Testing::setAccessibleName(logo_widget, qsl("AS ap logo_widget"));
            controls_layout->addWidget(logo_widget);
            controls_layout->setAlignment(logo_widget, Qt::AlignHCenter);

            QLabel* welcome_label = new QLabel(controls_widget);
            welcome_label->setObjectName(qsl("welcome_label"));
            welcome_label->setAlignment(Qt::AlignCenter);
            welcome_label->setProperty("WelcomeTitle", QVariant(true));

            auto title = build::GetProductVariant(QT_TRANSLATE_NOOP("login_page", "Welcome to ICQ"),
                                                  QT_TRANSLATE_NOOP("login_page", "Welcome to Mail.ru Agent"),
                                                  QT_TRANSLATE_NOOP("login_page", "Welcome to Myteam"),
                                                  QT_TRANSLATE_NOOP("login_page", "Welcome to Messenger"));

            welcome_label->setText(title);

            Testing::setAccessibleName(welcome_label, qsl("AS ap welcome_label"));
            controls_layout->addWidget(welcome_label);

            QLabel* label_choose_comment = new QLabel(main_widget);
            label_choose_comment->setObjectName(qsl("choose_account_comment_label"));
            label_choose_comment->setText(QT_TRANSLATE_NOOP("merge_accounts", "Now Mail.Ru Agent supports only one account. You can merge it with ICQ one."));
            label_choose_comment->setWordWrap(true);
            label_choose_comment->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            Testing::setAccessibleName(label_choose_comment, qsl("AS ap label_choose_comment"));
            controls_layout->addWidget(label_choose_comment);

            controls_layout->addSpacing(Utils::scale_value(24));

            QScrollArea* accounts_area = new QScrollArea(main_widget);
            QWidget* accounts_area_widget = new QWidget(main_widget);
            accounts_area_widget->setObjectName(qsl("accounts_area_widget"));
            accounts_area->setWidget(accounts_area_widget);
            accounts_area->setWidgetResizable(true);
            accounts_area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            QVBoxLayout* accounts_layout = new QVBoxLayout();
            accounts_layout->setSpacing(0);
            accounts_layout->setContentsMargins(0, 0, 0, 0);
            accounts_layout->setAlignment(Qt::AlignTop);

            accounts_area_widget->setLayout(accounts_layout);
            Testing::setAccessibleName(accounts_area, qsl("AS ap accounts_area"));
            controls_layout->addWidget(accounts_area);

            controls_layout->addSpacing(Utils::scale_value(24));

            class Skipper: public QPushButton
            {
            private:
                const MacProfile *profile_;

            public:
                explicit Skipper(QWidget *parent = nullptr): QPushButton(parent) { ; }
                void setProfile(const MacProfile &profile) { profile_ = &profile; }
                const MacProfile *const profile() const { return profile_; }
            }
            *label_choose = new Skipper(main_widget);
            label_choose->setObjectName(qsl("choose_account_button"));
            label_choose->setText(QT_TRANSLATE_NOOP("merge_accounts", "Choose Mail.Ru Agent account"));
            label_choose->setFlat(true);
            label_choose->setEnabled(false);
            Testing::setAccessibleName(label_choose, qsl("AS ap label_choose"));
            controls_layout->addWidget(label_choose);

            // Accounts work
            {
                auto enumerator = [](MacProfilesList &accounts,
                                                  const MacProfileType &possible,
                                                  QWidget *list,
                                                  QVBoxLayout *layout,
                                                  std::function<void(const MacProfile &)> listener)
                {
                    std::vector<account_widget *> widgets;
                    return widgets;
                };

                std::map<MacProfileType, std::vector<int>> types;
                for (int i = 0, iend = profiles_.size(); i < iend; ++i)
                {
                    types[profiles_[i].type()].push_back(i);
                }
                if (types.size() == 1 || types[MacProfileType::ICQ].size() == 1)
                {
                    manager_->loginProfile(profiles_[types[MacProfileType::ICQ].front()], MacProfile());
                    emit account_selected();
                }
                else if (types[MacProfileType::ICQ].size() > 1)
                {
                    label_choose_comment->setText(QT_TRANSLATE_NOOP("merge_accounts", "Choose ICQ account"));
                }
            }

            controls_layout->addSpacing(Utils::scale_value(50));

            Testing::setAccessibleName(controls_widget, qsl("AS ap controls_widget"));
            main_layout->addWidget(controls_widget);

            Testing::setAccessibleName(main_widget, qsl("AS ap main_widget"));
            verticalLayout->addWidget(main_widget);
        } else {
            setStyleSheet(Utils::LoadStyle(qsl(":/qss/accounts_page_agent")));

            for (auto account : profiles_)
            {
                if (account.type() == MacProfileType::ICQ)
                    ++icq_accounts_count_;
                else if (account.type() == MacProfileType::Agent)
                    ++agent_accounts_count_;
            }

            root_layout_ = new QVBoxLayout(this);
            {
                setObjectName(qsl("choose_account_page"));
                root_layout_->setSpacing(0);
                root_layout_->setContentsMargins(Utils::scale_value(72), Utils::scale_value(32), Utils::scale_value(72), Utils::scale_value(16));
                root_layout_->setAlignment(Qt::AlignTop);

                QLabel* label_choose = new QLabel(this);
                label_choose->setObjectName(qsl("choose_account_label"));
                label_choose->setText(QT_TRANSLATE_NOOP("merge_accounts", "Account settings"));
                label_choose->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                Testing::setAccessibleName(label_choose, qsl("AS ap label_choose"));
                root_layout_->addWidget(label_choose);

                root_layout_->addSpacing(Utils::scale_value(16));

                QLabel* label_choose_comment = new QLabel(this);
                label_choose_comment->setObjectName(qsl("choose_account_comment_label"));
                label_choose_comment->setText(QT_TRANSLATE_NOOP("merge_accounts", "Now Mail.ru Agent supports only one account. You can merge it with ICQ one."));
                label_choose_comment->setWordWrap(true);
                label_choose_comment->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                Testing::setAccessibleName(label_choose_comment, qsl("AS ap label_choose_comment"));
                root_layout_->addWidget(label_choose_comment);

                root_layout_->addSpacing(Utils::scale_value(12));

                QHBoxLayout* pagesLayout = new QHBoxLayout();

                pagesLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                agentPageIcon_ = new QWidget(this);
                agentPageIcon_->setObjectName(qsl("agent_page_icon"));
                agentPageIcon_->setProperty("state", qsl("normal"));
                agentPageIcon_->setFixedSize(Utils::scale_value(12), Utils::scale_value(12));
                Testing::setAccessibleName(agentPageIcon_, qsl("AS ap agentPageIcon_"));
                pagesLayout->addWidget(agentPageIcon_);

                pagesLayout->addSpacing(Utils::scale_value(4));

                agentPageText_ = new QLabel(this);
                agentPageText_->setObjectName(qsl("agent_page_text"));
                agentPageText_->setText(icq_accounts_count_ > 1 ? QT_TRANSLATE_NOOP("merge_accounts", "Mail.ru Agent") : QT_TRANSLATE_NOOP("merge_accounts", "Choose Mail.ru Agent account"));
                agentPageText_->setProperty("state", qsl("normal"));
                Testing::setAccessibleName(agentPageText_, qsl("AS ap agentPageText_"));
                pagesLayout->addWidget(agentPageText_);

                pagesLayout->addSpacing(Utils::scale_value(20));

                icqPageIcon_ = new QWidget(this);
                icqPageIcon_->setObjectName(qsl("icq_page_icon"));
                icqPageIcon_->setProperty("state", qsl("normal"));
                icqPageIcon_->setFixedSize(Utils::scale_value(12), Utils::scale_value(12));
                Testing::setAccessibleName(icqPageIcon_, qsl("AS ap icqPageIcon_"));
                pagesLayout->addWidget(icqPageIcon_);

                pagesLayout->addSpacing(Utils::scale_value(4));

                icqPageText_ = new QLabel(this);
                icqPageText_->setObjectName(qsl("icq_page_text"));
                icqPageText_->setText(agent_accounts_count_ > 1 ? QT_TRANSLATE_NOOP("merge_accounts", "ICQ") : QT_TRANSLATE_NOOP("merge_accounts", "Choose ICQ account"));
                icqPageText_->setProperty("state", qsl("normal"));
                Testing::setAccessibleName(icqPageText_, qsl("AS ap icqPageText_"));
                pagesLayout->addWidget(icqPageText_);

                pagesLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                root_layout_->addLayout(pagesLayout);

                root_layout_->addSpacing(Utils::scale_value(12));

                accounts_area_ = new QScrollArea(this);
                accounts_area_widget_ = new QWidget(this);
                accounts_area_widget_->setObjectName(qsl("accounts_area_widget"));
                accounts_area_->setWidget(accounts_area_widget_);
                accounts_area_->setWidgetResizable(true);
                accounts_area_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                accounts_layout_ = new QVBoxLayout();
                accounts_layout_->setSpacing(0);
                accounts_layout_->setContentsMargins(0, 0, 0, 0);
                accounts_layout_->setAlignment(Qt::AlignTop);

                accounts_area_widget_->setLayout(accounts_layout_);

                Testing::setAccessibleName(accounts_area_, qsl("AS ap accounts_area_"));
                root_layout_->addWidget(accounts_area_);

                root_layout_->addSpacing(Utils::scale_value(16));

                buttonLayout_ = new QHBoxLayout();
                buttonLayout_->setSpacing(Utils::scale_value(20));
                buttonLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                button_prev_ = new QPushButton(this);
                button_prev_->setContentsMargins(40, 0, 40, 0);
                button_prev_->setFixedHeight(Utils::scale_value(40));
                button_prev_->setFixedWidth(Utils::scale_value(120));
                button_prev_->setObjectName(qsl("frame_button"));
                button_prev_->setText(QT_TRANSLATE_NOOP("merge_accounts", "Back"));
                button_prev_->setCursor(QCursor(Qt::PointingHandCursor));
                Testing::setAccessibleName(button_prev_, qsl("AS ap button_prev_"));
                buttonLayout_->addWidget(button_prev_);
                connect(button_prev_, &QPushButton::clicked, this, &AccountsPage::prevClicked);

                button_next_ = new QPushButton(this);
                button_next_->setContentsMargins(40, 0, 40, 0);
                button_next_->setFixedHeight(Utils::scale_value(40));
                button_next_->setFixedWidth(Utils::scale_value(120));
                button_next_->setObjectName(qsl("custom_button"));
                button_next_->setText(QT_TRANSLATE_NOOP("merge_accounts", "Next"));
                button_next_->setCursor(QCursor(Qt::PointingHandCursor));
                Testing::setAccessibleName(button_next_, qsl("AS ap button_next_"));
                buttonLayout_->addWidget(button_next_);
                connect(button_next_, &QPushButton::clicked, this, &AccountsPage::nextClicked);

                buttonLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                root_layout_->addLayout(buttonLayout_);

                buttonSkip_ = new QPushButton(this);
                buttonSkip_->setText(QT_TRANSLATE_NOOP("merge_accounts", "Skip"));
                buttonSkip_->setCursor(QCursor(Qt::PointingHandCursor));
                buttonSkip_->setObjectName(qsl("skip_button"));
                buttonSkip_->setVisible(false);
                Testing::setAccessibleName(buttonSkip_, qsl("AS ap buttonSkip_"));
                root_layout_->addWidget(buttonSkip_);
                connect(buttonSkip_, &QPushButton::clicked, this, &AccountsPage::skipClicked);

                init_with_step(agent_accounts_count_ > 1 ? 1 : 2);
            }
            setLayout(root_layout_);
        }
    }


    AccountsPage::~AccountsPage()
    {
    }

    void AccountsPage::loginResult(int result)
    {
        if (result == 0)
        {
//            emit loggedIn();
        }
    }

    void AccountsPage::updateNextButtonState()
    {
        unsigned checkedCount = 0;
        for (auto w : account_widgets_)
        {
            if (w->isChecked())
                ++checkedCount;
        }

        button_next_->setDisabled(!checkedCount);
    }

    void AccountsPage::paintEvent(QPaintEvent *)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

}

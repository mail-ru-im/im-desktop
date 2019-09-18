#include "stdafx.h"

#include "GroupChatOperations.h"
#include "../core_dispatcher.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/CustomAbstractListModel.h"
#include "contact_list/RecentsModel.h"
#include "friendly/FriendlyContainer.h"
#include "../utils/utils.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "MainPage.h"
#include "../gui_settings.h"
#include "contact_list/ContactList.h"
#include "contact_list/SelectionContactsForGroupChat.h"

#include "history_control/MessageStyle.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_metrics.h"

#include "../controls/GeneralDialog.h"
#include "../controls/TextEditEx.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/SwitcherCheckbox.h"
#include "MainWindow.h"
#include "../styles/ThemeParameters.h"

namespace
{
    void switchToContact(const QString& _contact)
    {
        Logic::getRecentsModel()->moveToTop(_contact);

        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != _contact)
            emit Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::getContactListModel()->setCurrent(_contact, -1, true);
        Utils::InterConnector::instance().onSendMessage(_contact);
    }
}

namespace Ui
{
    GroupChatSettings::GroupChatSettings(QWidget *parent, const QString &buttonText, const QString &headerText, GroupChatOperations::ChatData &chatData, bool _channel)
        : chatData_(chatData), editorIsShown_(false), channel_(_channel)
    {
        content_ = new QWidget(parent);
        content_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        content_->setFixedWidth(Utils::scale_value(360));
        Utils::ApplyStyle(content_, qsl("height: 10dip;"));

        auto layout = Utils::emptyVLayout(content_);
        layout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16), 0);

        // name and photo
        auto subContent = new QWidget(content_);
        subContent->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        auto subLayout = new QHBoxLayout(subContent);
        subLayout->setSpacing(Utils::scale_value(16));
        subLayout->setContentsMargins(0, 0, 0, Utils::scale_value(12));
        subLayout->setAlignment(Qt::AlignTop);
        {
            photo_ = new ContactAvatarWidget(subContent, QString(), QString(), Utils::scale_value(56), true);
            photo_->SetMode(ContactAvatarWidget::Mode::CreateChat);
            photo_->SetVisibleShadow(false);
            photo_->SetVisibleSpinner(false);
            Testing::setAccessibleName(photo_, qsl("AS gco photo_"));
            subLayout->addWidget(photo_);

            chatName_ = new TextEditEx(subContent, Fonts::appFontScaled(18), MessageStyle::getTextColor(), true, true);
            Utils::ApplyStyle(chatName_, Styling::getParameters().getTextEditCommonQss(true));
            chatName_->setWordWrapMode(QTextOption::NoWrap);//WrapAnywhere);
            chatName_->setObjectName(qsl("chat_name"));
            chatName_->setPlaceholderText(_channel ? QT_TRANSLATE_NOOP("groupchats", "Channel name") : QT_TRANSLATE_NOOP("groupchats", "Chat name"));
            chatName_->setPlainText(QString());//chatData.name);
            chatName_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            chatName_->setAutoFillBackground(false);
            chatName_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            chatName_->setCatchEnter(true);
            chatName_->setMinimumWidth(Utils::scale_value(200));
            chatName_->document()->setDocumentMargin(0);
            chatName_->addSpace(Utils::scale_value(4));
            Testing::setAccessibleName(chatName_, qsl("AS gco chatName_"));
            subLayout->addWidget(chatName_);
        }
        Testing::setAccessibleName(subContent, qsl("AS gco subContent"));
        layout->addWidget(subContent);

        // settings
        static auto createItem = [](QWidget *parent, const QString &topic, const QString &text, SwitcherCheckbox *&switcher)
        {
            auto whole = new QWidget(parent);
            auto wholelayout = new QHBoxLayout(whole);
            whole->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            wholelayout->setContentsMargins(0, 0, 0, Utils::scale_value(12));
            {
                auto textpart = new QWidget(whole);
                auto textpartlayout = Utils::emptyVLayout(textpart);
                textpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                textpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(textpart, qsl("background-color: transparent; border: none;"));
                {
                    {
                        auto t = new TextEmojiWidget(textpart, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Utils::scale_value(16));
                        t->setText(topic);
                        t->setObjectName(qsl("topic"));
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        Testing::setAccessibleName(t, qsl("AS gco t"));
                        textpartlayout->addWidget(t);
                    }
                    {
                        auto t = new TextEmojiWidget(textpart, Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), Utils::scale_value(16));
                        t->setText(text);
                        t->setObjectName(qsl("text"));
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        t->setMultiline(true);
                        Testing::setAccessibleName(t, qsl("AS gco t"));
                        textpartlayout->addWidget(t);
                    }
                }
                Testing::setAccessibleName(textpart, qsl("AS gco textpart"));
                wholelayout->addWidget(textpart);

                auto switchpart = new QWidget(whole);
                auto switchpartlayout = Utils::emptyVLayout(switchpart);
                switchpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                switchpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(switchpart, qsl("background-color: transparent;"));
                {
                    switcher = new SwitcherCheckbox(switchpart);
                    switcher->setChecked(false);
                    Testing::setAccessibleName(switcher, qsl("AS gco switcher"));
                    switchpartlayout->addWidget(switcher);
                }
                Testing::setAccessibleName(switchpart, qsl("AS gco switchpart"));
                wholelayout->addWidget(switchpart);
            }
            return whole;
        };

        if (!channel_)
        {
            SwitcherCheckbox *approvedJoin;
            layout->addWidget(createItem(content_,
                QT_TRANSLATE_NOOP("groupchats", "Join with Approval"),
                QT_TRANSLATE_NOOP("groupchats", "New members are waiting for admin approval"),
                approvedJoin));
            approvedJoin->setChecked(chatData.approvedJoin);
            connect(approvedJoin, &SwitcherCheckbox::toggled, approvedJoin, [&chatData](bool checked) { chatData.approvedJoin = checked; });

            Testing::setAccessibleName(approvedJoin, qsl("AS approvedJoin"));
        }
        else
        {
            chatData.readOnly = true;
            chatData.publicChat = true;
        }

        SwitcherCheckbox *publicChat;
        const auto publicText = _channel
            ? QT_TRANSLATE_NOOP("groupchats", "The channel will appear in the app's showcase and any user can find it in the list")
            : QT_TRANSLATE_NOOP("groupchats", "The chat will appear in the app's showcase and any user can find it in the list");
        layout->addWidget(createItem(content_,
            _channel ? QT_TRANSLATE_NOOP("groupchats", "Public channel") : QT_TRANSLATE_NOOP("groupchats", "Public chat"),
            publicText,
            publicChat));
        publicChat->setChecked(chatData.publicChat);
        connect(publicChat, &SwitcherCheckbox::toggled, publicChat, [&chatData](bool checked) { chatData.publicChat = checked; });


        // general dialog
        dialog_ = std::make_unique<Ui::GeneralDialog>(content_, Utils::InterConnector::instance().getMainWindow());
        dialog_->setObjectName(qsl("chat.creation.settings"));
        dialog_->addLabel(headerText);
        dialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), buttonText, true);
        if (channel_)
            dialog_->setButtonActive(false);

        QObject::connect(chatName_, &TextEditEx::enter, this, &Ui::GroupChatSettings::enterPressed);
        QObject::connect(chatName_, &TextEditEx::textChanged, this, &Ui::GroupChatSettings::nameChanged);

        QTextCursor cursor = chatName_->textCursor();
        cursor.select(QTextCursor::Document);
        chatName_->setTextCursor(cursor);
        chatName_->setFrameStyle(QFrame::NoFrame);

        chatName_->setFocus();

        // editor
        auto hollow = new QWidget(dialog_->parentWidget());
        hollow->setObjectName(qsl("hollow"));
        hollow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        Utils::ApplyStyle(hollow, qsl("background-color: transparent; margin: 0; padding: 0; border: none;"));
        {
            auto hollowlayout = Utils::emptyVLayout(hollow);
            hollowlayout->setAlignment(Qt::AlignLeft);

            auto editor = new QWidget(hollow);
            editor->setObjectName(qsl("editor"));
            editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            Utils::ApplyStyle(editor, qsl("background-color: #ffffff; margin: 0; padding: 0dip; border: none;"));
            //photo_->SetImageCropHolder(editor);
            {
                auto effect = new QGraphicsOpacityEffect(editor);
                effect->setOpacity(.0);
                editor->setGraphicsEffect(effect);
            }
            Testing::setAccessibleName(editor, qsl("AS gco editor"));
            hollowlayout->addWidget(editor);

            // events

            QWidget::connect(photo_, &ContactAvatarWidget::avatarDidEdit, this, [=]()
            {
                lastCroppedImage_ = photo_->croppedImage();
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsShown, this, [=](QWidget *p)
            {
                if (!editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = true;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsHidden, this, [=](QWidget *p)
            {
                if (editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = false;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogResized, this, [=](QWidget *p)
            {
                editor->setFixedSize(p->size());
                p->move(0, 0);
            }, Qt::QueuedConnection);

            connect(dialog_.get(), &GeneralDialog::moved, this, [=](QWidget *p)
            {
                hollow->move(dialog_->x() + dialog_->width(), dialog_->y());
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::resized, this, [=](QWidget *p)
            {
                hollow->setFixedSize(hollow->parentWidget()->width() - dialog_->x() + dialog_->width(), dialog_->height());
                const auto preferredWidth = (dialog_->parentWidget()->geometry().size().width() - dialog_->geometry().size().width() - Utils::scale_value(24)*4);
                //photo_->SetImageCropSize(QSize(preferredWidth, content_->height()));
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::hidden, this, [=](QWidget *p)
            {
                emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
                emit Utils::InterConnector::instance().closeAnySemitransparentWindow(Utils::CloseWindowInfo());
            }, Qt::QueuedConnection);
        }
        hollow->show();

        Testing::setAccessibleName(content_, qsl("AS content_"));
        Testing::setAccessibleName(subContent, qsl("AS subContent"));
        Testing::setAccessibleName(publicChat, qsl("AS publicChat"));
        Testing::setAccessibleName(chatName_, qsl("AS chatName_"));
        Testing::setAccessibleName(hollow, qsl("AS hollow"));
    }

    GroupChatSettings::~GroupChatSettings()
    {
        //
    }

    bool GroupChatSettings::show()
    {
        if (!dialog_)
            return false;

        auto result = dialog_->showInCenter();
        if (!chatName_->getPlainText().isEmpty())
            chatData_.name = chatName_->getPlainText();
        return result;
    }

    void GroupChatSettings::showImageCropDialog()
    {
        auto editor = dialog_->parentWidget()->findChild<QWidget *>(qsl("editor"));
        if (!editor)
            return;
        auto hollow = dialog_->parentWidget()->findChild<QWidget *>(qsl("hollow"));
        if (!hollow)
            return;

        static const auto time = 200;
        const auto needHideEditor = editorIsShown_;
        editorIsShown_ = !editorIsShown_;
        // move settings
        {
            auto geometry = new QPropertyAnimation(dialog_.get(), QByteArrayLiteral("geometry"), Utils::InterConnector::instance().getMainWindow());
            geometry->setDuration(time);
            auto rect = dialog_->geometry();
            geometry->setStartValue(rect);
            if (needHideEditor)
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                rect.setX((screenRect.size().width() - dialogRect.size().width()) / 2);
            }
            else
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                const auto editorRect = editor->geometry();
                const auto hollowRect = hollow->geometry();
                const auto between = (hollowRect.left() - dialogRect.right());
//                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width() - Utils::scale_value(24)) / 2);
                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width()) / 2 - between);
            }
            geometry->setEndValue(rect);
            geometry->start(QAbstractAnimation::DeleteWhenStopped);
        }
        // fade editor
        {
            auto effect = new QGraphicsOpacityEffect(Utils::InterConnector::instance().getMainWindow());
            editor->setGraphicsEffect(effect);
            auto fading = new QPropertyAnimation(effect, QByteArrayLiteral("opacity"), Utils::InterConnector::instance().getMainWindow());
            fading->setEasingCurve(QEasingCurve::InOutQuad);
            fading->setDuration(time);
            if (needHideEditor)
            {
                fading->setStartValue(1.0);
                fading->setEndValue(0.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor, hollow]() { editor->setFixedSize(hollow->size()); });
            }
            else
            {
                fading->setStartValue(0.0);
                fading->setEndValue(1.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor]() { Utils::addShadowToWidget(editor); });
            }
            fading->start(QPropertyAnimation::DeleteWhenStopped);
        }
    }

    void GroupChatSettings::nameChanged()
    {
        if (!channel_)
            return;

        dialog_->setButtonActive(!chatName_->getPlainText().isEmpty());
    }

    void GroupChatSettings::enterPressed()
    {
        if (!channel_ || !chatName_->getPlainText().isEmpty())
            dialog_->accept();
    }

    void createGroupChat(const std::vector<QString>& _members_aimIds)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_open);

        SelectContactsWidget select_members_dialog(nullptr, Logic::MembersWidgetRegim::SELECT_MEMBERS, QT_TRANSLATE_NOOP("groupchats", "Select members"), QT_TRANSLATE_NOOP("popup_window", "NEXT"), Ui::MainPage::instance());

        for (const auto& member_aimId : _members_aimIds)
            Logic::getContactListModel()->setChecked(member_aimId, true /* is_checked */);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            std::shared_ptr<GroupChatSettings> groupChatSettings;

            GroupChatOperations::ChatData chatData;

            if (callChatNameEditor(Ui::MainPage::instance(), chatData, groupChatSettings, false))
            {
                const auto cropped = groupChatSettings->lastCroppedImage();
                if (!cropped.isNull())
                {
                    auto connection = std::make_shared<QMetaObject::Connection>();

                    *connection = QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatarId, [connection, chatData](QString avatarId)
                    {
                        if (!avatarId.isEmpty())
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_avatar);

                        Ui::MainPage::instance()->openCreatedGroupChat();

                        postCreateChatInfoToCore(MyInfo()->aimId(), chatData, avatarId);

                        Logic::getContactListModel()->clearChecked();

                        Ui::MainPage::instance()->clearSearchMembers();

                        QObject::disconnect(*connection);
                    });

                    groupChatSettings->photo()->UpdateParams(QString(), QString());
                    groupChatSettings->photo()->applyAvatar(cropped);
                }
                else
                {
                    Ui::MainPage::instance()->openCreatedGroupChat();
                    postCreateChatInfoToCore(MyInfo()->aimId(), chatData);
                    Logic::getContactListModel()->clearChecked();
                    Ui::MainPage::instance()->clearSearchMembers();
                }
            }
            else
            {
                Logic::getContactListModel()->clearChecked();
                Ui::MainPage::instance()->clearSearchMembers();
            }
        }
    }

    void createChannel()
    {
        std::shared_ptr<GroupChatSettings> groupChatSettings;
        GroupChatOperations::ChatData chatData;
        if (callChatNameEditor(Ui::MainPage::instance(), chatData, groupChatSettings, true))
        {
            const auto cropped = groupChatSettings->lastCroppedImage();
            if (!cropped.isNull())
            {
                auto connection = std::make_shared<QMetaObject::Connection>();
                *connection = QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatarId, [connection, chatData](QString avatarId)
                {
                    if (!avatarId.isEmpty())
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_avatar);
                    Ui::MainPage::instance()->openCreatedGroupChat();
                    postCreateChatInfoToCore(MyInfo()->aimId(), chatData, avatarId);

                    QObject::disconnect(*connection);
                });
                groupChatSettings->photo()->UpdateParams(QString(), QString());
                groupChatSettings->photo()->applyAvatar(cropped);
            }
            else
            {
                Ui::MainPage::instance()->openCreatedGroupChat();
                postCreateChatInfoToCore(MyInfo()->aimId(), chatData);
            }
        }
    }

    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings, bool _channel)
    {
        const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        chatData.name = MyInfo()->friendly();
        if (chatData.name.isEmpty())
            chatData.name = MyInfo()->aimId();
        for (int i = 0, contactsSize = std::min((int)selectedContacts.size(), 2); i < contactsSize; ++i)
            chatData.name += ql1s(", ") % Logic::GetFriendlyContainer()->getFriendly(selectedContacts[i]);

        auto chat_name = chatData.name;
        {
            const auto header = _channel ? QT_TRANSLATE_NOOP("popup_window", "Channel settings") : QT_TRANSLATE_NOOP("popup_window", "Chat settings");
            groupChatSettings = std::make_shared<GroupChatSettings>(_parent, QT_TRANSLATE_NOOP("popup_window","DONE"), header, chatData, _channel);
            auto result = groupChatSettings->show();
            if (chat_name != chatData.name)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_rename);
            chatData.name = chatData.name.isEmpty() ? chat_name : chatData.name;
            return result;
        }
    }

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, const QString& avatarId)
    {
        if (chatData.publicChat)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_public);
        if (chatData.approvedJoin)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_approval);
        if (chatData.readOnly)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_readonly);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_qstring("name", chatData.name);
        collection.set_value_as_qstring("avatar", avatarId);
        {
            const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
            core::ifptr<core::iarray> members_array(collection->create_array());
            members_array->reserve(static_cast<int>(selectedContacts.size()));
            for (const auto& contact : selectedContacts)
                members_array->push_back(collection.create_qstring_value(contact).get());
            collection.set_value_as_array("members", members_array.get());
        }
        collection.set_value_as_bool("public", chatData.publicChat);
        collection.set_value_as_bool("approved", chatData.approvedJoin);
        collection.set_value_as_bool("link", chatData.getJoiningByLink());
        collection.set_value_as_bool("ro", chatData.readOnly);
        Ui::GetDispatcher()->post_message_to_core("chats/create", collection.get());
    }

    void postAddChatMembersFromCLModelToCore(const QString& _aimId)
    {
        const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        Logic::getContactListModel()->clearChecked();

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        QStringList chat_members;
        chat_members.reserve(static_cast<int>(selectedContacts.size()));
        for (const auto& contact : selectedContacts)
            chat_members.push_back(contact);

        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_qstring("m_chat_members_to_add", chat_members.join(ql1c(';')));
        Ui::GetDispatcher()->post_message_to_core("add_members", collection.get());
    }

    void deleteMemberDialog(Logic::CustomAbstractListModel* _model, const QString& _chatAimId, const QString& _memberAimId, int _regim, QWidget* _parent)
    {
        if (_model)
        {
            auto member = _model->contains(_memberAimId);
            if (!member)
            {
                assert(!"member not found in model, probably it need to refresh");
                return;
            }
        }

        QString text;

        const bool isVideoConference = _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;

        if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this chat?");
        else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?");
        else if (isVideoConference)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this call?");

        auto left_button_text = QT_TRANSLATE_NOOP("popup_window", "CANCEL");
        auto right_button_text = QT_TRANSLATE_NOOP("popup_window", "DELETE");

        auto userName = Logic::GetFriendlyContainer()->getFriendly(_memberAimId);

        if (Utils::GetConfirmationWithTwoButtons(left_button_text, right_button_text, text, userName, _parent,
            isVideoConference ? _parent->parentWidget() : Utils::InterConnector::instance().getMainWindow()))
        {
            if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            {
                Ui::gui_coll_helper collection(::Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("aimid", _chatAimId);
                collection.set_value_as_qstring("m_chat_members_to_remove", _memberAimId);
                Ui::GetDispatcher()->post_message_to_core("remove_members", collection.get());
            }
            else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                Logic::getContactListModel()->ignoreContact(_memberAimId, false);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_remove);
                // NOTE : when delete from ignore list, we don't need add contact to CL
            }
            else if (isVideoConference)
            {
                Ui::GetDispatcher()->getVoipController().setDecline(_memberAimId.toStdString().c_str(), false);
            }
        }
    }



    int forwardMessage(const Data::QuotesVec& quotes, bool fromMenu, const QString& _labelText, const QString& _buttonText, bool _enableAuthorSetting)
    {
        auto result = 0;
        statistic::getGuiMetrics().eventStartForward();


        auto labelText = _labelText;
        if (labelText.isEmpty())
            labelText = QT_TRANSLATE_NOOP("popup_window", "Forward");

        auto buttonText = _buttonText;
        if (buttonText.isEmpty())
            buttonText = QT_TRANSLATE_NOOP("popup_window", "FORWARD");

        auto shareDialog = std::make_unique<SelectContactsWidget>(nullptr, Logic::MembersWidgetRegim::SHARE,
                                             labelText, buttonText, Ui::MainPage::instance(), true, nullptr, _enableAuthorSetting);

        QString chatId = quotes.isEmpty() ? QString() : quotes.first().chatId_;

        QString chatName;
        const auto senderChat = Logic::getContactListModel()->getContactItem(chatId);
        if (senderChat)
        {
            if (senderChat->is_chat())
            {
                const bool allSendersEqualToChat = std::all_of(quotes.begin(), quotes.end(), [&chatId](const auto & q) { return q.senderId_ == chatId; });
                if (allSendersEqualToChat)
                    shareDialog->setAuthorWidgetMode(AuthorWidget::Mode::Channel);
                else
                    shareDialog->setAuthorWidgetMode(AuthorWidget::Mode::Group);
            }
            else
            {
                shareDialog->setAuthorWidgetMode(AuthorWidget::Mode::Contact);
            }
            chatName = Logic::GetFriendlyContainer()->getFriendly(senderChat->get_aimid());
            shareDialog->setChatName(chatName);

            QSet<QString> authors;
            authors.reserve(quotes.size());
            for (auto & quote : quotes)
                authors << quote.senderFriendly_;
            shareDialog->setAuthors(authors);
        }

        statistic::getGuiMetrics().eventForwardLoaded();

        const auto action = shareDialog->show();
        if (action == QDialog::Accepted)
        {
            const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
            result = selectedContacts.size();

            const auto onlyOne = selectedContacts.size() == 1;
            const auto first = onlyOne ? selectedContacts.front() : QString();

            if (!selectedContacts.empty())
            {
                if (shareDialog->isAuthorsEnabled() && _enableAuthorSetting)
                {
                    Ui::gui_coll_helper  collection(Ui::GetDispatcher()->create_collection(), true);
                    core::ifptr<core::iarray> contactsArray(collection->create_array());
                    contactsArray->reserve(selectedContacts.size());
                    for (const auto& contact : selectedContacts)
                        contactsArray->push_back(collection.create_qstring_value(contact).get());
                    collection.set_value_as_array("contacts", contactsArray.get());
                    collection.set_value_as_string("message", std::string());
                    if (!quotes.isEmpty())
                    {
                        core::ifptr<core::iarray> quotesArray(collection->create_array());
                        quotesArray->reserve(quotes.size());

                        QString chatStamp;
                        if (senderChat && senderChat->is_chat())
                            chatStamp = senderChat->get_stamp();

                        Data::MentionMap mentions;

                        for (auto quote : quotes)
                        {
                            mentions.insert(quote.mentions_.begin(), quote.mentions_.end());

                            core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                            quote.isForward_ = true;
                            quote.serialize(quoteCollection.get());
                            core::coll_helper coll(collection->create_collection(), true);
                            core::ifptr<core::ivalue> val(collection->create_value());
                            val->set_as_collection(quoteCollection.get());
                            quotesArray->push_back(val.get());
                        }
                        collection.set_value_as_array("quotes", quotesArray.get());

                        if (!mentions.empty())
                        {
                            core::ifptr<core::iarray> mentArray(collection->create_array());
                            mentArray->reserve(mentions.size());
                            for (const auto& [aimid, friendly]: mentions)
                            {
                                core::ifptr<core::icollection> mentCollection(collection->create_collection());
                                Ui::gui_coll_helper coll(mentCollection.get(), false);
                                coll.set_value_as_qstring("aimid", aimid);
                                coll.set_value_as_qstring("friendly", friendly);

                                core::ifptr<core::ivalue> val(collection->create_value());
                                val->set_as_collection(mentCollection.get());
                                mentArray->push_back(val.get());
                            }
                            collection.set_value_as_array("mentions", mentArray.get());
                        }
                    }

                    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                }
                else
                {
                    for (auto quote : quotes)
                    {
                        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                        core::ifptr<core::iarray> contactsArray(collection->create_array());
                        contactsArray->reserve(selectedContacts.size());
                        for (const auto& contact : selectedContacts)
                            contactsArray->push_back(collection.create_qstring_value(contact).get());
                        collection.set_value_as_array("contacts", contactsArray.get());

                        Data::MentionMap mentions;
                        mentions.insert(quote.mentions_.begin(), quote.mentions_.end());

                        if (!mentions.empty())
                        {
                            core::ifptr<core::iarray> mentArray(collection->create_array());
                            mentArray->reserve(mentions.size());
                            for (const auto&[aimid, friendly] : mentions)
                            {
                                core::ifptr<core::icollection> mentCollection(collection->create_collection());
                                Ui::gui_coll_helper coll(mentCollection.get(), false);
                                coll.set_value_as_qstring("aimid", aimid);
                                coll.set_value_as_qstring("friendly", friendly);

                                core::ifptr<core::ivalue> val(collection->create_value());
                                val->set_as_collection(mentCollection.get());
                                mentArray->push_back(val.get());
                            }
                            collection.set_value_as_array("mentions", mentArray.get());
                        }

                        if (quote.isSticker())
                        {
                            collection.set_value_as_bool("sticker", true);
                            collection.set_value_as_int("sticker/set_id", quote.setId_);
                            collection.set_value_as_int("sticker/id", quote.stickerId_);
                        }
                        else
                        {
                            collection.set_value_as_qstring("message", quote.text_);
                        }

                        if (quote.sharedContact_)
                            quote.sharedContact_->serialize(collection.get());

                        collection.set_value_as_qstring("url", quote.url_);
                        collection.set_value_as_qstring("description", quote.description_);

                        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                    }
                }

                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_messagescount, { { "Forward_MessagesCount", std::to_string(quotes.size()) } });

                Ui::GetDispatcher()->post_stats_to_core(fromMenu ? core::stats::stats_event_names::forward_send_frommenu : core::stats::stats_event_names::forward_send_frombutton);

                if (onlyOne)
                    switchToContact(first);
            }
            Logic::getContactListModel()->clearChecked();
            emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        }

        return result;
    }

    void shareContact(const Data::Quote& _contact, const QString& _recieverAimId, const Data::QuotesVec& _quotes)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        core::ifptr<core::iarray> contactsArray(collection->create_array());
        contactsArray->reserve(1);
        contactsArray->push_back(collection.create_qstring_value(_recieverAimId).get());
        collection.set_value_as_array("contacts", contactsArray.get());

        collection.set_value_as_qstring("message", _contact.text_);

        if (_contact.sharedContact_)
            _contact.sharedContact_->serialize(collection.get());

        collection.set_value_as_qstring("url", _contact.url_);
        collection.set_value_as_qstring("description", _contact.description_);

        Data::serializeQuotes(collection, _quotes);
        Data::MentionMap mentions;
        for (const auto& q : _quotes)
            mentions.insert(q.mentions_.begin(), q.mentions_.end());
        Data::serializeMentions(collection, mentions);

        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_sharecontact_action);

        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    }

    int forwardMessage(const QString& _message, const QString& _labelText, const QString& _buttonText, bool _enableAuthorSetting)
    {
        Data::Quote q;
        q.text_ = _message;
        q.type_ = Data::Quote::Type::link;
        return forwardMessage({ std::move(q) }, false, _labelText, _buttonText, _enableAuthorSetting);
    }

    void eraseHistory(const QString& _aimid)
    {
        Ui::GetDispatcher()->eraseHistory(_aimid);
        Utils::InterConnector::instance().setSidebarVisible(false);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_delete);
    }

    void sharePhone(const QString& _name, const QString& _phone, const QString& _aimid)
    {
        auto shareDialog = std::make_unique<SelectContactsWidget>(nullptr, Logic::MembersWidgetRegim::SHARE,
                                                                  QT_TRANSLATE_NOOP("popup_window", "Share"), QT_TRANSLATE_NOOP("popup_window", "SEND"), Ui::MainPage::instance(), true, nullptr);

        const auto action = shareDialog->show();
        if (action == QDialog::Accepted)
        {
            const auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
            const auto onlyOne = selectedContacts.size() == 1;
            const auto first = onlyOne ? selectedContacts.front() : QString();

            if (!selectedContacts.empty())
            {
                sharePhone(_name, _phone, selectedContacts, _aimid);

                if (selectedContacts.size() == 1)
                    switchToContact(selectedContacts.front());
            }
        }

    }

    void sharePhone(const QString& _name, const QString& _phone, const std::vector<QString>& _selectedContacts, const QString& _aimid, const Data::QuotesVec& _quotes)
    {
        Ui::gui_coll_helper  collection(Ui::GetDispatcher()->create_collection(), true);
        core::ifptr<core::iarray> contactsArray(collection->create_array());
        contactsArray->reserve(_selectedContacts.size());
        for (const auto& contact : _selectedContacts)
            contactsArray->push_back(collection.create_qstring_value(contact).get());
        collection.set_value_as_array("contacts", contactsArray.get());

        core::ifptr<core::icollection> contactCollection(collection->create_collection());
        Ui::gui_coll_helper coll(contactCollection.get(), false);
        coll.set_value_as_qstring("name", _name);
        coll.set_value_as_qstring("phone", _phone);
        coll.set_value_as_qstring("sn", _aimid);

        collection.set_value_as_collection("shared_contact", contactCollection.get());

        Data::serializeQuotes(collection, _quotes);
        Data::MentionMap mentions;
        for (const auto& q : _quotes)
            mentions.insert(q.mentions_.begin(), q.mentions_.end());
        Data::serializeMentions(collection, mentions);

        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
    }

}

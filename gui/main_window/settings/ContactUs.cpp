#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../Placeholders.h"
#include "../ConnectionWidget.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/CustomButton.h"

#include "../../controls/GeneralCreator.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/CommonInput.h"
#include "../../controls/DialogButton.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../omicron/omicron_helper.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../my_info.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../../common.shared/config/config.h"
#include "../../../common.shared/string_utils.h"
#include "../../../common.shared/omicron_keys.h"
#include "styles/ThemeParameters.h"
#include "previewer/toast.h"
#include "ContactUs.h"

#include "../../controls/LoaderSpinner.h"

using namespace Ui;

namespace
{
    constexpr int MAX_FILE_SIZE = 1024 * 1024;
    constexpr int MAX_TOTAL_FILE_SIZE = 25 * 1024 * 1024;

    QString errorText(const double _size)
    {
        if (_size > 1024)
            return QT_TRANSLATE_NOOP("contactus_page", "File size exceeds 1 MB");
        else
            return QT_TRANSLATE_NOOP("contactus_page", "Attachments size exceeds 25 MB");
    }

    QString suggestionSizeErrorText(bool _tooLong = false)
    {
        if (_tooLong)
            return QT_TRANSLATE_NOOP("contactus_page", "Message is too long");
        else
            return QT_TRANSLATE_NOOP("contactus_page", "Message is too short");
    }

    QFont labelFont()
    {
        static const auto f = Fonts::appFontScaled(16);
        return f;
    }

    QFont captionFont()
    {
        static const auto f = Fonts::appFontScaled(16);
        return f;
    }

    static const QString defaultProblem = QT_TRANSLATE_NOOP("contactus_page", "Nothing selected");

    static const std::vector<QString>& getProblemsList()
    {
        static const std::vector<QString> problemsList =
        {
            QT_TRANSLATE_NOOP("contactus_page", "Choose your problem"),
            QT_TRANSLATE_NOOP("contactus_page", "Emoji and stickers"),
            QT_TRANSLATE_NOOP("contactus_page", "Avatars"),
            QT_TRANSLATE_NOOP("contactus_page", "Videoplayer"),
            QT_TRANSLATE_NOOP("contactus_page", "Gallery"),
            QT_TRANSLATE_NOOP("contactus_page", "Ptt"),
            QT_TRANSLATE_NOOP("contactus_page", "Hotkeys"),
            QT_TRANSLATE_NOOP("contactus_page", "Crash"),
            QT_TRANSLATE_NOOP("contactus_page", "Pinned message"),
            QT_TRANSLATE_NOOP("contactus_page", "Decelerations"),
            QT_TRANSLATE_NOOP("contactus_page", "VOIP"),
            QT_TRANSLATE_NOOP("contactus_page", "Sounds"),
            QT_TRANSLATE_NOOP("contactus_page", "History"),
            QT_TRANSLATE_NOOP("contactus_page", "Channels"),
            QT_TRANSLATE_NOOP("contactus_page", "Contact list"),
            QT_TRANSLATE_NOOP("contactus_page", "Settings"),
            QT_TRANSLATE_NOOP("contactus_page", "Nick"),
            QT_TRANSLATE_NOOP("contactus_page", "Search"),
            QT_TRANSLATE_NOOP("contactus_page", "Mail for domains"),
            QT_TRANSLATE_NOOP("contactus_page", "Previews"),
            QT_TRANSLATE_NOOP("contactus_page", "Connection"),
            QT_TRANSLATE_NOOP("contactus_page", "Messages"),
            QT_TRANSLATE_NOOP("contactus_page", "Statuses"),
            QT_TRANSLATE_NOOP("contactus_page", "Notifications"),
            QT_TRANSLATE_NOOP("contactus_page", "Mentions"),
            QT_TRANSLATE_NOOP("contactus_page", "Install"),
            QT_TRANSLATE_NOOP("contactus_page", "File sharing"),
            QT_TRANSLATE_NOOP("contactus_page", "Wallpapers"),
            QT_TRANSLATE_NOOP("contactus_page", "Group chats"),
            QT_TRANSLATE_NOOP("contactus_page", "Different problem")
        };

        return problemsList;
    }

    std::string get_feedback_url()
    {
        return Omicron::_o(omicron::keys::feedback_url, su::concat("https://", config::get().url(config::urls::feedback)));
    }

    constexpr std::chrono::milliseconds getDebugInfoTimeout() noexcept
    {
        return std::chrono::milliseconds(500);
    }

    auto getIconsSize()
    {
        return QSize(24, 24);
    }

    auto getIconsColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
    }

    auto getInfoTextColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto getDebugInfoSpinnerPenWidth()
    {
        return Utils::fscale_value(1.3);
    }

    auto getDebugInfoSpinnerWidth()
    {
        return Utils::fscale_value(20);
    }

    auto getMinWidth()
    {
        return Utils::scale_value(300);
    }

    auto getMaxWidth()
    {
        return Utils::scale_value(400);
    }

    auto getShadowWidth()
    {
        return Utils::scale_value(7);
    }

    auto getHorMargin()
    {
        return Utils::scale_value(13);
    }

    auto getVerMargin()
    {
        return Utils::scale_value(36);
    }

    auto getFullHorMargins()
    {
        return getShadowWidth() + getHorMargin();
    }
}

ContactUsWidget::ContactUsWidget(QWidget *_parent, bool _isPlain)
    : QWidget(_parent)
    , sendingPage_(nullptr)
    , successPage_(nullptr)
    , attachWidget_(nullptr)
    , dropper_(nullptr)
    , debugInfoWidget_(nullptr)
    , hasProblemDropper_(false)
    , isPlain_(_isPlain)
{
    setMinimumWidth(getMinWidth() + 2 * getFullHorMargins());
    hasProblemDropper_ = config::get().is_on(config::features::feedback_selected) && (get_gui_settings()->get_value(settings_language, QString()) == u"ru") && !isPlain_;

    init();
    setState(State::Feedback);

    connect(sendButton_, &QPushButton::pressed, this, &ContactUsWidget::sendFeedback);

    connect(GetDispatcher(), &core_dispatcher::feedbackSent, this, [this](bool succeeded)
    {
        resetState();

        if (!succeeded)
        {
            showError(ErrorReason::Feedback);
        }
        else
        {
            attachWidget_->clearFileList();
            suggestioner_->clear();
            setState(State::Success);
        }
    });
}

void ContactUsWidget::init()
{
    scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea_->setWidgetResizable(true);

    Utils::ApplyStyle(scrollArea_, Styling::getParameters().getContactUsQss());
    Utils::grabTouchWidget(scrollArea_->viewport(), true);

    mainWidget_ = new QWidget(scrollArea_);
    Utils::grabTouchWidget(mainWidget_);
    mainWidget_->installEventFilter(this);

    auto mainLayout = Utils::emptyVLayout(mainWidget_);
    mainLayout->setAlignment(Qt::AlignTop);
    if (!isPlain_)
        mainLayout->setContentsMargins(getHorMargin(), getVerMargin(), getHorMargin(), getVerMargin());

    scrollArea_->setWidget(mainWidget_);

    auto layout = Utils::emptyHLayout(this);
    Testing::setAccessibleName(scrollArea_, qsl("AS ContactUsPage scrollArea"));
    layout->addWidget(scrollArea_);

    sendingPage_ = new QWidget(scrollArea_);
    successPage_ = new QWidget(scrollArea_);

    Utils::grabTouchWidget(successPage_);
    Utils::grabTouchWidget(sendingPage_);

    Testing::setAccessibleName(successPage_, qsl("AS ContactUsPage successPage"));
    Testing::setAccessibleName(sendingPage_, qsl("AS ContactUsPage sendingPage"));

    initFeedback();
    initSuccess();
    mainLayout->addWidget(sendingPage_);
    mainLayout->addWidget(successPage_);
}

void ContactUsWidget::setState(const State &_state)
{
    if (_state == State::Feedback)
    {
        if (dropper_)
            dropper_->setCurrentIndex(0);
        successPage_->hide();
        sendingPage_->show();
    }
    else
    {
        sendingPage_->hide();
        successPage_->show();
    }
}

void ContactUsWidget::initSuccess()
{
    successPage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto successPageLayout = Utils::emptyVLayout(successPage_);
    successPageLayout->setAlignment(Qt::AlignCenter);
    {
        successPageLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        auto successWidget = new QWidget(successPage_);
        Utils::grabTouchWidget(successWidget);
        auto successImageLayout = Utils::emptyVLayout(successWidget);
        successImageLayout->setAlignment(Qt::AlignCenter);
        {
            auto successImage = new QPushButton(successWidget);
            successImage->setObjectName(qsl("successImage"));
            successImage->setFlat(true);
            successImage->setFixedSize(Utils::scale_value(96), Utils::scale_value(96));
            Testing::setAccessibleName(successImage, qsl("AS ContactUsPage successImage"));
            successImageLayout->addWidget(successImage);
        }
        Testing::setAccessibleName(successWidget, qsl("AS ContactUsPage successWidget"));
        successPageLayout->addWidget(successWidget);

        auto thanksWidget = new QWidget(successPage_);
        auto thanksLayout = Utils::emptyVLayout(thanksWidget);
        Utils::grabTouchWidget(thanksWidget);
        thanksLayout->setAlignment(Qt::AlignCenter);
        thanksLayout->addSpacing(Utils::scale_value(16));
        {
            auto thanksLabel = new QLabel(thanksWidget);
            thanksLabel->setStyleSheet(ql1s("color: %1;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));
            thanksLabel->setFont(labelFont());
            thanksLabel->setWordWrap(true);
            thanksLabel->setText(QT_TRANSLATE_NOOP("contactus_page", "Thank you for contacting us! We will reply as soon as possible."));
            thanksLabel->setAlignment(Qt::AlignCenter);
            thanksLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            thanksLabel->setFixedHeight(thanksLabel->heightForWidth(width()));
            Testing::setAccessibleName(thanksLabel, qsl("AS ContactUsPage thanksLabel"));
            thanksLayout->addWidget(thanksLabel);
        }
        Testing::setAccessibleName(thanksWidget, qsl("AS ContactUsPage thanksWidget"));
        successPageLayout->addWidget(thanksWidget);

        auto resendWidget = new QWidget(successPage_);
        auto resendLayout = Utils::emptyVLayout(resendWidget);
        Utils::grabTouchWidget(resendWidget);
        resendLayout->setAlignment(Qt::AlignCenter);
        {
            auto resendLink = new TextEmojiWidget(resendWidget, captionFont(),
                Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY }, Utils::scale_value(32));
            resendLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Send another review"));
            resendLink->setSizePolicy(QSizePolicy::Preferred, resendLink->sizePolicy().verticalPolicy());
            resendLink->setCursor(Qt::PointingHandCursor);

            connect(resendLink, &TextEmojiWidget::clicked, this, [this]()
            {
                setState(State::Feedback);
            });
            Testing::setAccessibleName(resendLink, qsl("AS ContactUsPage resendLink"));
            resendLayout->addWidget(resendLink);
        }
        Testing::setAccessibleName(resendWidget, qsl("AS ContactUsPage resendWidget"));
        successWidget->updateGeometry();
        successPageLayout->addWidget(resendWidget);
        successPageLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        successPage_->updateGeometry();
    }
}

void ContactUsWidget::initFeedback()
{
    auto sendingPageLayout = Utils::emptyVLayout(sendingPage_);
    sendingPageLayout->setContentsMargins(getShadowWidth(), 0, getShadowWidth(), 0);
    sendingPageLayout->setAlignment(Qt::AlignTop);
    const auto horAlignment = isPlain_ ? Qt::AlignHCenter : Qt::AlignLeft;

    static auto& problems = getProblemsList();

    if (hasProblemDropper_)
    {
        dropper_ = GeneralCreator::addComboBox
        (
            sendingPage_
            , sendingPageLayout
            , QString()
            , true
            , problems
            , 0
            , -1
            , [=](const QString&, int _idx) { selectedProblem_ = _idx == 0 ? defaultProblem : problems[_idx]; }
        );
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, dropper_, &QComboBox::hidePopup);
        sendingPageLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(13), QSizePolicy::Expanding, QSizePolicy::Fixed));
    }

    suggestioner_ = new TextEditInputContainer(sendingPage_);
    Utils::grabTouchWidget(suggestioner_);
    suggestioner_->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page", "Your comments or suggestions..."));
    suggestioner_->setMaximumWidth(getMaxWidth());
    sendingPageLayout->addWidget(suggestioner_);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsContactUsShown, suggestioner_, qOverload<>(&TextEditEx::setFocus));

    Testing::setAccessibleName(suggestioner_, qsl("AS ContactUsPage suggestionInput"));

    suggestionSizeError_ = new TextEmojiWidget(sendingPage_, captionFont(),
        Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION }, Utils::scale_value(12));
    Utils::grabTouchWidget(suggestionSizeError_);
    suggestionSizeError_->setVisible(false);
    Testing::setAccessibleName(suggestionSizeError_, qsl("AS ContactUsPage suggestionSizeError"));
    sendingPageLayout->addWidget(suggestionSizeError_);
    sendingPageLayout->setAlignment(suggestionSizeError_, horAlignment);
    sendingPageLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(13), QSizePolicy::Expanding, QSizePolicy::Fixed));

    attachWidget_ = new AttachFileWidget(sendingPage_);
    Testing::setAccessibleName(attachWidget_, qsl("feedback_filer"));
    Utils::grabTouchWidget(attachWidget_);
    attachWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sendingPageLayout->addWidget(attachWidget_);
    sendingPageLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Fixed));

    debugInfoWidget_ = new GetDebugInfoWidget(sendingPage_);
    Testing::setAccessibleName(debugInfoWidget_, qsl("AS ContactUsPage debugInfoWidget"));
    Utils::grabTouchWidget(debugInfoWidget_);
    sendingPageLayout->addWidget(debugInfoWidget_);
    sendingPageLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(17), QSizePolicy::Expanding, QSizePolicy::Fixed));

    email_ = new CommonInputContainer(sendingPage_);
    email_->setFont(labelFont());
    email_->setMaximumWidth(getMaxWidth());
    email_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    email_->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page", "Your Email"));
    email_->setVisible(!Features::isContactUsViaBackend());

    QString communication_email = get_gui_settings()->get_value(settings_feedback_email, QString());
    QString aimid = MyInfo()->aimId();

    if (communication_email.isEmpty() && config::get().is_on(config::features::contact_us_via_mail_default) && aimid.contains(u'@'))
        communication_email = aimid;
    email_->setText(communication_email);
    Testing::setAccessibleName(email_, qsl("AS ContactUsPage email"));
    sendingPageLayout->addWidget(email_);

    sendWidget_ = new QWidget(sendingPage_);
    auto sendLayout = Utils::emptyVLayout(sendWidget_);
    Utils::grabTouchWidget(sendWidget_);
    sendWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    errorOccuredWidget_ = new QWidget(sendWidget_);
    auto errorLayout = Utils::emptyVLayout(errorOccuredWidget_);
    errorOccuredSign_ = new TextEmojiWidget(errorOccuredWidget_, captionFont(),
        Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION }, Utils::scale_value(12));
    Utils::grabTouchWidget(errorOccuredSign_);
    Testing::setAccessibleName(errorOccuredSign_, qsl("AS ContactUsPage errorOccuredSign"));
    errorLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(9), QSizePolicy::Expanding, QSizePolicy::Fixed));
    errorLayout->addWidget(errorOccuredSign_);
    errorLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(16)));
    errorLayout->setAlignment(errorOccuredSign_, horAlignment);
    sendLayout->addWidget(errorOccuredWidget_);
    errorOccuredSign_->setVisible(false);
    QFontMetrics metrics(captionFont());
    errorOccuredWidget_->setFixedHeight(metrics.height() + Utils::scale_value(25));
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, this,
    [this](int type)
    {
        if ((Utils::CommonSettingsType)type == Utils::CommonSettingsType::CommonSettingsType_ContactUs && !Utils::isValidEmailAddress(email_->text()))
            email_->setText(QString());
        errorOccuredSign_->setVisible(false);
    });

    sendButton_ = new DialogButton(sendWidget_, QT_TRANSLATE_NOOP("contactus_page", "Send"));
    sendButton_->setCursor(Qt::PointingHandCursor);
    sendButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sendButton_->setMinimumWidth(Utils::scale_value(100));
    sendButton_->setFixedHeight(Utils::scale_value(40));
    sendButton_->setContentsMargins(Utils::scale_value(32), 0, Utils::scale_value(32), 0);
    sendButton_->setEnabled(!suggestioner_->getPlainText().isEmpty() && !email_->text().isEmpty());
    sendButton_->updateWidth();

    Testing::setAccessibleName(sendButton_, qsl("AS ContactUsPage sendButton"));
    sendLayout->addWidget(sendButton_);
    sendLayout->setAlignment(sendButton_, horAlignment);

    connect(suggestioner_->document(), &QTextDocument::contentsChanged, this, [this]()
    {
        bool state = !suggestioner_->getPlainText().isEmpty();
        if (state)
            suggestionSizeError_->setVisible(false);

        errorOccuredSign_->setVisible(false);
        sendButton_->setEnabled(state && !email_->text().isEmpty());
    });

    connect(email_, &Ui::CommonInputContainer::textChanged, this, [this](const QString &)
    {
        errorOccuredSign_->setVisible(false);
        sendButton_->setEnabled(!suggestioner_->getPlainText().isEmpty() && !email_->text().isEmpty());
    });

    sendSpinner_ = new RotatingSpinner(sendWidget_);
    sendSpinner_->setVisible(false);
    Testing::setAccessibleName(sendSpinner_, qsl("AS ContactUsPage sendSpinner"));
    sendLayout->addWidget(sendSpinner_);
    sendLayout->setAlignment(sendSpinner_, horAlignment);

    auto sendingSign = new TextEmojiWidget(sendWidget_, labelFont(),
        Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY }, Utils::scale_value(16));
    Utils::grabTouchWidget(sendingSign);
    sendingSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Sending..."));
    sendingSign->setVisible(false);
    Testing::setAccessibleName(sendingSign, qsl("AS ContactUsPage sendingSign"));
    sendLayout->addWidget(sendingSign);

    sendingPageLayout->addWidget(sendWidget_);
}

void ContactUsWidget::sendFeedback()
{
    get_gui_settings()->set_value(settings_feedback_email, email_->text());
    suggestionSizeError_->setVisible(false);
    errorOccuredSign_->setVisible(false);
    attachWidget_->hideError();
    auto sg = suggestioner_->getPlainText();
    sg.remove(ql1c(' '));

    if (sg.isEmpty() || sg.size() > 2048)
    {
        suggestionSizeError_->setText(suggestionSizeErrorText(!sg.isEmpty()));
        suggestionSizeError_->setVisible(true);
    }
    else if (!Utils::isValidEmailAddress(email_->text()))
    {
        showError(ErrorReason::Email);
    }
    else
    {
        sendButton_->setVisible(false);
        errorOccuredSign_->setVisible(false);
        if (sendSpinner_)
        {
            sendSpinner_->setVisible(true);
            sendSpinner_->startAnimation();
        }

        suggestioner_->setEnabled(false);
        attachWidget_->setEnabled(false);
        debugInfoWidget_->setEnabled(false);
        email_->setEnabled(false);

        const auto myInfo = MyInfo();

        Ui::gui_coll_helper col(GetDispatcher()->create_collection(), true);

        if (!Features::isContactUsViaBackend())
        {
            col.set_value_as_string("url", get_feedback_url());

            // fb.screen_resolution
            col.set_value_as_qstring("screen_resolution", (qsl("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())));
            // fb.referrer
            const auto shortName = config::get().string(config::values::product_name_short);
            col.set_value_as_string("referrer", shortName);
            {
                const auto appName = config::get().string(config::values::product_name);
                const QString icqVer = QString::fromUtf8(appName.data(), appName.size()) + QString::fromUtf8(VERSION_INFO_STR);
                auto osv = QSysInfo::prettyProductName();
                if (osv.isEmpty() || osv == u"unknown")
                    osv = QStringView(u"%1 %2 (%3 %4)").arg(QSysInfo::productType(), QSysInfo::productVersion(), QSysInfo::kernelType(), QSysInfo::kernelVersion());

                const auto concat = ql1s("%1 %2 %3:%4").arg(osv, icqVer, QString::fromUtf8(shortName.data(), shortName.size()), myInfo->aimId());
                // fb.question.3004
                col.set_value_as_qstring("version", concat);
                // fb.question.159
                col.set_value_as_qstring("os_version", osv);
                // fb.question.178
                col.set_value_as_string("build_type", build::is_debug() ? "beta" : "live");
                // fb.question.3005
                if constexpr (platform::is_apple())
                    col.set_value_as_string("platform", "macOS");
                else if constexpr (platform::is_windows())
                    col.set_value_as_string("platform", "Windows");
                else if constexpr (platform::is_linux())
                    col.set_value_as_string("platform", "Linux");
                else
                    col.set_value_as_string("platform", "Unknown");
            }

            auto username = myInfo->friendly();
            if (username.isEmpty())
                username = myInfo->nick();
            if (username.isEmpty())
                username = myInfo->aimId();

            // fb.user_name
            col.set_value_as_qstring("user_name", username);
        }
        col.set_value_as_qstring("aimid", myInfo->aimId());

        // fb.message
        col.set_value_as_qstring("message", suggestioner_->getPlainText());

        if (!Features::isContactUsViaBackend())
        {
            // communication_email
            col.set_value_as_qstring("communication_email", email_->text());
            // Lang
            col.set_value_as_qstring("language", QLocale::system().name());

            if (hasProblemDropper_)
            {
                const auto problem = config::get().string(config::values::feedback_selected_id);
                col.set_value_as_qstring(problem, selectedProblem_);
            }
        }
        // attachements_count
        const auto& filesToSend = attachWidget_->getFiles();
        col.set_value_as_string("attachements_count", std::to_string(filesToSend.size() + 1)); // +1 'coz we're sending log txt
        if (!filesToSend.empty())
        {
            core::ifptr<core::iarray> farray(col->create_array());
            farray->reserve((int)filesToSend.size());
            for (const auto &f : filesToSend)
                farray->push_back(col.create_qstring_value(f.second).get());

            // fb.attachement
            col.set_value_as_array("attachement", farray.get());
        }
        GetDispatcher()->post_message_to_core("feedback/send", col.get());
    }
}

void ContactUsWidget::resizeEvent(QResizeEvent *_event)
{
    QWidget::resizeEvent(_event);
    const auto currentWidth = width();
    const auto currentMargins = isPlain_ ? 2 * getShadowWidth() : 2 * getFullHorMargins();
    const auto maxWidth = (currentWidth > (getMaxWidth() + currentMargins)) ? getMaxWidth() : currentWidth - currentMargins;

    suggestioner_->setFixedWidth(maxWidth);
    email_->setFixedWidth(maxWidth);
}

void ContactUsWidget::resetState(ClearData _mode)
{
    if (_mode == ClearData::Yes)
    {
        suggestioner_->clear();
        attachWidget_->clearFileList();
    }
    if (sendSpinner_)
    {
        sendSpinner_->setVisible(false);
        sendSpinner_->stopAnimation();
    }
    suggestioner_->setEnabled(true);
    suggestionSizeError_->setVisible(false);
    attachWidget_->setEnabled(true);
    debugInfoWidget_->reset();
    debugInfoWidget_->setEnabled(true);
    email_->setEnabled(true);
    errorOccuredSign_->setVisible(false);
    sendButton_->setVisible(true);
    sendButton_->setEnabled(!suggestioner_->getPlainText().isEmpty() && !email_->text().isEmpty());
    setState(State::Feedback);
}

void ContactUsWidget::focusInEvent(QFocusEvent *_event)
{
    suggestioner_->setFocus();
    QWidget::focusInEvent(_event);
}

bool ContactUsWidget::eventFilter(QObject* _obj, QEvent* _event)
{
    if (_obj == mainWidget_ && _event->type() == QEvent::Resize)
        scrollArea_->ensureWidgetVisible(sendButton_);
    return QWidget::eventFilter(_obj, _event);
}

void ContactUsWidget::showError(ErrorReason _reason)
{
    const auto error = _reason == ErrorReason::Email
                        ? QT_TRANSLATE_NOOP("contactus_page", "Please enter a valid email address")
                        : QT_TRANSLATE_NOOP("contactus_page", "Error occurred, try again later");
    errorOccuredSign_->setText(error);
    errorOccuredSign_->setVisible(true);
}

AttachFileWidget::AttachFileWidget(QWidget *_parent)
    : QWidget(_parent)
    , totalFileSize_(0)
{
    auto layout = Utils::emptyVLayout(this);
    attachWidget_ = new QWidget(this);
    attachWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    attachLayout_ = Utils::emptyVLayout(attachWidget_);
    attachLayout_->setAlignment(Qt::AlignBottom);

    errorWidget_ = new QWidget(attachWidget_);
    auto errorLayout = Utils::emptyVLayout(errorWidget_);
    attachSizeError_ = new TextEmojiWidget(errorWidget_, captionFont(),
        Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION }, Utils::scale_value(12));
    Testing::setAccessibleName(attachSizeError_, qsl("AS ContactUsPage attachSizeError"));
    errorLayout->addWidget(attachSizeError_);
    errorLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(8), QSizePolicy::Expanding, QSizePolicy::Fixed));
    errorWidget_->setFixedHeight(attachSizeError_->height() + Utils::scale_value(8));
    attachLayout_->addWidget(errorWidget_);
    attachSizeError_->setVisible(false);

    filesLayout_ = Utils::emptyVLayout();
    attachLayout_->addLayout(filesLayout_);

    attachLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(8), QSizePolicy::Expanding, QSizePolicy::Fixed));
    auto attachLinkWidget = new QWidget(attachWidget_);
    auto attachLinkLayout = Utils::emptyHLayout(attachLinkWidget);
    Utils::grabTouchWidget(attachLinkWidget);
    attachLinkWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    attachLinkWidget->setCursor(Qt::PointingHandCursor);
    attachLinkLayout->setSpacing(Utils::scale_value(12));

    auto attachImage = new CustomButton(attachLinkWidget);
    attachImage->setDefaultImage(qsl(":/input/attach_photo"), getIconsColorKey(), getIconsSize());
    attachImage->setFixedSize(Utils::scale_value(getIconsSize()));
    connect(attachImage, &QPushButton::clicked, this, &AttachFileWidget::attachFile);
    Testing::setAccessibleName(attachImage, qsl("AS ContactUsPage attachImage"));
    attachLinkLayout->addWidget(attachImage);

    auto attachLink = new TextEmojiWidget(attachLinkWidget, labelFont(), getInfoTextColorKey(), Utils::scale_value(16));
    Utils::grabTouchWidget(attachLink);
    attachLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Attach screenshot"));
    connect(attachLink, &TextEmojiWidget::clicked, this, &AttachFileWidget::attachFile);
    Testing::setAccessibleName(attachLink, qsl("AS ContactUsPage attachLink"));
    attachLinkLayout->addWidget(attachLink);

    Testing::setAccessibleName(attachLinkWidget, qsl("AS ContactUsPage attachLinkWidget"));
    attachLayout_->addWidget(attachLinkWidget);

    layout->addWidget(attachWidget_);
}

void AttachFileWidget::attachFile()
{
    QWidget *parentForDialog = platform::is_linux() ? nullptr : attachWidget_;

    static auto dd = QDir::homePath();
    QFileDialog d(parentForDialog);
    d.setDirectory(dd);
    d.setFileMode(QFileDialog::ExistingFiles);
    d.setNameFilter(QT_TRANSLATE_NOOP("contactus_page", "Images (*.jpg *.jpeg *.png *.bmp)"));
    QStringList fileNames;
    if (d.exec())
    {
        dd = d.directory().absolutePath();
        const auto selectedFiles = d.selectedFiles();
        for (auto& f : selectedFiles)
        {
            hideError();
            processFile(f);
        }
    }
}

void AttachFileWidget::processFile(const QString& _filePath)
{
    auto normalizedFileName = QDir::fromNativeSeparators(_filePath);
    auto fileName = normalizedFileName.mid(normalizedFileName.lastIndexOf(ql1c('/')) + 1);
    const auto fileSize = QFileInfo(_filePath).size();
    if (filesToSend_.empty() || filesToSend_.find(fileName) == filesToSend_.end())
    {
        filesToSend_.insert(std::make_pair(fileName, _filePath));
        auto fileWidget = new QWidget(attachWidget_);
        auto fileLayout = Utils::emptyHLayout(fileWidget);
        Utils::grabTouchWidget(fileWidget);
        fileWidget->setObjectName(fileName);
        fileWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        fileWidget->setFixedHeight(Utils::scale_value(32));
        fileWidget->setProperty("fileWidget", true);
        fileLayout->setAlignment(Qt::AlignVCenter);

        if (fileSize > MAX_FILE_SIZE)
        {
            attachSizeError_->setText(errorText(fileSize));
            attachSizeError_->updateGeometry();
            attachSizeError_->setVisible(true);
            filesToSend_.erase(fileName);
            return;
        }
        if (totalFileSize_ + fileSize > MAX_TOTAL_FILE_SIZE)
        {
            attachSizeError_->setText(errorText(totalFileSize_ + fileSize));
            attachSizeError_->updateGeometry();
            attachSizeError_->setVisible(true);
            filesToSend_.erase(fileName);
            return;
        }
        totalFileSize_ += fileSize;

        double fs = fileSize / 1024.f;
        QString sizeString;
        if (fs < 100)
        {
            sizeString = QString::number(fs, 'f', 2) % ql1c(' ') % QT_TRANSLATE_NOOP("contactus_page", "KB");
        }
        else
        {
            fs /= 1024.;
            sizeString = QString::number(fs, 'f', 2) % ql1c(' ') % QT_TRANSLATE_NOOP("contactus_page", "MB");
        }

        auto fileInfoWidget = new QWidget(fileWidget);
        auto fileInfoLayout = Utils::emptyHLayout(fileInfoWidget);
        Utils::grabTouchWidget(fileInfoWidget);
        fileInfoWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        fileInfoLayout->setAlignment(Qt::AlignLeft);

        const auto metrics = QFontMetrics(labelFont());
        const auto maxFilenameLength = parentWidget()->width() - Utils::scale_value(144);
        if (metrics.horizontalAdvance(fileName) > maxFilenameLength)
            fileName = metrics.elidedText(fileName, Qt::ElideMiddle, maxFilenameLength);

        auto fileNameLabel = new QLabel(fileInfoWidget);
        Utils::grabTouchWidget(fileNameLabel);
        fileNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        fileNameLabel->setText(fileName);
        fileNameLabel->setFont(labelFont());
        QPalette pal = fileNameLabel->palette();
        pal.setColor(QPalette::Foreground, Styling::getColor(getInfoTextColorKey()));
        fileNameLabel->setPalette(pal);

        Testing::setAccessibleName(fileNameLabel, qsl("AS ContactUsPage fileName"));
        fileInfoLayout->addWidget(fileNameLabel);

        auto fileSizeLabel = new LabelEx(fileInfoWidget);
        Utils::grabTouchWidget(fileSizeLabel);
        fileSizeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        fileSizeLabel->setText(u" - " % sizeString);
        fileSizeLabel->setColor(getInfoTextColorKey());
        fileSizeLabel->setFont(labelFont());
        Testing::setAccessibleName(fileSizeLabel, qsl("AS ContactUsPage fileSize"));
        fileInfoLayout->addWidget(fileSizeLabel);

        Testing::setAccessibleName(fileInfoWidget, qsl("AS ContactUsPage fileInfoWidget"));
        fileLayout->addWidget(fileInfoWidget);

        auto deleteFile = new CustomButton(fileWidget, qsl(":/controls/close_icon"), QSize(12, 12), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
        deleteFile->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER });
        deleteFile->setFixedSize(Utils::scale_value(QSize(20, 20)));

        connect(deleteFile, &CustomButton::clicked, this, [this, fileWidget, fileSize]()
        {
            attachLayout_->invalidate();
            parentWidget()->layout()->invalidate();
            filesToSend_.erase(fileWidget->objectName());
            totalFileSize_ -= fileSize;
            attachSizeError_->setVisible(false);
            filesLayout_->removeWidget(fileWidget);
            fileWidget->deleteLater();
            update();
        });

        Testing::setAccessibleName(deleteFile, qsl("AS ContactUsPage deleteFile"));
        fileLayout->addWidget(deleteFile);
        fileLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(8), 0, QSizePolicy::Fixed));

        filesLayout_->insertWidget(0, fileWidget); // always insert at the top
    }
}

void AttachFileWidget::clearFileList()
{
    for (const auto& p : std::exchange(filesToSend_, {}))
    {
        auto f = attachWidget_->findChild<QWidget *>(p.first);
        if (f)
        {
            filesLayout_->removeWidget(f);
            f->deleteLater();
        }
    }

    totalFileSize_ = 0;
}

void AttachFileWidget::hideError() const
{
    attachSizeError_->setVisible(false);
}

GetDebugInfoWidget::GetDebugInfoWidget(QWidget* _parent)
    : QWidget(_parent)
    , icon_(nullptr)
    , progress_(nullptr)
    , checkTimer_(new QTimer(this))
{
    setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

    auto rootVLayout = Utils::emptyVLayout(this);
    rootVLayout->setAlignment(Qt::AlignTop);

    auto commonWidget = new QWidget(this);
    auto commonLayout = Utils::emptyHLayout(commonWidget);
    {
        commonLayout->setSpacing(Utils::scale_value(12));
        commonWidget->setCursor(Qt::PointingHandCursor);

        icon_ = new CustomButton(commonWidget);
        icon_->setDefaultImage(qsl(":/input/attach_documents"), getIconsColorKey(), getIconsSize());
        icon_->setFixedSize(Utils::scale_value(getIconsSize()));
        connect(icon_, &QPushButton::clicked, this, &GetDebugInfoWidget::onClick);
        Testing::setAccessibleName(icon_, qsl("AS ContactUsPage debug_icon"));
        commonLayout->addWidget(icon_);

        progress_ = new ProgressAnimation(commonWidget);
        progress_->setProgressWidth(getDebugInfoSpinnerWidth());
        progress_->setFixedSize(getIconsSize());
        progress_->hide();
        commonLayout->addWidget(progress_);

        auto getLogsButton = new TextEmojiWidget(commonWidget, labelFont(), getInfoTextColorKey(), Utils::scale_value(16));
        Utils::grabTouchWidget(getLogsButton);
        getLogsButton->setText(QT_TRANSLATE_NOOP("contactus_page", "Get debug information"));
        connect(getLogsButton, &TextEmojiWidget::clicked, this, &GetDebugInfoWidget::onClick);
        Testing::setAccessibleName(getLogsButton, qsl("AS ContactUsPage getLogsButton"));
        commonLayout->addWidget(getLogsButton);
    }

    rootVLayout->addWidget(commonWidget);

    checkTimer_->setSingleShot(true);
    checkTimer_->setInterval(getDebugInfoTimeout());

    connect(checkTimer_, &QTimer::timeout, this, &GetDebugInfoWidget::onTimeout);
}

void GetDebugInfoWidget::onClick()
{
    setEnabled(false);
    checkTimer_->start();

    Ui::gui_coll_helper col(GetDispatcher()->create_collection(), true);
    col.set_value_as_qstring("path", Ui::getDownloadPath());
    GetDispatcher()->post_message_to_core("create_logs_archive", col.get(), this,
        [this](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);
            onResult(QString::fromUtf8(coll.get_value_as_string("archive")));
        });
}

void GetDebugInfoWidget::onTimeout()
{
    startAnimation();
}

void GetDebugInfoWidget::onResult(const QString& _fileName)
{
    checkTimer_->stop();
    stopAnimation();

    setEnabled(true);

    Data::FileSharingDownloadResult result;
    result.filename_ = _fileName;
    Utils::showDownloadToast({}, result);
}

void GetDebugInfoWidget::startAnimation()
{
    icon_->hide();
    progress_->show();

    progress_->setProgressPenColorKey(getIconsColorKey());
    progress_->setProgressPenWidth(getDebugInfoSpinnerPenWidth());
    progress_->start();
}

void GetDebugInfoWidget::reset()
{
    stopAnimation();
}

void GetDebugInfoWidget::stopAnimation()
{
    progress_->stop();
    progress_->hide();

    icon_->show();
}

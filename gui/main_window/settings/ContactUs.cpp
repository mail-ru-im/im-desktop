#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../Placeholders.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/CustomButton.h"

#include "../../controls/GeneralCreator.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../omicron/omicron_helper.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../../../common.shared/version_info_constants.h"
#include "styles/ThemeParameters.h"
#include "../../controls/DialogButton.h"
#include "ContactUs.h"


using namespace Ui;

namespace
{
    constexpr int TEXTEDIT_MIN_HEIGHT = 88;
    constexpr int TEXTEDIT_MAX_HEIGHT = 252;

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

    static const std::vector<QString>& getProblemsList()
    {
        static const std::vector<QString> problemsList =
        {
            QT_TRANSLATE_NOOP("contactus_page", "Nothing selected"),
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
        const std::string defaultUrl = build::GetProductVariant(icq_feedback_url, agent_feedback_url, biz_feedback_url, dit_feedback_url);
        return Omicron::_o("feedback_url", defaultUrl);
    }
}

ContactUsWidget::ContactUsWidget(QWidget *_parent)
    : QWidget(_parent)
    , sendingPage_(nullptr)
    , successPage_(nullptr)
    , attachWidget_(nullptr)
    , dropper_(nullptr)
    , hasProblemDropper_(false)
{

    hasProblemDropper_ = (get_gui_settings()->get_value(settings_language, QString()) == qsl("ru")) && (build::is_icq() || build::is_agent());

    init();
    setState(State::Feedback);

    connect(sendButton_, &QPushButton::pressed, this, &ContactUsWidget::sendFeedback);

    connect(GetDispatcher(), &core_dispatcher::feedbackSent, this, [this](bool succeeded)
    {
        sendButton_->setVisible(true);
        sendSpinner_->setVisible(false);
        sendSpinner_->stopAnimation();

        suggestioner_->setEnabled(true);
        attachWidget_->setEnabled(true);
        email_->setEnabled(true);


        if (!succeeded)
        {
            errorOccuredSign_->setVisible(true);
        }
        else
        {
            attachWidget_->clearFileList();
            suggestioner_->setText(QString());
            setState(State::Success);
        }
    });
}

void ContactUsWidget::init()
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea->setWidgetResizable(true);

    Utils::ApplyStyle(scrollArea, Styling::getParameters().getContactUsQss());
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(36), Utils::scale_value(20), Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    Testing::setAccessibleName(scrollArea, qsl("AS contanctus scrollArea"));
    layout->addWidget(scrollArea);

    sendingPage_ = new QWidget(scrollArea);
    successPage_ = new QWidget(scrollArea);
    successPage_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    Utils::grabTouchWidget(successPage_);
    Utils::grabTouchWidget(sendingPage_);

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
    auto successPageLayout = Utils::emptyVLayout(successPage_);
    successPageLayout->setAlignment(Qt::AlignCenter);
    {
        auto successWidget = new QWidget(successPage_);
        Utils::grabTouchWidget(successWidget);
        auto successImageLayout = Utils::emptyVLayout(successWidget);
        successWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        successImageLayout->setAlignment(Qt::AlignCenter);
        {
            auto successImage = new QPushButton(successWidget);
            successImage->setObjectName(qsl("successImage"));
            successImage->setFlat(true);
            successImage->setFixedSize(Utils::scale_value(120), Utils::scale_value(136));
            Testing::setAccessibleName(successImage, qsl("AS contanctus successImage"));
            successImageLayout->addWidget(successImage);
        }
        Testing::setAccessibleName(successWidget, qsl("AS contanctus successWidget"));
        successPageLayout->addWidget(successWidget);

        auto thanksWidget = new QWidget(successPage_);
        auto thanksLayout = Utils::emptyVLayout(thanksWidget);
        Utils::grabTouchWidget(thanksWidget);
        thanksWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        thanksLayout->setAlignment(Qt::AlignCenter);
        thanksLayout->addSpacing(Utils::scale_value(16));
        {
            auto thanksLabel = new QLabel(thanksWidget);
            thanksLabel->setStyleSheet(qsl("color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));
            thanksLabel->setFont(labelFont());
            thanksLabel->setWordWrap(true);
            thanksLabel->setText(QT_TRANSLATE_NOOP("contactus_page", "Thank you for contacting us! We will reply as soon as possible."));
            thanksLabel->setAlignment(Qt::AlignCenter);
            thanksLabel->setSizePolicy(QSizePolicy::Expanding, thanksLabel->sizePolicy().verticalPolicy());
            Testing::setAccessibleName(thanksLabel, qsl("AS contanctus thanksLabel"));
            thanksLayout->addWidget(thanksLabel);
        }
        Testing::setAccessibleName(thanksWidget, qsl("AS contanctus thanksWidget"));
        successPageLayout->addWidget(thanksWidget);

        auto resendWidget = new QWidget(successPage_);
        auto resendLayout = Utils::emptyVLayout(resendWidget);
        Utils::grabTouchWidget(resendWidget);
        resendWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        resendLayout->setAlignment(Qt::AlignCenter);
        {
            auto resendLink = new TextEmojiWidget(resendWidget, captionFont(),
                                                  Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), Utils::scale_value(32));
            resendLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Send another review"));
            resendLink->setSizePolicy(QSizePolicy::Preferred, resendLink->sizePolicy().verticalPolicy());
            resendLink->setCursor(Qt::PointingHandCursor);

            connect(resendLink, &TextEmojiWidget::clicked, this, [this]()
            {
                setState(State::Feedback);
            });
            Testing::setAccessibleName(resendLink, qsl("AS contanctus resendLink"));
            resendLayout->addWidget(resendLink);
        }
        Testing::setAccessibleName(resendWidget, qsl("AS contanctus resendWidget"));
        successPageLayout->addWidget(resendWidget);
    }
}

void ContactUsWidget::initFeedback()
{
    auto sendingPageLayout = Utils::emptyVLayout(sendingPage_);
    sendingPageLayout->setSpacing(Utils::scale_value(16));
    sendingPageLayout->setContentsMargins(0, 0, 0, 0);
    sendingPageLayout->setAlignment(Qt::AlignTop);
    static auto& problems = getProblemsList();

    if (hasProblemDropper_)
    {
        dropper_ = GeneralCreator::addComboBox
        (
            sendingPage_
            , sendingPageLayout
            , QT_TRANSLATE_NOOP("settings", "Problem")
            , true
            , problems
            , 0
            , -1
            , [=](QString _s, int _idx, TextEmojiWidget*)
        {
            selectedProblem_ = problems[_idx];
        }
        , [](bool) -> QString { return QString(); }
        );
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, dropper_, &QComboBox::hidePopup);
    }

    suggestioner_ = new TextEditEx(sendingPage_, labelFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, false);

    Testing::setAccessibleName(suggestioner_, qsl("feedback_suggestioner_"));

    Utils::grabTouchWidget(suggestioner_);
    suggestioner_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    suggestioner_->setMinimumHeight(Utils::scale_value(TEXTEDIT_MIN_HEIGHT));
    suggestioner_->setMaximumHeight(Utils::scale_value(TEXTEDIT_MAX_HEIGHT));
    suggestioner_->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page","Your comments or suggestions..."));
    suggestioner_->setAutoFillBackground(false);
    suggestioner_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    suggestioner_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    suggestioner_->setProperty("normal", true);
    sendingPageLayout->addWidget(suggestioner_);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsContactUsShown, suggestioner_, Utils::QOverload<>::of(&TextEditEx::setFocus));

    connect(suggestioner_->document(), &QTextDocument::contentsChanged, this, &ContactUsWidget::updateSuggesionHeight);

    Testing::setAccessibleName(suggestioner_, qsl("AS contanctus suggestioner_"));

    suggestionSizeError_ = new TextEmojiWidget(suggestioner_, captionFont(),
                                                        Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), Utils::scale_value(12));
    Utils::grabTouchWidget(suggestionSizeError_);
    suggestionSizeError_->setVisible(false);
    Testing::setAccessibleName(suggestionSizeError_, qsl("AS contanctus suggestionSizeError"));
    sendingPageLayout->addWidget(suggestionSizeError_);

    attachWidget_ = new AttachFileWidget(sendingPage_);
    Testing::setAccessibleName(attachWidget_, qsl("feedback_filer"));
    Utils::grabTouchWidget(attachWidget_);
    attachWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sendingPageLayout->addWidget(attachWidget_);

    email_ = new LineEditEx(sendingPage_);
    email_->setFont(labelFont());
    email_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

    email_->setContentsMargins(0, 0, 0, 0);
    email_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    email_->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page", "Your Email"));
    email_->setAutoFillBackground(false);

    QString communication_email = get_gui_settings()->get_value(settings_feedback_email, QString());
    QString aimid = MyInfo()->aimId();

    if (communication_email.isEmpty() && (build::is_agent() || build::is_dit()) && aimid.contains(ql1c('@')))
        communication_email = aimid;
    email_->setText(communication_email);
    email_->setProperty("normal", true);
    Testing::setAccessibleName(email_, qsl("AS contanctus email"));
    sendingPageLayout->addWidget(email_);

    emailError_ = new TextEmojiWidget(sendingPage_, captionFont(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), Utils::scale_value(12));
    Utils::grabTouchWidget(emailError_);
    emailError_->setText(QT_TRANSLATE_NOOP("contactus_page","Please enter a valid email address"));
    emailError_->setVisible(false);
    Testing::setAccessibleName(emailError_, qsl("AS contanctus emailError"));
    sendingPageLayout->addWidget(emailError_);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, this,
            [this](int type)
    {
        if ((Utils::CommonSettingsType)type == Utils::CommonSettingsType::CommonSettingsType_ContactUs && !Utils::isValidEmailAddress(email_->text()))
            email_->setText(QString());
        if (!email_->property("normal").toBool())
            Utils::ApplyPropertyParameter(email_, "normal", true);
        emailError_->setVisible(false);
    });

    auto sendWidget = new QWidget(sendingPage_);
    auto sendLayout = Utils::emptyHLayout(sendWidget);
    Utils::grabTouchWidget(sendWidget);
    sendWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sendLayout->setContentsMargins(0, 0, 0, 0);
    sendLayout->setSpacing(Utils::scale_value(12));
    sendLayout->setAlignment(Qt::AlignLeft);

    sendButton_ = new DialogButton(sendWidget, QT_TRANSLATE_NOOP("contactus_page", "SEND"), (suggestioner_->getPlainText().length() && email_->text().length()) ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
    sendButton_->setCursor(Qt::PointingHandCursor);
    sendButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sendButton_->setMinimumWidth(Utils::scale_value(100));
    sendButton_->setFixedHeight(Utils::scale_value(40));

    Testing::setAccessibleName(sendButton_, qsl("AS contanctus sendButton"));
    sendLayout->addWidget(sendButton_);

    connect(suggestioner_->document(), &QTextDocument::contentsChanged, this, [this]()
    {
        bool state = suggestioner_->getPlainText().length();
        if (state && suggestioner_->property("normal").toBool() != state)
        {
            Utils::ApplyPropertyParameter(suggestioner_, "normal", state);
            suggestionSizeError_->setVisible(false);
        }

        updateSendButton(state && email_->text().length());
    });

    connect(email_, &QLineEdit::textChanged, this, [this](const QString &)
    {
        bool state = email_->text().length();

        if (state && email_->property("normal").toBool() != state)
        {
            Utils::ApplyPropertyParameter(email_, "normal", state);
            emailError_->setVisible(false);
        }
        updateSendButton(suggestioner_->getPlainText().length() && state);
    });

    errorOccuredSign_ = new TextEmojiWidget(sendWidget, captionFont(),
                                                Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), Utils::scale_value(16));
    Utils::grabTouchWidget(errorOccuredSign_);
    errorOccuredSign_->setText(QT_TRANSLATE_NOOP("contactus_page", "Error occurred, try again later"));
    errorOccuredSign_->setVisible(false);
    Testing::setAccessibleName(errorOccuredSign_, qsl("AS contanctus errorOccuredSign"));
    sendLayout->addWidget(errorOccuredSign_);

    sendSpinner_ = new RotatingSpinner(sendWidget);
    sendSpinner_->hide();
    Testing::setAccessibleName(sendSpinner_, qsl("AS contanctus sendSpinner"));
    sendLayout->addWidget(sendSpinner_);

    auto sendingSign = new TextEmojiWidget(sendWidget, labelFont(),
                                            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), Utils::scale_value(16));
    Utils::grabTouchWidget(sendingSign);
    sendingSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Sending..."));
    sendingSign->setVisible(false);
    Testing::setAccessibleName(sendingSign, qsl("AS contanctus sendingSign"));
    sendLayout->addWidget(sendingSign);

    sendingPageLayout->addWidget(sendWidget);
}

void ContactUsWidget::sendFeedback()
{
    get_gui_settings()->set_value(settings_feedback_email, email_->text());

    const auto sb = suggestioner_->property("normal").toBool();
    const auto eb = email_->property("normal").toBool();
    if (!sb)
        suggestioner_->setProperty("normal", true);
    if (!eb)
        email_->setProperty("normal", true);
    suggestionSizeError_->setVisible(false);
    emailError_->setVisible(false);
    attachWidget_->hideError();
    auto sg = suggestioner_->getPlainText();
    sg.remove(ql1c(' '));

    if (sg.isEmpty() || sg.length() > 2048)
    {
        suggestionSizeError_->setText(suggestionSizeErrorText(!sg.isEmpty()));
        suggestioner_->setProperty("normal", false);
        suggestionSizeError_->setVisible(true);
    }
    else if (!Utils::isValidEmailAddress(email_->text()))
    {
        email_->setProperty("normal", false);
        emailError_->setVisible(true);
    }
    else
    {
        sendButton_->setVisible(false);
        errorOccuredSign_->setVisible(false);
        sendSpinner_->setVisible(true);
        sendSpinner_->startAnimation();

        suggestioner_->setEnabled(false);
        attachWidget_->setEnabled(false);
        email_->setEnabled(false);

        const auto myInfo = MyInfo();

        Ui::gui_coll_helper col(GetDispatcher()->create_collection(), true);

        col.set_value_as_string("url", get_feedback_url());

        // fb.screen_resolution
        col.set_value_as_qstring("screen_resolution", (qsl("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())));
        // fb.referrer
        col.set_value_as_qstring("referrer", build::ProductNameShort());
        {
            const auto appName = build::ProductName();
            const QString icqVer = appName + QString::fromUtf8(VERSION_INFO_STR);
            auto osv = QSysInfo::prettyProductName();
            if (!osv.length() || osv == ql1s("unknown"))
                osv = qsl("%1 %2 (%3 %4)").arg(QSysInfo::productType(), QSysInfo::productVersion(), QSysInfo::kernelType(), QSysInfo::kernelVersion());

            const auto concat = build::GetProductVariant(qsl("%1 %2 icq:%3"), qsl("%1 %2 agent:%3"), qsl("%1 %2 biz:%3"), qsl("%1 %2 dit:%3")).arg(osv, icqVer, myInfo->aimId());
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

        // fb.message
        col.set_value_as_qstring("message", suggestioner_->getPlainText());

        // communication_email
        col.set_value_as_qstring("communication_email", email_->text());
        // Lang
        col.set_value_as_qstring("language", QLocale::system().name());

        if (hasProblemDropper_)
        {
            const auto problem = build::is_icq() ? "fb.question.53496" : "fb.question.53490";
            col.set_value_as_qstring(problem, selectedProblem_);
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
    if (sb != suggestioner_->property("normal").toBool())
        Utils::ApplyStyle(suggestioner_, suggestioner_->styleSheet());
    if (eb != email_->property("normal").toBool())
        Utils::ApplyStyle(email_, email_->styleSheet());
}

void ContactUsWidget::updateSuggesionHeight()
{
    if (!suggestioner_->property("normal").toBool())
        Utils::ApplyPropertyParameter(suggestioner_, "normal", true);

    auto textControlHeight = suggestioner_->document()->size().height() + Utils::scale_value(12 + 10 + 2); // paddings + border_width*2
    if (textControlHeight > suggestioner_->maximumHeight())
        suggestioner_->setMinimumHeight(suggestioner_->maximumHeight());
    else if (textControlHeight > Utils::scale_value(TEXTEDIT_MIN_HEIGHT))
        suggestioner_->setMinimumHeight(textControlHeight);
    else
        suggestioner_->setMinimumHeight(Utils::scale_value(TEXTEDIT_MIN_HEIGHT));
}

void ContactUsWidget::updateSendButton(bool _state)
{
    sendButton_->changeRole(_state ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
}

AttachFileWidget::AttachFileWidget(QWidget *_parent)
    : QWidget(_parent)
    , totalFileSize_(0)
{
    auto layout = Utils::emptyVLayout(this);
    attachWidget_ = new QWidget(this);
    attachWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    attachLayout_ = Utils::emptyVLayout(attachWidget_);
    attachLayout_->setContentsMargins(0, 0, 0, 0);

    attachSizeError_ = new TextEmojiWidget(attachWidget_, captionFont(),
            Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), Utils::scale_value(12));
    Testing::setAccessibleName(attachSizeError_, qsl("AS contanctus attachSizeError"));
    attachLayout_->addWidget(attachSizeError_);
    attachSizeError_->setVisible(false);

    auto attachLinkWidget = new QWidget(attachWidget_);
    auto attachLinkLayout = Utils::emptyHLayout(attachLinkWidget);
    Utils::grabTouchWidget(attachLinkWidget);
    attachLinkWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    attachLinkWidget->setCursor(Qt::PointingHandCursor);
    attachLinkLayout->setContentsMargins(0, 0, 0, 0);
    attachLinkLayout->setSpacing(Utils::scale_value(12));

    auto attachImage = new CustomButton(attachLinkWidget);
    attachImage->setDefaultImage(qsl(":/background_icon"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), QSize(24, 24));
    attachImage->setFixedSize(Utils::scale_value(24), Utils::scale_value(24));
    connect(attachImage, &QPushButton::clicked, this, &AttachFileWidget::attachFile);
    Testing::setAccessibleName(attachImage, qsl("AS contanctus attachImage"));
    attachLinkLayout->addWidget(attachImage);

    auto attachLink = new TextEmojiWidget(attachLinkWidget, labelFont(),
                                          Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), Utils::scale_value(16));
    Utils::grabTouchWidget(attachLink);
    attachLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Attach screenshot"));
    connect(attachLink, &TextEmojiWidget::clicked, this, &AttachFileWidget::attachFile);
    Testing::setAccessibleName(attachLink, qsl("AS contanctus attachLink"));
    attachLinkLayout->addWidget(attachLink);

    Testing::setAccessibleName(attachLinkWidget, qsl("AS contanctus attachLinkWidget"));
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
        fileLayout->setContentsMargins(Utils::scale_value(12), 0, 0, 0);
        fileLayout->setAlignment(Qt::AlignVCenter);

        if (fileSize > MAX_FILE_SIZE)
        {
            attachSizeError_->setText(errorText(fileSize));
            attachSizeError_->updateGeometry();
            attachSizeError_->setVisible(true);
            return;
        }
        if (totalFileSize_ + fileSize > MAX_TOTAL_FILE_SIZE)
        {
            attachSizeError_->setText(errorText(fileSize));
            attachSizeError_->updateGeometry();
            attachSizeError_->setVisible(true);
            return;
        }
        totalFileSize_ += fileSize;

        double fs = fileSize / 1024.f;
        QString sizeString;
        if (fs < 100)
        {
            sizeString = qsl("%1 %2").arg(QString::number(fs, 'f', 2), QT_TRANSLATE_NOOP("contactus_page", "KB"));
        }
        else
        {
            fs /= 1024.;
            sizeString = qsl("%1 %2").arg(QString::number(fs, 'f', 2), QT_TRANSLATE_NOOP("contactus_page", "MB"));
        }

        auto fileInfoWidget = new QWidget(fileWidget);
        auto fileInfoLayout = Utils::emptyHLayout(fileInfoWidget);
        Utils::grabTouchWidget(fileInfoWidget);
        fileInfoWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        fileInfoLayout->setAlignment(Qt::AlignLeft);

        const auto metrics = QFontMetrics(labelFont());
        const auto maxFilenameLength = parentWidget()->width() - Utils::scale_value(144);
        if (metrics.width(fileName) > maxFilenameLength)
            fileName = metrics.elidedText(fileName, Qt::ElideMiddle, maxFilenameLength);

        auto fileNameLabel = new QLabel(fileInfoWidget);
        Utils::grabTouchWidget(fileNameLabel);
        fileNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        fileNameLabel->setText(fileName);
        fileNameLabel->setFont(labelFont());
        QPalette pal = fileNameLabel->palette();
        pal.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        fileNameLabel->setPalette(pal);

        Testing::setAccessibleName(fileNameLabel, qsl("AS contanctus fileName"));
        fileInfoLayout->addWidget(fileNameLabel);

        auto fileSizeLabel = new LabelEx(fileInfoWidget);
        Utils::grabTouchWidget(fileSizeLabel);
        fileSizeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        fileSizeLabel->setText(qsl(" - %1").arg(sizeString));
        fileSizeLabel->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        fileSizeLabel->setFont(labelFont());
        Testing::setAccessibleName(fileSizeLabel, qsl("AS contanctus fileSize"));
        fileInfoLayout->addWidget(fileSizeLabel);

        Testing::setAccessibleName(fileInfoWidget, qsl("AS contanctus fileInfoWidget"));
        fileLayout->addWidget(fileInfoWidget);

        auto deleteFile = new CustomButton(fileWidget, qsl(":/controls/close_icon"), QSize(12, 12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        deleteFile->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
        deleteFile->setFixedSize(Utils::scale_value(QSize(20, 20)));

        connect(deleteFile, &CustomButton::clicked, this, [this, fileWidget, fileSize]()
        {
            filesToSend_.erase(fileWidget->objectName());
            totalFileSize_ -= fileSize;
            attachSizeError_->setVisible(false);
            fileWidget->setVisible(false);
            delete fileWidget;
        });

        Testing::setAccessibleName(deleteFile, qsl("AS contanctus deleteFile"));
        fileLayout->addWidget(deleteFile);
        fileLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(8), 0, QSizePolicy::Fixed));

        attachLayout_->insertWidget(0, fileWidget); // always insert at the top
    }
}

void AttachFileWidget::clearFileList()
{
    for (const auto& p : std::exchange(filesToSend_, {}))
    {
        auto f = attachWidget_->findChild<QWidget *>(p.first);
        if (f)
        {
            f->setVisible(false);
            delete f;
        }
    }

    totalFileSize_ = 0;
}

void AttachFileWidget::hideError() const
{
    attachSizeError_->setVisible(true);
}


#include "stdafx.h"
#include "TermsPrivacyWidget.h"

#include "controls/TextBrowserEx.h"
#include "controls/PictureWidget.h"
#include "controls/TransparentScrollBar.h"

#include "controls/DialogButton.h"
#include "utils/utils.h"
#include "utils/Text.h"
#include "utils/gui_coll_helper.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "core_dispatcher.h"
#include "AcceptAgreementInfo.h"
#include "../gui_settings.h"
#include "../../common.shared/config/config.h"
#if defined(__APPLE__)
#include "utils/macos/mac_support.h"
#endif

#include "../styles/ThemesContainer.h"

UI_NS_BEGIN

namespace
{
    constexpr int DIALOG_WIDTH = 340;

    constexpr int ICON_WIDTH = 380;
    constexpr int ICON_HEIGHT = 280;

    QString getGDPRImagePath()
    {
        if (get_gui_settings()->get_value(settings_language, QString()) == u"ru")
            return qsl(":/gdpr/gdpr_100");

        return qsl(":/gdpr/gdpr_100_en");
    }
}

const QString& legalTermsUrl()
{
    static const QString termsUrl = []() -> QString
    {
        const auto url = config::get().url(config::urls::legal_terms);
        return u"https://" % QString::fromUtf8(url.data(), url.size());
    }();

    return termsUrl;
}

const QString& privacyPolicyUrl()
{
    static const QString ppUrl = []() -> QString
    {
        const auto url = config::get().url(config::urls::legal_privacy_policy);
        return u"https://" % QString::fromUtf8(url.data(), url.size());
    }();

    return ppUrl;
}

TermsPrivacyWidget::TermsPrivacyWidget(const QString& _titleHtml,
    const QString& _descriptionHtml,
    const Options &_options,
    QWidget *_parent)
    : QWidget(_parent),
    layout_(new QVBoxLayout(this)),
    options_(_options)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget* mainWidget = new QWidget();
    mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* verticalLayout = Utils::emptyVLayout(mainWidget);
    verticalLayout->setAlignment(Qt::AlignCenter);

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("background: transparent; border: none;"));
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    Utils::grabTouchWidget(scrollArea->viewport(), true);
    Testing::setAccessibleName(scrollArea, qsl("AS GDPR scrollArea"));
    layout_->addWidget(scrollArea);
    scrollArea->setWidget(mainWidget);

    QWidget *iconWidget = new QWidget(mainWidget);
    iconWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    Utils::ApplyStyle(iconWidget, qsl("background-color: %1;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE)));

    auto iconLayout = Utils::emptyVLayout(iconWidget);

    icon_ = new PictureWidget(iconWidget, getGDPRImagePath());
    icon_->setFixedSize(Utils::scale_value(QSize(ICON_WIDTH, ICON_HEIGHT)));
    iconLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    iconLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    Testing::setAccessibleName(icon_, qsl("AS GDPR icon"));
    iconLayout->addWidget(icon_);

    TextBrowserEx::Options titleOptions;
    titleOptions.font_ = Fonts::appFontScaled(23, Fonts::FontWeight::Medium);
    titleOptions.useDocumentMargin_ = true;

    title_ = new TextBrowserEx(titleOptions, mainWidget);
    title_->setHtml(_titleHtml);
    title_->setFixedWidth(Utils::scale_value(ICON_WIDTH));
    title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    title_->setPlaceholderText(QString());
    title_->setAutoFillBackground(false);
    title_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setAlignment(Qt::AlignCenter);
    title_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    title_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    TextBrowserExUtils::setAppropriateHeight(*title_);

    TextBrowserEx::Options descriptionOptions;
    descriptionOptions.font_ = Fonts::appFontScaled(15);
    descriptionOptions.noTextDecoration_ = !Styling::getThemesContainer().getCurrentTheme()->isLinksUnderlined();
    descriptionOptions.useDocumentMargin_ = true;
    descriptionOptions.openExternalLinks_ = false;

    auto descriptionWidget = new QWidget();
    descriptionWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto descLayout = Utils::emptyHLayout(descriptionWidget);
    descLayout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);

    description_ = new TextBrowserEx(descriptionOptions, descriptionWidget);
    description_->setHtml(_descriptionHtml);
    description_->setMinimumWidth(Utils::scale_value(DIALOG_WIDTH));
    description_->setMaximumWidth(Utils::scale_value(460));
    description_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    description_->setPlaceholderText(QString());
    description_->setAutoFillBackground(false);
    description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    description_->setAlignment(Qt::AlignCenter);
    description_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    description_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    TextBrowserExUtils::setAppropriateHeight(*description_);
    Testing::setAccessibleName(description_, qsl("AS GDPR description"));
    descLayout->addWidget(description_);
    descLayout->setAlignment(description_, Qt::AlignCenter);

    connect(description_, &QTextBrowser::anchorClicked, this, &TermsPrivacyWidget::onAnchorClicked);

    const auto text_ = QT_TRANSLATE_NOOP("terms_privacy_widget", "Accept and agree");
    Testing::setAccessibleName(mainWidget, qsl("AS GDPR"));
    agreeButton_ = new DialogButton(mainWidget, text_, DialogButtonRole::CONFIRM);
    agreeButton_->setFixedHeight(Utils::scale_value(40));
    agreeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    agreeButton_->setContentsMargins(Utils::scale_value(32), Utils::scale_value(8), Utils::scale_value(32), Utils::scale_value(12));
    agreeButton_->setCursor(QCursor(Qt::PointingHandCursor));
    agreeButton_->setFont(Fonts::appFontScaled(18, Fonts::FontWeight::Medium));
    agreeButton_->setEnabled(true);
    agreeButton_->updateWidth();

    layout_->setContentsMargins(QMargins());

    auto bottomWidget = new QWidget(mainWidget);
    auto bottomLayout = Utils::emptyVLayout(bottomWidget);
    bottomWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    Testing::setAccessibleName(iconWidget, qsl("AS GDPR iconWidget"));
    verticalLayout->addWidget(iconWidget);
    bottomLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Fixed, QSizePolicy::Fixed));
    Testing::setAccessibleName(title_, qsl("AS GDPR title"));
    bottomLayout->addWidget(title_);
    bottomLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Fixed, QSizePolicy::Fixed));
    Testing::setAccessibleName(descriptionWidget, qsl("AS GDPR descriptionWidget"));
    bottomLayout->addWidget(descriptionWidget);
    bottomLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Fixed, QSizePolicy::Fixed));
    Testing::setAccessibleName(agreeButton_, qsl("AS GDPR agreeButton"));
    bottomLayout->addWidget(agreeButton_);
    bottomLayout->setAlignment(title_, Qt::AlignHCenter | Qt::AlignTop);
    bottomLayout->setAlignment(descriptionWidget, Qt::AlignHCenter | Qt::AlignTop);
    bottomLayout->setAlignment(agreeButton_, Qt::AlignHCenter | Qt::AlignTop);
    bottomLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    Testing::setAccessibleName(bottomWidget, qsl("AS GDPR bottomWidget"));
    verticalLayout->addWidget(bottomWidget);

    icon_->show();
    title_->show();
    description_->show();
    agreeButton_->show();

    connect(agreeButton_, &QPushButton::clicked, this, &TermsPrivacyWidget::onAgreeClicked);

    if (options_.isGdprUpdate_)
    {
        connect(this, &TermsPrivacyWidget::agreementAccepted,
            this, &TermsPrivacyWidget::reportAgreementAccepted);
    }

#if defined(__APPLE__)
    macMenuBlocker_ = std::make_unique<MacMenuBlocker>();
#endif
    setFocus();
}

TermsPrivacyWidget::~TermsPrivacyWidget() = default;

void TermsPrivacyWidget::onAgreeClicked()
{
    AcceptParams params;
    params.accepted_ = true;

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_start);

    Q_EMIT agreementAccepted(params);

#if defined(__APPLE__)
    macMenuBlocker_->unblock();
#endif
}

void TermsPrivacyWidget::reportAgreementAccepted(const TermsPrivacyWidget::AcceptParams &_acceptParams)
{
    // report to core
    gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_int("accept_flag", static_cast<int32_t>(_acceptParams.accepted_ ? AcceptAgreementInfo::AgreementAction::Accept
        : AcceptAgreementInfo::AgreementAction::Ignore));
    collection.set_value_as_bool("reset", false);

    GetDispatcher()->post_message_to_core("agreement/gdpr", collection.get());
    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_update);
}

void TermsPrivacyWidget::keyPressEvent(QKeyEvent *_event)
{
    if (_event->key() == Qt::Key_Escape || _event->key() == Qt::Key_Down || _event->key() == Qt::Key_Up)
    {
        setFocus();
        return;
    }

    return QWidget::keyPressEvent(_event);
}

void TermsPrivacyWidget::showEvent(QShowEvent *_event)
{
#if defined(__APPLE__)
    macMenuBlocker_->block();
#endif
    QWidget::showEvent(_event);
    setFocus();
}

void TermsPrivacyWidget::onAnchorClicked(const QUrl& _link) const
{
    if (!_link.isValid())
        return;

    if (const auto linkStr = _link.toString(); !linkStr.isEmpty())
    {
        if (linkStr == legalTermsUrl())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_click_eula);
        else if (linkStr == privacyPolicyUrl())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_click_privacypolicy);

        Utils::openUrl(linkStr);
    }
}

UI_NS_END

#include "stdafx.h"
#include "TermsPrivacyWidgetBase.h"
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
#include "../gui_settings.h"
#include "../../common.shared/config/config.h"
#if defined(__APPLE__)
#include "utils/macos/mac_support.h"
#endif

#include "../styles/ThemesContainer.h"

UI_NS_BEGIN

namespace
{
    constexpr int ICON_WIDTH = 380;
    constexpr int ICON_HEIGHT = 280;
    constexpr int DIALOG_MIN_WIDTH = 340;
    constexpr int DIALOG_MAX_WIDTH = 460;
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
    : TermsPrivacyWidgetBase(QT_TRANSLATE_NOOP("terms_privacy_widget", "Accept and agree"), true, _parent)
    , options_(_options)
    , titleHtml_(_titleHtml)
    , descriptionHtml_(_descriptionHtml)
{
    setTestWidgetName(qsl("GDPR"));
    if (options_.isGdprUpdate_)
        connect(this, &TermsPrivacyWidget::agreementAccepted, this, &TermsPrivacyWidget::reportAgreementAccepted);
}

void TermsPrivacyWidget::initContent()
{
    TermsPrivacyWidgetBase::initContent();
    if (!mainWidget_)
        return;

    TextBrowserEx::Options titleOptions;
    titleOptions.font_ = Fonts::appFontScaled(23, Fonts::FontWeight::Medium);
    titleOptions.useDocumentMargin_ = true;

    addLinkWithOptions(titleOptions, title_, titleHtml_, DIALOG_MIN_WIDTH, DIALOG_MAX_WIDTH, mainWidget_, qsl("title"), false);
    addDescriptionWidget(descriptionHtml_);
}

void TermsPrivacyWidget::showAll()
{
    TermsPrivacyWidgetBase::showAll();
    if (title_)
        title_->show();
    if (description_)
        description_->show();
}


TermsPrivacyWidget::~TermsPrivacyWidget() = default;

void TermsPrivacyWidget::onAgreeClicked()
{
    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_start);

    Q_EMIT agreementAccepted();

    TermsPrivacyWidgetBase::onAgreeClicked();
}

void TermsPrivacyWidget::reportAgreementAccepted()
{
    // report to core
    gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    GetDispatcher()->post_message_to_core("agreement/gdpr", collection.get());
    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_update);
}


void TermsPrivacyWidget::reportLink(const QString& _link) const
{
    if (_link == legalTermsUrl())
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_click_eula);
    else if (_link == privacyPolicyUrl())
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_click_privacypolicy);
}

QString TermsPrivacyWidget::getImagePath() const
{
    if (get_gui_settings()->get_value(settings_language, QString()) == u"ru")
        return qsl(":/gdpr/gdpr_100");

    return qsl(":/gdpr/gdpr_100_en");
}

QSize TermsPrivacyWidget::getImageSize() const
{
    return QSize(ICON_WIDTH, ICON_HEIGHT);
}

void TermsPrivacyWidget::addContentToLayout()
{
    TermsPrivacyWidgetBase::addContentToLayout();
    if (!bottomLayout_ || !title_ || !descriptionWidget_)
        return;

    addVSpacer(20);
    Testing::setAccessibleName(title_, qsl("AS %1 title").arg(testWidgetName_));
    bottomLayout_->addWidget(title_);
    bottomLayout_->setAlignment(title_, Qt::AlignHCenter | Qt::AlignTop);
    addVSpacer(16);

    bottomLayout_->addWidget(descriptionWidget_);
    bottomLayout_->setAlignment(descriptionWidget_, Qt::AlignHCenter | Qt::AlignTop);
    addVSpacer(20);
}

void TermsPrivacyWidget::addDescriptionWidget(const QString& _descriptionHtml)
{
    if (!descriptionWidget_)
        descriptionWidget_ = new QWidget();
    if (descriptionWidget_)
    {
        descriptionWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        if (!descLayout_)
            descLayout_ = Utils::emptyHLayout(descriptionWidget_);
        if (descLayout_)
            descLayout_->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);

        addLink(description_, _descriptionHtml, DIALOG_MIN_WIDTH, DIALOG_MAX_WIDTH, descriptionWidget_, qsl("description"));
        if (description_ && descLayout_)
        {
            descLayout_->addWidget(description_);
            descLayout_->setAlignment(description_, Qt::AlignCenter);
        }
    }
}

UI_NS_END

#include "TermsPrivacyWidget.h"

#include "controls/TextBrowserEx.h"
#include "controls/PictureWidget.h"

#include "controls/DialogButton.h"
#include "utils/utils.h"
#include "utils/Text.h"
#include "utils/gui_coll_helper.h"
#include "fonts.h"
#include "core_dispatcher.h"
#include "AcceptAgreementInfo.h"
#if defined(__APPLE__)
#include "utils/macos/mac_support.h"
#endif

#include "../styles/ThemesContainer.h"

UI_NS_BEGIN

namespace
{
    constexpr int DIALOG_WIDTH = 380;
//    constexpr int DIALOG_HEIGHT = 390;

//    constexpr int ICON_WIDTH = 78;
//    constexpr int ICON_HEIGHT = 80;
}

TermsPrivacyWidget::TermsPrivacyWidget(const QString& _titleHtml,
                                       const QString& _descriptionHtml,
                                       const Options &_options,
                                       QWidget *_parent)
    : QWidget(_parent),
      layout_(new QVBoxLayout(this)),
      options_(_options)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
    //setFixedSize(Utils::scale_value(DIALOG_WIDTH), Utils::scale_value(DIALOG_HEIGHT));

    //iconPixmap_ = Utils::renderSvg(build::is_icq() ? qsl(":/logo/logo_icq")
    //                                               : qsl(":/logo/logo_agent"),
    //                               QSize(Utils::scale_value(ICON_WIDTH), Utils::scale_value(ICON_HEIGHT)));

    //icon_ = new PictureWidget(this);
    //icon_->setImage(iconPixmap_, -1 /* no radius */);
    //icon_->setIconAlignment(Qt::AlignLeft | Qt::AlignTop);
    //icon_->setFixedSize(Utils::scale_value(ICON_WIDTH), Utils::scale_value(ICON_HEIGHT));
    //icon_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    //icon_->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));

    TextBrowserEx::Options titleOptions;
    titleOptions.font_ = Fonts::appFontScaled(23);
    titleOptions.useDocumentMargin_ = true;

    title_ = new TextBrowserEx(titleOptions, this);
    title_->setHtml(_titleHtml);
    title_->setFixedWidth(Utils::scale_value(308));
    title_->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
    title_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    title_->setPlaceholderText(QString());
    title_->setAutoFillBackground(false);
    title_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    title_->setAlignment(Qt::AlignCenter);
    title_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    TextBrowserExUtils::setAppropriateHeight(*title_);

    TextBrowserEx::Options descriptionOptions;
    descriptionOptions.font_ = Fonts::appFontScaled(15);
    descriptionOptions.noTextDecoration_ = Styling::getThemesContainer().getCurrentTheme()->linksUnderlined() == Ui::TextRendering::LinksStyle::PLAIN;
    descriptionOptions.useDocumentMargin_ = true;

    description_ = new TextBrowserEx(descriptionOptions, this);
    description_->setHtml(_descriptionHtml);
    description_->setFixedWidth(Utils::scale_value(308));
    description_->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
    description_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    description_->setPlaceholderText(QString());
    description_->setAutoFillBackground(false);
    description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    description_->setAlignment(Qt::AlignCenter);
    description_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    TextBrowserExUtils::setAppropriateHeight(*description_);

    agreeButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("terms_privacy_widget", "I AGREE"), DialogButtonRole::CONFIRM);
    agreeButton_->setFixedHeight(Utils::scale_value(32));
    agreeButton_->setMinimumWidth(Utils::scale_value(100));
    agreeButton_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    agreeButton_->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
    agreeButton_->setCursor(QCursor(Qt::PointingHandCursor));
    agreeButton_->setEnabled(true);

    layout_->setSpacing(0);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(44), QSizePolicy::Fixed, QSizePolicy::Fixed));
    //Testing::setAccessibleName(icon_, qsl("AS tpw icon_"));
    //layout_->addWidget(icon_);
    //layout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(28), QSizePolicy::Fixed, QSizePolicy::Fixed));
    Testing::setAccessibleName(title_, qsl("AS tpw title_"));
    layout_->addWidget(title_);
    layout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(28), QSizePolicy::Fixed, QSizePolicy::Fixed));

    auto bottomLayout = Utils::emptyVLayout();
    bottomLayout->setContentsMargins(0, -1, 0, Utils::scale_value(16));
    bottomLayout->setAlignment(Qt::AlignBottom);

    Testing::setAccessibleName(description_, qsl("AS tpw description_"));
    bottomLayout->addWidget(description_);
    bottomLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(32), QSizePolicy::Fixed, QSizePolicy::Fixed));
    Testing::setAccessibleName(agreeButton_, qsl("AS tpw agreeButton_"));
    bottomLayout->addWidget(agreeButton_);
    bottomLayout->setAlignment(description_, Qt::AlignCenter);
    bottomLayout->setAlignment(agreeButton_, Qt::AlignCenter);

    layout_->addLayout(bottomLayout);

    //layout_->setAlignment(icon_, Qt::AlignCenter);
    layout_->setAlignment(title_, Qt::AlignCenter);
    layout_->setAlignment(description_, Qt::AlignCenter);

    //icon_->show();
    title_->show();
    description_->show();
    agreeButton_->show();

    connect(agreeButton_, &QPushButton::clicked, this, &TermsPrivacyWidget::onAgreeClicked);

    if (options_.blockUntilAccepted_)
    {
        connect(this, &TermsPrivacyWidget::agreementAccepted,
                this, &TermsPrivacyWidget::reportAgreementAccepted);
    }

    connect(qApp, &QApplication::focusChanged, this, &TermsPrivacyWidget::onFocusChanged);

#if defined(__APPLE__)
    macMenuBlocker_ = std::make_unique<MacMenuBlocker>();
#endif
    setFocus();
}

TermsPrivacyWidget::~TermsPrivacyWidget()
{
}

void TermsPrivacyWidget::setContainingDialog(QDialog *_containingDialog)
{
    options_.containingDialog_ = _containingDialog;
}

void TermsPrivacyWidget::onAgreeClicked()
{
    AcceptParams params;
    params.accepted_ = true;

    emit agreementAccepted(params);
}

void TermsPrivacyWidget::reportAgreementAccepted(const TermsPrivacyWidget::AcceptParams &_acceptParams)
{
    // report to core
    gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_int("accept_flag", static_cast<int32_t>(_acceptParams.accepted_ ? AcceptAgreementInfo::AgreementAction::Accept
                                                                                            : AcceptAgreementInfo::AgreementAction::Ignore));
    collection.set_value_as_bool("reset", false);

    GetDispatcher()->post_message_to_core("agreement/gdpr", collection.get());

    if (options_.controlContainingDialog_ && options_.containingDialog_)
    {
        // containing dialog is controlled for current users (gdpr "update")
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_update);
        options_.containingDialog_->close();
    }
}

void TermsPrivacyWidget::onFocusChanged(QWidget *_from, QWidget *_to)
{
    Q_UNUSED(_from);
    if (!_to || isAncestorOf(_to))
        return;

    _to->clearFocus();
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
    QWidget::showEvent(_event);
    setFocus();
}

UI_NS_END

#include "stdafx.h"
#include "my_info.h"
#include "core_dispatcher.h"
#include "app_config.h"
#include "cache/avatars/AvatarStorage.h"
#include "gui_settings.h"
#include "utils/translator.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "controls/GeneralDialog.h"
#include "main_window/PhoneWidget.h"
#include "main_window/MainWindow.h"
#include "main_window/TermsPrivacyWidget.h"

namespace
{
    std::unique_ptr<Ui::GeneralDialog> attachPhoneDialog;
}

namespace Ui
{
    std::unique_ptr<my_info> g_my_info;

    AttachPhoneInfo::AttachPhoneInfo()
        : show_(false)
        , close_(false)
        , timeout_(-1)
    {
    }

    AttachPhoneInfo::AttachPhoneInfo(const AttachPhoneInfo& _other)
        : show_(_other.show_)
        , close_(_other.close_)
        , timeout_(_other.timeout_)
        , text_(_other.text_)
        , title_(_other.title_)
        , aimId_(_other.aimId_)
    {

    }

    AttachPhoneInfo& AttachPhoneInfo::operator=(const AttachPhoneInfo& _other)
    {
        if (*this != _other)
        {
            aimId_ = _other.aimId_;
            show_ = _other.show_;
            close_ = _other.close_;
            timeout_ = _other.timeout_;
            text_ = _other.text_;
            title_ = _other.title_;
        }

        return *this;
    }

    bool AttachPhoneInfo::operator==(const AttachPhoneInfo& _other)
    {
        return aimId_ == _other.aimId_ &&
               show_ == _other.show_ &&
               close_ == _other.close_ &&
               timeout_ == _other.timeout_ &&
               text_ == _other.text_ &&
               title_ == _other.title_;
    }

    bool AttachPhoneInfo::operator!=(const AttachPhoneInfo& _other)
    {
        return !(*this == _other);
    }

    bool AttachPhoneInfo::show_dialog()
    {
        auto phoneWidget = new PhoneWidget(0, PhoneWidgetState::ENTER_PHONE_STATE, title_, text_, close_, true);
        attachPhoneDialog = std::make_unique<GeneralDialog>(phoneWidget, Utils::InterConnector::instance().getMainWindow(), false, true, close_);
        QObject::connect(phoneWidget, &PhoneWidget::requestClose, attachPhoneDialog.get(), &GeneralDialog::acceptDialog);
        attachPhoneDialog->showInCenter();

        auto result = phoneWidget->succeeded();
        attachPhoneDialog.reset();
        return result;
    }

    void AttachPhoneInfo::close_dialog()
    {
        if (attachPhoneDialog)
            attachPhoneDialog->rejectDialog(Utils::CloseWindowInfo());
    }

    my_info::my_info()
    {
        attachPhoneTimer_.setSingleShot(true);
        connect(&attachPhoneTimer_, &QTimer::timeout, this, &my_info::attachStuff);

        connect(GetDispatcher(), &core_dispatcher::attachPhoneInfo, this, &my_info::attachPhoneInfo);
    }

    void my_info::unserialize(core::coll_helper* _collection)
    {
        prevData_ = data_;
        data_.aimId_ = QString::fromUtf8(_collection->get_value_as_string("aimId"));
        data_.nick_ = QString::fromUtf8(_collection->get_value_as_string("nick"));
        data_.friendly_ = QString::fromUtf8(_collection->get_value_as_string("friendly"));
        data_.state_ = QString::fromUtf8( _collection->get_value_as_string("state"));
        data_.userType_ = QString::fromUtf8(_collection->get_value_as_string("userType"));
        data_.phoneNumber_ = QString::fromUtf8(_collection->get_value_as_string("attachedPhoneNumber"));

        const auto biz_phone_disabled = build::is_biz() && !Features::bizPhoneAllowed();
        if (!data_.phoneNumber_.isEmpty() || build::is_dit() || biz_phone_disabled)
        {
            attachInfo_ = AttachPhoneInfo();
        }
        else if (!phoneAttachment_)
        {
            needAttachPhone_ = true;
        }

        data_.flags_ = _collection->get_value_as_uint("globalFlags");
        data_.largeIconId_ = QString::fromUtf8(_collection->get_value_as_string("largeIconId"));
        data_.hasMail_ = _collection->get_value_as_bool("hasMail");

        get_gui_settings()->set_value(login_page_last_entered_phone, data_.phoneNumber_);
        get_gui_settings()->set_value(login_page_last_entered_uin, data_.aimId_);

        data_.need_to_accept_types_.clear();
        core::iarray* valuesArray = _collection->get_value_as_array("userAgreement");
        for (int i = 0; i < valuesArray->size(); ++i)
        {
            const core::ivalue* val = valuesArray->get_at(i);
            data_.need_to_accept_types_.push_back(static_cast<AgreementType>(val->get_as_int()));
        }

        auto appConfig = GetAppConfig();

        auto it = std::find(data_.need_to_accept_types_.begin(), data_.need_to_accept_types_.end(), my_info::AgreementType::gdpr_pp);
        if (it != data_.need_to_accept_types_.end()
            && !gdprAgreement_
            && !appConfig.GDPR_AgreedButAwaitingSend())
        {
            needAttachPhone_ = phoneAttachment_ || needAttachPhone_;
            attachInfo_.close_dialog();

            showGDPRAgreement();
        }

        if (needAttachPhone_)
            needAttachPhoneNumber();

        emit received();
    }

    my_info* MyInfo()
    {
        if (!g_my_info)
            g_my_info = std::make_unique<my_info>();

        return g_my_info.get();
    }

    void ResetMyInfo()
    {
        g_my_info.reset();
    }

    void my_info::CheckForUpdate() const
    {
        if (prevData_.largeIconId_ != data_.largeIconId_)
        {
            core::coll_helper helper(Ui::GetDispatcher()->create_collection(), true);
            helper.set<QString>("contact", data_.aimId_);
            Ui::GetDispatcher()->post_message_to_core("avatars/remove", helper.get());

            Logic::GetAvatarStorage()->updateAvatar(aimId());
            emit Logic::GetAvatarStorage()->avatarChanged(aimId());
        }
    }

    void my_info::needAttachPhoneNumber()
    {
        core::coll_helper helper(Ui::GetDispatcher()->create_collection(), true);
        auto lang = Utils::GetTranslator()->getLang();
        QLocale locale(lang);
        helper.set<QString>("locale", locale.name());
        helper.set<QString>("lang", lang);
        attachPhoneInfoSeq_ = Ui::GetDispatcher()->post_message_to_core("get_attach_phone_info", helper.get());
    }

    void my_info::attachPhoneInfo(const int64_t _seq, const AttachPhoneInfo& _info)
    {
        if (_seq != attachPhoneInfoSeq_)
            return;

        if (_info.show_)
        {
            if (attachInfo_ == _info && !needAttachPhone_)
                return;

            attachInfo_ = _info;

            if (attachPhoneTimer_.isActive())
                return;

            if (gdprAgreement_)
                needAttachPhone_ = true;
            else
                attachStuff();
        }
    }

    bool my_info::isGdprAgreementInProgress() const
    {
        return gdprAgreement_;
    }

    bool my_info::isPhoneAttachmentInProgress() const
    {
        return phoneAttachment_;
    }

    void my_info::attachStuff()
    {

        if (!attachInfo_.show_ || attachPhoneDialog)
            return;

        phoneAttachment_ = true;
        if (!attachInfo_.show_dialog())
            attachPhoneTimer_.start(attachInfo_.timeout_ * 1000);

        phoneAttachment_ = false;
        needAttachPhone_ = false;
    }

    void my_info::showGDPRAgreement()
    {
        gdprAgreement_ = true;

        TermsPrivacyWidget::Options options;
        options.blockUntilAccepted_ = true;
        options.controlContainingDialog_ = true;

        auto termsWidget = new TermsPrivacyWidget(QT_TRANSLATE_NOOP("terms_privacy_widget", "Terms and Privacy Policy"),
            QT_TRANSLATE_NOOP("terms_privacy_widget",
                "Please pay your attention that we have updated our "
                "<a href=\"%1\">Terms</a> "
                "and <a href=\"%2\">Privacy Policy</a>. "
                "By clicking \"I Agree\", you confirm that you have read updated"
                " documents carefully and agree to them.")
            .arg(build::GetProductVariant(TermsUrlICQ, TermsUrlAgent, TermsUrlBiz, TermsUrlDit))
            .arg(build::GetProductVariant(PrivacyPolicyUrlICQ, PrivacyPolicyUrlAgent, PrivacyPolicyUrlBiz, PrivacyPolicyUrlDit)),
            options,
            nullptr);

        auto generalDialog = std::make_unique<GeneralDialog>(termsWidget,
            Utils::InterConnector::instance().getMainWindow(),
            true,
            false,
            false /* rejectable */);

        termsWidget->setContainingDialog(generalDialog.get());

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_show_update);

        generalDialog->setModal(true);
        generalDialog->setWindowModality(Qt::ApplicationModal);
        generalDialog->setIgnoreClicksInSemiWindow(true);
        generalDialog->setIgnoredKeys({ Qt::Key_Escape });
        generalDialog->showInCenter();

        gdprAgreement_ = false;
    }

    void my_info::setFriendly(const QString& _friendly)
    {
        assert(!_friendly.isEmpty());
        data_.friendly_ = _friendly;
        emit received();
    }
}

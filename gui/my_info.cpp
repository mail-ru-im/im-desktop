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
#include "../common.shared/config/config.h"

namespace
{
    std::unique_ptr<Ui::GeneralDialog> attachPhoneDialog;
}

namespace Ui
{
    std::unique_ptr<my_info> g_my_info;

    AttachPhoneInfo::AttachPhoneInfo() = default;

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
        auto phoneWidget = new PhoneWidget(0, PhoneWidgetState::ENTER_PHONE_STATE, title_, text_, close_, Ui::AttachState::FORCE_LOGOUT);
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
        if (!data_.phoneNumber_.isEmpty())
            emit Utils::InterConnector::instance().phoneAttached(true);

        const auto phoneDisabled = !Features::phoneAllowed();
        if (!data_.phoneNumber_.isEmpty() || phoneDisabled)
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
            && !appConfig.GDPR_AgreedButAwaitingSend()
            && config::get().is_on(config::features::need_gdpr_window))
        {
            needAttachPhone_ = phoneAttachment_ || needAttachPhone_;
            attachInfo_.close_dialog();

            emit needGDPR();
        }

        if (needAttachPhone_)
        {
            if (auto show = Features::showAttachPhoneNumberPopup())
            {
                Ui::AttachPhoneInfo info;
                info.show_ = show;
                info.aimId_ = data_.aimId_;
                info.close_ = Features::closeBtnAttachPhoneNumberPopup();
                info.timeout_ = Features::showTimeoutAttachPhoneNumberPopup();
                info.text_ = Features::textAttachPhoneNumberPopup();
                info.title_ = Features::titleAttachPhoneNumberPopup();

                attachPhoneInfo(info);
            }
        }

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

    void my_info::attachPhoneInfo(const AttachPhoneInfo& _info)
    {
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

    void my_info::setFriendly(const QString& _friendly)
    {
        assert(!_friendly.isEmpty());
        data_.friendly_ = _friendly;
        emit received();
    }

    void my_info::setAimId(const QString& _aimId)
    {
        data_.aimId_ = _aimId;
    }
}

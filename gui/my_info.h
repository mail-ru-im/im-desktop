#pragma once

#include "../corelib/collection_helper.h"

namespace Ui
{
    class AttachPhoneInfo
    {
    public:
        AttachPhoneInfo();
        AttachPhoneInfo(const AttachPhoneInfo& _other);

        AttachPhoneInfo& operator=(const AttachPhoneInfo& _other);
        bool operator==(const AttachPhoneInfo& _other);
        bool operator!=(const AttachPhoneInfo& _other);

        bool show_dialog();
        void close_dialog();

    public:
        bool show_ = false;
        bool close_ = false;
        qint32 timeout_ = -1;
        QString text_;
        QString title_;
        QString aimId_;
    };

    class my_info : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void received();
        void needGDPR();

    public:
        enum class AgreementType
        {
            gdpr_pp,
        };

    private:
        struct MyInfoData
        {
            QString aimId_;
            QString nick_;
            QString friendly_;
            QString state_;
            QString userType_;
            QString phoneNumber_;
            uint32_t flags_ = 0;
            QString largeIconId_;
            bool auto_created_ = false;
            bool hasMail_ = false;
            bool trusted_ = false;

            std::vector<AgreementType> need_to_accept_types_;
        };

        MyInfoData data_;
        MyInfoData prevData_;
        AttachPhoneInfo attachInfo_;
        QTimer attachPhoneTimer_;
        bool gdprAgreement_ = false;
        bool phoneAttachment_ = false;
        bool needAttachPhone_ = false;

        void attachStuff();

    public:
        my_info();
        void unserialize(core::coll_helper* _collection);
        bool haveConnectedEmail() const noexcept { return data_.hasMail_; }

        void stopAttachPhoneNotifications();
        void resumeAttachPhoneNotifications();

        const QString& aimId() const { return data_.aimId_; }
        const QString& nick() const { return data_.nick_; }
        const QString& friendly() const {return data_.friendly_; }
        const QString& state() const { return data_.state_; }
        const QString& userType() const { return data_.userType_; }
        const QString& phoneNumber() const { return data_.phoneNumber_; }
        uint32_t flags() const noexcept { return data_.flags_; }
        bool auto_created() const noexcept { return data_.auto_created_; }
        bool isTrusted() const noexcept { return data_.trusted_; }
        const QString& largeIconId() const { return data_.largeIconId_; }
        bool isEmailProfile() const { return aimId().contains(u'@'); }

        void CheckForUpdate() const;
        void attachPhoneInfo(const AttachPhoneInfo& _info);
        bool isGdprAgreementInProgress() const;
        bool isPhoneAttachmentInProgress() const;

        void setFriendly(const QString& _friendly);
        void setAimId(const QString& _aimId);
    };

    my_info* MyInfo();
    void ResetMyInfo();
}

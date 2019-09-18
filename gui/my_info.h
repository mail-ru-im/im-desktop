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
        bool show_;
        bool close_;
        qint32 timeout_;
        QString text_;
        QString title_;
        QString aimId_;
    };

    class my_info : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void received();

    public:
        enum class AgreementType
        {
            gdpr_pp,
        };

    private:
        struct MyInfoData
        {
            MyInfoData()
                : flags_(0)
                , auto_created_(false)
                , hasMail_(false)
            {}

            QString aimId_;
            QString nick_;
            QString friendly_;
            QString state_;
            QString userType_;
            QString phoneNumber_;
            uint32_t flags_;
            QString largeIconId_;
            bool auto_created_;
            bool hasMail_;

            std::vector<AgreementType> need_to_accept_types_;
        };

        MyInfoData data_;
        MyInfoData prevData_;
        AttachPhoneInfo attachInfo_;
        QTimer attachPhoneTimer_;
        bool gdprAgreement_ = false;
        bool phoneAttachment_ = false;
        bool needAttachPhone_ = false;
        int64_t attachPhoneInfoSeq_ = 0;

        void attachStuff();

    public:
        my_info();
        void unserialize(core::coll_helper* _collection);
        bool haveConnectedEmail() const noexcept { return data_.hasMail_; }

        QString aimId() const { return data_.aimId_; };
        QString nick() const { return data_.nick_; };
        QString friendly() const {return data_.friendly_; };
        QString state() const { return data_.state_; };
        QString userType() const { return data_.userType_; };
        QString phoneNumber() const { return data_.phoneNumber_; };
        uint32_t flags() const noexcept { return data_.flags_; };
        bool auto_created() const noexcept { return data_.auto_created_; };
        QString largeIconId() const { return data_.largeIconId_; };

        void CheckForUpdate() const;
        void needAttachPhoneNumber();
        void attachPhoneInfo(const int64_t _seq, const AttachPhoneInfo& _info);
        bool isGdprAgreementInProgress() const;
        bool isPhoneAttachmentInProgress() const;

        void setFriendly(const QString& _friendly);

    private:
        void showGDPRAgreement();
    };

    my_info* MyInfo();
    void ResetMyInfo();
}
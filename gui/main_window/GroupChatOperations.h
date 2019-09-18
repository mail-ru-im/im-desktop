#pragma once

#include "../types/message.h"

namespace Logic
{
    class CustomAbstractListModel;
}

namespace Ui
{
    class GeneralDialog;
    class TextEditEx;
    class TextEmojiWidget;
    class ContactAvatarWidget;
}

namespace GroupChatOperations
{
    struct ChatData
    {
        QString name;
        bool publicChat = false;
        bool approvedJoin = false;
        bool joiningByLink = false;
        bool readOnly = false;

        bool getJoiningByLink() const noexcept
        {
            return publicChat || joiningByLink;
        }
    };
}

namespace Ui
{
    class GroupChatSettings final: public QWidget
    {
        Q_OBJECT

    private:
        std::unique_ptr<GeneralDialog> dialog_;
        QWidget *content_;
        TextEditEx *chatName_;
        ContactAvatarWidget *photo_;
        QPixmap lastCroppedImage_;

        GroupChatOperations::ChatData &chatData_;

        bool editorIsShown_;
        bool channel_;

    public:
        GroupChatSettings(QWidget *parent, const QString &buttonText, const QString &headerText, GroupChatOperations::ChatData &chatData, bool channel = false);
        ~GroupChatSettings();

        bool show();
        inline const QPixmap &lastCroppedImage() const { return lastCroppedImage_; }
        inline ContactAvatarWidget *photo() const { return photo_; }

    private:
        void showImageCropDialog();

    private Q_SLOTS:
        void nameChanged();
        void enterPressed();
    };

    void createGroupChat(const std::vector<QString>& _members_aimIds);
    void createChannel();
    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings, bool _channel);

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, const QString& avatarId = QString());
    void postAddChatMembersFromCLModelToCore(const QString& _aimId);

    void deleteMemberDialog(Logic::CustomAbstractListModel* _model, const QString& _chatAimId, const QString& _memberAimId, int _regim, QWidget* _parent);

    int forwardMessage(const Data::QuotesVec& quotes, bool fromMenu, const QString& _labelText = QString(), const QString& _buttonText = QString(), bool _enableAuthorSetting = true);

    void shareContact(const Data::Quote& _contact, const QString& _recieverAimId, const Data::QuotesVec& _quotes = {});

    int forwardMessage(const QString& _message, const QString& _labelText = QString(), const QString& _buttonText = QString(), bool _enableAuthorSetting = true);

    void eraseHistory(const QString& _aimid);

    void sharePhone(const QString& _name, const QString& _phone, const QString& _aimid);

    void sharePhone(const QString& _name, const QString& _phone, const std::vector<QString>& _selectedContacts, const QString& _aimid, const Data::QuotesVec& _quotes = {});
}

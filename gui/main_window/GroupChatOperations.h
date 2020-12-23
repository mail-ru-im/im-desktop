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
        bool isChannel = false;

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

    enum class CreateChatSource
    {
        dots,
        pencil,
        profile,
    };

    void createGroupVideoCall();
    void createGroupChat(const std::vector<QString>& _members_aimIds, const CreateChatSource _source);
    void createChannel(const CreateChatSource _source);
    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings, bool _channel);

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, const QString& avatarId = QString());
    qint64 postAddChatMembersFromCLModelToCore(const QString& _aimId);
    qint64 postAddChatMembersToCore(const QString& _chatAimId, std::vector<QString> _contacts, bool _unblockConfirmed);

    bool deleteMemberDialog(Logic::CustomAbstractListModel* _model, const QString& _chatAimId, const QString& _memberAimId, int _regim, QWidget* _parent);

    int forwardMessage(const Data::QuotesVec& quotes, bool fromMenu, const QString& _labelText = QString(), const QString& _buttonText = QString(), bool _enableAuthorSetting = true);

    void shareContact(const Data::Quote& _contact, const QString& _recieverAimId, const Data::QuotesVec& _quotes = {});

    int forwardMessage(const QString& _message, const QString& _labelText = QString(), const QString& _buttonText = QString(), bool _enableAuthorSetting = true);

    enum class ForwardWithAuthor
    {
        Yes,
        No
    };

    enum class ForwardSeparately
    {
        Yes,
        No
    };

    void sendForwardedMessages(const Data::QuotesVec& quotes, std::vector<QString> _contacts, ForwardWithAuthor _forwardWithAuthor, ForwardSeparately _forwardSeparately);

    void eraseHistory(const QString& _aimid);

    void sharePhone(const QString& _name, const QString& _phone, const QString& _aimid);

    void sharePhone(const QString& _name, const QString& _phone, const std::vector<QString>& _selectedContacts, const QString& _aimid, const Data::QuotesVec& _quotes = {});

    enum class ChatMembersShowMode
    {
        All,
        WithoutMe
    };

    bool selectChatMembers(const QString& _aimId, const QString& _title, const QString& _buttonName, ChatMembersShowMode _mode,  std::vector<QString>& _contacts);

    enum class CallType
    {
        Audio,
        Video
    };

    enum class CallPlace
    {
        Chat,
        Profile,
        CallLog
    };

    enum class JoinChatRoom
    {
        No,
        Yes
    };

    void doVoipCall(const QString& _aimId, CallType _type, CallPlace _place);
}

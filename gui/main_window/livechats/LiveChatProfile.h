#pragma once

namespace Data
{
    class ChatInfo;
}

namespace core
{
    enum class group_chat_info_errors;
}

namespace Ui
{
    class ContactAvatarWidget;
    class LiveChatMembersControl;
    class GeneralDialog;
    class TextEditEx;

    class LiveChats : public QObject
    {
        Q_OBJECT

    private Q_SLOTS:
        void needJoinLiveChatStamp(const QString& _stamp);
        void needJoinLiveChatAimId(const QString& _aimId);
        void onChatInfo(qint64, const std::shared_ptr<Data::ChatInfo>& _info, const int _requestMembersLimit);
        void onChatInfoFailed(qint64, core::group_chat_info_errors _error, const QString& _aimId);
        void liveChatJoined(const QString& _aimid);

    public:
        explicit LiveChats(QObject* _parent);
        virtual ~LiveChats();

    private:
        void onNeedJoin(const QString& _stamp, const QString& _aimId);

        bool connected_;
        GeneralDialog* activeDialog_;
        QString joinedLiveChat_;

        qint64 seq_;
    };

    class LiveChatProfileWidget : public QWidget
    {
        Q_OBJECT

        QString stamp_;
        QVBoxLayout* rootLayout_;
        ContactAvatarWidget* avatar_;
        LiveChatMembersControl* members_;
        TextEditEx* name_;
        int membersCount_;
        int initialiNameHeight_;

        void requestProfile();

    private Q_SLOTS:
        void nameResized(int, int);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        void viewChat(std::shared_ptr<Data::ChatInfo> _info);
        void setStamp(const QString& _stamp);

        LiveChatProfileWidget(QWidget* _parent, const QString& _stamp = QString());
        virtual ~LiveChatProfileWidget();
    };

    class LiveChatErrorWidget : public QWidget
    {
        Q_OBJECT

    private:

        const QString errorText_;

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        LiveChatErrorWidget(QWidget* _parent, const QString& _errorText);
        virtual ~LiveChatErrorWidget();

        void show();
    };
}

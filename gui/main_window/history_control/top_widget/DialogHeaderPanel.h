#pragma once

namespace Data
{
    class ChatInfo;
    struct DialogGalleryState;
}

namespace Ui
{
    class ContactAvatarWidget;
    class ClickableTextWidget;
    class CustomButton;
    class ConnectionWidget;
    class OverlayTopChatWidget;
    class StartCallButton;
    enum class ConnectionState;

    class DialogHeaderPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void switchToPrevDialog(bool _byKeyboard, QPrivateSignal) const;
        void needUpdateMembers(QPrivateSignal) const;

    public:
        DialogHeaderPanel(const QString& _aimId, QWidget* _parent);

        void updateStyle();
        void setPrevChatButtonVisible(bool _visible);
        void setOverlayTopWidgetVisible(bool _visible);
        void updatePendingButtonPosition();
        void updateCallButtonsVisibility();

        void updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _info);
        void setUnreadBadgeText(const QString& _text);

        void suspendConnectionWidget();
        void resumeConnectionWidget();

        void initStatus();
        void updateName();

    protected:
        void resizeEvent(QResizeEvent* _e);

    private:
        void deactivateSearchButton();
        void setContactStatusClickable(bool _isEnabled);

        void onInfoClicked();
        void onStatusClicked();
        void updateContactNameTooltip(bool _show);
        void searchButtonClicked();
        void pendingButtonClicked();
        void addMember();
        void connectionStateChanged(ConnectionState _state);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        int32_t friendlyProxyFlags() const;
        int32_t avatarProxyFlags() const;

        void updateGalleryButtonVisibility();
        void requestDialogGallery();

    private:
        QString aimId_;

        int32_t chatMembersCount_ = -1;

        QWidget* topWidgetLeftPadding_ = nullptr;
        QWidget* prevChatButtonWidget_ = nullptr;
        ContactAvatarWidget* avatar_ = nullptr;
        QWidget* contactWidget_ = nullptr;
        ClickableTextWidget* contactName_ = nullptr;
        ClickableTextWidget* contactStatus_ = nullptr;
        ConnectionWidget* connectionWidget_ = nullptr;
        QWidget* buttonsWidget_ = nullptr;
        CustomButton* searchButton_ = nullptr;
        CustomButton* galleryButton_ = nullptr;
        CustomButton* addMemberButton_ = nullptr;
        CustomButton* pendingButton_ = nullptr;
        StartCallButton* callButton_ = nullptr;
        OverlayTopChatWidget* overlayTopChatWidget_ = nullptr;
        OverlayTopChatWidget* overlayPendings_ = nullptr;
        QTimer* contactStatusTimer_ = nullptr;

        bool hasGallery_ = false;
        bool youMember_ = false;
    };
}

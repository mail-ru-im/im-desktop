#pragma once

#include "Common.h"

namespace Logic
{
    class CustomAbstractListModel;
    class AbstractSearchModel;
}

namespace Ui
{
    class SearchWidget;
    class ContactListWidget;
    class GeneralDialog;
    class qt_gui_settings;
    class DialogButton;
    class TextEditEx;
    class ContactAvatarWidget;
    class TextWidget;

    struct AvatarData
    {
        QString aimId_;
        QPixmap avatar_;
        bool hovered_ = false;
    };

    class AvatarsArea : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void removed();
        void showed();
        void resized(QPrivateSignal);

    private Q_SLOTS:
        void onAvatarChanged(const QString& aimId);

    public:
        AvatarsArea(QWidget* _parent, int _regim, Logic::CustomAbstractListModel* _membersModel, Logic::AbstractSearchModel* _searchModel);
        void add(const QString& _aimId, QPixmap _avatar);
        void add(const QString& _aimId);
        void remove(const QString& _aimId);

        size_t avatars() const noexcept { return avatars_.size(); }

        void setReplaceFavorites(bool _enable);

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void mouseMoveEvent(QMouseEvent*) override;
        virtual void leaveEvent(QEvent*) override;
        virtual void wheelEvent(QWheelEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void focusInEvent(QFocusEvent* _event) override;
        virtual void focusOutEvent(QFocusEvent* _event) override;
        virtual void keyPressEvent(QKeyEvent* _event) override;

    private:
        void updateHover(const QPoint& _pos);
        QPixmap getAvatar(const QString& _aimId);

        enum class UpdateFocusOrder
        {
            Current,
            Next,
            Previous
        };

        void updateFocus(const UpdateFocusOrder& _order);

    private:
        std::vector<AvatarData> avatars_;
        QPoint pressPoint_;
        int diff_;
        int avatarOffset_;
        QVariantAnimation* heightAnimation_;
        QVariantAnimation* avatarAnimation_;
        bool replaceFavorites_;
        Logic::CustomAbstractListModel* membersModel_;
        Logic::AbstractSearchModel* searchModel_;
        int regim_;
    };

    class AuthorWidget_p;

    class AuthorWidget : public QWidget
    {
        Q_OBJECT
    public:
        enum class Mode
        {
            Contact,
            Group,
            Channel
        };

        AuthorWidget(QWidget* _parent = nullptr);
        ~AuthorWidget();

        void setAuthors(const QSet<QString>& _authors);
        void setChatName(const QString& _chatName);
        void setMode(Mode _mode);

        bool isAuthorsEnabled() const;

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void keyPressEvent(QKeyEvent* _e) override;

    private:
        void updateHover(const QPoint& _pos);

    private:
        std::unique_ptr<AuthorWidget_p> d;
    };

    struct SelectContactsWidgetOptions
    {
        bool handleKeyPressEvents_ = true;
        Logic::AbstractSearchModel* searchModel_ = nullptr;
        bool enableAuthorWidget_ = false;
        bool chatCreation_ = false;
        bool isChannel_ = false;
        bool withSemiwindow_ = true;
        bool selectForThread_ = false;
    };

    class SelectContactsWidget : public QDialog
    {
        Q_OBJECT

    Q_SIGNALS:
        void moreClicked(const QString& _aimId, QPrivateSignal);
    private Q_SLOTS:
        void itemClicked(const QString&);
        void searchEnd();
        void escapePressed();
        void enterPressed();
        void nameChanged();
        void containsPreCheckedMembers(const std::vector<QString>& _names);
        void copyCallLink();

    public Q_SLOTS:
        void UpdateMembers();
        void setEmptyLabelVisible(bool _isEmptyIgnoreList);
        void UpdateContactList();
        void reject() override;

        void updateSize();

    public:
        SelectContactsWidget(const QString& _labelText, QWidget* _parent, Logic::DrawIcons _needDrawIcons = Logic::NoNeedDrawIcons);

        SelectContactsWidget(
            Logic::CustomAbstractListModel* _chatMembersModel,
            int _regim,
            const QString& _labelText,
            const QString& _buttonText,
            QWidget* _parent,
            SelectContactsWidgetOptions _options = SelectContactsWidgetOptions(),
            Logic::DrawIcons _needDrawIcons = Logic::NoNeedDrawIcons);

        ~SelectContactsWidget();

        virtual bool show();
        inline const QPixmap &lastCroppedImage() const { return lastCroppedImage_; }
        inline ContactAvatarWidget *photo() const { return photo_; }
        QString getName() const;

        void setAuthors(const QSet<QString> &_authors);
        void setChatName(const QString& _chatName);
        void setAuthorWidgetMode(AuthorWidget::Mode _mode);
        bool isAuthorsEnabled() const;

        // Set maximum restriction for selected item count. Used for video conference.
        virtual void setMaximumSelectedCount(int number);

        void setChatAimId(const QString& _chatId);

        void rewindToTop();
    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

        bool isCheckboxesVisible() const;
        bool isShareModes() const;
        bool isCreateGroupMode() const;
        bool isVideoModes() const;
        bool isPopupWithCloseBtn() const;
        bool isPopupWithCancelBtn() const;

        void updateFocus(bool _order);

        void addAvatarToArea(const QString& _aimId);
        void removeAvatarFromArea(const QString& _aimId);
        void addAvatarsToArea(const std::vector<QString>& _aimIds);

        enum class FocusPosition
        {
            begin,

            Name,
            Search,
            Accept,
            Cancel,
            Avatars,
            Author,

            end
        };

        FocusPosition currentFocusPosition() const;

        SearchWidget* searchWidget_;
        ContactListWidget* contactList_;
        std::unique_ptr<GeneralDialog> mainDialog_;
        QVBoxLayout* globalLayout_;
        int regim_;
        Logic::CustomAbstractListModel* const chatMembersModel_;
        QWidget* mainWidget_;

        bool chatCreation_ = false;
        int maximumSelectedCount_ = -1;
        QString chatAimId_;
        Logic::AbstractSearchModel* searchModel_ = nullptr;
        AvatarsArea* avatarsArea_ = nullptr;
        QPointer<AuthorWidget> authorWidget_;
        DialogButton* cancelButton_ = nullptr;
        DialogButton* acceptButton_ = nullptr;

        TextEditEx* chatName_ = nullptr;
        ContactAvatarWidget* photo_ = nullptr;
        QPixmap lastCroppedImage_;

        std::map<QWidget*, FocusPosition> focusWidget_;
        Logic::DrawIcons needDrawIcons_;
    };
}

#pragma once
#include "../../animation/animation.h"

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

    public:
        AvatarsArea(QWidget* _parent);
        void add(const QString& _aimid, QPixmap _avatar);
        void remove(const QString& _aimid);

        size_t avatars() const noexcept { return avatars_.size(); }

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
        anim::Animation heightAnimation_;
        anim::Animation avatarAnimation_;
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

    class SelectContactsWidget : public QDialog
    {
        Q_OBJECT

    private Q_SLOTS:
        void itemClicked(const QString&);
        void searchEnd();
        void escapePressed();
        void enterPressed();
        void recalcAvatarArea();

    public Q_SLOTS:
        void UpdateMembers();
        void UpdateViewForIgnoreList(bool _isEmptyIgnoreList);
        void UpdateContactList();
        void reject() override;

    public:
        SelectContactsWidget(const QString& _labelText, QWidget* _parent);

        SelectContactsWidget(Logic::CustomAbstractListModel* _chatMembersModel,
            int _regim,
            const QString& _labelText,
            const QString& _buttonText,
            QWidget* _parent,
            bool _handleKeyPressEvents = true,
            Logic::AbstractSearchModel* searchModel = nullptr,
            bool _enableAuthorWidget = false);

        ~SelectContactsWidget();

        virtual bool show();

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

    protected:
        void init(const QString& _labelText, const QString& _buttonText = QString());

        int calcListHeight() const;
        bool isCheckboxesVisible() const;
        bool isShareMode() const;
        bool isVideoConference() const;
        bool isPopupWithCloseBtn() const;
        bool isPopupWithCancelBtn() const;

        void updateFocus(bool _order);
        void updateSpacer();

        enum class FocusPosition
        {
            begin,

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
        QWidget* clHost_;
        QSpacerItem* clSpacer_;
        int regim_;
        Logic::CustomAbstractListModel* chatMembersModel_;
        QWidget* mainWidget_;
        bool sortCL_;
        bool handleKeyPressEvents_;
        bool enableAuthorWidget_;
        int maximumSelectedCount_;
        QString chatAimId_;
        Logic::AbstractSearchModel* searchModel_;
        AvatarsArea* avatarsArea_;
        QPointer<AuthorWidget> authorWidget_;
        DialogButton* cancelButton_;
        DialogButton* acceptButton_;

        std::map<QWidget*, FocusPosition> focusWidget_;
    };
}

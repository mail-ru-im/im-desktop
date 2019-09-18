#pragma once

#include "main_window/sidebar/SidebarUtils.h"
#include "utils/utils.h"
#include "../../types/chat.h"
#include "../../types/contact.h"

namespace Ui
{
    class GeneralDialog;
    class DialogButton;

    enum class TransitState
    {
        ON_PROFILE,
        EMPTY_PROFILE,
        UNCHECKED,
    };

    class TransitProfileSharingWidget : public QWidget
    {
        Q_OBJECT

    public:
        TransitProfileSharingWidget(QWidget* _parent, const QString& _aimid);
        TransitProfileSharingWidget(QWidget* _parent, TransitState _error);
        bool sharePhone() const;
        void onUserInfo(const Data::UserInfo& _info);
    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
    private:
        void connectAvatarSlots();
        void init();
        void initShort();
        void initError();

        void updateSpacer(const int _height);

        QPixmap getButtonPixmap();

    private:
        Data::UserInfo info_;
        QString aimId_;

        ContactAvatarWidget* avatar_;

        Ui::TextRendering::TextUnitPtr friendlyTextUnit_;
        Ui::TextRendering::TextUnitPtr nickTextUnit_;
        Ui::TextRendering::TextUnitPtr errorTextUnit_;

        Ui::TextRendering::TextUnitPtr phoneCaption_;
        Ui::TextRendering::TextUnitPtr phone_;
        QPixmap btnSharePhone_;

        bool nicknameVisible_;
        bool phoneExists_;
        TransitState error_;
        bool isPhoneSelected_;

        QRect btnRect_;
        bool btnHovered_;
        bool btnPressed_;

        QVBoxLayout* mainLayout_;

        QWidget* resizingHost_;
        QHBoxLayout* resizingLayout_;
        QSpacerItem* resizingSpacer_;
    };

    class TransitProfileSharing : public QWidget
    {
        Q_OBJECT

    public:
        TransitProfileSharing(QWidget* _parent, const QString& _aimId);

        void showProfile();
        const QString& getPhone() const {return phone_;};

    Q_SIGNALS:
        void accepted(const QString& _nick, const bool _sharePhone);
        void declined();
        void openChat();
    private Q_SLOTS:
        void onUserInfo(const int64_t, const QString& _aimid, const Data::UserInfo& _info);
    protected:
        void keyPressEvent(QKeyEvent * _e) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;
    private:
        GeneralDialog* userProfile_;
        GeneralDialog* errorUnchecked_;
        TransitProfileSharingWidget* transitProfile_;
        QString aimId_;
        QString phone_;
        QString nick_;
        QPair<DialogButton* , DialogButton*> btnPair_;
        TransitState state_;
        bool declinable_;
    private:
        void setState(const TransitState _state);
    };
}
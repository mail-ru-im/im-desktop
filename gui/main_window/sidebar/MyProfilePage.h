#pragma once

#include "Sidebar.h"
#include "../../types/contact.h"
#include "../../controls/TextUnit.h"

namespace Ui
{
    class ContactAvatarWidget;
    class FlatMenu;
    class LabelEx;

    class TextResizedWidget : public QWidget
    {
        Q_OBJECT

    public:
        template <typename... Args>
        TextResizedWidget(QWidget* _parent, Args&&... args)
            : QWidget(_parent)
            , maxAvalibleWidth_(0)
            , opacity_(1.0)
        {
            text_ = TextRendering::MakeTextUnit(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void init(Args&&... args)
        {
            text_->init(std::forward<Args>(args)...);
            update();
        }

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        void setMaximumWidth(int _width);

        void setText(const QString& _text, const QColor& _color = QColor());
        void setOpacity(qreal _opacity);

        QString getText() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        TextRendering::TextUnitPtr text_;
        int maxAvalibleWidth_;
        qreal opacity_;
    };

    class MyInfoPlate : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();

    public:
        MyInfoPlate(QWidget* _parent, const QString& _iconPath, int _leftMargin = 0, const QString& _infoEmptyStr = QString(), Qt::Alignment _align = Qt::AlignTop, int _maxInfoLinesCount = -1, int _correctTopMargin = 0, int _correctBottomMargin = 0);
        void setHeader(const QString& header);
        void setInfo(const QString& info, const QString& prefix = QString());
        QString getInfoText() const;
        QString getInfoStr() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        int leftMargin_;
        TextResizedWidget* header_;
        TextResizedWidget* info_;
        QString infoStr_;
        QString infoEmptyStr_;
        QLabel* iconArea_;
        QPixmap iconHovered_;
        QPixmap iconPressed_;
        QPoint pos_;
        bool hovered_;
        bool pressed_;
    };

    class MyProfilePage : public SidebarPage
    {
        Q_OBJECT
    public:
        explicit MyProfilePage(QWidget* _parent);
        void initFor(const QString& _aimId) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;

    signals:
        void headerBackButtonClicked() const;

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private Q_SLOTS:
        void changed();
        void onUserInfo(const int64_t _seq, const QString& _aimid, const Data::UserInfo& _info);
        void onUpdateProfile(int _error);
        void editAvatarClicked();
        void viewAvatarClicked();
        void avatarChanged();
        void avatarCancelSelect();
        void showAvatarMenu();
        void triggeredAvatarMenu(QAction* _action);
        void editNameClicked();
        void editAboutMeClicked();
        void editNicknameClicked();
        void changePhoneNumber();
        void permissionsInfoClicked() const;
        void signOutClicked();

    private:
        void init();
        void updateInfo();
        void sendCommonStat();

    private:
        QString currentAimId_;
        QString firstName_;
        QString lastName_;
        QWidget* mainWidget_;
        ContactAvatarWidget* avatar_;
        MyInfoPlate* name_;
        MyInfoPlate* aboutMe_;
        MyInfoPlate* phone_;
        MyInfoPlate* nickName_; // for icq-account only
        MyInfoPlate* email_; // for agent-account only
        LabelEx* permissionsInfo_;
        LabelEx* signOut_;

        FrameCountMode frameCountMode_;

        bool isActive_;
        bool isSendCommonStat_;
        bool isDefaultAvatar_;
    };
}

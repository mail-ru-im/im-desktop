#pragma once

#include "controls/SimpleListWidget.h"
#include "types/contact.h"

namespace Ui
{
    class ContactAvatarWidget;
    namespace TextRendering
    {
        class TextUnit;
    }

    class ProfileSettingsRow : public SimpleListItem
    {
        Q_OBJECT;

    public:
        explicit ProfileSettingsRow(QWidget* _parent = nullptr);
        ~ProfileSettingsRow();

        void setSelected(bool _value) override;
        bool isSelected() const override;

        void setCompactMode(bool _value) override;
        bool isCompactMode() const override;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        void onMyInfo();
        void onUserInfo(const int64_t _seq, const QString& _aimid, Data::UserInfo _info);
        void onNicknameClicked();
        void onNicknameContextMenu(const QPoint& _pos);
        void onContextMenuItemClicked(QAction* _action);

        void updateArrowIcon();
        void updateTextColor();
        void updateHeight();
        void updateNickname();
        void updateNicknameColor();
        void updateAvatarPos();

        void copyNickToClipboard();

        bool isOverNickname(const QPoint& _pos) const;
        bool hasNickname() const;

        QString getNickname() const;

        bool isEmailProfile() const;

        void updateNickVisibility();

    private:
        bool isSelected_;
        bool isCompactMode_;

        bool nickHovered_;
        bool nickPressed_;

        bool nicknameVisible_;

        ContactAvatarWidget* avatar_;
        QString nickname_;
        QPixmap arrow_;

        std::unique_ptr<Ui::TextRendering::TextUnit> friendlyTextUnit_;
        std::unique_ptr<Ui::TextRendering::TextUnit> nickTextUnit_;
    };
}
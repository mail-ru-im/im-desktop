#pragma once

#include "Common.h"
#include "../../types/contact.h"

namespace Fonts
{
    enum class FontWeight;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class Item
    {
    public:
        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) = 0;
        virtual ~Item() = default;

        static void drawContactsDragOverlay(QPainter &_painter);
        static int drawMore(QPainter &_painter, Ui::ContactListParams& _contactList, const Ui::ViewParams& _viewParams);

        static int drawAddContact(QPainter &_painter, const int _rightMargin, const bool _isSelected, ContactListParams& _recentParams);
        static void drawCheckbox(QPainter &_painter, const VisualDataBase &_visData, const ContactListParams& _contactList);
        static int drawRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams);
        static int drawUnreads(QPainter &_painter, const int _unreads, const bool _isMuted, const bool _isSelected, const bool _isHovered, const ViewParams& _viewParams, const ContactListParams& _clParams, const bool _isUnknownHeader = false);
    };

    class ServiceContact : public Item
    {
    public:
        ServiceContact(const QString& _name, Data::ContactType _type);
        ~ServiceContact();

        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> name_;
        Data::ContactType type_;
    };

    class ContactItem : public Item
    {
    public:
        ContactItem(bool _membersView);
        ~ContactItem();

        void updateParams(const VisualDataBase& _item, const ViewParams& _viewParams);

        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;

    private:
        VisualDataBase item_;
        ViewParams params_;

        std::unique_ptr<TextRendering::TextUnit> contactName_;
        std::unique_ptr<TextRendering::TextUnit> contactNick_;
        std::unique_ptr<TextRendering::TextUnit> contactStatus_;
        std::unique_ptr<TextRendering::TextUnit> contactRole_;

        Fonts::FontWeight getNameWeight() const;
        bool membersView_ = false;
    };

    class GroupItem : public Item
    {
    public:
        GroupItem(const QString& _name);
        ~GroupItem();

        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> groupName_;
    };

    class ServiceButton : public Item
    {
    public:
        ServiceButton(const QString& _name, const QPixmap& _icon, const QPixmap& _hoverIcon, const QPixmap& _pressedIcon, int _height);
        ~ServiceButton();

        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> name_;
        QPixmap icon_;
        QPixmap hoverIcon_;
        QPixmap pressedIcon_;
        int height_;
    };
}

namespace Logic
{
    class CustomAbstractListModel;

    class ContactListItemDelegate : public AbstractItemDelegateWithRegim
    {
        Q_OBJECT
    public:
        ContactListItemDelegate(QObject* parent, int _regim, CustomAbstractListModel* chatMembersModel = nullptr);

        virtual ~ContactListItemDelegate();

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        virtual void setFixedWidth(int width) override;

        virtual void blockState(bool value) override;

        virtual void setDragIndex(const QModelIndex& index) override;

        virtual void setRegim(int _regim) override;

        int regim() const;

        void setLeftMargin(int margin);

        void setRightMargin(int margin);

        void setRenderRole(bool render);

        void setPictOnlyView(bool _pictOnlyView);

        void clearCache();

        void setMembersView();

    private:
        bool StateBlocked_;
        bool renderRole_;
        bool membersView_;
        QModelIndex DragIndex_;
        Ui::ViewParams viewParams_;
        CustomAbstractListModel* chatMembersModel_;

        mutable std::unordered_map<QString, std::unique_ptr<Ui::Item>, Utils::QStringHasher> items;
        mutable std::unordered_map<QString, std::unique_ptr<Ui::ContactItem>, Utils::QStringHasher> contactItems_;
    };
}

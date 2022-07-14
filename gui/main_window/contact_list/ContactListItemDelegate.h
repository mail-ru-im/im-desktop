#pragma once

#include "Common.h"
#include "../../types/contact.h"
#include "IconsDelegate.h"

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

        static int drawMore(QPainter& _painter, Ui::ContactListParams& _contactList, const Ui::ViewParams& _viewParams);

        static int drawAddContact(QPainter& _painter, const int _rightMargin, const bool _isSelected, ContactListParams& _recentParams);
        static void drawCheckbox(QPainter& _painter, const VisualDataBase& _visData, const ContactListParams& _contactList);
        static int drawRemove(QPainter& _painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams);
        static int drawUnreads(QPainter& _painter, const int _unreads, const bool _isMuted, const bool _isSelected, const bool _isHovered, const ViewParams& _viewParams, const ContactListParams& _clParams, const bool _isUnknownHeader = false);

        virtual bool needsTooltip() const { return false; }
    protected:
        static QPixmap getCheckBoxImage(const VisualDataBase& _visData);
        static double getCheckBoxWidth(const VisualDataBase& _visData, const QPixmap* _img = nullptr);
        void fixMaxWidthIfCheckable(int& _maxWidth, const VisualDataBase& _visData, const Ui::ViewParams& _params) const;
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
        ContactItem(bool _membersView, Logic::DrawIcons _needDrawIcons = Logic::NoNeedDrawIcons);
        ~ContactItem();

        void updateParams(const VisualDataBase& _item, const ViewParams& _viewParams);
        virtual void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;
        bool needsTooltip() const override;

    private:
        VisualDataBase item_;
        ViewParams params_;
        std::unique_ptr<TextRendering::TextUnit> contactName_;
        std::unique_ptr<TextRendering::TextUnit> contactNick_;
        std::unique_ptr<TextRendering::TextUnit> contactStatus_;
        std::unique_ptr<TextRendering::TextUnit> contactRole_;
        bool membersView_ = false;
        Logic::DrawIcons needDrawIcons_;

    private:
        void fixMaxWidthIfIcons(int& _maxWidth) const;
        bool isUnknown() const;
        bool needDrawUnreads(const Data::DlgState& _state) const;
        bool needDrawMentions(const Data::DlgState& _state) const;
        bool needDrawAttention(const Data::DlgState& _state) const;
        bool needDrawDraft(const Data::DlgState& _state) const;
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
        ServiceButton(const QString& _name, const Utils::StyledPixmap& _icon, const Utils::StyledPixmap& _hoverIcon, const Utils::StyledPixmap& _pressedIcon, int _height);
        ~ServiceButton();

        void paint(QPainter& _painter, const bool _isHovered = false, const bool _isActive = false, const bool _isPressed = false, const int _curWidth = 0) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> name_;
        Utils::StyledPixmap icon_;
        Utils::StyledPixmap hoverIcon_;
        Utils::StyledPixmap pressedIcon_;
        int height_;
    };
}

namespace Logic
{
    class CustomAbstractListModel;
    class SearchModel;

    class ContactListItemDelegate : public AbstractItemDelegateWithRegim, public IconsDelegate
    {
        Q_OBJECT
    public:
        ContactListItemDelegate(QObject* parent, int _regim, CustomAbstractListModel* chatMembersModel = nullptr, DrawIcons _needDrawIcons = NoNeedDrawIcons);

        virtual ~ContactListItemDelegate();

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        void setFixedWidth(int width) override;

        void blockState(bool value) override;

        void setDragIndex(const QModelIndex& index) override;

        void setRegim(int _regim) override;

        int regim() const;

        void setLeftMargin(int margin);

        void setRightMargin(int margin);

        void setRenderRole(bool render);

        void setPictOnlyView(bool _pictOnlyView);

        bool isPictOnlyView() const;

        void clearCache();

        void setMembersView();

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;

        int getUnknownUnreads(const QRect& _rect) const;

        DrawIcons needDrawIcons() const { return needDrawIcons_; };

        bool needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;

        void setModel(Logic::SearchModel* _model){ model_ = _model; };

    public Q_SLOTS:
        void dlgStateChanged(const Data::DlgState&);

    private:
        bool StateBlocked_;
        bool renderRole_;
        bool membersView_;
        QModelIndex DragIndex_;
        Ui::ViewParams viewParams_;
        CustomAbstractListModel* chatMembersModel_;
        mutable QPoint dp_ = {};
        Logic::SearchModel* model_ = nullptr;

        mutable std::unordered_map<QString, std::unique_ptr<Ui::Item>, Utils::QStringHasher> items_;
        mutable std::unordered_map<QString, std::unique_ptr<Ui::ContactItem>, Utils::QStringHasher> contactItems_;
        DrawIcons needDrawIcons_;

        bool isUnknown() const;
        bool isUnknownAndNotPictOnly(const Ui::ViewParams& _viewParams) const;

        bool needDrawUnreads(const Data::DlgState& _state) const override;
        bool needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        bool needDrawAttention(const Data::DlgState& _state) const override;

        void drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const override;
        void drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& unreadsX, int& unreadsY) const override;
        void drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& unreadsX, int& unreadsY) const override;
        void drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& unreadsX, int& unreadsY) const override;

    };
}

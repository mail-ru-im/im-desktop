#pragma once

#include "../../types/message.h"
#include "../../types/typing.h"
#include "Common.h"
#include "IconsDelegate.h"

#include "../../controls/TextUnit.h"
#include "../../main_window/tray/RecentMessagesAlert.h"

namespace Ui
{
    enum class LastStatus;
    enum class MediaType;

    using DlgStateHead = std::pair<QString, QString>;

    //////////////////////////////////////////////////////////////////////////
    // RecentItemBase
    //////////////////////////////////////////////////////////////////////////
    class RecentItemBase : public QObject
    {
        Q_OBJECT

    protected:

        const QString aimid_;

    public:

        RecentItemBase(const Data::DlgState& _state);
        virtual ~RecentItemBase();

        virtual void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const;

        virtual void drawMouseState(QPainter& _p, const QRect& _rect, const bool _isHovered, const bool _isSelected, const bool _isKeyboardFocused = false) const;

        const QString& getAimid() const;

        virtual bool needsTooltip(QPoint _posCursor = {}) const { return false; }
        virtual QRect getDraftIconRectWrapper() const {return {};};
    };

    class PinnedServiceItem : public RecentItemBase, Logic::IconsDelegate
    {
        Q_OBJECT
    public:
        PinnedServiceItem(const Data::DlgState& _state);
        ~PinnedServiceItem();

        QRect getDraftIconRectWrapper() const override {return Logic::IconsDelegate::getDraftIconRect();};
        bool needsTooltip(QPoint _posCursor = {}) const override;
        bool needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;

    protected:
        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

        bool needDrawUnreads(const Data::DlgState& _state) const override;
        bool needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        bool needDrawAttention(const Data::DlgState& _state) const override { return false; };

        void drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const override;
        void drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        void drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        void drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override {};

    private:
        ::Ui::TextRendering::TextUnitPtr text_;
        Data::DlgState::PinnedServiceItemType type_;
        int unreadCount_;
        int unreadMentionsCount_;
    };

    //////////////////////////////////////////////////////////////////////////
    // RecentItemService
    //////////////////////////////////////////////////////////////////////////

    enum ServiceItemType
    {
        recents,
        pinned,
        unknowns,
        more_people,
        messages,
        unimportant
    };

    class RecentItemService : public RecentItemBase
    {
        Q_OBJECT

        ServiceItemType type_;

        ::Ui::TextRendering::TextUnitPtr text_;
        ::Ui::TextRendering::TextUnitPtr badgeTextUnit_;

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

    public:

        RecentItemService(const Data::DlgState& _state);
        RecentItemService(const QString& _text);
        virtual ~RecentItemService();
    };


    //////////////////////////////////////////////////////////////////////////
    // RecentItemUnknowns
    //////////////////////////////////////////////////////////////////////////
    class RecentItemUnknowns : public RecentItemBase
    {
        Q_OBJECT

        ::Ui::TextRendering::TextUnitPtr text_;

        const bool compactMode_;

        const int count_;

        int unreadBalloonWidth_;

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

    public:

        RecentItemUnknowns(const Data::DlgState& _state);
        virtual ~RecentItemUnknowns();
    };

    //////////////////////////////////////////////////////////////////////////
    // RecentItemRecent
    //////////////////////////////////////////////////////////////////////////
    class RecentItemRecent : public RecentItemBase, public Logic::IconsDelegate
    {
        Q_OBJECT
    public:
        bool needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        QRect getDraftIconRectWrapper() const override { return Logic::IconsDelegate::getDraftIconRect(); };

    protected:

        const bool multichat_;

        QString displayName_;

        ::Ui::TextRendering::TextUnitPtr name_;
        int nameWidth_;

        ::Ui::TextRendering::TextUnitPtr messageShortName_;
        ::Ui::TextRendering::TextUnitPtr messageLongName_;
        int messageNameWidth_;

        ::Ui::TextRendering::TextUnitPtr time_;

        bool draft_ = false;

        const QString state_;

        const bool compactMode_;

        const int unreadCount_;

        const bool attention_;

        bool mention_;

        mutable int prevWidthName_;

        mutable int prevWidthMessage_;

        const bool muted_;

        const bool online_;

        bool official_ = false;

        const bool pin_;

        int unreadBalloonWidth_;

        bool drawLastReadAvatar_;

        LastStatus lastStatus_;

        const int typersCount_;

        const QString typer_;

        MediaType mediaType_;

        const bool readOnlyChat_;

        std::vector<DlgStateHead> heads_;

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

        int getUnknownUnreads(const Ui::ViewParams& _viewParams, const QRect& _rect) const;

        bool isPictOnly(const Ui::ViewParams& _viewParams) const;
        bool isUnknown(const Ui::ViewParams& _viewParams) const;
        bool isUnknownAndNotPictOnly(const Ui::ViewParams& _viewParams) const;

        bool needDrawUnreads(const Data::DlgState& _state) const override;
        bool needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        bool needDrawAttention(const Data::DlgState& _state) const override;

        void fixMaxWidthIfIcons(const Ui::ViewParams& _viewParams, int& _maxWidth) const;

        void drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const override;
        void drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        void drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        void drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;

        void drawRemoveIcon(QPainter& _p, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams) const;

    public:

        RecentItemRecent(
            const Data::DlgState& _state,
            bool _compactMode,
            bool _hideSender,
            bool _hideMessage,
            bool _showHeads,
            AlertType _alertType = AlertType::alertTypeMessage,
            bool _isNotification = false);
        virtual ~RecentItemRecent();

        bool needsTooltip(QPoint _posCursor = {}) const override;
    };



    //////////////////////////////////////////////////////////////////////////
    // MessageAlertItem
    //////////////////////////////////////////////////////////////////////////
    class MessageAlertItem : public RecentItemRecent
    {
        Q_OBJECT

    public:

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

        MessageAlertItem(const Data::DlgState& _state, const bool _compactMode, Ui::AlertType _alertType);
        virtual ~MessageAlertItem();

    private:
        QImage cachedLogo_;
    };


    //////////////////////////////////////////////////////////////////////////
    // MailAlertItem
    //////////////////////////////////////////////////////////////////////////
    class MailAlertItem : public RecentItemBase
    {
        Q_OBJECT

        QString fromNick_;
        QString fromMail_;

        ::Ui::TextRendering::TextUnitPtr name_;
        int nameWidth_;

        ::Ui::TextRendering::TextUnitPtr messageShortName_;
        ::Ui::TextRendering::TextUnitPtr messageLongName_;
        int messageNameWidth_;

    public:

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

        MailAlertItem(const Data::DlgState& _state);
        virtual ~MailAlertItem();
    };


    //////////////////////////////////////////////////////////////////////////
    // ComplexMailAlertItem
    //////////////////////////////////////////////////////////////////////////
    class ComplexMailAlertItem : public RecentItemBase
    {
        Q_OBJECT

        ::Ui::TextRendering::TextUnitPtr name_;
        int nameWidth_;

    public:

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) const override;

        ComplexMailAlertItem(const Data::DlgState& _state);
        virtual ~ComplexMailAlertItem();
    };


    //////////////////////////////////////////////////////////////////////////
    // RecentItemDelegate
    //////////////////////////////////////////////////////////////////////////
    class RecentItemDelegate : public Logic::AbstractItemDelegateWithRegim
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId);
        void onItemClicked(const QString& _aimId);
        void dlgStateChanged(const Data::DlgState& _dlgState);
        void refreshAll();

    public:
        RecentItemDelegate(QObject* parent);
        virtual ~RecentItemDelegate();

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;
        QSize sizeHintForAlert() const;

        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;

        void blockState(bool value) override;
        void setDragIndex(const QModelIndex& index) override;
        void setFixedWidth(int _newWidth) override;
        void setRegim(int _regim) override;

        static std::unique_ptr<RecentItemBase> createItem(const Data::DlgState& _state);
        static bool shouldDisplayHeads(const Data::DlgState& _state);

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;
        QRect getDraftIconRectWrapper(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;

    private:
        void onFriendlyChanged(const QString& _aimId, const QString& _friendly);
        void removeItem(const QString& _aimId);

    private:
        struct ItemKey
        {
            const bool IsSelected;

            const bool IsHovered;

            const int UnreadDigitsNumber;

            ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber);

            bool operator < (const ItemKey &_key) const;
        };

        bool stateBlocked_;

        QModelIndex dragIndex_;

        Ui::ViewParams viewParams_;

        mutable QSet<QString> pendingsDialogs_;

        QString selectedAimId_;

        mutable std::unordered_map<QString, std::unique_ptr<RecentItemBase>, Utils::QStringHasher> items_;
    };
}

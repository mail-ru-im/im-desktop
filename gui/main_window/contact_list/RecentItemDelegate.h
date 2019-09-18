#pragma once

#include "../../types/message.h"
#include "../../types/typing.h"
#include "Common.h"

#include "../../controls/TextUnit.h"

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
            const bool _isKeyboardFocused);

        virtual void drawMouseState(QPainter& _p, const QRect& _rect, const bool _isHovered, const bool _isSelected, const bool _isKeyboardFocused = false);

        const QString& getAimid() const;
    };


    //////////////////////////////////////////////////////////////////////////
    // RecentItemService
    //////////////////////////////////////////////////////////////////////////

    enum ServiceItemType
    {
        recents,
        favorites,
        unknowns,
        more_people,
        messages
    };

    class RecentItemService : public RecentItemBase
    {
        Q_OBJECT

        ServiceItemType type_;

        ::Ui::TextRendering::TextUnitPtr text_;

        void draw(
            QPainter& _p,
            const QRect& _rect,
            const Ui::ViewParams& _viewParams,
            const bool _isSelected,
            const bool _isHovered,
            const bool _isDrag,
            const bool _isKeyboardFocused) override;

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
            const bool _isKeyboardFocused) override;

    public:

        RecentItemUnknowns(const Data::DlgState& _state);
        virtual ~RecentItemUnknowns();
    };



    typedef std::shared_ptr<const QPixmap> QPixmapSCptr;



    //////////////////////////////////////////////////////////////////////////
    // RecentItemRecent
    //////////////////////////////////////////////////////////////////////////
    class RecentItemRecent : public RecentItemBase
    {
        Q_OBJECT

    protected:

        const bool multichat_;

        QString displayName_;

        ::Ui::TextRendering::TextUnitPtr name_;
        int nameWidth_;

        ::Ui::TextRendering::TextUnitPtr messageShortName_;
        ::Ui::TextRendering::TextUnitPtr messageLongName_;
        int messageNameWidth_;

        ::Ui::TextRendering::TextUnitPtr time_;

        const QString state_;

        const bool compactMode_;

        const int unreadCount_;

        const bool attention_;

        bool mention_;

        int prevWidthName_;

        int prevWidthMessage_;

        const bool muted_;

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
            const bool _isKeyboardFocused) override;

    public:

        RecentItemRecent(
            const Data::DlgState& _state,
            const bool _compactMode,
            const bool _hideMessage,
            const bool _showHeads);
        virtual ~RecentItemRecent();
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
            const bool _isKeyboardFocused) override;

        MessageAlertItem(const Data::DlgState& _state, const bool _compactMode);
        virtual ~MessageAlertItem();
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
            const bool _isKeyboardFocused) override;

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
            const bool _isKeyboardFocused) override;

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

        void onContactSelected(const QString& _aimId, qint64 _msgid, qint64);
        void onItemClicked(const QString& _aimId);
        void dlgStateChanged(const Data::DlgState& _dlgState);
        void refreshAll();

    public:
        RecentItemDelegate(QObject* parent);
        virtual ~RecentItemDelegate();

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;
        QSize sizeHintForAlert() const;

        void addTyping(const Logic::TypingFires& _typing);
        void removeTyping(const Logic::TypingFires& _typing);

        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;

        void blockState(bool value) override;
        void setDragIndex(const QModelIndex& index) override;
        void setFixedWidth(int _newWidth) override;
        void setRegim(int _regim) override;

        static std::unique_ptr<RecentItemBase> createItem(const Data::DlgState& _state);
        static bool shouldDisplayHeads(const Data::DlgState& _state);

    private:
        void onFriendlyChanged(const QString& _aimId, const QString& _friendly);
        void removeItem(const QString& _aimId);

    private:

        std::list<Logic::TypingFires> typings_;

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

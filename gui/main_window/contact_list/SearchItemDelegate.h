#pragma once

#include "Common.h"
#include "IconsDelegate.h"

#include "../../types/search_result.h"
#include "../../controls/TextUnit.h"

namespace Logic
{
    class SearchModel;
    class SearchItemDelegate: public AbstractItemDelegateWithRegim, public IconsDelegate
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId, qint64 _msgId, qint64 _threadMsgId);
        void clearSelection();
        void dropCache();
        void dlgStateChanged(const Data::DlgState&);

    public:
        SearchItemDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        void setRegim(int _regim) override;
        void setFixedWidth(int width) override;
        void blockState(bool value) override;
        void setDragIndex(const QModelIndex& _index) override;

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        bool setHoveredSuggetRemove(const QModelIndex & _itemIndex);

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;
        bool needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        void setModel(Logic::SearchModel* _model){ model_ = _model; };

    private:
        void drawContactOrChatItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, bool _isSelected, bool _isDrag) const;
        void drawMessageItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::MessageSearchResultSptr& _item, const bool _isSelected) const;
        void drawServiceItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::ServiceSearchResultSptr& _item) const;
        void drawSuggest(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::SuggestSearchResultSptr& _item, const QModelIndex& _index, const bool _isHovered) const;

        bool needDrawUnreads(const Data::DlgState& _state) const override;
        bool needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const override;
        bool needDrawAttention(const Data::DlgState& _state) const override;

        virtual void drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const override;
        virtual void drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        virtual void drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;
        virtual void drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const override;

        void fixMaxWidthIfIcons(int& maxWidth, const Data::AbstractSearchResultSptr& _item) const;

        void removeItem(const QString& _aimId);

        struct CachedItem
        {
            int cachedWidth_ = 0;
            bool isSelected_ = false;
            bool isOfficial_ = false;
            bool hasDraft_ = false;
            bool hasUnread_ = false;
            bool hasAttention_ = false;
            bool hasMentions_ = false;

            Data::AbstractSearchResultSptr searchResult_ = nullptr;
            Ui::TextRendering::TextUnitPtr name_ = nullptr;
            Ui::TextRendering::TextUnitPtr nick_ = nullptr;
            Ui::TextRendering::TextUnitPtr text_ = nullptr;
            Ui::TextRendering::TextUnitPtr time_ = nullptr;

            void draw(QPainter& _painter);
            bool needUpdate(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, bool _isSelected);
            bool needUpdateWidth(const QStyleOptionViewItem& _option);
            void update(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, bool _isSelected);
            void addName(bool _replaceFavorites, const int _maxTextWidth);
            bool addText(const int _maxTextWidth, const int _leftOffset);
            int addNick(bool _replaceFavorites, const int _maxTextWidth, int _nickOffset, int _nameVerOffset, int _leftOffset);
            void addTime(const QStyleOptionViewItem& _option);
            void setOffsets(const int _nickOffset, const int _nameVerOffset, const int _leftOffset, const int _textWidth);
            int fixNameWidth(const int _nickOffset, const int _nameVerOffset, const int _leftOffset, const int _maxTextWidth);
            void addMessageName(bool _replaceFavorites, const int _maxTextWidth);
            void addMessageText(const QString& _senderName, const int _maxTextWidth);
        };
        using ContactsMap = std::unordered_map<QString, std::unique_ptr<CachedItem>, Utils::QStringHasher>;
        using MessagesMap = std::unordered_map<qint64, std::unique_ptr<CachedItem>>;

        bool updateItem(std::unique_ptr<CachedItem>& _ci, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected, const int _maxTextWidth) const;
        CachedItem* getCachedItem(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const;
        CachedItem* drawCachedItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const;

        void fillContactItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;
        void fillMessageItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;
        void reelideMessage(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;

        std::pair<bool, bool> getMouseState(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

        void drawAvatar(QPainter& _painter, const Ui::ContactListParams& _params, const QString& _aimid, const int _itemHeight, const bool _isSelected, const bool _isMessage) const;

        QString getSenderName(const Data::MessageBuddySptr& _msg, const bool _fromDialogSearch) const;

        mutable ContactsMap contactCache_;
        mutable MessagesMap messageCache_;

        qint64 selectedMessage_;

        mutable QTime lastDrawTime_;
        QTimer cacheTimer_;

        QModelIndex hoveredRemoveSuggestBtn_;
        mutable QPoint drawOffset_ = {};

        Logic::SearchModel* model_ = nullptr;
        QModelIndex dragIndex_;
    };
}

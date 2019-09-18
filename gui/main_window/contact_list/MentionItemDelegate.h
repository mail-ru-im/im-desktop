#pragma once

#include "Common.h"
#include "MentionModel.h"
#include "../../controls/TextUnit.h"

namespace Logic
{
    class MentionItemDelegate : public QItemDelegate
    {
        Q_OBJECT

    public:
        MentionItemDelegate(QObject* parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        int itemHeight() const;
        int serviceItemHeight() const;

        void dropCache();

    private:
        void renderServiceItem(QPainter& _painter, const QRect& _itemRect, const MentionSuggest& _item) const;
        void renderContactItem(QPainter& _painter, const QRect& _itemRect, const MentionSuggest& _item) const;

        bool isFirstItem(const QModelIndex& _index) const;
        bool isLastItem(const QModelIndex& _index) const;

    private:
        struct CachedItem
        {
            int cachedWidth_;
            Ui::TextRendering::TextUnitPtr itemText_;
            Ui::TextRendering::TextUnitPtr itemNick_;
            QString highlight_;
        };
        using ContactsMap = std::unordered_map<QString, std::unique_ptr<CachedItem>, Utils::QStringHasher>;

        mutable ContactsMap contactCache_;

        mutable QTime lastDrawTime_;
        QTimer* cacheTimer_;
    };
}

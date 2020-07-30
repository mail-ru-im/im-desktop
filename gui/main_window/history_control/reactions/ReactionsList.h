#pragma once

#include "types/reactions.h"

namespace Ui
{
    enum class ConnectionState;

    class ReactionsList_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsList
    //////////////////////////////////////////////////////////////////////////

    class ReactionsList : public QWidget
    {
        Q_OBJECT
        friend class ReactionsList_p;
    public:
        ReactionsList(int64_t _msgId, const QString& _contact, const QString& _reaction, const Data::Reactions& _reactions, QWidget* _parent = nullptr);
        ~ReactionsList();

    private Q_SLOTS:
        void onHeaderItemClicked(const QString& _reaction);
        void onListItemClicked(const QString& _contact);
        void onRemoveMyReactionClicked();
        void onReactionRemoveResult(int64_t _seq, bool _success);

    private:
        std::unique_ptr<ReactionsList_p> d;
    };

    class ReactionsListHeader_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsListHeader
    //////////////////////////////////////////////////////////////////////////

    class ReactionsListHeader : public QWidget
    {
        Q_OBJECT
        friend class ReactionsListHeader_p;
    public:
        ReactionsListHeader(QWidget* _parent);
        ~ReactionsListHeader();

        void setReactions(const Data::Reactions& _reactions);
        void setChecked(const QString& _reaction);

    Q_SIGNALS:
        void itemClicked(const QString &_reaction);

    private Q_SLOTS:
        void onItemClicked(const QString& _reaction);

    private:
        std::unique_ptr<ReactionsListHeader_p> d;
    };

    class ReactionsListHeaderItem_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsListHeaderItem
    //////////////////////////////////////////////////////////////////////////

    class ReactionsListHeaderItem : public QWidget
    {
        Q_OBJECT
    public:
        ReactionsListHeaderItem(QWidget* _parent);
        ~ReactionsListHeaderItem();

        void setText(const QString& _text);
        void setReaction(const QString& _reaction);
        void setTextAndReaction(const QString& _text, const QString& _reaction);
        void setChecked(bool _checked);

    Q_SIGNALS:
        void clicked(const QString& _reaction); // empty string for all reactions item

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        std::unique_ptr<ReactionsListHeaderItem_p> d;
    };

    class ReactionsListTab_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsListTab
    //////////////////////////////////////////////////////////////////////////

    class ReactionsListTab : public QWidget
    {
        Q_OBJECT
    public:
        ReactionsListTab(const QString& _reaction, int64_t _msgId, const QString& _contact, QWidget* _parent);
        ~ReactionsListTab();

        void removeItem(const QString& _contact);
        void addItem(const Data::ReactionsPage::Reaction& _reaction);

    Q_SIGNALS:
        void itemClicked(const QString& _contact);
        void removeMyReactionClicked();

    private Q_SLOTS:
        void onReactionsPage(int64_t _seq, const Data::ReactionsPage& _page, bool _success);
        void onConnectionState(const ConnectionState& _state);
        void onScrolled(int _value);
        void onSpinnerTimer();

    private:
        std::unique_ptr<ReactionsListTab_p> d;
    };

    class ReactionsListContent_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsListContent
    //////////////////////////////////////////////////////////////////////////

    class ReactionsListContent : public QWidget
    {
        Q_OBJECT
    public:
        ReactionsListContent(QWidget* _parent);
        ~ReactionsListContent();

        void addReactions(const Data::ReactionsPage& _page);
        void removeItem(const QString& _contact);

    Q_SIGNALS:
        void itemClicked(const QString& _contact);
        void removeMyReactionClicked();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onAvatarChanged(const QString& _contact);
        void onLeaveTimer();

    private:
        std::unique_ptr<ReactionsListContent_p> d;
    };
}

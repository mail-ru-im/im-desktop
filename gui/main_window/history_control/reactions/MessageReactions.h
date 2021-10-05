#pragma once

#include "types/reactions.h"
#include "cache/emoji/Emoji.h"

namespace Ui
{
    class HistoryControlPageItem;
    class ReactionsPlate;
    class AddReactionPlate;
    class AddReactionButton;
    class PlateWithShadow;

    class MessageReactions_p;

    enum class ReactionsPlateType
    {
        Regular,
        Media
    };

    //////////////////////////////////////////////////////////////////////////
    // MessageReactions
    //////////////////////////////////////////////////////////////////////////

    class MessageReactions : public QObject
    {
        Q_OBJECT
        friend class MessageReactions_p;

    public:
        MessageReactions(HistoryControlPageItem* _item);
        ~MessageReactions();

        void setReactions(const Data::Reactions& _reactions);
        const Data::Reactions& getReactions() const;

        void setOutgoing(bool _outgoing);
        void subscribe();

        QString contact() const;
        int64_t msgId() const;
        bool hasReactions() const;
        QRect plateRect() const;

        void onMouseLeave();
        void onMouseMove(const QPoint& _pos);
        void onMousePress(const QPoint& _pos);
        void onMouseRelease(const QPoint& _pos);
        void onMessageSizeChanged();

        void deleteControls();

        void initAddReactionButton();

        static bool enabled();
        static AddReactionPlate* createAddReactionPlate(const HistoryControlPageItem* const _item, const MessageReactions* const _reactions);

    public Q_SLOTS:
        void onAddReactionClicked(const QString& _reaction);
        void onRemoveReactionClicked();

    Q_SIGNALS:
        void platePositionChanged();

    private Q_SLOTS:
        void onReactionButtonClicked();
        void onReactionClicked(const QString& _reaction);
        void onReactionAddResult(int64_t _seq, const Data::Reactions& _reactions, bool _success);
        void onReactionRemoveResult(int64_t _seq, bool _success);

    private:
        std::unique_ptr<MessageReactions_p> d;
    };
}

#pragma once

namespace Ui
{
    struct ReactionWithTooltip
    {
        QString reaction_;
        QString tooltip_;
    };

    class DefaultReactions_p;

    class DefaultReactions : public QObject
    {
        Q_OBJECT
    public:
        DefaultReactions(const DefaultReactions&) = delete;
        DefaultReactions& operator=(const DefaultReactions &) = delete;
        ~DefaultReactions();

        static DefaultReactions* instance();

        const std::vector<ReactionWithTooltip>& reactionsWithTooltip() const;

    private Q_SLOTS:
        void onOmicronUpdated();

    private:
        DefaultReactions();

        std::unique_ptr<DefaultReactions_p> d;
    };
}

#pragma once

#include "../contact_list/MentionModel.h"
#include "animation/animation.h"

namespace Logic
{
    class MentionItemDelegate;
}

namespace Utils
{
    class OpacityEffect;
}

namespace Ui
{
    class FocusableListView;

    class MentionCompleter : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void contactSelected(const QString& _aimId, const QString& _friendly);
        void results(int _count);
        void hidden();

    private Q_SLOTS:
        bool itemClicked(const QModelIndex& _current);
        void onResults();

    private:
        Logic::MentionModel* model_;
        Logic::MentionItemDelegate* delegate_;
        FocusableListView* view_;
        int arrowOffset_;
        Utils::OpacityEffect* opacityEffect_;
        anim::Animation opacityAnimation_;

    public:
        MentionCompleter(QWidget* _parent);

        void setSearchPattern(const QString& _pattern, const Logic::MentionModel::SearchCondition _cond = Logic::MentionModel::SearchCondition::IfPatternChanged);
        QString getSearchPattern() const;

        void setDialogAimId(const QString& _aimid);
        bool insertCurrent();
        void keyUpOrDownPressed(const bool _isUpPressed);
        void recalcHeight();

        bool completerVisible() const;
        int itemCount() const;
        bool hasSelectedItem() const;

        void updateStyle();
        void showAnimated(const QPoint& _pos);
        void hideAnimated();

        void setArrowPosition(const QPoint& _pos);

    protected:
        void hideEvent(QHideEvent*) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void paintEvent(QPaintEvent* _e) override;
        bool focusNextPrevChild(bool) override { return false; }

    private:
        int calcHeight() const;

        enum class statSelectSource
        {
            mouseClick,
            keyEnter
        };
        void sendSelectionStats(const QModelIndex& _current, const statSelectSource _source);
    };
}
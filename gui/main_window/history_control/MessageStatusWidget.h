#pragma once

#include "../../controls/TextUnit.h"

namespace Ui
{

    class HistoryControlPageItem;

    class MessageTimeWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit MessageTimeWidget(HistoryControlPageItem *messageItem);

        void setTime(const int32_t _timestamp);
        void setContact(const QString& _aimId);
        QColor getTimeColor() const;

        void setUnderlayVisible(const bool _visible);
        bool isUnderlayVisible() const noexcept;

        void setEdited(const bool _edited);
        bool isEdited() const noexcept;

        int getHorMargin() const noexcept;
        int getVerMargin() const noexcept;

        void setOutgoing(const bool _outgoing);
        bool isOutgoing() const noexcept;

        void setSelected(const bool _selected);
        bool isSelected() const noexcept;

        void updateStyle();
        void updateText();

    protected:
        void paintEvent(QPaintEvent *) override;

    private:
        TextRendering::TextUnitPtr text_;

        QString aimId_;
        QString TimeText_;

        bool isEdited_;
        bool withUnderlay_;
        bool isOutgoing_;
        bool isSelected_;
    };

}
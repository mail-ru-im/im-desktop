#pragma once

#include "controls/SimpleListWidget.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class RadioTextRow : public SimpleListItem
    {
        Q_OBJECT;

    public:
        explicit RadioTextRow(const QString& _name, QWidget* _parent = nullptr);
        ~RadioTextRow();

        void setSelected(bool _value) override;
        bool isSelected() const override;

        void setComment(const QString& _comment);
        void resetComment();

    signals:
        void clicked(const QString&, QPrivateSignal) const;

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        const QString name_;
        std::unique_ptr<Ui::TextRendering::TextUnit> nameTextUnit_;
        std::unique_ptr<Ui::TextRendering::TextUnit> commentTextUnit_;
        bool isSelected_;
    };
}

#pragma once

#include "../../controls/TextUnit.h"

namespace Ui
{
    class CheckboxList;

    class CloseChatWidget : public QWidget
    {
        Q_OBJECT

    public:
        CloseChatWidget(QWidget* _parent, const QString& _aimid);

        bool isIgnoreSelected() const;
        bool isReportSelected() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        CheckboxList* options_;
        TextRendering::TextUnitPtr title_;
        QString aimId_;
    };

    void CloseChat(const QString& _aimId);
}

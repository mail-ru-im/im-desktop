#pragma once

#include "../../types/task.h"
#include "../EscapeCancellable.h"

namespace Ui
{
    class TaskStatusPopupContent;

    class TaskStatusPopup : public QWidget, public IEscapeCancellable
    {
        Q_OBJECT
    public:
        explicit TaskStatusPopup(const Data::TaskData& _taskToEdit, const QRect& _statusBubbleRect);
        ~TaskStatusPopup();

        void setFocus();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;

    private Q_SLOTS:
        void onWindowResized();
        void onSidebarClosed();

    private:
        TaskStatusPopupContent* content_ = nullptr;
    };

}

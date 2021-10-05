#pragma once

#include "controls/LineEditEx.h"

namespace Ui
{
    class AssigneeEdit : public LineEditEx
    {
        Q_OBJECT
    public:
        explicit AssigneeEdit(QWidget* _parent, const Options& _options = Options());
        ~AssigneeEdit();

    public Q_SLOTS:
        void selectContact(const QString& _aimId);
        std::optional<QString> selectedContact() const;

    Q_SIGNALS:
        void selectedContactChanged(const QString& _aimId);

    protected:
        void paintEvent(QPaintEvent* _event);

    private Q_SLOTS:
        void resetSelection();

    private:
        QPixmap avatar_;
        std::optional<QString> selectedContact_;
    };
}

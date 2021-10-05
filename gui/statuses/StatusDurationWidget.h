#pragma once

#include "Status.h"

namespace Statuses
{

    class StatusDurationWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // StatusDurationWidget
    //////////////////////////////////////////////////////////////////////////

    class StatusDurationWidget : public QWidget
    {
        Q_OBJECT
    public:
        StatusDurationWidget(QWidget* _parent);
        ~StatusDurationWidget();

        void setStatus(const QString& _status, const QString& _description);
        void setDateTime(const QDateTime& _dateTime);

    Q_SIGNALS:
        void backClicked(QPrivateSignal);

    protected:
        void showEvent(QShowEvent* _event) override;

    private Q_SLOTS:
        bool validateDate();
        void applyNewDuration();

    private:
        std::unique_ptr<StatusDurationWidget_p> d;
    };
}

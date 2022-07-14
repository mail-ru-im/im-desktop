#pragma once

#include "Status.h"

namespace Statuses
{

    class CustomStatusWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // CustomStatusWidget
    //////////////////////////////////////////////////////////////////////////

    class CustomStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        CustomStatusWidget(QWidget* _parent);
        ~CustomStatusWidget();

        void setStatus(const Status& _status);

    Q_SIGNALS:
        void backClicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private Q_SLOTS:
        void onTextChanged();
        void onNextClicked();
        void tryToConfirmStatus();

    private:
        void confirmStatus();

        std::unique_ptr<CustomStatusWidget_p> d;
    };
}

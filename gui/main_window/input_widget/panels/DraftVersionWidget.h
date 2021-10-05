#pragma once
#include "types/message.h"

namespace Ui
{

class DraftVersionWidget_p;

class DraftVersionWidget : public QWidget
{
    Q_OBJECT
public:
    DraftVersionWidget(QWidget* _parent);
    ~DraftVersionWidget();

    void setDraft(const Data::Draft& _draft);

Q_SIGNALS:
    void accept(QPrivateSignal);
    void cancel(QPrivateSignal);

private:
    std::unique_ptr<DraftVersionWidget_p> d;
};

}

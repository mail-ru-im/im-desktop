#pragma once

namespace Utils
{

class RegisterCustomSchemeTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    RegisterCustomSchemeTask();

    void run() override;

Q_SIGNALS:
    void finished();

};

}

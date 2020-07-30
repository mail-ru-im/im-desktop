#pragma once

#include "../namespaces.h"
#include "../main_window/mplayer/VideoPlayer.h"

UTILS_NS_BEGIN

class LoadMovieFromFileTask
    : public QObject
    , public QRunnable
{
    Q_OBJECT

Q_SIGNALS:
    void loadedSignal(QSharedPointer<QMovie> movie);

public:
    explicit LoadMovieFromFileTask(const QString& path);

    virtual ~LoadMovieFromFileTask();

    void run() override;

private:
    const QString Path_;

};

UTILS_NS_END
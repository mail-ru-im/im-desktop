#include "stdafx.h"

#include "LoadMovieFromFileTask.h"

UTILS_NS_BEGIN

LoadMovieFromFileTask::LoadMovieFromFileTask(const QString& path)
    : Path_(path)
{
    im_assert(!Path_.isEmpty());
    im_assert(QFile::exists(Path_));
}

LoadMovieFromFileTask::~LoadMovieFromFileTask()
{
}

void LoadMovieFromFileTask::run()
{
    Q_EMIT loadedSignal(QSharedPointer<QMovie>::create(Path_));
}

UTILS_NS_END
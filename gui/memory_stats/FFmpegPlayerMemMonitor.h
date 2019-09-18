#pragma once

#include <QObject>
#include <vector>
#include <array>

namespace Utils
{
class QObjectWatcher;
}

namespace Ui
{
class FFMpegPlayer;

class FFmpegPlayerMemMonitor: public QObject
{
public:
    struct FFMpegPlayerInfo
    {
        uint32_t mediaId_ = std::numeric_limits<uint32_t>::max();
        int64_t decodedFramesCount_ = 0;
        int64_t decodedFramesPixmapSize_ = 0;
    };

    using FFMpegStats = std::vector<FFMpegPlayerInfo> ;


public:
    static FFmpegPlayerMemMonitor& instance();

    bool watchFFmpegPlayer(Ui::FFMpegPlayer* _player);
    FFMpegStats getCurrentStats();

private:
    FFmpegPlayerMemMonitor(QObject *_parent);
    Utils::QObjectWatcher *ffmpegPlayersWatcher();

    using FFMpegObjects = QList<Ui::FFMpegPlayer *>;

    FFMpegObjects currentPlayers();

private:
    Utils::QObjectWatcher *ffmpegPlayersWatcher_ = nullptr;
};

}

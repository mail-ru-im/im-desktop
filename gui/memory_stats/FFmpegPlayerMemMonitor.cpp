#include "FFmpegPlayerMemMonitor.h"

#include "../utils/QObjectWatcher.h"
#include "../utils/memory_utils.h"
#include "../main_window/mplayer/FFMpegPlayer.h"

namespace  Ui {

template <typename From, typename To>
struct Static_Caster
{
    To* operator()(From* _p)
    {
        return static_cast<To*>(_p);
    }
};

FFmpegPlayerMemMonitor &FFmpegPlayerMemMonitor::instance()
{
    static FFmpegPlayerMemMonitor memMonitor(nullptr);

    return memMonitor;
}

FFmpegPlayerMemMonitor::FFmpegPlayerMemMonitor(QObject *parent)
    : QObject(parent)
{
}

Utils::QObjectWatcher *FFmpegPlayerMemMonitor::ffmpegPlayersWatcher()
{
    if (!ffmpegPlayersWatcher_)
        ffmpegPlayersWatcher_ = new Utils::QObjectWatcher(this);

    return ffmpegPlayersWatcher_;
}

FFmpegPlayerMemMonitor::FFMpegObjects FFmpegPlayerMemMonitor::currentPlayers()
{
    FFmpegPlayerMemMonitor::FFMpegObjects result;

    auto objs = ffmpegPlayersWatcher()->allObjects();
    result.reserve(objs.size());

    std::transform(objs.begin(), objs.end(), std::back_inserter(result), Static_Caster<QObject, FFMpegPlayer>());

    return result;
}

bool FFmpegPlayerMemMonitor::watchFFmpegPlayer(Ui::FFMpegPlayer* _player)
{
    return ffmpegPlayersWatcher()->addObject(static_cast<QObject *>(_player));
}

FFmpegPlayerMemMonitor::FFMpegStats FFmpegPlayerMemMonitor::getCurrentStats()
{
    auto stats = FFMpegStats();

    auto players = currentPlayers();
    stats.reserve(players.count());

    for (auto player: players)
    {
        FFMpegPlayerInfo info;
        info.decodedFramesCount_ = player->decodedFrames_.size();
        info.decodedFramesPixmapSize_ = 0;
        for (const auto &decodedFrame: player->decodedFrames_)
        {
            info.decodedFramesPixmapSize_ += Utils::getMemoryFootprint(decodedFrame.image_);
        }
        info.mediaId_ = player->mediaId_;

        stats.push_back(info);
    }

    return stats;
}

}

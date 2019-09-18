#include "gui_memory_monitor.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/stickers/stickers.h"
#include "../styles/ThemesContainer.h"
#include "../utils/memory_utils.h"
#include "../utils/InterConnector.h"
#include "../main_window/ContactDialog.h"
#include "../main_window/history_control/HistoryControl.h"
#include "../main_window/history_control/HistoryControlPage.h"
#include "memory_stats/MessageItemMemMonitor.h"
#include "memory_stats/FFmpegPlayerMemMonitor.h"
#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/log/log.h"

namespace
{
const Memory_Stats::NameString_t ReporteeName("GuiMemoryMonitor");

qint64 calculateFootprintFor(const Logic::AvatarStorage::CacheMap& cacheMap);

constexpr std::chrono::milliseconds sendStatInterval = std::chrono::hours(1);
constexpr std::chrono::milliseconds sendStatIntervalDebug = std::chrono::minutes(1);
constexpr int64_t memoryLimitStep = 200 * (1 << 20);

struct QSizeComparator
{
    bool operator()(const QSize& lhs, const QSize& rhs) const
    {
        return lhs.width() * lhs.height() > rhs.width() * rhs.height();
    }
};
}

GuiMemoryMonitor &GuiMemoryMonitor::instance()
{
    static GuiMemoryMonitor guiMonitor;
    return guiMonitor;
}

GuiMemoryMonitor::GuiMemoryMonitor(QObject *_parent)
    : QObject(_parent)
    , statTimer_(new QTimer(this))
{
    const int ms = sendStatInterval.count();

    statTimer_->start(build::is_debug() ? sendStatIntervalDebug.count() : sendStatInterval.count());

    QObject::connect(statTimer_, &QTimer::timeout, this, &GuiMemoryMonitor::onStatTimer);

    memoryLimit_ = memoryLimitStep;
}

Memory_Stats::ReportsList GuiMemoryMonitor::syncGetReportsFor(const Memory_Stats::RequestedTypes &_types)
{
    Memory_Stats::ReportsList reports;

    for (auto type: _types)
    {
        switch (type)
        {
        case Memory_Stats::StatType::CachedAvatars:
            reports.push_back(getAvatarsReport());
            break;
        case Memory_Stats::StatType::CachedThemes:
            reports.push_back(getThemesReport());
            break;
        case Memory_Stats::StatType::CachedEmojis:
            reports.push_back(getEmojisReport());
            break;
        case Memory_Stats::StatType::CachedPreviews:
            reports.push_back(getPreviewsReport());
            break;
        case Memory_Stats::StatType::VideoPlayerInitialization:
            reports.push_back(getVideoPlayersReport());
            break;
        case Memory_Stats::StatType::CachedStickers:
            reports.push_back(getStickersReport());
            break;
        default:
            break;
        }
    }

    return reports;
}

bool GuiMemoryMonitor::isRequestFinished(const Memory_Stats::RequestHandle &_req_handle) const
{
    static std::vector<Memory_Stats::StatType> NonGuiTypes = { Memory_Stats::StatType::VoipInitialization };

    auto it = request_reports_.find(_req_handle);
    if (it == request_reports_.end())
        return false;

    auto req_types_copy = it->first.info_.req_types_;

    for (auto report: it->second)
    {
        req_types_copy.erase(report.getStatType());
    }

    for (auto nonguitype: NonGuiTypes)
    {
        req_types_copy.erase(nonguitype);
    }

    return req_types_copy.size() == 0;
}

void GuiMemoryMonitor::asyncGetReportsFor(const Memory_Stats::RequestHandle &_req_handle)
{
    assert(request_reports_.count(_req_handle) == 0);
    request_reports_[_req_handle] = syncGetReportsFor(_req_handle.info_.req_types_);

    if (isRequestFinished(_req_handle.id_))
        return onRequestFinished(_req_handle.id_);
}

void GuiMemoryMonitor::onRequestFinished(const Memory_Stats::RequestHandle &_req_handle)
{
    auto it = request_reports_.find(_req_handle);
    if (it == request_reports_.end())
        return;

    emit reportsReadyFor(it->first,
                         it->second);

    request_reports_.erase(it);
}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getAvatarsReport()
{
    qint64 total = 0;

    qint64 totalByAimId = calculateFootprintFor(Logic::GetAvatarStorage()->GetByAimId());
    total += totalByAimId;

    qint64 totalByAimIdAndSize = calculateFootprintFor(Logic::GetAvatarStorage()->GetByAimIdAndSize());
    total += totalByAimIdAndSize;

    qint64 totalRounded = calculateFootprintFor(Logic::GetAvatarStorage()->GetRoundedByAimId());
    total += totalRounded;

    Memory_Stats::MemoryStatsReport report(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::CachedAvatars);

    report.addSubcategory("by_aim_id", totalByAimId);
    report.addSubcategory("by_aim_id_and_size", totalByAimIdAndSize);
    report.addSubcategory("rounded", totalRounded);

    return report;
}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getThemesReport()
{
    qint64 total = 0;
    qint64 wpImages = 0;
    qint64 wpThumbs = 0;

    const auto& wallpapers = Styling::getThemesContainer().getAllAvailableWallpapers();
    for (auto w : wallpapers)
    {
        const auto& bg = w->getWallpaperImage();
        wpImages += Utils::getMemoryFootprint(bg);

        if (const auto& preview = w->getPreviewImage(); preview.cacheKey() != bg.cacheKey())
            wpThumbs += Utils::getMemoryFootprint(preview);

        total += Utils::getMemoryFootprint(w);
    }

    Memory_Stats::MemoryStatsReport report(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::CachedThemes);
    report.addSubcategory("theme_images", wpImages);
    report.addSubcategory("theme_thumbs", wpThumbs);

    return report;
}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getEmojisReport()
{
    qint64 total = 0;
    std::set<QSize, QSizeComparator> differentSizes;

    const auto& emojisCache = Emoji::GetEmojiCache();
    for (const auto& [_, image] : emojisCache)
    {
        total += Utils::getMemoryFootprint(image);
        differentSizes.insert(image.size());
    }


    Memory_Stats::MemoryStatsReport report(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::CachedEmojis);

    std::string emojiCacheSub = "Emojis in cache (" + std::to_string(emojisCache.size()) + ")";
    for (const auto& size: differentSizes)
    {
        emojiCacheSub += "\n";
        emojiCacheSub += std::to_string(size.width()) + "x" + std::to_string(size.height());
    }

    report.addSubcategory(emojiCacheSub, total);

    return report;
}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getStickersReport()
{
    qint64 total = 0;

    auto calcStickers = [&total](const Ui::Stickers::setsMap& _stickerSets)
    {
        for (const auto& it: _stickerSets)
        {
            Ui::Stickers::setSptr set = it.second;
            auto stickersMap = set->getStickersTree();
            for (const auto& sticker: stickersMap)
            {
                const auto &stickerImgs = sticker.second->getImages();
                for (const auto &stickerImgIt: stickerImgs)
                {
                    total += Utils::getMemoryFootprint(std::get<0>(stickerImgIt.second));
                }
            }
        }
    };

    calcStickers(Ui::Stickers::getSetsTree());
    calcStickers(Ui::Stickers::getStoreTree());

    Memory_Stats::MemoryStatsReport report(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::CachedStickers);
    return report;

}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getPreviewsReport()
{
    qint64 total = 0;

    auto categoriesArray = MessageItemMemMonitor::instance().getMessageItemsFootprint();

    for (size_t i = 1; i < categoriesArray.size(); ++i)
    {
        total += categoriesArray[i].second;
    }

    Memory_Stats::MemoryStatsReport report(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::CachedPreviews);

    report.addSubcategory("total <b>(" + std::to_string(categoriesArray[0].first) + ")</b>",
            categoriesArray[0].second);

    report.addSubcategory("text blocks (" + std::to_string(categoriesArray[1].first) + ")",
            categoriesArray[1].second);
    report.addSubcategory("image preview blocks (" + std::to_string(categoriesArray[2].first) + ")",
            categoriesArray[2].second);
    report.addSubcategory("link blocks (" + std::to_string(categoriesArray[3].first) + ")",
            categoriesArray[3].second);
    report.addSubcategory("sticker blocks (" + std::to_string(categoriesArray[4].first) + ")",
            categoriesArray[4].second);
    report.addSubcategory("file sharing blocks (" + std::to_string(categoriesArray[5].first) + ")",
            categoriesArray[5].second);
    report.addSubcategory("quote blocks (" + std::to_string(categoriesArray[6].first) + ")",
            categoriesArray[6].second);

    return report;
}

Memory_Stats::MemoryStatsReport GuiMemoryMonitor::getVideoPlayersReport()
{
    qint64 total = 0;
    Memory_Stats::MemoryStatsReport result(ReporteeName,
                                           total,
                                           Memory_Stats::StatType::VideoPlayerInitialization);

    for (auto &videoStat: Ui::FFmpegPlayerMemMonitor::instance().getCurrentStats())
    {
        QString format(qsl("Player:\nmedia_id = %1\ndecodedFrames(%2):\n\t\t\t"));

        result.addSubcategory(format.arg(videoStat.mediaId_)
                              .arg(videoStat.decodedFramesCount_).toStdString(),
                              videoStat.decodedFramesPixmapSize_);
    }

    return result;
}

std::string_view getPropertyName(const Memory_Stats::StatType _type)
{
    switch (_type)
    {
    case Memory_Stats::StatType::CachedAvatars:
        return "_avatars";
    case Memory_Stats::StatType::CachedThemes:
        return "_themes";
    case Memory_Stats::StatType::CachedEmojis:
        return "_emoji";
    case Memory_Stats::StatType::CachedPreviews:
        return "_previews";
    case Memory_Stats::StatType::VideoPlayerInitialization:
        return "_video_player";
    case Memory_Stats::StatType::CachedStickers:
        return "_stickers";
    default:
        return std::string_view();
    }

}

void GuiMemoryMonitor::onStatTimer()
{
    Ui::GetDispatcher()->post_message_to_core("get_ram_usage", nullptr, this, [this](core::icollection* _coll)
    {
        Ui::gui_coll_helper coll(_coll, false);
        const int64_t ramUsageFull = coll.get_value_as_int64("ram_used");
        const int32_t ramUsageMb = int32_t(ramUsageFull >> 20);
        const int64_t ramUsageArchiveIndex = coll.get_value_as_int64("archive_index");
        const int64_t ramUsageArchiveGallery = coll.get_value_as_int64("archive_gallery");
        const int64_t ramVoipInit = coll.get_value_as_int64("voip_initialization");

        if (ramUsageFull == 0)
            return;

        core::stats::event_props_type props;
        props.emplace_back("_overall", core::stats::round_value(ramUsageMb, 50, 1000));

        Memory_Stats::ReportsList reports = syncGetReportsFor({
            Memory_Stats::StatType::CachedAvatars,
            Memory_Stats::StatType::CachedThemes,
            Memory_Stats::StatType::CachedEmojis,
            Memory_Stats::StatType::CachedPreviews,
            Memory_Stats::StatType::VideoPlayerInitialization,
            Memory_Stats::StatType::CachedStickers });

        for (const auto& _report : reports)
        {
            props.emplace_back(getPropertyName(_report.getStatType()), std::to_string(int(100 * double(_report.getOccupiedMemory()) / double(ramUsageFull))));
        }

        props.emplace_back(std::string_view("_archive_index"), std::to_string(int(100 * double(ramUsageArchiveIndex) / double(ramUsageFull))));
        props.emplace_back(std::string_view("_archive_gallery"), std::to_string(int(100 * double(ramUsageArchiveGallery) / double(ramUsageFull))));
        props.emplace_back(std::string_view("_voip_initialization"), std::to_string(int(100 * double(ramVoipInit) / double(ramUsageFull))));

        Ui::GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::memory_state, props);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::memory_state, props);

        if (ramUsageFull > memoryLimit_)
        {
            memoryLimit_ += memoryLimitStep;

            Ui::GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::memory_state_limit, props);
        }
    });
}

void GuiMemoryMonitor::writeLogMemoryReport(std::function<void()> _onComplete)
{
    Ui::GetDispatcher()->post_message_to_core("get_ram_usage", nullptr, this, [this, _onComplete](core::icollection* _coll)
    {
        Ui::gui_coll_helper coll(_coll, false);
        const int64_t ramUsageFull = coll.get_value_as_int64("ram_used");
        const int64_t ramUsageArchiveIndex = coll.get_value_as_int64("archive_index");
        const int64_t ramUsageArchiveGallery = coll.get_value_as_int64("archive_gallery");
        const int64_t ramVoipInit = coll.get_value_as_int64("voip_initialization");
        const int32_t ramUsageMb = int32_t(ramUsageFull >> 20);


        std::stringstream logData;
        logData << "Memory usage:\r\n" << "total " << ramUsageMb << " Mb\r\n";

        Memory_Stats::ReportsList reports = syncGetReportsFor({
            Memory_Stats::StatType::CachedAvatars,
            Memory_Stats::StatType::CachedThemes,
            Memory_Stats::StatType::CachedEmojis,
            Memory_Stats::StatType::CachedPreviews,
            Memory_Stats::StatType::VideoPlayerInitialization,
            Memory_Stats::StatType::CachedStickers });

        for (const auto& _report : reports)
        {
            logData << getPropertyName(_report.getStatType()) << " " << (_report.getOccupiedMemory() >> 10) << " Kb\r\n";
        }

        logData << std::string_view("_archive_index") << " " << (ramUsageArchiveIndex >> 10) << " Kb\r\n";
        logData << std::string_view("_archive_gallery") << " " << (ramUsageArchiveGallery >> 10) << " Kb\r\n";
        logData << std::string_view("_voip_initialization") << " " << (ramVoipInit >> 10) << " Kb\r\n";

        Log::write_network_log(logData.str());

        _onComplete();
    });

}

//common::memory_stats::memory_stats_report GuiMemoryMonitor::getPreviewsReport(int64_t _img_cache, int64_t _img_loaders)
//{
//    qint64 total = 0;
//    total += _img_cache;
//    total+= _img_loaders;

//    common::memory_stats::memory_stats_report report(ReporteeName,
//                                                     total,
//                                                     common::memory_stats::stat_type::cached_previews);
//    report.addSubcategory("gallery_widget: img cache", _img_cache);
//    report.addSubcategory("gallery_widget: img loaders", _img_loaders);

//    auto historyControl = Utils::InterConnector::instance().getContactDialog()->getHistoryControl();
//    assert(historyControl);
//    if (historyControl)
//    {
//        auto pages = historyControl->allPages();
//        for (auto pageIt = pages.begin(); pageIt != pages.end(); ++pageIt)
//        {
//            Ui::HistoryControlPage* page = pageIt.value();
//            if (!page)
//                continue;

//            auto itemDataList = page->getItemsDataList();
//            for (auto itemDataIt = itemDataList.begin(); itemDataIt != itemDataList.end(); ++itemDataIt)
//            {
//            }
//        }
//    }

//    return report;
//}

namespace
{
qint64 calculateFootprintFor(const Logic::AvatarStorage::CacheMap& cacheMap)
{
    qint64 result = 0;

    for (auto it = cacheMap.begin(); it != cacheMap.end(); it++)
    {
        result += Utils::getMemoryFootprint(it->second.get());
    }

    return result;
}
}

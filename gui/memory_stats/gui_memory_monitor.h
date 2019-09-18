#pragma once

#include <QObject>
#include <unordered_map>

#include "memory_stats/MemoryStatsCommon.h"
#include "memory_stats/MemoryStatsRequest.h"
#include "memory_stats/MemoryStatsReport.h"

class GuiMemoryMonitor: public QObject
{
    Q_OBJECT

public:
    static GuiMemoryMonitor& instance();
    ~GuiMemoryMonitor() = default;

    void asyncGetReportsFor(const Memory_Stats::RequestHandle& _req_handle);
    void writeLogMemoryReport(std::function<void()> _onComplete = []{});

Q_SIGNALS:
    void reportsReadyFor(const Memory_Stats::RequestHandle& _req_handle,
                         const Memory_Stats::ReportsList& _reports_list);

    void requestGalleryWidgetMemory(Memory_Stats::RequestId _req_id);

private Q_SLOTS:

    void onStatTimer();

private:
    GuiMemoryMonitor(QObject* _parent = nullptr);

    Memory_Stats::ReportsList syncGetReportsFor(const Memory_Stats::RequestedTypes& _types);
    bool isRequestFinished(const Memory_Stats::RequestHandle& _req_handle) const;

    Memory_Stats::MemoryStatsReport getAvatarsReport();
    Memory_Stats::MemoryStatsReport getThemesReport();
    Memory_Stats::MemoryStatsReport getEmojisReport();
    Memory_Stats::MemoryStatsReport getStickersReport();
    Memory_Stats::MemoryStatsReport getPreviewsReport();
    Memory_Stats::MemoryStatsReport getVideoPlayersReport();

    void onRequestFinished(const Memory_Stats::RequestHandle& _req_handle);

private:
    std::unordered_map<Memory_Stats::RequestHandle,
                       Memory_Stats::ReportsList,
                       Memory_Stats::RequestHandleHash> request_reports_;

    QTimer* statTimer_;
    int64_t memoryLimit_;
};

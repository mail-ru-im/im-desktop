#include "NativeEventFilter.h"
#include "gui_metrics.h"

namespace Utils
{

bool NativeEventFilter::nativeEventFilter(const QByteArray &_eventType, void *_message, long *)
{
#if defined(_WIN32)
    MSG* msg = static_cast< MSG* >(_message);

    if (msg->message == WM_POWERBROADCAST)
    {
        switch (msg->wParam) {
        case PBT_APMPOWERSTATUSCHANGE:
            break;
        case PBT_APMRESUMEAUTOMATIC:
            statistic::getGuiMetrics().eventAppWakeFromSuspend();
            break;
        case PBT_APMRESUMESUSPEND:
            statistic::getGuiMetrics().eventAppWakeFromSuspend();
            break;
        case PBT_APMSUSPEND:
            statistic::getGuiMetrics().eventAppSleep();
            break;
        }
    }
#endif // _WIN32

    return false;
}

}

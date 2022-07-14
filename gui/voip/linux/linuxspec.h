#pragma once
#ifdef __linux__
class QWidget;
namespace platform_linux
{
    bool isNetWmDeleteWindowEvent(void* _ev);
    void showInWorkspace(QWidget* _w, QWidget* _initiator);
    bool windowIsOverlapped(QWidget* _window);
    bool isSpacebarDown();
}
#endif
